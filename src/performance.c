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
#include <stdio.h>
#include <string.h>
#include <rtems.h>
#include <mtklib.h>
#include <keycodes.h>

#include "input.h"
#include "fb.h"
#include "messagebox.h"
#include "config.h"
#include "compiler.h"
#include "renderer.h"
#include "guirender.h"
#include "performance.h"

#define FILENAME_LEN 384

#define MAX_PATCHES 64

struct patch_info {
	char filename[FILENAME_LEN];
	struct patch *p;
};

static int npatches;
static struct patch_info patches[MAX_PATCHES];

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

static int firstpatch;
static int keyboard_patches[26];
static int ir_patches[64];
static int midi_channel;
static int midi_patches[128];
static int osc_patches[64];

static void add_firstpatch()
{
	const char *filename;

	firstpatch = -1;
	filename = config_read_string("firstpatch");
	if(filename == NULL) return;
	firstpatch = add_patch(filename);
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

static int appid;
static int started;

static void close_callback(mtk_event *e, void *arg)
{
	mtk_cmd(appid, "w.close()");
}

void init_performance()
{
	appid = mtk_init_app("Performance");

	mtk_cmd_seq(appid,
		"g = new Grid()",

		"l_text = new Label()",
		"progress = new LoadDisplay()",
		"b_close = new Button(-text \"Close\")",

		"g.place(l_text, -column 1 -row 1)",
		"g.place(progress, -column 1 -row 2)",
		"g.place(b_close, -column 1 -row 3)",
		"g.columnconfig(1, -size 150)",

		"w = new Window(-content g -title \"Performance\")",
		0);

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

	return patch_compile(buf, dummy_rmc);
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

static void event_callback(mtk_event *e, int count)
{
	int i;
	int index;

	for(i=0;i<count;i++) {
		index = -1;
		if(e[i].type == EVENT_TYPE_PRESS) {
			index = keycode_to_index(e[i].press.code);
			if(index != -1)
				index = keyboard_patches[index];
		} else if(e[i].type == EVENT_TYPE_IR) {
			index = e[i].press.code;
			index = ir_patches[index];
		} else if(e[i].type == EVENT_TYPE_MIDI) {
			if(((e[i].press.code & 0x0f00) >> 8) == midi_channel) {
				index = e[i].press.code & 0x7f;
				index = midi_patches[index];
			}
		} else if(e[i].type == EVENT_TYPE_OSC) {
			index = e[i].press.code & 0x3f;
			index = osc_patches[index];
		}
		if(index != -1)
			renderer_set_patch(patches[index].p);
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
	
	t = rtems_clock_get_ticks_since_boot();
	if(t >= next_update) {
		if(compiled_patches >= 0) {
			mtk_cmdf(appid, "progress.barconfig(load, -value %d)", (100*compiled_patches)/npatches);
			if(compiled_patches == npatches) {
				/* All patches compiled. Start rendering. */
				input_delete_callback(refresh_callback);
				input_add_callback(event_callback);
				mtk_cmd(appid, "l_text.set(-text \"Done.\")");
				if(!guirender(appid, patches[firstpatch].p, stop_callback))
					stop_callback();
				return;
			}
		} else {
			int error_patch;

			error_patch = -compiled_patches-1;
			mtk_cmdf(appid, "l_text.set(-text \"Failed to compile patch %s\")", patches[error_patch].filename);
			input_delete_callback(refresh_callback);
			started = 0;
			free_patches();
			fb_unblank();
			return;
		}
		next_update = t + UPDATE_PERIOD;
	}
}

static rtems_id comp_task_id;

void start_performance()
{
	rtems_status_code sc;
	
	if(started) return;
	started = 1;

	/* build patch list */
	npatches = 0;
	add_firstpatch();
	if(npatches < 1) {
		messagebox("Error", "No first patch defined!\n");
		started = 0;
		fb_unblank();
		return;
	}
	add_keyboard_patches();
	add_ir_patches();
	add_midi_patches();
	add_osc_patches();

	/* start patch compilation task */
	compiled_patches = 0;
	mtk_cmd(appid, "l_text.set(-text \"Compiling patches...\")");
	mtk_cmd(appid, "progress.barconfig(load, -value 0)");
	mtk_cmd(appid, "w.open()");
	next_update = rtems_clock_get_ticks_since_boot() + UPDATE_PERIOD;
	input_add_callback(refresh_callback);
	sc = rtems_task_create(rtems_build_name('C', 'O', 'M', 'P'), 20, 64*1024,
		RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR,
		0, &comp_task_id);
	assert(sc == RTEMS_SUCCESSFUL);
	sc = rtems_task_start(comp_task_id, comp_task, 0);
	assert(sc == RTEMS_SUCCESSFUL);
}
