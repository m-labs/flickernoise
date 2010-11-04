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

#include <stdio.h>
#include <mtklib.h>

#include "audio.h"
#include "dmx.h"
#include "patcheditor.h"
#include "monitor.h"
#include "firstpatch.h"
#include "sysettings.h"
#include "about.h"
#include "shutdown.h"

#include "config.h"
#include "filedialog.h"

#include "cp.h"

static int appid;
static int load_appid;
static int save_appid;

static int changed;

void cp_notify_changed()
{
	if(changed) return;
	changed = 1;
	mtk_cmd(appid, "w.set(-title \"Control panel [modified]\")");
}

static void clear_changed()
{
	mtk_cmd(appid, "w.set(-title \"Control panel\")");
	changed = 0;
}

static void loadok_callback(mtk_event *e, void *arg)
{
	char buf[32768];

	get_filedialog_selection(load_appid, buf, 32768);
	config_load(buf);
	clear_changed();
	close_filedialog(load_appid);
}

static void loadcancel_callback(mtk_event *e, void *arg)
{
	close_filedialog(load_appid);
}

static void saveok_callback(mtk_event *e, void *arg)
{
	char buf[32768];

	get_filedialog_selection(save_appid, buf, 32768);
	config_save(buf);
	clear_changed();
	close_filedialog(save_appid);
}

static void savecancel_callback(mtk_event *e, void *arg)
{
	close_filedialog(save_appid);
}

enum {
	CP_ITEM_KEYB,
	CP_ITEM_IR,
	CP_ITEM_AUDIO,
	CP_ITEM_MIDI,
	CP_ITEM_OSC,
	CP_ITEM_DMX,
	CP_ITEM_VIDEOIN,

	CP_ITEM_EDITOR,
	CP_ITEM_MONITOR,

	CP_ITEM_START,
	CP_ITEM_FIRSTPATCH,
	CP_ITEM_LOAD,
	CP_ITEM_SAVE,

	CP_ITEM_SYSETTINGS,
	CP_ITEM_FILEMANAGER,
	CP_ITEM_ABOUT,
	CP_ITEM_SHUTDOWN
};

static void cp_callback(mtk_event *e, void *arg)
{
	switch((int)arg) {
		case CP_ITEM_KEYB:
			printf("keyboard\n");
			break;
		case CP_ITEM_IR:
			printf("ir\n");
			break;
		case CP_ITEM_AUDIO:
			open_audio_window();
			break;
		case CP_ITEM_MIDI:
			printf("midi\n");
			break;
		case CP_ITEM_OSC:
			printf("osc\n");
			break;
		case CP_ITEM_DMX:
			open_dmx_window();
			break;
		case CP_ITEM_VIDEOIN:
			printf("videoin\n");
			break;

		case CP_ITEM_EDITOR:
			open_patcheditor_window();
			break;
		case CP_ITEM_MONITOR:
			open_monitor_window();
			break;

		case CP_ITEM_START:
			printf("start\n");
			break;
		case CP_ITEM_FIRSTPATCH:
			open_firstpatch_window();
			break;
		case CP_ITEM_LOAD:
			open_filedialog(load_appid, "/");
			break;
		case CP_ITEM_SAVE:
			open_filedialog(save_appid, "/");
			break;

		case CP_ITEM_SYSETTINGS:
			open_sysettings_window();
			break;
		case CP_ITEM_FILEMANAGER:
			break;
		case CP_ITEM_ABOUT:
			open_about_window();
			break;
		case CP_ITEM_SHUTDOWN:
			open_shutdown_window();
			break;

		default:
			break;
	}
}

