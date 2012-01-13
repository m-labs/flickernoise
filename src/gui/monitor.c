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
#include <rtems.h>
#include <mtklib.h>

#include "../input.h"
#include "resmgr.h"
#include "../renderer/framedescriptor.h"
#include "../renderer/sampler.h"

#include "monitor.h"

static int appid;

static int w_open;

static float time2;
static float bass, mid, treb;
static float bass_att, mid_att, treb_att;
static float idmx[IDMX_COUNT];
static float osc[OSC_COUNT];
static float midi[MIDI_COUNT];

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
	for(i=0;i<OSC_COUNT;i++)
		osc[i] = frd->osc[i];
	for(i=0;i<MIDI_COUNT;i++)
		midi[i] = frd->midi[i];
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
	else if(strcmp(name, "idmx5") == 0) return &idmx[4];
	else if(strcmp(name, "idmx6") == 0) return &idmx[5];
	else if(strcmp(name, "idmx7") == 0) return &idmx[6];
	else if(strcmp(name, "idmx8") == 0) return &idmx[7];

	else if(strcmp(name, "osc1") == 0) return &osc[0];
	else if(strcmp(name, "osc2") == 0) return &osc[1];
	else if(strcmp(name, "osc3") == 0) return &osc[2];
	else if(strcmp(name, "osc4") == 0) return &osc[3];
	
	else if(strcmp(name, "midi1") == 0) return &midi[0];
	else if(strcmp(name, "midi2") == 0) return &midi[1];
	else if(strcmp(name, "midi3") == 0) return &midi[2];
	else if(strcmp(name, "midi4") == 0) return &midi[3];
	else if(strcmp(name, "midi5") == 0) return &midi[4];
	else if(strcmp(name, "midi6") == 0) return &midi[5];
	else if(strcmp(name, "midi7") == 0) return &midi[6];
	else if(strcmp(name, "midi8") == 0) return &midi[7];

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
				mtk_cmdf(appid, "val%d.set(-text \"\e--\")", i);
			else
				mtk_cmdf(appid, "val%d.set(-text \"\e%.2f\")", i, *var);
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
	w_open = 0;
}

void init_monitor(void)
{
	int i;
	int column;
	int brow;

	appid = mtk_init_app("Variable monitor");

	mtk_cmd(appid, "g = new Grid()");
	for(i=0;i<8;i++) {
		column = i > 3 ? 3 : 1;
		brow = i & 3;
		mtk_cmdf(appid, "var%d = new Entry()", i);
		mtk_cmdf(appid, "val%d = new Label(-text \"\e--\")", i);
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

void open_monitor_window(void)
{
	if(w_open) return;

	if(!resmgr_acquire_multiple("Variable monitor",
	  RESOURCE_AUDIO,
	  RESOURCE_DMX_IN,
	  RESOURCE_SAMPLER,
	  INVALID_RESOURCE))
		return;

	w_open = 1;

	mtk_cmd(appid, "w.open()");
	next_update = rtems_clock_get_ticks_since_boot() + UPDATE_PERIOD;
	input_add_callback(monitor_update);
	sampler_start(sampler_callback);
}

void monitor_notify_changed(void)
{
	if(!w_open) return;
	sampler_stop();
	sampler_start(sampler_callback);
}
