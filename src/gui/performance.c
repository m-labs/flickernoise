/*
 * Flickernoise
 * Copyright (C) 2010, 2011 Sebastien Bourdeauducq
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <rtems.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <mtklib.h>
#include <mtkeycodes.h>
#include <bsp/milkymist_video.h>

#include "../input.h"
#include "../fb.h"
#include "messagebox.h"
#include "../config.h"
#include "../renderer/compiler.h"
#include "../renderer/renderer.h"
#include "guirender.h"
#include "performance.h"
#include "../renderer/osd.h"

#define FILENAME_LEN 384

#define MAX_PATCHES 64

struct patch_info {
	char filename[FILENAME_LEN];
	struct patch *p;
};

static int npatches;
static struct patch_info patches[MAX_PATCHES];
static int simple_mode;
static int simple_mode_current;
static int dt_mode;
static int as_mode;
static int input_video;

static int add_patch(const char *filename)
{
	int i;

	for(i=0;i<npatches;i++) {
		if(strcmp(patches[i].filename, filename) == 0)
			return i;
	}
	if(npatches == MAX_PATCHES) return -1;
	strcpy(patches[npatches].filename, filename);
	patches[npatches].p = NULL;
	return npatches++;
}

static int keyboard_patches[26];
static int ir_patches[64];
static int midi_channel;
static int midi_patches[128];
static int osc_patches[64];

static void add_firstpatch()
{
	const char *filename;

	filename = config_read_string("firstpatch");
	if(filename == NULL) return;
	add_patch(filename);
}

static void add_keyboard_patches()
{
	int i;
	char config_key[6];
	const char *filename;

	strcpy(config_key, "key_");
	config_key[5] = 0;
	for(i=0;i<26;i++) {
		config_key[4] = 'a' + i;
		filename = config_read_string(config_key);
		if(filename != NULL)
			keyboard_patches[i] = add_patch(filename);
		else
			keyboard_patches[i] = -1;
	}
}

static void add_ir_patches()
{
	int i;
	char config_key[6];
	const char *filename;

	for(i=0;i<64;i++) {
		sprintf(config_key, "ir_%02x", i);
		filename = config_read_string(config_key);
		if(filename != NULL)
			ir_patches[i] = add_patch(filename);
		else
			ir_patches[i] = -1;
	}
}

static void add_midi_patches()
{
	int i;
	char config_key[8];
	const char *filename;

	midi_channel = config_read_int("midi_channel", 0);
	for(i=0;i<128;i++) {
		sprintf(config_key, "midi_%02x", i);
		filename = config_read_string(config_key);
		if(filename != NULL)
			midi_patches[i] = add_patch(filename);
		else
			midi_patches[i] = -1;
	}
}

static void add_osc_patches()
{
	int i;
	char config_key[7];
	const char *filename;

	for(i=0;i<64;i++) {
		sprintf(config_key, "osc_%02x", i);
		filename = config_read_string(config_key);
		if(filename != NULL)
			osc_patches[i] = add_patch(filename);
		else
			osc_patches[i] = -1;
	}
}

static void add_simple_patches()
{
	DIR *d;
	struct dirent *entry;
	struct stat s;
	char fullname[384];
	char *c;
	
	d = opendir(SIMPLE_PATCHES_FOLDER);
	if(!d)
		return;
	while((entry = readdir(d))) {
		if(entry->d_name[0] == '.') continue;
		strncpy(fullname, SIMPLE_PATCHES_FOLDER"/", sizeof(fullname));
		strncat(fullname, entry->d_name, sizeof(fullname));
		lstat(fullname, &s);
		if(!S_ISDIR(s.st_mode)) {
			c = strrchr(entry->d_name, '.');
			if((c != NULL) && (strcmp(c, ".fnp") == 0))
				add_patch(fullname);
		}
	}
	closedir(d);
}

static int appid;
static int started;

static void close_callback(mtk_event *e, void *arg)
{
	mtk_cmd(appid, "w.close()");
}

static void update_buttons()
{
	mtk_cmdf(appid, "b_mode_simple.set(-state %s)", simple_mode ? "on" : "off");
	mtk_cmdf(appid, "b_mode_file.set(-state %s)", !simple_mode ? "on" : "off");
	mtk_cmdf(appid, "b_mode_simple_dt.set(-state %s)", dt_mode ? "on" : "off");
	mtk_cmdf(appid, "b_mode_simple_as.set(-state %s)", as_mode ? "on" : "off");
}

static void simple_callback(mtk_event *e, void *arg)
{
	simple_mode = 1;
	update_buttons();
}

static void file_callback(mtk_event *e, void *arg)
{
	simple_mode = 0;
	update_buttons();
}

static void dt_callback(mtk_event *e, void *arg)
{
	dt_mode = !dt_mode;
	update_buttons();
}

static void as_callback(mtk_event *e, void *arg)
{
	as_mode = !as_mode;
	update_buttons();
}

static void go_callback(mtk_event *e, void *arg)
{
	start_performance(simple_mode, dt_mode, as_mode);
}

void init_performance()
{
	appid = mtk_init_app("Performance");

	mtk_cmd_seq(appid,
		"g = new Grid()",

		"gs = new Grid()",
		"l_mode = new Label(-text \"Mode:\")",
		"b_mode_simple = new Button(-text \"Simple:\")",
		"b_mode_simple_dt = new Button(-text \"Display titles\")",
		"b_mode_simple_as = new Button(-text \"Auto switch\")",
		"b_mode_file = new Button(-text \"Configured\")",
		"gs.place(l_mode, -column 1 -row 1)",
		"gs.place(b_mode_simple, -column 2 -row 1)",
		"gs.place(b_mode_simple_dt, -column 3 -row 1)",
		"gs.place(b_mode_simple_as, -column 4 -row 1)",
		"gs.place(b_mode_file, -column 2 -row 2)",

		"separator1 = new Separator()",
		"l_status = new Label(-text \"Ready.\")",
		"progress = new LoadDisplay()",
		"separator2 = new Separator()",
		
		"gb = new Grid()",
		"b_go = new Button(-text \"Go!\")",
		"b_close = new Button(-text \"Close\")",
		"gb.place(b_go, -column 1 -row 1)",
		"gb.place(b_close, -column 2 -row 1)",
		"gb.columnconfig(2, -size 0)",

		"g.place(gs, -row 1 -column 1)",
		"g.place(separator1, -row 2 -column 1)",
		"g.place(l_status, -row 3 -column 1)",
		"g.place(progress, -row 4 -column 1)",
		"g.place(separator2, -row 5 -column 1)",
		"g.place(gb, -column 1 -row 6 -column 1)",

		"w = new Window(-content g -title \"Performance\")",
		0);

	mtk_bind(appid, "b_mode_simple", "press", simple_callback, NULL);
	mtk_bind(appid, "b_mode_file", "press", file_callback, NULL);
	mtk_bind(appid, "b_mode_simple_dt", "press", dt_callback, NULL);
	mtk_bind(appid, "b_mode_simple_as", "press", as_callback, NULL);
	
	mtk_bind(appid, "b_go", "commit", go_callback, NULL);
	mtk_bind(appid, "b_close", "commit", close_callback, NULL);

	mtk_bind(appid, "w", "close", close_callback, NULL);
}

static int compiled_patches;
#define UPDATE_PERIOD 20
static rtems_interval next_update;

static void dummy_rmc(const char *msg)
{
}

static struct patch *compile_patch(const char *filename)
{
	FILE *fd;
	int r;
	char buf[32768];

	fd = fopen(filename, "r");
	if(fd == NULL) return NULL;
	r = fread(buf, 1, 32767, fd);
	if(r <= 0) {
		fclose(fd);
		return NULL;
	}
	buf[r] = 0;
	fclose(fd);

	return patch_compile_filename(filename, buf, dummy_rmc);
}

static rtems_task comp_task(rtems_task_argument argument)
{
	for(;compiled_patches<npatches;compiled_patches++) {
		patches[compiled_patches].p = compile_patch(patches[compiled_patches].filename);
		if(patches[compiled_patches].p == NULL) {
			compiled_patches = -compiled_patches-1;
			break;
		}
	}
	rtems_task_delete(RTEMS_SELF);
}

static void free_patches()
{
	int i;

	for(i=0;i<npatches;i++) {
		if(patches[i].p != NULL)
			patch_free(patches[i].p);
	}
}

static int keycode_to_index(int keycode)
{
	switch(keycode) {
		case MTK_KEY_A: return 0;
		case MTK_KEY_B: return 1;
		case MTK_KEY_C: return 2;
		case MTK_KEY_D: return 3;
		case MTK_KEY_E: return 4;
		case MTK_KEY_F: return 5;
		case MTK_KEY_G: return 6;
		case MTK_KEY_H: return 7;
		case MTK_KEY_I: return 8;
		case MTK_KEY_J: return 9;
		case MTK_KEY_K: return 10;
		case MTK_KEY_L: return 11;
		case MTK_KEY_M: return 12;
		case MTK_KEY_N: return 13;
		case MTK_KEY_O: return 14;
		case MTK_KEY_P: return 15;
		case MTK_KEY_Q: return 16;
		case MTK_KEY_R: return 17;
		case MTK_KEY_S: return 18;
		case MTK_KEY_T: return 19;
		case MTK_KEY_U: return 20;
		case MTK_KEY_V: return 21;
		case MTK_KEY_W: return 22;
		case MTK_KEY_X: return 23;
		case MTK_KEY_Y: return 24;
		case MTK_KEY_Z: return 25;
		default:
			return -1;
	}
}

static rtems_interval next_as_time;
#define AUTOSWITCH_PERIOD_MIN (2*60*100)
#define AUTOSWITCH_PERIOD_MAX (4*60*100)
static void update_next_as_time()
{
	rtems_interval t;
	
	t = rtems_clock_get_ticks_since_boot();
	next_as_time = t + AUTOSWITCH_PERIOD_MIN + (rand() % (AUTOSWITCH_PERIOD_MAX - AUTOSWITCH_PERIOD_MIN));
}

static int suitable_for_simple(struct patch *p)
{
	int suitable;
	
	suitable = 1;
	if(p->require & REQUIRE_DMX) suitable = 0;
	if(p->require & REQUIRE_OSC) suitable = 0;
	if(p->require & REQUIRE_MIDI) suitable = 0;
	if((p->require & REQUIRE_VIDEO) && !input_video) suitable = 0;
	return suitable;
}

static void event_callback(mtk_event *e, int count)
{
	int i;
	int index;
	int next;
	rtems_interval t;
	int looped;

	index = -1;
	if(simple_mode) {
		next = 0;
		for(i=0;i<count;i++) {
			if(e[i].type == EVENT_TYPE_PRESS) {
				if(e[i].press.code == MTK_KEY_F1)
					osd_event(patches[simple_mode_current].filename);
				if(e[i].press.code == MTK_KEY_F11)
					next = 1;
				if(e[i].press.code == MTK_KEY_F9)
					next = -1;
			}
		}
		if(as_mode) {
			t = rtems_clock_get_ticks_since_boot();
			if(t >= next_as_time)
				next = 1;
		}
		if(next) {
			looped = simple_mode_current;
			do {
				simple_mode_current += next;
				if(simple_mode_current == npatches)
					simple_mode_current = 0;
				if(simple_mode_current < 0)
					simple_mode_current = npatches - 1;
			} while(!suitable_for_simple(patches[simple_mode_current].p) && (looped != simple_mode_current));
			renderer_pulse_patch(patches[simple_mode_current].p);
			if(as_mode)
				update_next_as_time();
		}
		if(dt_mode && (index != -1))
			osd_event(patches[index].filename);
	} else {
		for(i=0;i<count;i++) {
			if(e[i].type == EVENT_TYPE_PRESS) {
				index = keycode_to_index(e[i].press.code);
				if(index != -1) {
					index = keyboard_patches[index];
					renderer_add_patch(patches[index].p);
				}
			} else if(e[i].type == EVENT_TYPE_RELEASE) {
				index = keycode_to_index(e[i].release.code);
				if(index != -1) {
					index = keyboard_patches[index];
					renderer_del_patch(patches[index].p);
				}
			} else if(e[i].type == EVENT_TYPE_IR) {
				index = e[i].press.code;
				index = ir_patches[index];
				if(index != -1)
					renderer_pulse_patch(patches[index].p);
			} else if(e[i].type == EVENT_TYPE_MIDI_NOTEON) {
				if(((e[i].press.code & 0x0f0000) >> 16) == midi_channel) {
					index = e[i].press.code & 0x7f;
					index = midi_patches[index];
					if(index != -1)
						renderer_add_patch(patches[index].p);
				}
			} else if(e[i].type == EVENT_TYPE_MIDI_NOTEOFF) {
				if(((e[i].press.code & 0x0f0000) >> 16) == midi_channel) {
					index = e[i].press.code & 0x7f;
					index = midi_patches[index];
					if(index != -1)
						renderer_del_patch(patches[index].p);
				}
			} else if(e[i].type == EVENT_TYPE_OSC) {
				index = e[i].press.code & 0x3f;
				index = osc_patches[index];
				if(index != -1)
					renderer_pulse_patch(patches[index].p);
			}
		}
	}
}

static void stop_callback()
{
	free_patches();
	started = 0;
	input_delete_callback(event_callback);
}

static void refresh_callback(mtk_event *e, int count)
{
	rtems_interval t;
	int looped;
	int index;
	
	t = rtems_clock_get_ticks_since_boot();
	if(t >= next_update) {
		if(compiled_patches >= 0) {
			mtk_cmdf(appid, "progress.barconfig(load, -value %d)", (100*compiled_patches)/npatches);
			if(compiled_patches == npatches) {
				/* All patches compiled. Start rendering. */
				input_delete_callback(refresh_callback);
				update_next_as_time();
				input_add_callback(event_callback);
				mtk_cmd(appid, "l_status.set(-text \"Ready.\")");
				
				if(simple_mode) {
					looped = simple_mode_current;
					while(!suitable_for_simple(patches[simple_mode_current].p)) {
						simple_mode_current++;
						if(simple_mode_current == npatches)
							simple_mode_current = 0;
						if(looped == simple_mode_current)
							break;
					}
					index = simple_mode_current;
				} else
					index = 0;

				if(!guirender(appid, patches[index].p, stop_callback))
					stop_callback();
				return;
			}
		} else {
			int error_patch;

			error_patch = -compiled_patches-1;
			mtk_cmdf(appid, "l_status.set(-text \"Failed to compile patch %s\")", patches[error_patch].filename);
			input_delete_callback(refresh_callback);
			started = 0;
			free_patches();
			fb_unblank();
			return;
		}
		next_update = t + UPDATE_PERIOD;
	}
}

