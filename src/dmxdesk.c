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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <mtklib.h>

#include "util.h"
#include "resmgr.h"
#include "dmxdesk.h"

static int appid;
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
		mtk_cmdf(appid, "slide%d.set(-value %d)", i, 255-values[i]);
	ignore_slide_event = 0;
}

static void chanbtn_callback(mtk_event *e, void *arg)
{
	int i;

	mtk_cmdf(appid, "chanbtn%d.set(-state off)", current_chanbtn);
	current_chanbtn = (int)arg;
	mtk_cmdf(appid, "chanbtn%d.set(-state on)", current_chanbtn);

	for(i=0;i<8;i++)
		mtk_cmdf(appid, "label%d.set(-text \"%d\")", i, i+current_chanbtn*8+1);
	load_channels(current_chanbtn*8);
}

static void slide_callback(mtk_event *e, void *arg)
{
	int channel;
	unsigned char value;
	char control_name[32];

	if(ignore_slide_event)
		return;

	channel = (int)arg;

	sprintf(control_name, "slide%d.value", channel);
	value = 255 - mtk_req_i(appid, control_name);
	lseek(dmx_fd, 8*current_chanbtn+channel, SEEK_SET);
	write(dmx_fd, &value, 1);
}

static void close_callback(mtk_event *e, void *arg)
{
	close_dmxdesk_window();
}

void init_dmxdesk()
{
	int i;

	appid = mtk_init_app("DMX desk");
	mtk_cmd(appid, "g = new Grid()");

	for(i=0;i<64;i++) {
		mtk_cmdf(appid, "chanbtn%d = new Button(-text \"\e%d-%d\")", i, 8*i+1, 8*i+8);
		mtk_cmdf(appid, "g.place(chanbtn%d, -column %d -row %d)", i, i % 8, i / 8);
		mtk_bindf(appid, "chanbtn%d", "press", chanbtn_callback, (void *)i, i);
	}
	for(i=0;i<8;i++) {
		mtk_cmdf(appid, "slide%d = new Scale(-from 0 -to 255 -value 255 -orient vertical)", i);
		mtk_cmdf(appid, "label%d = new Label(-text \"\e%d\")", i, i+1);
		mtk_cmdf(appid, "g.place(slide%d, -column %d -row 8)", i, i);
		mtk_cmdf(appid, "g.place(label%d, -column %d -row 9)", i, i);
	}

	current_chanbtn = 0;
	mtk_cmd(appid, "chanbtn0.set(-state on)");

	mtk_cmd(appid, "g.rowconfig(8, -size 150)");
	mtk_cmd(appid, "w = new Window(-content g -title \"DMX desk\" -workx 20 -worky 35)");

	for(i=0;i<8;i++)
		mtk_bindf(appid, "slide%d", "change", slide_callback, (void *)i, i);

	mtk_bind(appid, "w", "close", close_callback, NULL);
}

static int w_open;

void open_dmxdesk_window()
{
	if(w_open) return;
	w_open = 1;
	if(!resmgr_acquire("DMX desk", RESOURCE_DMX_OUT))
		return;
	dmx_fd = open("/dev/dmx_out", O_RDWR);
	if(dmx_fd == -1) {
		perror("Unable to open DMX device");
		resmgr_release(RESOURCE_DMX_OUT);
		return;
	}
	load_channels(0);
	mtk_cmd(appid, "w.open()");
}

void close_dmxdesk_window()
{
	if(!w_open) return;
	w_open = 0;
	close(dmx_fd);
	resmgr_release(RESOURCE_DMX_OUT);
	mtk_cmd(appid, "w.close()");
}
