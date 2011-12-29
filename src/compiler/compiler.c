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

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <fpvm/fpvm.h>
#include <fpvm/schedulers.h>
#include <fpvm/pfpu.h>

#include "../pixbuf/pixbuf.h"
#include "fpvm.h"
#include "unique.h"
#include "parser_helper.h"
#include "parser.h"
#include "compiler.h"

#include "infra-fnp.h"

struct compiler_sc {
	struct patch *p;

	const char *basedir;
	report_message rmc;
	int linenr;

	struct fpvm_fragment pfv_fragment;
	struct fpvm_fragment pvv_fragment;
};

static void comp_report(struct compiler_sc *sc, const char *format, ...)
{
	va_list args;
	int len;
	char outbuf[512];

	va_start(args, format);
	len = vsnprintf(outbuf, sizeof(outbuf), format, args);
	va_end(args);
	sc->rmc(outbuf);
}

/****************************************************************/
/* PER-FRAME VARIABLES                                          */
/****************************************************************/

const char pfv_names[COMP_PFV_COUNT][FPVM_MAXSYMLEN] = {
	"sx",
	"sy",
	"cx",
	"cy",
	"rot",
	"dx",
	"dy",
	"zoom",
	"decay",
	"wave_mode",
	"wave_scale",
	"wave_additive",
	"wave_usedots",
	"wave_brighten",
	"wave_thick",
	"wave_x",
	"wave_y",
	"wave_r",
	"wave_g",
	"wave_b",
	"wave_a",

	"ob_size",
	"ob_r",
	"ob_g",
	"ob_b",
	"ob_a",
	"ib_size",
	"ib_r",
	"ib_g",
	"ib_b",
	"ib_a",

	"nMotionVectorsX",
	"nMotionVectorsY",
	"mv_dx",
	"mv_dy",
	"mv_l",
	"mv_r",
	"mv_g",
	"mv_b",
	"mv_a",

	"bTexWrap",

	"time",
	"bass",
	"mid",
	"treb",
	"bass_att",
	"mid_att",
	"treb_att",

	"warp",
	"fWarpAnimSpeed",
	"fWarpScale",

	"q1",
	"q2",
	"q3",
	"q4",
	"q5",
	"q6",
	"q7",
	"q8",

	"fVideoEchoAlpha",
	"fVideoEchoZoom",
	"nVideoEchoOrientation",

	"dmx1",
	"dmx2",
	"dmx3",
	"dmx4",
	"dmx5",
	"dmx6",
	"dmx7",
	"dmx8",

	"idmx1",
	"idmx2",
	"idmx3",
	"idmx4",
	"idmx5",
	"idmx6",
	"idmx7",
	"idmx8",

	"osc1",
	"osc2",
	"osc3",
	"osc4",

	"midi1",
	"midi2",
	"midi3",
	"midi4",
	"midi5",
	"midi6",
	"midi7",
	"midi8",

	"video_a",

	"image1_a",
	"image1_x",
	"image1_y",
	"image1_zoom",
	"image2_a",
	"image2_x",
	"image2_y",
	"image2_zoom"
};

static int pfv_from_name(const char *name)
{
	int i;

	for(i=0;i<COMP_PFV_COUNT;i++) {
		if(strcmp(pfv_names[i], name) == 0)
			return i;
	}

	if(strcmp(name, "fDecay") == 0) return pfv_decay;
	if(strcmp(name, "nWaveMode") == 0) return pfv_wave_mode;
	if(strcmp(name, "fWaveScale") == 0) return pfv_wave_scale;
	if(strcmp(name, "bAdditiveWaves") == 0) return pfv_wave_additive;
	if(strcmp(name, "bWaveDots") == 0) return pfv_wave_usedots;
	if(strcmp(name, "bMaximizeWaveColor") == 0) return pfv_wave_brighten;
	if(strcmp(name, "bWaveThick") == 0) return pfv_wave_thick;
	if(strcmp(name, "fWaveAlpha") == 0) return pfv_wave_a;
	return -1;
}

static void pfv_update_patch_requires(struct compiler_sc *sc, int pfv)
{
	if(pfv >= pfv_dmx1 && pfv <= pfv_idmx8)
		sc->p->require |= REQUIRE_DMX;
	if(pfv >= pfv_osc1 && pfv <= pfv_osc4)
		sc->p->require |= REQUIRE_OSC;
	if(pfv >= pfv_midi1 && pfv <= pfv_midi8)
		sc->p->require |= REQUIRE_MIDI;
	if(pfv == pfv_video_a)
		sc->p->require |= REQUIRE_VIDEO;
}