void open_performance_window()
{
	update_buttons();
	mtk_cmd(appid, "w.open()");
}

static rtems_id comp_task_id;

static int check_input_video()
{
	int fd;
	unsigned int status;

	fd = open("/dev/video", O_RDWR);
	if(fd == -1) {
		perror("Unable to open video device");
		return 0;
	}
	ioctl(fd, VIDEO_GET_SIGNAL, &status);
	status &= 0x01;
	close(fd);
	return status;
}

void start_performance(int simple, int dt, int as)
{
	rtems_status_code sc;
	
	if(started) return;
	started = 1;

	input_video = 1;
	simple_mode = simple;
	dt_mode = dt;
	as_mode = as;
	open_performance_window();

	/* build patch list */
	npatches = 0;
	simple_mode_current = 0;
	if(simple) {
		input_video = check_input_video();
		add_simple_patches();
		if(npatches < 1) {
			messagebox("Error", "No patches found!");
			started = 0;
			fb_unblank();
			return;
		}
	} else {
		add_firstpatch();
		if(npatches < 1) {
			messagebox("Error", "No first patch defined!");
			started = 0;
			fb_unblank();
			return;
		}
		add_keyboard_patches();
		add_ir_patches();
		add_midi_patches();
		add_osc_patches();
	}

	/* start patch compilation task */
	compiled_patches = 0;
	mtk_cmd(appid, "l_status.set(-text \"Compiling patches...\")");
	mtk_cmd(appid, "progress.barconfig(load, -value 0)");
	next_update = rtems_clock_get_ticks_since_boot() + UPDATE_PERIOD;
	input_add_callback(refresh_callback);
	sc = rtems_task_create(rtems_build_name('C', 'O', 'M', 'P'), 20, 300*1024,
		RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR,
		0, &comp_task_id);
	assert(sc == RTEMS_SUCCESSFUL);
	sc = rtems_task_start(comp_task_id, comp_task, 0);
	assert(sc == RTEMS_SUCCESSFUL);
}
