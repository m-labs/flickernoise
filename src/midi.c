/*
 * Flickernoise
 * Copyright (C) 2010 Sebastien Bourdeauducq
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

#include <bsp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mtklib.h>

#include "util.h"
#include "input.h"
#include "config.h"
#include "cp.h"
#include "messagebox.h"
#include "filedialog.h"

#include "midi.h"

static void strmidi(char *buf, int n)
{
	int octave, note;

	octave = (n / 12) - 2;
	note = n % 12;

	switch(note) {
		case 0:
			*(buf++) = 'C';
			break;
		case 1:
			*(buf++) = 'C';
			*(buf++) = '#';
			break;
		case 2:
			*(buf++) = 'D';
			break;
		case 3:
			*(buf++) = 'D';
			*(buf++) = '#';
			break;
		case 4:
			*(buf++) = 'E';
			break;
		case 5:
			*(buf++) = 'F';
			break;
		case 6:
			*(buf++) = 'F';
			*(buf++) = '#';
			break;
		case 7:
			*(buf++) = 'G';
			break;
		case 8:
			*(buf++) = 'G';
			*(buf++) = '#';
			break;
		case 9:
			*(buf++) = 'A';
			break;
		case 10:
			*(buf++) = 'A';
			*(buf++) = '#';
			break;
		case 11:
			*(buf++) = 'B';
			break;
	}
	sprintf(buf, "%d", octave);
}

static int midistr(const char *str)
{
	int octave, note;
	char *c;
	int r;
	
	switch(*str) {
		case 'c':
		case 'C':
			note = 0;
			break;
		case 'd':
		case 'D':
			note = 2;
			break;
		case 'e':
		case 'E':
			note = 4;
			break;
		case 'f':
		case 'F':
			note = 5;
			break;
		case 'g':
		case 'G':
			note = 7;
			break;
		case 'a':
		case 'A':
			note = 9;
			break;
		case 'b':
		case 'B':
			note = 11;
			break;
		default:
			return -1;
	}
	str++;

	octave = 0;
	if(*str == '#') {
		note++;
		if(note == 12) {
			octave++;
			note = 0;
		}
		str++;
	} else if(*str == 'b') {
		note--;
		if(note == -1) {
			octave--;
			note = 11;
		}
		str++;
	}

	octave += strtol(str, &c, 0);
	if(*c != 0) return -1;

	r = 12*(octave + 2) + note;
	if(r < 0) r = 0;
	if(r > 127) r = 127;

	return r;
}

static int appid;
static int browse_appid;

static int w_open;

static char midi_bindings[128][384];

static void load_config()
{
	char config_key[8];
	int i;
	const char *val;

	mtk_cmdf(appid, "e_channel.set(-text \"%d\")", config_read_int("midi_channel", 0));
	for(i=0;i<128;i++) {
		sprintf(config_key, "midi_%02x", i);
		val = config_read_string(config_key);
		if(val != NULL)
			strcpy(midi_bindings[i], val);
		else
			midi_bindings[i][0] = 0;
	}
}

static void set_config()
{
	char config_key[8];
	int i;

	for(i=0;i<128;i++) {
		sprintf(config_key, "midi_%02x", i);
		if(midi_bindings[i][0] != 0)
			config_write_string(config_key, midi_bindings[i]);
		else
			config_delete(config_key);
	}
	config_write_int("midi_channel", mtk_req_i(appid, "e_channel.text"));
}

static void update_list()
{
	char str[32768];
	char note[16];
	char *p;
	int i;

	str[0] = 0;
	p = str;
	for(i=0;i<128;i++) {
		if(midi_bindings[i][0] != 0) {
			strmidi(note, i);
			p += sprintf(p, "%s: %s\n", note, midi_bindings[i]);
		}
	}
	/* remove last \n */
	if(p != str) {
		p--;
		*p = 0;
	}
	mtk_cmdf(appid, "lst_existing.set(-text \"%s\" -selection 0)", str);
}

