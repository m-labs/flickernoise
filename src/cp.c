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

#include <rtems.h>
#include <stdio.h>
#include <mtklib.h>

#include "performance.h"
#include "keyboard.h"
#include "ir.h"
#include "fb.h"
#include "audio.h"
#include "midi.h"
#include "oscsettings.h"
#include "dmx.h"
#include "videoin.h"
#include "rsswall.h"
#include "patcheditor.h"
#include "monitor.h"
#include "firstpatch.h"
#include "filemanager.h"
#ifdef WITH_PDF
#include "pdfreader.h"
#endif
#include "sysettings.h"
#include "about.h"
#include "shutdown.h"

#include "config.h"
#include "filedialog.h"
#include "messagebox.h"
#include "sysconfig.h"

#include "cp.h"

static int appid;
static struct filedialog *load_dlg;
static struct filedialog *save_dlg;

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

static void on_config_change()
{
	/* reload config for controls that need modification of some state */
	load_audio_config();
	load_dmx_config();
}

static void loadok_callback(void *arg)
{
	char buf[8192];

	get_filedialog_selection(load_dlg, buf, 8192);
	if(!config_load(buf)) {
		messagebox("Performance load", "Unable to load the performance file");
		return;
	}
	on_config_change();
	clear_changed();
}

static void saveok_callback(void *arg)
{
	char buf[4096];

	get_filedialog_selection(save_dlg, buf, sizeof(buf));
	config_save(buf);
	clear_changed();
}

enum {
	CP_ITEM_KEYB,
	CP_ITEM_IR,
	CP_ITEM_AUDIO,
	CP_ITEM_MIDI,
	CP_ITEM_OSC,
	CP_ITEM_DMX,
	CP_ITEM_VIDEOIN,
	
	CP_ITEM_RSSWALL,
	CP_ITEM_WEBUPDATE,

	CP_ITEM_EDITOR,
	CP_ITEM_MONITOR,

	CP_ITEM_NEW,
	CP_ITEM_LOAD,
	CP_ITEM_SAVE,
	CP_ITEM_FIRSTPATCH,
	CP_ITEM_START,
	CP_ITEM_STARTSIMPLE,

	CP_ITEM_FILEMANAGER,
	CP_ITEM_PDFREADER,

	CP_ITEM_SYSETTINGS,
	CP_ITEM_ABOUT,
	CP_ITEM_SHUTDOWN
};

