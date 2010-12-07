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
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <mtklib.h>
#include <rtems.h>
#include <bsp/milkymist_usbinput.h>
#include <bsp/milkymist_ac97.h>
#include <bsp/milkymist_pfpu.h>
#include <bsp/milkymist_tmu.h>
#include <bsp/milkymist_memcard.h>
#include <bsp/milkymist_flash.h>
#include <bsp/milkymist_dmx.h>
#include <bsp/milkymist_ir.h>
#include <bsp/milkymist_midi.h>
#include <bsp/milkymist_versions.h>
#include <bsp/milkymist_gpio.h>
#include <rtems/stackchk.h>
#include <rtems/shell.h>
#include <rtems/bdpart.h>
#include <rtems/rtems_bsdnet.h>
#include <rtems/ftpd.h>
#include <rtems/telnetd.h>
#include <yaffs.h>

#include "shellext.h"
#include "sysconfig.h"
#include "pngload.h"
#include "fb.h"
#include "input.h"
#include "reboot.h"
#include "osc.h"
#include "messagebox.h"
#include "performance.h"
#include "cp.h"
#include "keyboard.h"
#include "ir.h"
#include "audio.h"
#include "midi.h"
#include "oscsettings.h"
#include "dmxspy.h"
#include "dmxtable.h"
#include "dmx.h"
#include "patcheditor.h"
#include "firstpatch.h"
#include "monitor.h"
#include "sysettings.h"
#include "shutdown.h"
#include "about.h"
#include "flash.h"
#include "filedialog.h"

static rtems_task gui_task(rtems_task_argument argument)
{
	config_wallpaper_bitmap = png_load("/memcard/wallpaper.png", &config_wallpaper_w, &config_wallpaper_h);
	if(config_wallpaper_bitmap == NULL)
		config_wallpaper_bitmap = png_load("/flash/wallpaper.png", &config_wallpaper_w, &config_wallpaper_h);
	init_fb_mtk();
	init_input();
	input_add_callback(mtk_input);
	init_reboot();
	init_osc();
	init_messagebox();
	init_performance();
	init_cp();
	init_keyboard();
	init_ir();
	init_audio();
	init_midi();
	init_oscsettings();
	init_dmxspy();
	init_dmxtable();
	init_dmx();
	init_patcheditor();
	init_monitor();
	init_firstpatch();
	init_sysettings();
	init_about();
	init_flash();
	init_shutdown();

	cp_autostart();

	/* FIXME: work around "black screen" bug in MTK */
	mtk_cmd(1, "screen.refresh()");
	
	input_eventloop();
}

static rtems_id gui_task_id;

static void start_memcard()
{
	rtems_status_code sc;
	rtems_bdpart_format format;
	rtems_bdpart_partition pt[RTEMS_BDPART_PARTITION_NUMBER_HINT];
	size_t count;
	
	count = RTEMS_BDPART_PARTITION_NUMBER_HINT;
	sc = rtems_bdpart_read("/dev/memcard", &format, pt, &count);
	if(sc != RTEMS_SUCCESSFUL)
		return;
	sc = rtems_bdpart_register("/dev/memcard", pt, count);
	if(sc != RTEMS_SUCCESSFUL)
		return;

	mkdir("/memcard", 0777);
	mount("/dev/memcard1", "/memcard", "dosfs", 0, "");
}

rtems_task Init(rtems_task_argument argument)
{
	rtems_status_code sc;

	/* FIXME: can this be moved into the initialization table? */
	memcard_register();

	mkdir("/ramdisk", 0777);
	start_memcard();
	mkdir("/flash", 0777);
	mount("/dev/flash5", "/flash", "yaffs", 0, "");

	sysconfig_load();
	rtems_bsdnet_initialize_network();
	rtems_initialize_ftpd();
	rtems_telnetd_initialize();

	sc = rtems_task_create(rtems_build_name('G', 'U', 'I', ' '), 9, 1024*1024,
		RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR,
		0, &gui_task_id);
	assert(sc == RTEMS_SUCCESSFUL);
	sc = rtems_task_start(gui_task_id, gui_task, 0);
	assert(sc == RTEMS_SUCCESSFUL);

	sc = rtems_shell_init(
		"SHLL",
		RTEMS_MINIMUM_STACK_SIZE * 8,
		1, /* We want it to work */
		"/dev/console",
		false,
		false,
		NULL
	);
	if(sc != RTEMS_SUCCESSFUL)
		printf("Unable to start shell (error code %d)\n", sc);

	rtems_task_delete(RTEMS_SELF);
}

static struct rtems_ftpd_hook ftp_hooks[] = {
	{NULL, NULL}
};

struct rtems_ftpd_configuration rtems_ftpd_configuration = {
	20,			/* FTPD task priority */
	512*1024,		/* Maximum hook 'file' size */
	0,			/* Use default port */
	ftp_hooks,		/* Local ftp hooks */
	NULL,			/* Root */
	3,			/* Task count */
	3600,			/* Idle timeout */
	0,			/* Access */
	sysconfig_login_check	/* Login */
};