static void load_defaults(struct compiler_sc *sc)
{
	int i;

	for(i=0;i<COMP_PFV_COUNT;i++)
		sc->p->pfv_initial[i] = 0.0;
	sc->p->pfv_initial[pfv_sx] = 1.0;
	sc->p->pfv_initial[pfv_sy] = 1.0;
	sc->p->pfv_initial[pfv_cx] = 0.5;
	sc->p->pfv_initial[pfv_cy] = 0.5;
	sc->p->pfv_initial[pfv_zoom] = 1.0;
	sc->p->pfv_initial[pfv_decay] = 1.0;
	sc->p->pfv_initial[pfv_wave_mode] = 1.0;
	sc->p->pfv_initial[pfv_wave_scale] = 1.0;
	sc->p->pfv_initial[pfv_wave_r] = 1.0;
	sc->p->pfv_initial[pfv_wave_g] = 1.0;
	sc->p->pfv_initial[pfv_wave_b] = 1.0;
	sc->p->pfv_initial[pfv_wave_a] = 1.0;
	sc->p->pfv_initial[pfv_wave_x] = 0.5;
	sc->p->pfv_initial[pfv_wave_y] = 0.5;

	sc->p->pfv_initial[pfv_mv_x] = 16.0;
	sc->p->pfv_initial[pfv_mv_y] = 12.0;
	sc->p->pfv_initial[pfv_mv_dx] = 0.02;
	sc->p->pfv_initial[pfv_mv_dy] = 0.02;
	sc->p->pfv_initial[pfv_mv_l] = 1.0;

	sc->p->pfv_initial[pfv_warp_scale] = 1.0;

	sc->p->pfv_initial[pfv_video_echo_zoom] = 1.0;

	sc->p->pfv_initial[pfv_image1_x] = 0.5;
	sc->p->pfv_initial[pfv_image1_y] = 0.5;
	sc->p->pfv_initial[pfv_image1_zoom] = 1.0;
	sc->p->pfv_initial[pfv_image2_x] = 0.5;
	sc->p->pfv_initial[pfv_image2_y] = 0.5;
	sc->p->pfv_initial[pfv_image2_zoom] = 1.0;
}

static void set_initial(struct compiler_sc *sc, int pfv, float x)
{
	sc->p->pfv_initial[pfv] = x;
}

static void initial_to_pfv(struct compiler_sc *sc, int pfv)
{
	int r;

	r = sc->p->pfv_allocation[pfv];
	if(r < 0) return;
	sc->p->perframe_regs[r] = sc->p->pfv_initial[pfv];
}

static void all_initials_to_pfv(struct compiler_sc *sc)
{
	int i;
	for(i=0;i<COMP_PFV_COUNT;i++)
		initial_to_pfv(sc, i);
}

static void pfv_bind_callback(void *_sc, const char *sym, int reg)
{
	struct compiler_sc *sc = _sc;
	int pfv;

	pfv = pfv_from_name(sym);
	if(pfv >= 0) {
		pfv_update_patch_requires(sc, pfv);
		sc->p->pfv_allocation[pfv] = reg;
	}
}

static bool init_pfv(struct compiler_sc *sc)
{
	int i;

	fpvm_init(&sc->pfv_fragment, 0);
	fpvm_set_bind_mode(&sc->pfv_fragment, FPVM_BIND_ALL);
	for(i=0;i<COMP_PFV_COUNT;i++)
		sc->p->pfv_allocation[i] = -1;
	fpvm_set_bind_callback(&sc->pfv_fragment, pfv_bind_callback, sc);
	return true;
}

static bool finalize_pfv(struct compiler_sc *sc)
{
	/* assign dummy values for output */
	if(!fpvm_chunk(&sc->pfv_fragment, FINISH_PFV_FNP))
		goto fail_fpvm;
	#ifdef COMP_DEBUG
	printf("per-frame FPVM fragment:\n");
	fpvm_dump(&sc->pfv_fragment);
	#endif

	return true;
fail_fpvm:
	comp_report(sc, "failed to finalize per-frame variables: %s",
	    fpvm_get_last_error(&sc->pfv_fragment));
	return false;
}

static bool schedule_pfv(struct compiler_sc *sc)
{
	sc->p->perframe_prog_length = fpvm_default_schedule(&sc->pfv_fragment,
		(unsigned int *)sc->p->perframe_prog,
		(unsigned int *)sc->p->perframe_regs);
	if(sc->p->perframe_prog_length < 0) {
		comp_report(sc, "per-frame VLIW scheduling failed");
		return false;
	}
	all_initials_to_pfv(sc);
	#ifdef COMP_DEBUG
	printf("per-frame PFPU fragment:\n");
	pfpu_dump(sc->p->perframe_prog, sc->p->perframe_prog_length);
	#endif

	return true;
}

/****************************************************************/
/* PER-VERTEX VARIABLES                                         */
/****************************************************************/

