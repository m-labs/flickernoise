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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <dopelib.h>
#include <vscreen.h>

#include "util.h"
#include "resmgr.h"
#include "dmxtable.h"

static long appid;
static int dmx_fd;
static int current_chanbtn;
static int ignore_slide_event;

static void load_channels(int start)
{
	int i;
	unsigned char values[8];

	lseek(dmx_fd, start, SEEK_SET);
	read(dmx_fd, values, 8);

	ignore_slide_event = 1;
	for(i=0;i<8;i++)
		dope_cmdf(appid, "slide%d.set(-value %d)", i, 255-values[i]);
	ignore_slide_event = 0;
}

static void chanbtn_callback(dope_event *e, void *arg)
{
	int i;

	dope_cmdf(appid, "chanbtn%d.set(-state off)", current_chanbtn);
	current_chanbtn = (int)arg;
	dope_cmdf(appid, "chanbtn%d.set(-state on)", current_chanbtn);

	for(i=0;i<8;i++)
		dope_cmdf(appid, "label%d.set(-text \"%d\")", i, i+current_chanbtn*8+1);
	load_channels(current_chanbtn*8);
}

static void slide_callback(dope_event *e, void *arg)
{
	int channel;
	unsigned char value;
	char control_name[32];

	if(ignore_slide_event)
		return;

	channel = (int)arg;

	sprintf(control_name, "slide%d.value", channel);
	value = 255 - dope_req_l(appid, control_name);
	lseek(dmx_fd, 8*current_chanbtn+channel, SEEK_SET);
	write(dmx_fd, &value, 1);
}

static void close_callback(dope_event *e, void *arg)
{
	close_dmxtable_window();
}

void init_dmxtable()
{
	int i;

	appid = dope_init_app("DMX table");
	dope_cmd(appid, "g = new Grid()");

	for(i=0;i<64;i++) {
		dope_cmdf(appid, "chanbtn%d = new Button(-text \"%d-%d\")", i, 8*i+1, 8*i+8);
		dope_cmdf(appid, "g.place(chanbtn%d, -column %d -row %d)", i, i % 8, i / 8);
		dope_bindf(appid, "chanbtn%d", "press", chanbtn_callback, (void *)i, i);
	}
	for(i=0;i<8;i++) {
		dope_cmdf(appid, "slide%d = new Scale(-from 0 -to 255 -value 255 -orient vertical)", i);
		dope_cmdf(appid, "label%d = new Label(-text \"%d\")", i, i+1);
		dope_cmdf(appid, "g.place(slide%d, -column %d -row 8)", i, i);
		dope_cmdf(appid, "g.place(label%d, -column %d -row 9)", i, i);
	}

	current_chanbtn = 0;
	dope_cmd(appid, "chanbtn0.set(-state on)");

	dope_cmd(appid, "g.rowconfig(8, -size 150)");
	dope_cmd(appid, "w = new Window(-content g -title \"DMX table\" -workx 20 -worky 35)");

	for(i=0;i<8;i++)
		dope_bindf(appid, "slide%d", "change", slide_callback, (void *)i, i);

	dope_bind(appid, "w", "close", close_callback, NULL);
}

void open_dmxtable_window()
{
	if(!resmgr_acquire("DMX table", RESOURCE_DMX_OUT))
		return;
	dmx_fd = open("/dev/dmx_out", O_RDWR);
	if(dmx_fd == -1) {
		perror("Unable to open DMX device");
		resmgr_release(RESOURCE_DMX_OUT);
		return;
	}
	load_channels(0);
	dope_cmd(appid, "w.open()");
}

void close_dmxtable_window()
{
	close(dmx_fd);
	resmgr_release(RESOURCE_DMX_OUT);
	dope_cmd(appid, "w.close()");
}