// FIXME: those functions should be exported by RTEMS
extern rtems_shell_env_t *rtems_shell_init_env(
  rtems_shell_env_t *shell_env_p
);
extern void rtems_shell_env_free(
  void *ptr
);

static void rtems_telnet_shell(char *pty_name, void *cmd_arg)
{
	rtems_shell_env_t *shell_env;

	shell_env = rtems_shell_init_env(NULL);
	if(shell_env == NULL) {
		printf("Unable to allocate shell environment");
		return;
	}
	shell_env->devname       = pty_name;
	shell_env->taskname      = pty_name;
	shell_env->exit_shell    = false;
	shell_env->forever       = false;
	shell_env->echo          = true;
	shell_env->input         = strdup("stdin");
	shell_env->output        = strdup("stdout");
	shell_env->output_append = false;
	shell_env->wake_on_end   = RTEMS_INVALID_ID;
	shell_env->login_check   = sysconfig_login_check;

	rtems_shell_main_loop(shell_env);
	
	// shell_env is freed by rtems_shell_main_loop
	// (using the "task variable" mechanism)
}

rtems_telnetd_config_table rtems_telnetd_config = {
	.command = rtems_telnet_shell,
	.arg = NULL,
	.priority = 9,
	.stack_size = 20 * RTEMS_MINIMUM_STACK_SIZE,
	.login_check = NULL,
	.keep_stdio = false
};

#define CONFIGURE_SHELL_COMMANDS_INIT
#define CONFIGURE_SHELL_COMMANDS_ALL
#define CONFIGURE_SHELL_COMMANDS_ALL_NETWORKING
#define CONFIGURE_SHELL_USER_COMMANDS &shell_usercmd
#include <rtems/shellconfig.h>

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_FRAME_BUFFER_DRIVER
#define CONFIGURE_APPLICATION_EXTRA_DRIVERS \
	FLASH_DRIVER_TABLE_ENTRY, \
	USBINPUT_DRIVER_TABLE_ENTRY, \
	AC97_DRIVER_TABLE_ENTRY, \
	PFPU_DRIVER_TABLE_ENTRY, \
	TMU_DRIVER_TABLE_ENTRY, \
	DMX_DRIVER_TABLE_ENTRY, \
	IR_DRIVER_TABLE_ENTRY, \
	MIDI_DRIVER_TABLE_ENTRY, \
	VERSIONS_DRIVER_TABLE_ENTRY, \
	GPIO_DRIVER_TABLE_ENTRY

#define CONFIGURE_MAXIMUM_DRIVERS 16

#define CONFIGURE_APPLICATION_NEEDS_LIBBLOCK
#define CONFIGURE_USE_IMFS_AS_BASE_FILESYSTEM
#define CONFIGURE_HAS_OWN_FILESYSTEM_TABLE
#include <rtems/imfs.h>
#include <rtems/dosfs.h>
#include <librtemsNfs.h>
const rtems_filesystem_table_t rtems_filesystem_table[] = {
	{ RTEMS_FILESYSTEM_TYPE_IMFS, IMFS_initialize },
	{ RTEMS_FILESYSTEM_TYPE_DOSFS, rtems_dosfs_initialize },
	{ RTEMS_FILESYSTEM_TYPE_NFS, rtems_nfsfs_initialize },
	{ "yaffs", yaffs_initialize },
	{ NULL, NULL }
};

#define CONFIGURE_EXECUTIVE_RAM_SIZE (16*1024*1024)

#define CONFIGURE_LIBIO_MAXIMUM_FILE_DESCRIPTORS 32
#define CONFIGURE_MAXIMUM_PTYS 4
#define CONFIGURE_MAXIMUM_TASKS 32
#define CONFIGURE_MAXIMUM_MESSAGE_QUEUES 16
#define CONFIGURE_MAXIMUM_SEMAPHORES 32

#define CONFIGURE_TICKS_PER_TIMESLICE 3
#define CONFIGURE_MICROSECONDS_PER_TICK 10000

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE
#define CONFIGURE_INIT_TASK_STACK_SIZE (8*1024)
#define CONFIGURE_INIT_TASK_PRIORITY 100
#define CONFIGURE_INIT_TASK_ATTRIBUTES 0
#define CONFIGURE_INIT_TASK_INITIAL_MODES \
	(RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR | \
	RTEMS_INTERRUPT_LEVEL(0))

//#define CONFIGURE_MALLOC_STATISTICS
//#define CONFIGURE_ZERO_WORKSPACE_AUTOMATICALLY TRUE
//#define CONFIGURE_STACK_CHECKER_ENABLED

#define CONFIGURE_INIT
#include <rtems/confdefs.h>