static void selchange_callback(mtk_event *e, void *arg)
{
	int sel;
	int count;
	int i;
	char note[16];

	sel = mtk_req_i(appid, "lst_existing.selection");
	count = 0;
	for(i=0;i<128;i++) {
		if(midi_bindings[i][0] != 0) {
			if(count == sel)
				break;
			count++;
		}
	}
	if(count != 0) {
		strmidi(note, i);
		mtk_cmdf(appid, "e_note.set(-text \"%s\")", note);
		mtk_cmdf(appid, "e_filename.set(-text \"%s\")", midi_bindings[i]);
	}
}

static int capturing;

static void midi_event(mtk_event *e, int count)
{
	int i;
	char note[16];

	for(i=0;i<count;i++) {
		if(e[i].type == EVENT_TYPE_MIDI) {
			if(((e[i].press.code & 0x0f00) >> 8) == mtk_req_i(appid, "e_channel.text")) {
				strmidi(note, e[i].press.code & 0x7f);
				mtk_cmdf(appid, "e_note.set(-text \"%s\")", note);
				mtk_cmd(appid, "b_note.set(-state off)");
				capturing = 0;
				input_delete_callback(midi_event);
				break;
			}
		}
	}
}

static void capture_callback(mtk_event *e, void *arg)
{
	if(capturing) {
		input_delete_callback(midi_event);
		mtk_cmd(appid, "b_note.set(-state off)");
	} else {
		input_add_callback(midi_event);
		mtk_cmd(appid, "b_note.set(-state on)");
	}
	capturing = !capturing;
}

static void close_window()
{
	mtk_cmd(appid, "w.close()");
	if(capturing)
		capture_callback(NULL, NULL);
	w_open = 0;
	close_filedialog(browse_appid);
}

static void ok_callback(mtk_event *e, void *arg)
{
	set_config();
	cp_notify_changed();
	close_window();
}

static void cancel_callback(mtk_event *e, void *arg)
{
	close_window();
}

static void browse_ok_callback()
{
	char filename[384];

	get_filedialog_selection(browse_appid, filename, sizeof(filename));
	close_filedialog(browse_appid);
	mtk_cmdf(appid, "e_filename.set(-text \"%s\")", filename);
}

static void browse_cancel_callback()
{
	close_filedialog(browse_appid);
}

static void browse_callback(mtk_event *e, void *arg)
{
	open_filedialog(browse_appid, "/");
}

static void clear_callback(mtk_event *e, void *arg)
{
	mtk_cmd(appid, "e_filename.set(-text \"\")");
}

static void addupdate_callback(mtk_event *e, void *arg)
{
	char note[8];
	int notecode;
	char filename[384];

	mtk_req(appid, note, sizeof(note), "e_note.text");
	mtk_req(appid, filename, sizeof(filename), "e_filename.text");
	notecode = midistr(note);
	if(notecode < 0) {
		messagebox("Error", "Invalid note.");
		return;
	}
	strcpy(midi_bindings[notecode], filename);
	update_list();
	mtk_cmd(appid, "e_note.set(-text \"\")");
	mtk_cmd(appid, "e_filename.set(-text \"\")");
}