void init_cp()
{
	appid = mtk_init_app("Control panel");

	mtk_cmd_seq(appid,
		"g = new Grid()",

		"g_iosetup0 = new Grid()",
		"l_iosetup = new Label(-text \"Interfaces\" -font \"title\")",
		"s_iosetup1 = new Separator(-vertical no)",
		"s_iosetup2 = new Separator(-vertical no)",
		"g_iosetup0.place(s_iosetup1, -column 1 -row 1)",
		"g_iosetup0.place(l_iosetup, -column 2 -row 1)",
		"g_iosetup0.place(s_iosetup2, -column 3 -row 1)",
		"g_iosetup1 = new Grid()",
		"g_iosetup2 = new Grid()",
		"b_keyb = new Button(-text \"Keyboard\")",
		"b_ir = new Button(-text \"IR remote\")",
		"b_audio = new Button(-text \"Audio\")",
		"b_midi = new Button(-text \"MIDI\")",
		"b_osc = new Button(-text \"OSC\")",
		"b_dmx = new Button(-text \"DMX512\")",
		"b_videoin = new Button(-text \"Video in\")",
		"g_iosetup1.place(b_keyb, -column 1 -row 1)",
		"g_iosetup1.place(b_ir, -column 2 -row 1)",
		"g_iosetup1.place(b_audio, -column 3 -row 1)",
		"g_iosetup2.place(b_midi, -column 1 -row 2)",
		"g_iosetup2.place(b_osc, -column 2 -row 2)",
		"g_iosetup2.place(b_dmx, -column 3 -row 2)",
		"g_iosetup2.place(b_videoin, -column 4 -row 2)",
		"g.place(g_iosetup0, -column 1 -row 1)",
		"g.place(g_iosetup1, -column 1 -row 2)",
		"g.place(g_iosetup2, -column 1 -row 3)",

		"g_patches0 = new Grid()",
		"l_patches = new Label(-text \"Patches\" -font \"title\")",
		"s_patches1 = new Separator(-vertical no)",
		"s_patches2 = new Separator(-vertical no)",
		"g_patches0.place(s_patches1, -column 1 -row 1)",
		"g_patches0.place(l_patches, -column 2 -row 1)",
		"g_patches0.place(s_patches2, -column 3 -row 1)",
		"g_patches = new Grid()",
		"b_editor = new Button(-text \"Patch editor\")",
		"b_monitor = new Button(-text \"Variable monitor\")",
		"g_patches.place(b_editor, -column 1 -row 1)",
		"g_patches.place(b_monitor, -column 2 -row 1)",
		"g.place(g_patches0, -column 1 -row 4)",
		"g.place(g_patches, -column 1 -row 5)",

		"g_performance0 = new Grid()",
		"l_performance = new Label(-text \"Performance\" -font \"title\")",
		"s_performance1 = new Separator(-vertical no)",
		"s_performance2 = new Separator(-vertical no)",
		"g_performance0.place(s_performance1, -column 1 -row 1)",
		"g_performance0.place(l_performance, -column 2 -row 1)",
		"g_performance0.place(s_performance2, -column 3 -row 1)",
		"g_performance = new Grid()",
		"b_start = new Button(-text \"Start!\")",
		"b_firstpatch = new Button(-text \"First patch\")",
		"b_load = new Button(-text \"Load\")",
		"b_save = new Button(-text \"Save\")",
		"g_performance.place(b_firstpatch, -column 1 -row 1)",
		"g_performance.place(b_load, -column 2 -row 1)",
		"g_performance.place(b_save, -column 3 -row 1)",
		"g.place(g_performance0, -column 1 -row 6)",
		"g.place(b_start, -column 1 -row 7)",
		"g.place(g_performance, -column 1 -row 8)",

		"g_system0 = new Grid()",
		"l_system = new Label(-text \"System\" -font \"title\")",
		"s_system1 = new Separator(-vertical no)",
		"s_system2 = new Separator(-vertical no)",
		"g_system0.place(s_system1, -column 1 -row 1)",
		"g_system0.place(l_system, -column 2 -row 1)",
		"g_system0.place(s_system2, -column 3 -row 1)",
		"g_system = new Grid()",
		"b_sysettings = new Button(-text \"Settings\")",
		"b_filemanager = new Button(-text \"File manager\")",
		"b_shutdown = new Button(-text \"Shutdown\")",
		"b_about = new Button(-text \"About\")",
		"g_system.place(b_sysettings, -column 1 -row 1)",
		"g_system.place(b_filemanager, -column 2 -row 1)",
		"g_system.place(b_shutdown, -column 2 -row 2)",
		"g_system.place(b_about, -column 1 -row 2)",
		"g.place(g_system0, -column 1 -row 9)",
		"g.place(g_system, -column 1 -row 10)",

		"w = new Window(-content g -title \"Control panel\" -workx 150 -worky 120)",

		"w.open()",
		0);

	mtk_bind(appid, "b_keyb", "commit", cp_callback, (void *)CP_ITEM_KEYB);
	mtk_bind(appid, "b_ir", "commit", cp_callback, (void *)CP_ITEM_IR);
	mtk_bind(appid, "b_audio", "commit", cp_callback, (void *)CP_ITEM_AUDIO);
	mtk_bind(appid, "b_midi", "commit", cp_callback, (void *)CP_ITEM_MIDI);
	mtk_bind(appid, "b_osc", "commit", cp_callback, (void *)CP_ITEM_OSC);
	mtk_bind(appid, "b_dmx", "commit", cp_callback, (void *)CP_ITEM_DMX);
	mtk_bind(appid, "b_videoin", "commit", cp_callback, (void *)CP_ITEM_VIDEOIN);
	mtk_bind(appid, "b_editor", "commit", cp_callback, (void *)CP_ITEM_EDITOR);
	mtk_bind(appid, "b_monitor", "commit", cp_callback, (void *)CP_ITEM_MONITOR);
	mtk_bind(appid, "b_start", "commit", cp_callback, (void *)CP_ITEM_START);
	mtk_bind(appid, "b_firstpatch", "commit", cp_callback, (void *)CP_ITEM_FIRSTPATCH);
	mtk_bind(appid, "b_load", "commit", cp_callback, (void *)CP_ITEM_LOAD);
	mtk_bind(appid, "b_save", "commit", cp_callback, (void *)CP_ITEM_SAVE);
	mtk_bind(appid, "b_sysettings", "commit", cp_callback, (void *)CP_ITEM_SYSETTINGS);
	mtk_bind(appid, "b_filemanager", "commit", cp_callback, (void *)CP_ITEM_FILEMANAGER);
	mtk_bind(appid, "b_about", "commit", cp_callback, (void *)CP_ITEM_ABOUT);
	mtk_bind(appid, "b_shutdown", "commit", cp_callback, (void *)CP_ITEM_SHUTDOWN);

	mtk_bind(appid, "w", "close", cp_callback, (void *)CP_ITEM_SHUTDOWN);

	load_appid = create_filedialog("Load performance", 0, loadok_callback, NULL, loadcancel_callback, NULL);
	save_appid = create_filedialog("Save performance", 1, saveok_callback, NULL, savecancel_callback, NULL);
}
