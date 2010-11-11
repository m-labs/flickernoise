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
#include <rtems.h>
#include <mtklib.h>

#include "input.h"
#include "resmgr.h"
#include "framedescriptor.h"
#include "sampler.h"

#include "monitor.h"

static int appid;

static float time2;
static float bass, mid, treb;
static float bass_att, mid_att, treb_att;
static float idmx[IDMX_COUNT];

static void sampler_callback(struct frame_descriptor *frd)
{
	int i;
	
	time2 = frd->time;

	bass = frd->bass;
	mid = frd->mid;
	treb = frd->treb;

	bass_att = frd->bass_att;
	mid_att = frd->mid_att;
	treb_att = frd->treb_att;

	for(i=0;i<IDMX_COUNT;i++)
		idmx[i] = frd->idmx[i];
	sampler_return(frd);
}

static float *get_variable(const char *name)
{
	if(strcmp(name, "time") == 0) return &time2;

	else if(strcmp(name, "bass") == 0) return &bass;
	else if(strcmp(name, "mid") == 0) return &mid;
	else if(strcmp(name, "treb") == 0) return &treb;

	else if(strcmp(name, "bass_att") == 0) return &bass_att;
	else if(strcmp(name, "mid_att") == 0) return &mid_att;
	else if(strcmp(name, "treb_att") == 0) return &treb_att;

	else if(strcmp(name, "idmx1") == 0) return &idmx[0];
	else if(strcmp(name, "idmx2") == 0) return &idmx[1];
	else if(strcmp(name, "idmx3") == 0) return &idmx[2];
	else if(strcmp(name, "idmx4") == 0) return &idmx[3];

	else return NULL;
}

#define UPDATE_PERIOD 10
static rtems_interval next_update;

static void monitor_update(mtk_event *e, int count)
{
	int i;
	float *var;
	char str[16];
	rtems_interval t;

	t = rtems_clock_get_ticks_since_boot();
	if(t >= next_update) {
		for(i=0;i<8;i++) {
			mtk_reqf(appid, str, sizeof(str), "var%d.text", i);
			var = get_variable(str);
			if(var == NULL)
				mtk_cmdf(appid, "val%d.set(-text \"N/A\")", i);
			else
				mtk_cmdf(appid, "val%d.set(-text \"%.2f\")", i, *var);
		}
		next_update = t + UPDATE_PERIOD;
	}
}

static void close_callback(mtk_event *e, void *arg)
{
	mtk_cmd(appid, "w.close()");
	sampler_stop();
	input_delete_callback(monitor_update);
	resmgr_release(RESOURCE_AUDIO);
	resmgr_release(RESOURCE_DMX_IN);
	resmgr_release(RESOURCE_SAMPLER);
}

void init_monitor()
{
	int i;
	int column;
	int brow;

	appid = mtk_init_app("Monitor");

	mtk_cmd(appid, "g = new Grid()");
	for(i=0;i<8;i++) {
		column = i > 3 ? 3 : 1;
		brow = i & 3;
		mtk_cmdf(appid, "var%d = new Entry()", i);
		mtk_cmdf(appid, "val%d = new Label(-text \"N/A\")", i);
		mtk_cmdf(appid, "g.place(var%d, -column %d -row %d)", i, column, 3*brow+1);
		mtk_cmdf(appid, "g.place(val%d, -column %d -row %d)", i, column, 3*brow+2);
		/* Put a horizontal separator everywhere except for the last row */
		if(brow != 3)  {
			mtk_cmdf(appid, "sep%d = new Separator(-vertical no)", i);
			mtk_cmdf(appid, "g.place(sep%d, -column %d -row %d)", i, column, 3*brow+3);
		}
	}
	for(i=0;i<11;i++) {
		mtk_cmdf(appid, "vsep%d = new Separator(-vertical yes)", i);
		mtk_cmdf(appid, "g.place(vsep%d, -column 2 -row %d)", i, i+1);
	}
	mtk_cmd(appid, "g.columnconfig(1, -weight 1 -size 100)");
	mtk_cmd(appid, "g.columnconfig(3, -weight 1 -size 100)");
	mtk_cmd(appid, "w = new Window(-content g -title \"Variable monitor\")");

	mtk_bind(appid, "w", "close", close_callback, NULL);
}

void open_monitor_window()
{
	if(!resmgr_acquire_multiple("monitor",
	  RESOURCE_AUDIO,
	  RESOURCE_DMX_IN,
	  RESOURCE_SAMPLER,
	  INVALID_RESOURCE))
		return;

	mtk_cmd(appid, "w.open()");
	next_update = rtems_clock_get_ticks_since_boot() + UPDATE_PERIOD;
	input_add_callback(monitor_update);
	sampler_start(sampler_callback);
}
