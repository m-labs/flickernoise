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

#include <rtems.h>
#include <math.h>
#include <string.h>
#include <sys/ioctl.h>
#include <bsp/milkymist_tmu.h>

#include "font.h"
#include "osd.h"

#define OSD_W 600
#define OSD_H 82
#define OSD_CORNER 15
#define OSD_CHROMAKEY 0x001f

static struct tmu_vertex osd_vertices[TMU_MESH_MAXSIZE][TMU_MESH_MAXSIZE] __attribute__((aligned(8)));
static unsigned short int osd_fb[OSD_W*OSD_H] __attribute__((aligned(32)));
static struct font_context osd_font;
static int osd_alpha;
static int osd_timer;
static void (*osd_callback)(void) = NULL;

static void round_corners(void)
{
	int i;
	int d;
	int x;

	for(i=0;i<OSD_CORNER;i++) {
		d = OSD_CORNER - sqrtf(2*OSD_CORNER*i - i*i);
		for(x=0;x<d;x++) {
			osd_fb[i*OSD_W+x] = OSD_CHROMAKEY;
			osd_fb[i*OSD_W+(OSD_W-1-x)] = OSD_CHROMAKEY;
			osd_fb[(OSD_H-1-i)*OSD_W+x] = OSD_CHROMAKEY;
			osd_fb[(OSD_H-1-i)*OSD_W+(OSD_W-1-x)] = OSD_CHROMAKEY;
		}
	}
}

void osd_init(void)
{
	osd_vertices[0][0].x = 0;
	osd_vertices[0][0].y = 0;
	osd_vertices[0][1].x = OSD_W << TMU_FIXEDPOINT_SHIFT;
	osd_vertices[0][1].y = 0;
	osd_vertices[1][0].x = 0;
	osd_vertices[1][0].y = OSD_H << TMU_FIXEDPOINT_SHIFT;
	osd_vertices[1][1].x = OSD_W << TMU_FIXEDPOINT_SHIFT;
	osd_vertices[1][1].y = OSD_H << TMU_FIXEDPOINT_SHIFT;

	memset(osd_fb, 0, sizeof(osd_fb));
	font_init_context(&osd_font, vera20_tff, osd_fb, OSD_W, OSD_H);
	round_corners();
	
	osd_alpha = 0;
	osd_timer = 0;
}

#define OSD_DURATION 90
#define OSD_MAX_ALPHA 40

static void clear_user_area(void)
{
	int x, y;
	
	for(y=OSD_CORNER;y<(OSD_H-OSD_CORNER);y++)
		for(x=0;x<OSD_W;x++)
			osd_fb[x+y*OSD_W] = 0;
}

void osd_event(const char *string)
{
	clear_user_area();
	font_draw_string(&osd_font, OSD_CORNER, OSD_CORNER, 0, string);
	osd_timer = OSD_DURATION;
}

void osd_event_cb(const char *string, void (*faded)(void))
{
	osd_event(string);
	osd_callback = faded; 
}

void osd_per_frame(int tmu_fd, unsigned short *dest, int hres, int vres)
{
	struct tmu_td td;
	int osd_x;
	int osd_y;

	if(osd_timer > 0) {
		osd_timer--;
		osd_alpha += 4;
		if(osd_alpha > OSD_MAX_ALPHA)
			osd_alpha = OSD_MAX_ALPHA;
	} else {
		osd_alpha--;
		if(osd_alpha < 0)
			osd_alpha = 0;
	}
	
	if(osd_alpha == 0) {
		if(osd_callback)
			osd_callback();
		osd_callback = NULL;
		return;
	}
	
	osd_x = (hres - OSD_W) >> 1;
	osd_y = vres - OSD_H - 20;

	td.flags = TMU_FLAG_CHROMAKEY;
	td.hmeshlast = 1;
	td.vmeshlast = 1;
	td.brightness = TMU_BRIGHTNESS_MAX;
	td.chromakey = OSD_CHROMAKEY;
	td.vertices = &osd_vertices[0][0];
	td.texfbuf = osd_fb;
	td.texhres = OSD_W;
	td.texvres = OSD_H;
	td.texhmask = TMU_MASK_NOFILTER;
	td.texvmask = TMU_MASK_NOFILTER;
	td.dstfbuf = dest;
	td.dsthres = hres;
	td.dstvres = vres;
	td.dsthoffset = osd_x;
	td.dstvoffset = osd_y;
	td.dstsquarew = OSD_W;
	td.dstsquareh = OSD_H;
	td.alpha = osd_alpha;
	td.invalidate_before = true;
	td.invalidate_after = false;

	ioctl(tmu_fd, TMU_EXECUTE, &td);
}