void init_midi()
{
	appid = mtk_init_app("MIDI");

	mtk_cmd_seq(appid,
		"g = new Grid()",

		"g_parameters0 = new Grid()",
		"l_parameters = new Label(-text \"Parameters\" -font \"title\")",
		"s_parameters1 = new Separator(-vertical no)",
		"s_parameters2 = new Separator(-vertical no)",
		"g_parameters0.place(s_parameters1, -column 1 -row 1)",
		"g_parameters0.place(l_parameters, -column 2 -row 1)",
		"g_parameters0.place(s_parameters2, -column 3 -row 1)",
		"g_parameters = new Grid()",
		"l_channel = new Label(-text \"Channel (0-15):\")",
		"e_channel = new Entry()",
		"g_parameters.place(l_channel, -column 1 -row 1)",
		"g_parameters.place(e_channel, -column 2 -row 1)",

		"g_existing0 = new Grid()",
		"l_existing = new Label(-text \"Existing bindings\" -font \"title\")",
		"s_existing1 = new Separator(-vertical no)",
		"s_existing2 = new Separator(-vertical no)",
		"g_existing0.place(s_existing1, -column 1 -row 1)",
		"g_existing0.place(l_existing, -column 2 -row 1)",
		"g_existing0.place(s_existing2, -column 3 -row 1)",
		"lst_existing = new List()",
		"lst_existingf = new Frame(-content lst_existing -scrollx no -scrolly yes)",

		"g_addedit0 = new Grid()",
		"l_addedit = new Label(-text \"Add/edit\" -font \"title\")",
		"s_addedit1 = new Separator(-vertical no)",
		"s_addedit2 = new Separator(-vertical no)",
		"g_addedit0.place(s_addedit1, -column 1 -row 1)",
		"g_addedit0.place(l_addedit, -column 2 -row 1)",
		"g_addedit0.place(s_addedit2, -column 3 -row 1)",
		
		"g_addedit1 = new Grid()",
		"l_note = new Label(-text \"Note:\")",
		"e_note = new Entry()",
		"b_note = new Button(-text \"Capture\")",
		"g_addedit1.place(l_note, -column 1 -row 1)",
		"g_addedit1.place(e_note, -column 2 -row 1)",
		"g_addedit1.place(b_note, -column 3 -row 1)",
		"l_filename = new Label(-text \"Filename:\")",
		"e_filename = new Entry()",
		"b_filename = new Button(-text \"Browse\")",
		"b_filenameclear = new Button(-text \"Clear\")",
		"g_addedit1.place(l_filename, -column 1 -row 2)",
		"g_addedit1.place(e_filename, -column 2 -row 2)",
		"g_addedit1.place(b_filename, -column 3 -row 2)",
		"g_addedit1.place(b_filenameclear, -column 4 -row 2)",
		"g_addedit1.columnconfig(3, -size 0)",
		"g_addedit1.columnconfig(4, -size 0)",
		"b_addupdate = new Button(-text \"Add/update\")",

		"g_btn = new Grid()",
		"b_ok = new Button(-text \"OK\")",
		"b_cancel = new Button(-text \"Cancel\")",
		"g_btn.columnconfig(1, -size 200)",
		"g_btn.place(b_ok, -column 2 -row 1)",
		"g_btn.place(b_cancel, -column 3 -row 1)",

		"g.place(g_parameters0, -column 1 -row 1)",
		"g.place(g_parameters, -column 1 -row 2)",
		"g.place(g_existing0, -column 1 -row 3)",
		"g.place(lst_existingf, -column 1 -row 4)",
		"g.rowconfig(4, -size 130)",
		"g.place(g_addedit0, -column 1 -row 5)",
		"g.place(g_addedit1, -column 1 -row 6)",
		"g.place(b_addupdate, -column 1 -row 7)",
		"g.rowconfig(8, -size 10)",
		"g.place(g_btn, -column 1 -row 9)",

		"w = new Window(-content g -title \"MIDI settings\")",
		0);

	mtk_bind(appid, "lst_existing", "selchange", selchange_callback, NULL);
	mtk_bind(appid, "lst_existing", "selcommit", selchange_callback, NULL);
	mtk_bind(appid, "b_note", "press", capture_callback, NULL);
	mtk_bind(appid, "b_filename", "commit", browse_callback, NULL);
	mtk_bind(appid, "b_filenameclear", "commit", clear_callback, NULL);
	mtk_bind(appid, "b_addupdate", "commit", addupdate_callback, NULL);

	mtk_bind(appid, "b_ok", "commit", ok_callback, NULL);
	mtk_bind(appid, "b_cancel", "commit", cancel_callback, NULL);

	mtk_bind(appid, "w", "close", cancel_callback, NULL);

	browse_appid = create_filedialog("MIDI patch select", 0, browse_ok_callback, NULL, browse_cancel_callback, NULL);
}

void open_midi_window()
{
	if(w_open) return;
	w_open = 1;
	load_config();
	update_list();
	mtk_cmd(appid, "w.open()");
}
