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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <fpvm/fpvm.h>
#include <fpvm/schedulers.h>
#include <fpvm/pfpu.h>

#include "../pixbuf/pixbuf.h"
#include "compiler.h"

struct compiler_sc {
	struct patch *p;

	const char *basedir;
	report_message rmc;
	int linenr;

	struct fpvm_fragment pfv_fragment;
	int pfv_preallocation[COMP_PFV_COUNT];	/* < where per-frame variables can be mapped in PFPU regf */

	struct fpvm_fragment pvv_fragment;
	int pvv_preallocation[COMP_PVV_COUNT];	/* < where per-vertex variables can be mapped in PFPU regf */
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

static const char pfv_names[COMP_PFV_COUNT][FPVM_MAXSYMLEN] = {
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

static int pfv_from_name(struct compiler_sc *sc, const char *name)
{
	int i;
	for(i=0;i<COMP_PFV_COUNT;i++) {
		if(strcmp(pfv_names[i], name) == 0) {
			if(i >= pfv_dmx1 && i <= pfv_idmx8)	sc->p->require |= REQUIRE_DMX;
			if(i >= pfv_osc1 && i <= pfv_osc4)	sc->p->require |= REQUIRE_OSC;
			if(i >= pfv_midi1 && i <= pfv_midi8)	sc->p->require |= REQUIRE_MIDI;
			if(i == pfv_video_a)			sc->p->require |= REQUIRE_VIDEO;
			return i;
		}
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
	
	sc->p->pfv_initial[pfv_image1_zoom] = 1.0;
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

static bool init_pfv(struct compiler_sc *sc)
{
	int i;

	fpvm_init(&sc->pfv_fragment, 0);
	sc->pfv_fragment.bind_mode = 1; /* < keep user-defined variables from frame to frame */
	for(i=0;i<COMP_PFV_COUNT;i++) {
		sc->pfv_preallocation[i] = fpvm_bind(&sc->pfv_fragment, pfv_names[i]);
		if(sc->pfv_preallocation[i] == FPVM_INVALID_REG) {
			comp_report(sc, "failed to bind per-frame variable %s: %s", pfv_names[i], sc->pfv_fragment.last_error);
			return false;
		}
	}
	return true;
}

static bool finalize_pfv(struct compiler_sc *sc)
{
	int i;
	int references[FPVM_MAXBINDINGS];

	/* assign dummy values for output */
	if(!fpvm_assign(&sc->pfv_fragment, "_Xo", "_Xi")) goto fail_fpvm;
	if(!fpvm_assign(&sc->pfv_fragment, "_Yo", "_Yi")) goto fail_fpvm;
	if(!fpvm_finalize(&sc->pfv_fragment)) goto fail_fpvm;
	#ifdef COMP_DEBUG
	printf("per-frame FPVM fragment:\n");
	fpvm_dump(&sc->pfv_fragment);
	#endif

	/* Build variable allocation table */
	fpvm_get_references(&sc->pfv_fragment, references);
	for(i=0;i<COMP_PFV_COUNT;i++)
		if(references[sc->pfv_preallocation[i]])
			sc->p->pfv_allocation[i] = sc->pfv_preallocation[i];
		else
			sc->p->pfv_allocation[i] = -1;

	return true;
fail_fpvm:
	comp_report(sc, "failed to finalize per-frame variables: %s", sc->pfv_fragment.last_error);
	return false;
}

static bool schedule_pfv(struct compiler_sc *sc)
{
	sc->p->perframe_prog_length = fpvm_default_schedule(&sc->pfv_fragment,
		(unsigned int *)sc->p->perframe_prog, (unsigned int *)sc->p->perframe_regs);
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

static bool add_per_frame(struct compiler_sc *sc, char *dest, char *val)
{
	if(!fpvm_assign(&sc->pfv_fragment, dest, val)) {
		comp_report(sc, "failed to add per-frame equation l. %d: %s", sc->linenr, sc->pfv_fragment.last_error);
		return false;
	}
	return true;
}

/****************************************************************/
/* PER-VERTEX VARIABLES                                         */
/****************************************************************/

static const char pvv_names[COMP_PVV_COUNT][FPVM_MAXSYMLEN] = {
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

static bool init_pvv(struct compiler_sc *sc)
{
	int i;

	fpvm_init(&sc->pvv_fragment, 1);

	for(i=0;i<COMP_PVV_COUNT;i++) {
		sc->pvv_preallocation[i] = fpvm_bind(&sc->pvv_fragment, pvv_names[i]);
		if(sc->pvv_preallocation[i] == FPVM_INVALID_REG) {
			comp_report(sc, "failed to bind per-vertex variable %s: %s", pvv_names[i], sc->pvv_fragment.last_error);
			return false;
		}
	}

	#define A(dest, val) if(!fpvm_assign(&sc->pvv_fragment, dest, val)) goto fail_assign
	A("x", "i2f(_Xi)*_hmeshsize");
	A("y", "i2f(_Yi)*_vmeshsize");
	A("rad", "sqrt(sqr(x-0.5)+sqr(y-0.5))");
	/* TODO: generate ang */
	#undef A

	return true;

fail_assign:
	comp_report(sc, "failed to add equation to per-vertex header: %s", sc->pvv_fragment.last_error);
	return false;
}

static int finalize_pvv(struct compiler_sc *sc)
{
	int i;
	int references[FPVM_MAXBINDINGS];

	#define A(dest, val) if(!fpvm_assign(&sc->pvv_fragment, dest, val)) goto fail_assign

	/* Zoom */
	A("_invzoom", "1/zoom");
	A("_xz", "_invzoom*(x-0.5)+0.5");
	A("_yz", "_invzoom*(y-0.5)+0.5");

	/* Scale */
	A("_xs", "(_xz-cx)/sx+cx");
	A("_ys", "(_yz-cy)/sy+cy");

	/* Warp */
	A("_warptime", "time*fWarpAnimSpeed");
	A("_invwarpscale", "1/fWarpScale");
	A("_f0", "11.68 + 4.0*cos(_warptime*1.413 + 10)");
	A("_f1", "8.77 + 3.0*cos(_warptime*1.113 + 7)");
	A("_f2", "10.54 + 3.0*cos(_warptime*1.233 + 3)");
	A("_f3", "11.49 + 4.0*cos(_warptime*0.933 + 5)");
	A("_ox2", "2*x-1");
	A("_oy2", "2*y-1");
	A("_xw", "_xs+warp*0.0035*("
		"sin(_warptime*0.333+_invwarpscale*(_ox2*_f0-_oy2*_f3))"
		"+cos(_warptime*0.753-_invwarpscale*(_ox2*_f1-_oy2*_f2)))");
	A("_yw", "_ys+warp*0.0035*("
		"cos(_warptime*0.375-_invwarpscale*(_ox2*_f2+_oy2*_f1))"
		"+sin(_warptime*0.825+_invwarpscale*(_ox2*_f0+_oy2*_f3)))");

	/* Rotate */
	A("_cosr", "cos(rot)");
	A("_sinr", "sin(0-rot)");
	A("_u", "_xw-cx");
	A("_v", "_yw-cy");
	A("_xr", "_u*_cosr-_v*_sinr+cx");
	A("_yr", "_u*_sinr+_v*_cosr+cy");

	/* Translate */
	A("_xd", "_xr-dx");
	A("_yd", "_yr-dy");

	/* Convert to framebuffer coordinates */
	A("_Xo", "f2i(_xd*_texsize)");
	A("_Yo", "f2i(_yd*_texsize)");

	#undef A

	if(!fpvm_finalize(&sc->pvv_fragment)) goto fail_finalize;
	#ifdef COMP_DEBUG
	printf("per-vertex FPVM fragment:\n");
	fpvm_dump(&sc->pvv_fragment);
	#endif

	/* Build variable allocation table */
	fpvm_get_references(&sc->pvv_fragment, references);
	for(i=0;i<COMP_PVV_COUNT;i++)
		if(references[sc->pvv_preallocation[i]])
			sc->p->pvv_allocation[i] = sc->pvv_preallocation[i];
		else
			sc->p->pvv_allocation[i] = -1;


	return true;
fail_assign:
	comp_report(sc, "failed to add equation to per-vertex footer: %s", sc->pvv_fragment.last_error);
	return false;
fail_finalize:
	comp_report(sc, "failed to finalize per-vertex variables: %s", sc->pvv_fragment.last_error);
	return false;
}

static bool schedule_pvv(struct compiler_sc *sc)
{
	sc->p->pervertex_prog_length = fpvm_default_schedule(&sc->pvv_fragment,
		(unsigned int *)sc->p->pervertex_prog, (unsigned int *)sc->p->pervertex_regs);
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

static bool add_per_vertex(struct compiler_sc *sc, char *dest, char *val)
{
	if(!fpvm_assign(&sc->pvv_fragment, dest, val)) {
		comp_report(sc, "failed to add per-vertex equation l. %d: %s\n", sc->linenr, sc->pvv_fragment.last_error);
		return false;
	}
	return true;
}

/****************************************************************/
/* PARSING                                                      */
/****************************************************************/

static bool process_equation(struct compiler_sc *sc, char *equation, bool per_vertex)
{
	char *c, *c2;

	c = strchr(equation, '=');
	if(!c) {
		comp_report(sc, "error l.%d: malformed equation (%s)", sc->linenr, equation);
		return false;
	}
	*c = 0;

	if(*equation == 0) {
		comp_report(sc, "error l.%d: missing lvalue", sc->linenr);
		return false;
	}
	c2 = c - 1;
	while((c2 > equation) && (*c2 == ' ')) *c2-- = 0;

	c++;
	while(*c == ' ') c++;

	if(*equation == 0) {
		comp_report(sc, "error l.%d: missing lvalue", sc->linenr);
		return false;
	}
	if(*c == 0) {
		comp_report(sc, "error l.%d: missing rvalue", sc->linenr);
		return false;
	}

	if(per_vertex)
		return add_per_vertex(sc, equation, c);
	else
		return add_per_frame(sc, equation, c);
}

static bool process_equations(struct compiler_sc *sc, char *equations, bool per_vertex)
{
	char *c;

	while(*equations) {
		c = strchr(equations, ';');
		if(!c)
			return process_equation(sc, equations, per_vertex);
		*c = 0;
		if(!process_equation(sc, equations, per_vertex)) return false;
		equations = c + 1;
	}
	return true;
}

static bool process_top_assign(struct compiler_sc *sc, char *left, char *right)
{
	int pfv;

	while(*right == ' ') right++;
	if(*right == 0) return true;
	
	if(strncmp(left, "imagefile", 9) == 0) {
		int image_n;
		char *totalname;
		
		image_n = atoi(left+9);
		if((image_n < 1) || (image_n > IMAGE_COUNT)) {
			comp_report(sc, "warning l.%d: ignoring image with out of bounds number %d", sc->linenr, image_n);
			return true;
		}
		totalname = malloc(strlen(sc->basedir) + strlen(right) + 0);
		if(totalname == NULL) return true;
		strcpy(totalname, sc->basedir);
		strcat(totalname, right);
		pixbuf_dec_ref(sc->p->images[image_n]);
		sc->p->images[image_n] = pixbuf_get(totalname);
		free(totalname);
	}

	pfv = pfv_from_name(sc, left);
	if(pfv >= 0) {
		/* patch initial condition or global parameter */
		set_initial(sc, pfv, atof(right));
		return true;
	}

	if(strncmp(left, "per_frame", 9) == 0)
		/* per-frame equation */
		return process_equations(sc, right, false);

	if((strncmp(left, "per_vertex", 10) == 0) || (strncmp(left, "per_pixel", 9) == 0))
		/* per-vertex equation */
		return process_equations(sc, right, true);

	comp_report(sc, "warning l.%d: ignoring unknown parameter %s", sc->linenr, left);

	return true;
}

static bool process_line(struct compiler_sc *sc, char *line)
{
	char *c;

	while(*line == ' ') line++;
	if(*line == 0) return true;
	if(*line == '[') return true;

	c = strstr(line, "//");
	if(c) *c = 0;

	c = line + strlen(line) - 1;
	while((c >= line) && (*c == ' ')) *c-- = 0;
	if(*line == 0) return true;

	c = strchr(line, '=');
	if(!c) {
		comp_report(sc, "error l.%d: '=' expected", sc->linenr);
		return false;
	}
	*c = 0;
	return process_top_assign(sc, line, c+1);
}

static bool parse_patch(struct compiler_sc *sc, char *patch_code)
{
	char *eol;

	while(*patch_code) {
		sc->linenr++;
		eol = strchr(patch_code, '\n');
		if(!eol) {
			if(!process_line(sc, patch_code))
				return false;
			else
				return true;
		}
		*eol = 0;
		if(*patch_code == 0) {
			patch_code = eol + 1;
			continue;
		}
		if(*(eol - 1) == '\r') *(eol - 1) = 0;
		if(!process_line(sc, patch_code))
			return false;
		patch_code = eol + 1;
	}

	return true;
}

struct patch *patch_compile(const char *basedir, const char *patch_code, report_message rmc)
{
	struct compiler_sc *sc;
	struct patch *p;
	char *patch_code_copy;
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

	patch_code_copy = strdup(patch_code);
	if(patch_code_copy == NULL) {
		rmc("Failed to allocate memory for patch code");
		goto fail;
	}
	if(!parse_patch(sc, patch_code_copy)) {
		free(patch_code_copy);
		goto fail;
	}
	free(patch_code_copy);

	if(!finalize_pfv(sc)) goto fail;
	if(!schedule_pfv(sc)) goto fail;
	if(!finalize_pvv(sc)) goto fail;
	if(!schedule_pvv(sc)) goto fail;

	free(sc);
	return p;
fail:
	free(sc->p);
	free(sc);
	return NULL;
}

struct patch *patch_compile_filename(const char *filename, const char *patch_code, report_message rmc)
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