static void cp_callback(mtk_event *e, void *arg)
{
	switch((int)arg) {
		case CP_ITEM_KEYB:
			open_keyboard_window();
			break;
		case CP_ITEM_IR:
			open_ir_window();
			break;
		case CP_ITEM_AUDIO:
			open_audio_window();
			break;
		case CP_ITEM_MIDI:
			open_midi_window();
			break;
		case CP_ITEM_OSC:
			open_oscsettings_window();
			break;
		case CP_ITEM_DMX:
			open_dmx_window();
			break;
		case CP_ITEM_VIDEOIN:
			open_videoin_window();
			break;

		case CP_ITEM_RSSWALL:
			open_rsswall_window();
			break;
		case CP_ITEM_WEBUPDATE:
			break;

		case CP_ITEM_EDITOR:
			open_patcheditor_window();
			break;
		case CP_ITEM_MONITOR:
			open_monitor_window();
			break;

		case CP_ITEM_NEW:
			config_free();
			on_config_change();
			clear_changed();
			break;
		case CP_ITEM_LOAD:
			open_filedialog(load_dlg);
			break;
		case CP_ITEM_SAVE:
			open_filedialog(save_dlg);
			break;
		case CP_ITEM_FIRSTPATCH:
			open_firstpatch_window();
			break;
		case CP_ITEM_START:
			start_performance(false);
			break;
		case CP_ITEM_STARTSIMPLE:
			start_performance(true);
			break;

		case CP_ITEM_FILEMANAGER:
			open_filemanager_window();
			break;
#ifdef WITH_PDF
		case CP_ITEM_PDFREADER:
			open_pdfreader_window();
			break;
#endif

		case CP_ITEM_SYSETTINGS:
			open_sysettings_window();
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
		
		"g_online0 = new Grid()",
		"l_online = new Label(-text \"Online\" -font \"title\")",
		"s_online1 = new Separator(-vertical no)",
		"s_online2 = new Separator(-vertical no)",
		"g_online0.place(s_online1, -column 1 -row 1)",
		"g_online0.place(l_online, -column 2 -row 1)",
		"g_online0.place(s_online2, -column 3 -row 1)",
		"g_online = new Grid()",
		"b_rsswall = new Button(-text \"RSS wall\")",
		"b_webupdate = new Button(-text \"Web update\")",
		"g_online.place(b_rsswall, -column 1 -row 1)",
		"g_online.place(b_webupdate, -column 2 -row 1)",
		"g.place(g_online0, -column 1 -row 4)",
		"g.place(g_online, -column 1 -row 5)",

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
		"g.place(g_patches0, -column 1 -row 6)",
		"g.place(g_patches, -column 1 -row 7)",

		"g_performance0 = new Grid()",
		"l_performance = new Label(-text \"Performance\" -font \"title\")",
		"s_performance1 = new Separator(-vertical no)",
		"s_performance2 = new Separator(-vertical no)",
		"g_performance0.place(s_performance1, -column 1 -row 1)",
		"g_performance0.place(l_performance, -column 2 -row 1)",
		"g_performance0.place(s_performance2, -column 3 -row 1)",
		"g_performance = new Grid()",
		"b_new = new Button(-text \"New\")",
		"b_load = new Button(-text \"Load\")",
		"b_save = new Button(-text \"Save\")",
		"b_firstpatch = new Button(-text \"First patch\")",
		"g_start = new Grid()",
		"b_start = new Button(-text \"Start!\")",
		"b_startsimple = new Button(-text \"Simple mode\")",
		"g_start.place(b_start, -column 1 -row 1)",
		"g_start.place(b_startsimple, -column 2 -row 1)",
		"g_performance.place(b_new, -column 1 -row 1)",
		"g_performance.place(b_load, -column 2 -row 1)",
		"g_performance.place(b_save, -column 3 -row 1)",
		"g.place(g_performance0, -column 1 -row 8)",
		"g.place(g_performance, -column 1 -row 9)",
		"g.place(b_firstpatch, -column 1 -row 10)",
		"g.place(g_start, -column 1 -row 11)",

		"g_tools0 = new Grid()",
		"l_tools = new Label(-text \"Tools\" -font \"title\")",
		"s_tools1 = new Separator(-vertical no)",
		"s_tools2 = new Separator(-vertical no)",
		"g_tools0.place(s_tools1, -column 1 -row 1)",
		"g_tools0.place(l_tools, -column 2 -row 1)",
		"g_tools0.place(s_tools2, -column 3 -row 1)",
		"g_tools = new Grid()",
		"b_filemanager = new Button(-text \"File manager\")",
#ifdef WITH_PDF
		"b_pdfreader = new Button(-text \"PDF reader\")",
#endif
		"g_tools.place(b_filemanager, -column 1 -row 1)",
#ifdef WITH_PDF
		"g_tools.place(b_pdfreader, -column 2 -row 1)",
#endif
		"g.place(g_tools0, -column 1 -row 12)",
		"g.place(g_tools, -column 1 -row 13)",

		"g_system0 = new Grid()",
		"l_system = new Label(-text \"System\" -font \"title\")",
		"s_system1 = new Separator(-vertical no)",
		"s_system2 = new Separator(-vertical no)",
		"g_system0.place(s_system1, -column 1 -row 1)",
		"g_system0.place(l_system, -column 2 -row 1)",
		"g_system0.place(s_system2, -column 3 -row 1)",
		"g_system = new Grid()",
		"b_sysettings = new Button(-text \"Settings\")",
		"b_about = new Button(-text \"About\")",
		"b_shutdown = new Button(-text \"Shutdown\")",
		"g_system.place(b_about, -column 1 -row 1)",
		"g_system.place(b_shutdown, -column 2 -row 1)",
		"g.place(g_system0, -column 1 -row 14)",
		"g.place(b_sysettings, -column 1 -row 15)",
		"g.place(g_system, -column 1 -row 16)",

		"w = new Window(-content g -title \"Control panel\" -workx 150 -worky 90)",

		"w.open()",
		0);

	mtk_bind(appid, "b_keyb", "commit", cp_callback, (void *)CP_ITEM_KEYB);
	mtk_bind(appid, "b_ir", "commit", cp_callback, (void *)CP_ITEM_IR);
	mtk_bind(appid, "b_audio", "commit", cp_callback, (void *)CP_ITEM_AUDIO);
	mtk_bind(appid, "b_midi", "commit", cp_callback, (void *)CP_ITEM_MIDI);
	mtk_bind(appid, "b_osc", "commit", cp_callback, (void *)CP_ITEM_OSC);
	mtk_bind(appid, "b_dmx", "commit", cp_callback, (void *)CP_ITEM_DMX);
	mtk_bind(appid, "b_videoin", "commit", cp_callback, (void *)CP_ITEM_VIDEOIN);
	mtk_bind(appid, "b_rsswall", "commit", cp_callback, (void *)CP_ITEM_RSSWALL);
	mtk_bind(appid, "b_webupdate", "commit", cp_callback, (void *)CP_ITEM_WEBUPDATE);
	mtk_bind(appid, "b_editor", "commit", cp_callback, (void *)CP_ITEM_EDITOR);
	mtk_bind(appid, "b_monitor", "commit", cp_callback, (void *)CP_ITEM_MONITOR);
	mtk_bind(appid, "b_new", "commit", cp_callback, (void *)CP_ITEM_NEW);
	mtk_bind(appid, "b_load", "commit", cp_callback, (void *)CP_ITEM_LOAD);
	mtk_bind(appid, "b_save", "commit", cp_callback, (void *)CP_ITEM_SAVE);
	mtk_bind(appid, "b_firstpatch", "commit", cp_callback, (void *)CP_ITEM_FIRSTPATCH);
	mtk_bind(appid, "b_start", "commit", cp_callback, (void *)CP_ITEM_START);
	mtk_bind(appid, "b_startsimple", "commit", cp_callback, (void *)CP_ITEM_STARTSIMPLE);
	mtk_bind(appid, "b_filemanager", "commit", cp_callback, (void *)CP_ITEM_FILEMANAGER);
#ifdef WITH_PDF
	mtk_bind(appid, "b_pdfreader", "commit", cp_callback, (void *)CP_ITEM_PDFREADER);
#endif
	mtk_bind(appid, "b_sysettings", "commit", cp_callback, (void *)CP_ITEM_SYSETTINGS);
	mtk_bind(appid, "b_about", "commit", cp_callback, (void *)CP_ITEM_ABOUT);
	mtk_bind(appid, "b_shutdown", "commit", cp_callback, (void *)CP_ITEM_SHUTDOWN);

	mtk_bind(appid, "w", "close", cp_callback, (void *)CP_ITEM_SHUTDOWN);

	load_dlg = create_filedialog("Load performance", 0, "per", loadok_callback, NULL, NULL, NULL);
	save_dlg = create_filedialog("Save performance", 1, "per", saveok_callback, NULL, NULL, NULL);
}

void cp_autostart()
{
	char autostart[256];

	switch(sysconfig_get_autostart_mode()) {
		case SC_AUTOSTART_NONE:
			break;
		case SC_AUTOSTART_SIMPLE:
			start_performance(true);
			break;
		case SC_AUTOSTART_FILE:
			sysconfig_get_autostart(autostart);
			if(autostart[0] == 0) {
				messagebox("Autostart failed", "No performance file specified.");
				fb_unblank();
				return;
			}
			if(!config_load(autostart)) {
				messagebox("Autostart failed", "Unable to load the specified performance file.\nCheck the 'Autostart' section in the 'System settings' dialog box.");
				fb_unblank();
				return;
			}
			on_config_change();
			start_performance(false);
			break;
	}
}