const char pvv_names[COMP_PVV_COUNT][FPVM_MAXSYMLEN] = {
	/* System */
	"_texsize",
	"_hmeshsize",
	"_vmeshsize",

	/* MilkDrop */
	"sx",
	"sy",
	"cx",
	"cy",
	"rot",
	"dx",
	"dy",
	"zoom",

	"time",
	"bass",
	"mid",
	"treb",
	"bass_att",
	"mid_att",
	"treb_att",

	"warp",
	"fWarpAnimSpeed",
	"fWarpScale",

	"q1",
	"q2",
	"q3",
	"q4",
	"q5",
	"q6",
	"q7",
	"q8",

	"idmx1",
	"idmx2",
	"idmx3",
	"idmx4",
	"idmx5",
	"idmx6",
	"idmx7",
	"idmx8",

	"osc1",
	"osc2",
	"osc3",
	"osc4",

	"midi1",
	"midi2",
	"midi3",
	"midi4",
	"midi5",
	"midi6",
	"midi7",
	"midi8",
};

static int pvv_from_name(const char *name)
{
	int i;

	for(i=0;i<COMP_PVV_COUNT;i++) {
		if(strcmp(pvv_names[i], name) == 0)
			return i;
	}
	return -1;
}

static void pvv_update_patch_requires(struct compiler_sc *sc, int pvv)
{
	if(pvv >= pvv_idmx1 && pvv <= pvv_idmx8)
		sc->p->require |= REQUIRE_DMX;
	if(pvv >= pvv_osc1 && pvv <= pvv_osc4)
		sc->p->require |= REQUIRE_OSC;
	if(pvv >= pvv_midi1 && pvv <= pvv_midi8)
		sc->p->require |= REQUIRE_MIDI;
}

static void pvv_bind_callback(void *_sc, const char *sym, int reg)
{
	struct compiler_sc *sc = _sc;
	int pvv;

	pvv = pvv_from_name(sym);
	if(pvv >= 0) {
		pvv_update_patch_requires(sc, pvv);
		sc->p->pvv_allocation[pvv] = reg;
	}
}

static bool init_pvv(struct compiler_sc *sc)
{
	int i;

	fpvm_init(&sc->pvv_fragment, 1);
	for(i=0;i<COMP_PVV_COUNT;i++)
		sc->p->pvv_allocation[i] = -1;
	fpvm_set_bind_callback(&sc->pvv_fragment, pvv_bind_callback, sc);

	fpvm_set_bind_mode(&sc->pvv_fragment, FPVM_BIND_SOURCE);
	if(!fpvm_chunk(&sc->pvv_fragment, INIT_PVV_FNP))
		goto fail_assign;
	fpvm_set_bind_mode(&sc->pvv_fragment, FPVM_BIND_ALL);

	return true;

fail_assign:
	comp_report(sc, "failed to add equation to per-vertex header: %s",
	    fpvm_get_last_error(&sc->pvv_fragment));
	return false;
}

static int finalize_pvv(struct compiler_sc *sc)
{
	fpvm_set_bind_mode(&sc->pvv_fragment, FPVM_BIND_SOURCE);

	if(!fpvm_chunk(&sc->pvv_fragment, FINISH_PVV_FNP))
		goto fail_assign;
	if(!fpvm_finalize(&sc->pvv_fragment)) goto fail_finalize;
	#ifdef COMP_DEBUG
	printf("per-vertex FPVM fragment:\n");
	fpvm_dump(&sc->pvv_fragment);
	#endif

	return true;

fail_assign:
	comp_report(sc, "failed to add equation to per-vertex footer: %s",
	    fpvm_get_last_error(&sc->pvv_fragment));
	return false;

fail_finalize:
	comp_report(sc, "failed to finalize per-vertex variables: %s",
	    fpvm_get_last_error(&sc->pvv_fragment));
	return false;
}

static bool schedule_pvv(struct compiler_sc *sc)
{
	sc->p->pervertex_prog_length = fpvm_default_schedule(&sc->pvv_fragment,
		(unsigned int *)sc->p->pervertex_prog,
		(unsigned int *)sc->p->pervertex_regs);
	if(sc->p->pervertex_prog_length < 0) {
		comp_report(sc, "per-vertex VLIW scheduling failed");
		return false;
	}
	#ifdef COMP_DEBUG
	printf("per-vertex PFPU fragment:\n");
	pfpu_dump(sc->p->pervertex_prog, sc->p->pervertex_prog_length);
	#endif

	return true;
}

/****************************************************************/
/* PARSING                                                      */
/****************************************************************/

static const char *assign_default(struct parser_comm *comm,
    const char *label, struct ast_node *node)
{
	struct compiler_sc *sc = comm->u.sc;
	int pfv;
	float v;

	pfv = pfv_from_name(label);
	if(pfv < 0)
		return strdup("unknown parameter");

	switch(node->op) {
	case op_constant:
		v = node->contents.constant;
		break;
	case op_not:
		if(node->contents.branches.a->op == op_constant) {
			v = -node->contents.branches.a->contents.constant;
			break;
		}
		/* fall through */
	default:
		return strdup("value must be a constant");
	}

	/* patch initial condition or global parameter */
	pfv_update_patch_requires(sc, pfv);
	set_initial(sc, pfv, v);
	return NULL;
}

