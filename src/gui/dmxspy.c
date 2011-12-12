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

#include <bsp.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <rtems.h>
#include <mtklib.h>

#include "cp.h"

#include "resmgr.h"
#include "../input.h"
#include "dmxspy.h"

static int appid;

static int w_open;

#define UPDATE_PERIOD 10
static rtems_interval next_update;

static int dmx_fd;
static unsigned char old_values[512];
static unsigned char new_values[512];

static void dmxspy_update(mtk_event *e, int count)
{
	rtems_interval t;
	int last_channel;
	int last_value;

	t = rtems_clock_get_ticks_since_boot();
	if(t >= next_update) {
		int i;
		
		lseek(dmx_fd, SEEK_SET, 0);
		read(dmx_fd, new_values, 512);

		last_channel = -1;
		last_value = -1;
		for(i=0;i<512;i++) {
			if(new_values[i] != old_values[i]) {
				last_channel = i;
				last_value = new_values[i];
			}
			old_values[i] = new_values[i];
		}

		if(last_channel != -1)
			mtk_cmdf(appid, "list.set(-text \"%d (value: %d)\")", last_channel+1, last_value);
		
		next_update = t + UPDATE_PERIOD;
	}
}

static void close_callback(mtk_event *e, void *arg)
{
	close_dmxspy_window();
}

void init_dmxspy(void)
{
	appid = mtk_init_app("DMX spy");

	mtk_cmd_seq(appid,
		"g = new Grid()",

		"l = new Label(-text \"Latest active channel:\")",
		"list = new Label(-text \"\")",
		"bclose = new Button(-text \"Close\")",

		"g.place(l, -column 1 -row 1)",
		"g.place(list, -column 1 -row 2)",
		"g.place(bclose, -column 1 -row 3)",

		"w = new Window(-content g -title \"DMX spy\")",
		0);

	mtk_bind(appid, "bclose", "commit", close_callback, NULL);

	mtk_bind(appid, "w", "close", close_callback, NULL);

}

void open_dmxspy_window(void)
{
	if(w_open) return;
	if(!resmgr_acquire("DMX spy", RESOURCE_DMX_IN)) return;
	dmx_fd = open("/dev/dmx_in", O_RDONLY);
	if(dmx_fd == -1) {
		resmgr_release(RESOURCE_DMX_IN);
		return;
	}
	read(dmx_fd, old_values, 512);
	w_open = 1;
	mtk_cmd(appid, "list.set(-text \"\")");
	mtk_cmd(appid, "w.open()");
	next_update = rtems_clock_get_ticks_since_boot() + UPDATE_PERIOD;
	input_add_callback(dmxspy_update);
}

void close_dmxspy_window(void)
{
	mtk_cmd(appid, "w.close()");
	input_delete_callback(dmxspy_update);
	close(dmx_fd);
	resmgr_release(RESOURCE_DMX_IN);
	w_open = 0;
}
