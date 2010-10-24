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
#include <bsp/milkymist_pfpu.h>
#include <bsp/milkymist_tmu.h>
#include <bsp/milkymist_memcard.h>
#include <rtems/stackchk.h>
#include <rtems/shell.h>
#include <rtems/rtems_bsdnet.h>

#include "cp.h"
#include "audio.h"
#include "patcheditor.h"
#include "monitor.h"
#include "shutdown.h"
#include "about.h"
#include "flash.h"
#include "filedialog.h"

static rtems_task gui_task(rtems_task_argument argument)
{
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

	memcard_register();
	/* TODO: read network configuration */
	rtems_bsdnet_initialize_network();

	assert(rtems_task_create(rtems_build_name('G','U','I',' '), 110, 512*1024,
		RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR,
		0, &gui_task_id) == RTEMS_SUCCESSFUL);
	assert(rtems_task_start(gui_task_id, gui_task, 0) == RTEMS_SUCCESSFUL);

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

#define CONFIGURE_SHELL_COMMANDS_INIT
#define CONFIGURE_SHELL_COMMANDS_ALL
#define CONFIGURE_SHELL_COMMANDS_ALL_NETWORKING
#include <rtems/shellconfig.h>

static struct rtems_bsdnet_ifconfig netdriver_config = {
	RTEMS_BSP_NETWORK_DRIVER_NAME,
	RTEMS_BSP_NETWORK_DRIVER_ATTACH,
	NULL,
	"192.168.0.42",
	"255.255.255.0",
	NULL,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	NULL
};


struct rtems_bsdnet_config rtems_bsdnet_config = {
	&netdriver_config,
	NULL,
	0,
	0,
	0,
	"milkymist",
	"local.domain",
	"192.168.0.42",
	NULL,
	{"192.168.0.14" },
	{"192.168.0.14" },
	0,
	0,
	0,
	0,
	0
};

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_FRAME_BUFFER_DRIVER
#define CONFIGURE_APPLICATION_EXTRA_DRIVERS \
	USBINPUT_DRIVER_TABLE_ENTRY, \
	AC97_DRIVER_TABLE_ENTRY, \
	PFPU_DRIVER_TABLE_ENTRY, \
	TMU_DRIVER_TABLE_ENTRY

#define CONFIGURE_MAXIMUM_DRIVERS 16

#define CONFIGURE_APPLICATION_NEEDS_LIBBLOCK
#define CONFIGURE_USE_IMFS_AS_BASE_FILESYSTEM
// TODO: this was renamed CONFIGURE_FILESYSTEM_NFS in latest RTEMS CVS
#define CONFIGURE_FILESYSTEM_NFSFS
#define CONFIGURE_FILESYSTEM_DOSFS

#define CONFIGURE_EXECUTIVE_RAM_SIZE (16*1024*1024)

#define CONFIGURE_LIBIO_MAXIMUM_FILE_DESCRIPTORS 16
#define CONFIGURE_MAXIMUM_TASKS 16
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
