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

#include <bsp.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <dopelib.h>
#include <vscreen.h>
#include <bsp/milkymist_usbinput.h>
#include <bsp/milkymist_ac97.h>
#include <rtems/stackchk.h>
#include <rtems/shell.h>

#include "cp.h"
#include "audio.h"
#include "patcheditor.h"
#include "monitor.h"
#include "shutdown.h"
#include "about.h"
#include "flash.h"
#include "filedialog.h"

rtems_task gui_task(rtems_task_argument argument)
{
	printf("GUI task started\n");
	if(dope_init())
		return;

	init_cp();
        init_audio();
	init_patcheditor();
	init_monitor();
        init_about();
	init_flash();
        init_shutdown();

	dope_eventloop(0);
}

static rtems_id gui_task_id;
rtems_task Init(rtems_task_argument argument)
{
	rtems_status_code sc;

	printf("Starting GUI task...\n");

	assert(rtems_task_create(rtems_build_name('G','U','I',' '), 100, 512*1024, RTEMS_PREEMPT | RTEMS_TIMESLICE | RTEMS_NO_ASR | \
		RTEMS_INTERRUPT_LEVEL(0), RTEMS_FLOATING_POINT, &gui_task_id) == RTEMS_SUCCESSFUL);
	assert(rtems_task_start(gui_task_id, gui_task, 0) == RTEMS_SUCCESSFUL);

	/* disable shell because it uses 100% CPU (polled console driver) */
#if 0
	printf("Starting shell task...\n");
	sc = rtems_shell_init(
		"SHLL",
		RTEMS_MINIMUM_STACK_SIZE * 4,
		100,
		"/dev/console",
		false,
		false,
		NULL
	);
	if(sc != RTEMS_SUCCESSFUL)
		printf("Unable to start shell (error code %d)\n", sc);
#endif

	rtems_task_delete(RTEMS_SELF);
}

#define CONFIGURE_SHELL_COMMANDS_INIT
#define CONFIGURE_SHELL_COMMANDS_ALL
#include <rtems/shellconfig.h>

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_FRAME_BUFFER_DRIVER
#define CONFIGURE_APPLICATION_EXTRA_DRIVERS \
        AC97_DRIVER_TABLE_ENTRY, \
	USBINPUT_DRIVER_TABLE_ENTRY

#define CONFIGURE_APPLICATION_NEEDS_LIBBLOCK
#define CONFIGURE_USE_IMFS_AS_BASE_FILESYSTEM

#define CONFIGURE_EXECUTIVE_RAM_SIZE (16*1024*1024)

#define CONFIGURE_LIBIO_MAXIMUM_FILE_DESCRIPTORS 16
#define CONFIGURE_MAXIMUM_TASKS 4
#define CONFIGURE_MAXIMUM_POSIX_MUTEXES 8

#define CONFIGURE_TICKS_PER_TIMESLICE 3
#define CONFIGURE_MICROSECONDS_PER_TICK 10000

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE
#define CONFIGURE_INIT_TASK_STACK_SIZE (8*1024)
#define CONFIGURE_INIT_TASK_PRIORITY 100
#define CONFIGURE_INIT_TASK_ATTRIBUTES 0
#define CONFIGURE_INIT_TASK_INITIAL_MODES \
	(RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR | \
	RTEMS_INTERRUPT_LEVEL(0))

//#define CONFIGURE_ZERO_WORKSPACE_AUTOMATICALLY TRUE
//#define CONFIGURE_STACK_CHECKER_ENABLED

#define CONFIGURE_INIT
#include <rtems/confdefs.h>
