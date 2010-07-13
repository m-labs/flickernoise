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

/* Inspired by "test.c" from Genode FX (DOpE-embedded demo application) */

#ifdef RTEMS
#include <bsp.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#include <dopelib.h>
#include <vscreen.h>
#ifndef RTEMS
#include <SDL.h> /* for SDL_Quit */
#endif

#include "patcheditor.h"
#include "monitor.h"
#include "shutdown.h"
#include "flash.h"
#include "filedialog.h"


static long appid;
long fileDialog_id;
char fileDialogPath[8192];


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
	CP_ITEM_LOAD,
	CP_ITEM_SAVE,

	CP_ITEM_AUTOSTART,
	CP_ITEM_FILEMANAGER,
	CP_ITEM_SHUTDOWN,

	CP_ITEM_FLASH
};

static void cp_callback(dope_event *e, void *arg)
{
	switch((int)arg) {
		case CP_ITEM_KEYB:
			printf("keyboard\n");
			break;
		case CP_ITEM_IR:
			printf("ir\n");
			break;
		case CP_ITEM_AUDIO:
			printf("audio\n");
			break;
		case CP_ITEM_MIDI:
			printf("midi\n");
			break;
		case CP_ITEM_OSC:
			printf("osc\n");
			break;
		case CP_ITEM_DMX:
			printf("dmx\n");
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
		case CP_ITEM_LOAD:
			printf("load\n");
			break;
		case CP_ITEM_SAVE:
			printf("save\n");
			break;
		case CP_ITEM_AUTOSTART:
			printf("autostart\n");
			break;
		case CP_ITEM_FILEMANAGER:
			open_filedialog(fileDialog_id, "/");	// TODO
			break;
		case CP_ITEM_SHUTDOWN:
			open_shutdown_window();
			break;
		case CP_ITEM_FLASH:
			open_flash_window();
			break;
		default:
			// TODO
			break;

	}
}

static void init_cp()
{
	appid = dope_init_app("Control panel");

	dope_cmd_seq(appid,
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
		"b_load = new Button(-text \"Load\")",
		"b_save = new Button(-text \"Save\")",
		"g_performance.place(b_load, -column 1 -row 1)",
		"g_performance.place(b_save, -column 2 -row 1)",
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
		"b_autostart = new Button(-text \"Autostart\")",
		"b_filemanager = new Button(-text \"File manager\")",
		"b_shutdown = new Button(-text \"Shutdown\")",
		"b_flash = new Button(-text \"Flash\")",
		"g_system.place(b_autostart, -column 1 -row 1)",
		"g_system.place(b_filemanager, -column 2 -row 1)",
		"g_system.place(b_shutdown, -column 2 -row 2)",
		"g_system.place(b_flash, -column 1 -row 2)",
		"g.place(g_system0, -column 1 -row 9)",
		"g.place(g_system, -column 1 -row 10)",

		"w = new Window(-content g -title \"Control panel\" -workx 150 -worky 120)",

		"w.open()",
		0);

	dope_bind(appid, "b_keyb", "commit", cp_callback, (void *)CP_ITEM_KEYB);
	dope_bind(appid, "b_ir", "commit", cp_callback, (void *)CP_ITEM_IR);
	dope_bind(appid, "b_audio", "commit", cp_callback, (void *)CP_ITEM_AUDIO);
	dope_bind(appid, "b_midi", "commit", cp_callback, (void *)CP_ITEM_MIDI);
	dope_bind(appid, "b_osc", "commit", cp_callback, (void *)CP_ITEM_OSC);
	dope_bind(appid, "b_dmx", "commit", cp_callback, (void *)CP_ITEM_DMX);
	dope_bind(appid, "b_videoin", "commit", cp_callback, (void *)CP_ITEM_VIDEOIN);
	dope_bind(appid, "b_editor", "commit", cp_callback, (void *)CP_ITEM_EDITOR);
	dope_bind(appid, "b_monitor", "commit", cp_callback, (void *)CP_ITEM_MONITOR);
	dope_bind(appid, "b_start", "commit", cp_callback, (void *)CP_ITEM_START);
	dope_bind(appid, "b_load", "commit", cp_callback, (void *)CP_ITEM_LOAD);
	dope_bind(appid, "b_save", "commit", cp_callback, (void *)CP_ITEM_SAVE);
	dope_bind(appid, "b_autostart", "commit", cp_callback, (void *)CP_ITEM_AUTOSTART);
	dope_bind(appid, "b_filemanager", "commit", cp_callback, (void *)CP_ITEM_FILEMANAGER);
	dope_bind(appid, "b_shutdown", "commit", cp_callback, (void *)CP_ITEM_SHUTDOWN);

	dope_bind(appid, "b_flash", "commit", cp_callback, (void *)CP_ITEM_FLASH);

	dope_bind(appid, "w", "close", cp_callback, (void *)CP_ITEM_SHUTDOWN);
}

void filedialog_ok_callback(dope_event *e, void *arg)
{
	char filepath[384];
	get_filedialog_selection(fileDialog_id, filepath, sizeof(filepath));
	close_filedialog(fileDialog_id);

	printf("filedialog_ok_callback : %s\n", filepath);
}

void filedialog_cancel_callback(dope_event *e, void *arg)
{
	printf("filedialog_cancel_callback\n");
	close_filedialog(fileDialog_id);
}



int main(int argc, char *argv[])
{
	if(dope_init()) return 2;
	atexit(dope_deinit);
#ifndef RTEMS
	atexit(SDL_Quit); /* FIXME: this should be done by DoPE */
#endif
	
	init_cp();
	init_patcheditor();
	init_monitor();
	init_shutdown();
	init_flash();

	fileDialog_id = create_filedialog("Filedialog", 0, filedialog_ok_callback, fileDialogPath,filedialog_cancel_callback, NULL);

	dope_eventloop(0);

	return 0;
}

#ifdef RTEMS
void *POSIX_Init(void *argument)
{
	main(0, NULL);
	return NULL;
}

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_FRAME_BUFFER_DRIVER
#define CONFIGURE_EXTRA_TASK_STACKS 1900
#define CONFIGURE_LIBIO_MAXIMUM_FILE_DESCRIPTORS 4
#define CONFIGURE_MAXIMUM_POSIX_THREADS         1
#define CONFIGURE_POSIX_INIT_THREAD_TABLE
#define CONFIGURE_MAXIMUM_POSIX_MUTEXES 1
#define CONFIGURE_INIT
#include <rtems/confdefs.h>
#endif
