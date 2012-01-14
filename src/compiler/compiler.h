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

#ifndef __COMPILER_H
#define __COMPILER_H

#ifndef STANDALONE
#include <rtems.h>
#include <bsp/milkymist_pfpu.h>
#else
#include STANDALONE
#endif /* STANDALONE */

#include <sys/stat.h>

#include <fpvm/fpvm.h>

#include "../renderer/framedescriptor.h"
#include "../pixbuf/pixbuf.h"

enum {
	pfv_sx = 0,
	pfv_sy,
	pfv_cx,
	pfv_cy,
	pfv_rot,
	pfv_dx,
	pfv_dy,
	pfv_zoom,
	pfv_decay,
	pfv_wave_mode,
	pfv_wave_scale,
	pfv_wave_additive,
	pfv_wave_usedots,
	pfv_wave_brighten,
	pfv_wave_thick,
	pfv_wave_x,
	pfv_wave_y,
	pfv_wave_r,
	pfv_wave_g,
	pfv_wave_b,
	pfv_wave_a,

	pfv_ob_size,
	pfv_ob_r,
	pfv_ob_g,
	pfv_ob_b,
	pfv_ob_a,
	pfv_ib_size,
	pfv_ib_r,
	pfv_ib_g,
	pfv_ib_b,
	pfv_ib_a,

	pfv_mv_x,
	pfv_mv_y,
	pfv_mv_dx,
	pfv_mv_dy,
	pfv_mv_l,
	pfv_mv_r,
	pfv_mv_g,
	pfv_mv_b,
	pfv_mv_a,

	pfv_tex_wrap,

	pfv_time,
	pfv_frame,
	pfv_bass,
	pfv_mid,
	pfv_treb,
	pfv_bass_att,
	pfv_mid_att,
	pfv_treb_att,

	pfv_warp,
	pfv_warp_anim_speed,
	pfv_warp_scale,

	pfv_q1,
	pfv_q2,
	pfv_q3,
	pfv_q4,
	pfv_q5,
	pfv_q6,
	pfv_q7,
	pfv_q8,

	pfv_video_echo_alpha,
	pfv_video_echo_zoom,
	pfv_video_echo_orientation,

	pfv_dmx1,
	pfv_dmx2,
	pfv_dmx3,
	pfv_dmx4,
	pfv_dmx5,
	pfv_dmx6,
	pfv_dmx7,
	pfv_dmx8,

	pfv_idmx1,
	pfv_idmx2,
	pfv_idmx3,
	pfv_idmx4,
	pfv_idmx5,
	pfv_idmx6,
	pfv_idmx7,
	pfv_idmx8,

	pfv_osc1,
	pfv_osc2,
	pfv_osc3,
	pfv_osc4,

	pfv_midi1,
	pfv_midi2,
	pfv_midi3,
	pfv_midi4,
	pfv_midi5,
	pfv_midi6,
	pfv_midi7,
	pfv_midi8,

	pfv_video_a,

	pfv_image1_a,
	pfv_image1_x,
	pfv_image1_y,
	pfv_image1_zoom,
	pfv_image2_a,
	pfv_image2_x,
	pfv_image2_y,
	pfv_image2_zoom,

	COMP_PFV_COUNT /* must be last */
};

enum {
	/* System */
	pvv_texsize,
	pvv_hmeshsize,
	pvv_vmeshsize,

	/* MilkDrop */
	pvv_sx,
	pvv_sy,
	pvv_cx,
	pvv_cy,
	pvv_rot,
	pvv_dx,
	pvv_dy,
	pvv_zoom,

	pvv_time,
	pvv_frame,
	pvv_bass,
	pvv_mid,
	pvv_treb,
	pvv_bass_att,
	pvv_mid_att,
	pvv_treb_att,

	pvv_warp,
	pvv_warp_anim_speed,
	pvv_warp_scale,

	pvv_q1,
	pvv_q2,
	pvv_q3,
	pvv_q4,
	pvv_q5,
	pvv_q6,
	pvv_q7,
	pvv_q8,

	pvv_idmx1,
	pvv_idmx2,
	pvv_idmx3,
	pvv_idmx4,
	pvv_idmx5,
	pvv_idmx6,
	pvv_idmx7,
	pvv_idmx8,

	pvv_osc1,
	pvv_osc2,
	pvv_osc3,
	pvv_osc4,

	pvv_midi1,
	pvv_midi2,
	pvv_midi3,
	pvv_midi4,
	pvv_midi5,
	pvv_midi6,
	pvv_midi7,
	pvv_midi8,

	COMP_PVV_COUNT /* must be last */
};

#define REQUIRE_DMX	(1 << 0)
#define REQUIRE_OSC	(1 << 1)
#define REQUIRE_MIDI	(1 << 2)
#define REQUIRE_VIDEO	(1 << 3)

struct image {
	struct pixbuf *pixbuf;	/* NULL if unused */
	const char *filename;	/* undefined if unused */
	struct stat st;
};

struct patch {
	/* per-frame */
	struct image images[IMAGE_COUNT];		/* < images used in this patch */
	float pfv_initial[COMP_PFV_COUNT]; 		/* < patch initial conditions */
	int pfv_allocation[COMP_PFV_COUNT];		/* < where per-frame variables are mapped in PFPU regf, -1 if unmapped */
	int perframe_prog_length;			/* < how many instructions in perframe_prog */
	unsigned int perframe_prog[PFPU_PROGSIZE];	/* < PFPU per-frame microcode */
	float perframe_regs[PFPU_REG_COUNT];		/* < PFPU initial per-frame regf */
	/* per-vertex */
	int pvv_allocation[COMP_PVV_COUNT];		/* < where per-vertex variables are mapped in PFPU regf, -1 if unmapped */
	int pervertex_prog_length;			/* < how many instructions in pervertex_prog */
	unsigned int pervertex_prog[PFPU_PROGSIZE];	/* < PFPU per-vertex microcode */
	float pervertex_regs[PFPU_REG_COUNT];		/* < PFPU initial per-vertex regf */
	/* meta */
	unsigned int require;				/* < bitmask: dmx, osc, midi, video */
	void *original;					/* < original patch (with initial register values) */
	int ref;					/* reference count */
	struct patch *next;				/* < used when chaining patches in mashups */
};

typedef void (*report_message)(const char *);

static inline struct patch *patch_clone(struct patch *p)
{
	p->ref++;
	return p;
}

struct patch *patch_compile(const char *basedir, const char *patch_code, report_message rmc);
struct patch *patch_compile_filename(const char *filename, const char *patch_code, report_message rmc);
struct patch *patch_copy(struct patch *p);
void patch_free(struct patch *p);
int patch_images_uptodate(const struct patch *p);

#endif /* __COMPILER_H */