static const char *assign_fragment(struct fpvm_fragment *frag,
    const char *label, struct ast_node *node)
{
	if(fpvm_do_assign(frag, label, node))
		return NULL;
	else
		return strdup(fpvm_get_last_error(frag));
}

static const char *assign_per_frame(struct parser_comm *comm,
    const char *label, struct ast_node *node)
{
	return assign_fragment(&comm->u.sc->pfv_fragment, label, node);
}

static const char *assign_per_vertex(struct parser_comm *comm,
    const char *label, struct ast_node *node)
{
	return assign_fragment(&comm->u.sc->pvv_fragment, label, node);
}

static const char *assign_image_name(struct parser_comm *comm,
    int number, const char *name)
{
#ifndef STANDALONE
	struct compiler_sc *sc = comm->u.sc;
	char *totalname;

	if(number > IMAGE_COUNT)
		return strdup("image number out of bounds");
	number--;
	
	if(*name == '/')
		totalname = strdup(name);
	else {
		totalname = malloc(strlen(sc->basedir) + strlen(name) + 1);
		if(totalname == NULL)
			return strdup("out of memory");
		strcpy(totalname, sc->basedir);
		strcat(totalname, name);
	}
	pixbuf_dec_ref(sc->p->images[number]);
	sc->p->images[number] = pixbuf_get(totalname);
	free(totalname);
#endif /* STANDALONE */
	return NULL;
}

static bool parse_patch(struct compiler_sc *sc, const char *patch_code)
{
	struct parser_comm comm = {
		.u.sc = sc,
		.assign_default = assign_default,
		.assign_per_frame = assign_per_frame,
		.assign_per_vertex = assign_per_vertex,
		.assign_image_name = assign_image_name,
	};
	const char *error;

	error = fpvm_parse(patch_code, TOK_START_ASSIGN, &comm);
	if(error) {
		sc->rmc(error);
		free((void *) error);
	}
	return !error;
}

struct patch *patch_compile(const char *basedir, const char *patch_code,
    report_message rmc)
{
	struct compiler_sc *sc;
	struct patch *p;
	int i;

	sc = malloc(sizeof(struct compiler_sc));
	if(sc == NULL) {
		rmc("Failed to allocate memory for compiler internal data");
		return NULL;
	}
	p = sc->p = malloc(sizeof(struct patch));
	if(sc->p == NULL) {
		rmc("Failed to allocate memory for patch");
		free(sc);
		return NULL;
	}
	for(i=0;i<IMAGE_COUNT;i++)
		sc->p->images[i] = NULL;
	sc->p->require = 0;
	sc->p->original = NULL;
	sc->p->next = NULL;

	sc->basedir = basedir;
	sc->rmc = rmc;
	sc->linenr = 0;

	load_defaults(sc);
	if(!init_pfv(sc)) goto fail;
	if(!init_pvv(sc)) goto fail;

	if(!parse_patch(sc, patch_code))
		goto fail;

	if(!finalize_pfv(sc)) goto fail;
	if(!schedule_pfv(sc)) goto fail;
	if(!finalize_pvv(sc)) goto fail;
	if(!schedule_pvv(sc)) goto fail;

	unique_free();

	free(sc);
	return p;

fail:
	free(sc->p);
	free(sc);
	return NULL;
}

struct patch *patch_compile_filename(const char *filename,
    const char *patch_code, report_message rmc)
{
	char *basedir;
	char *c;
	struct patch *p;

	basedir = strdup(filename);
	if(basedir == NULL) return NULL;
	c = strrchr(basedir, '/');
	if(c != NULL) {
		c++;
		*c = 0;
		p = patch_compile(basedir, patch_code, rmc);
	} else
		p = patch_compile("/", patch_code, rmc);
	free(basedir);
	return p;
}

#ifndef STANDALONE

struct patch *patch_copy(struct patch *p)
{
	struct patch *new_patch;
	int i;

	new_patch = malloc(sizeof(struct patch));
	assert(new_patch != NULL);
	memcpy(new_patch, p, sizeof(struct patch));
	new_patch->original = p;
	new_patch->next = NULL;
	for(i=0;i<IMAGE_COUNT;i++)
		pixbuf_inc_ref(new_patch->images[i]);
	return new_patch;
}

void patch_free(struct patch *p)
{
	int i;

	for(i=0;i<IMAGE_COUNT;i++)
		pixbuf_dec_ref(p->images[i]);
	free(p);
}

#endif
