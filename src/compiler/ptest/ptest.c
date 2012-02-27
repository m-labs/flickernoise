/*
 * ptest.c - FNP parser tester
 *
 * Copyright 2011-2012 by Werner Almesberger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 */


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "fpvm/pfpu.h"
#include "fpvm/schedulers.h"

#include "../parser_helper.h"
#include "../parser.h"
#include "../compiler.h"
#include "../symtab.h"


static int quiet = 0;
static int symbols = 0;
static const char *fail = NULL;
static const char *trace_var = NULL;
static const char *buffer;


/* ----- Parse the patch into the AST -------------------------------------- */


static void dump_ast(const struct ast_node *ast);


static void branch(const struct ast_node *ast)
{
	if (ast) {
		putchar(' ');
		dump_ast(ast);
	}
}


static void op(const char *s, const struct ast_node *ast)
{
	printf("(%s", s);
	branch(ast->contents.branches.a);
	branch(ast->contents.branches.b);
	branch(ast->contents.branches.c);
	putchar(')');
}


static void dump_ast(const struct ast_node *ast)
{
	switch (ast->op) {
        case op_ident:
		printf("%s", ast->sym->name);
		break;
	case op_constant:
		printf("%g", ast->contents.constant);
		break;
        case op_plus:
		op("+", ast);
		break;
        case op_minus:
		op("-", ast);
		break;
        case op_multiply:
		op("*", ast);
		break;
        case op_divide:
		op("/", ast);
		break;
        case op_percent:
		op("%", ast);
		break;
	case op_abs:
		op("abs", ast);
		break;
        case op_isin:
		op("isin", ast);
		break;
        case op_icos:
		op("icos", ast);
		break;
        case op_sin:
		op("sin", ast);
		break;
        case op_cos:
		op("cos", ast);
		break;
        case op_above:
		op("above", ast);
		break;
        case op_below:
		op("below", ast);
		break;
        case op_equal:
		op("equal", ast);
		break;
        case op_i2f:
		op("i2f", ast);
		break;
        case op_f2i:
		op("f2i", ast);
		break;
        case op_if:
		op("if", ast);
		break;
        case op_tsign:
		op("tsign", ast);
		break;
        case op_quake:
		op("quake", ast);
		break;
	case op_negate:
		op("-", ast);
		break;
	case op_sqr:
		op("sqr", ast);
		break;
	case op_sqrt:
		op("sqrt", ast);
		break;
	case op_invsqrt:
		op("invsqrt", ast);
		break;
	case op_min:
		op("min", ast);
		break;
        case op_max:
		op("max", ast);
		break;
        case op_int:
		op("int", ast);
		break;
	case op_bnot:
		op("!", ast);
		break;
	case op_band:
		op("&&", ast);
		break;
	case op_bor:
		op("||", ast);
		break;
	default:
		abort();
	}
}


static const char *assign_default(struct parser_comm *comm,
    struct sym *sym, struct ast_node *node)
{
	if (fail)
		return strdup(fail);
	if (!quiet) {
		printf("%s = ", sym->fpvm_sym.name);
		dump_ast(node);
		putchar('\n');
	}
	return NULL;
}


static const char *assign_per_frame(struct parser_comm *comm,
    struct sym *sym, struct ast_node *node)
{
	if (!quiet) {
		printf("per_frame = %s = ", sym->fpvm_sym.name);
		dump_ast(node);
		putchar('\n');
	}
	return NULL;
}


static const char *assign_per_vertex(struct parser_comm *comm,
    struct sym *sym, struct ast_node *node)
{
	if (!quiet) {
		printf("per_vertex = %s = ", sym->fpvm_sym.name);
		dump_ast(node);
		putchar('\n');
	}
	return NULL;
}


static const char *assign_image_name(struct parser_comm *comm,
    int number, const char *name)
{
	if (!quiet)
		printf("image %d = \"%s\"\n", number, name);
	return NULL;
}


static void report(const char *s)
{
	fprintf(stderr, "%s\n", s);
}


static const char *read_stdin(void)
{
	char *buf = NULL;
	int len = 0, size = 0;
	ssize_t got;

	do {
		if (len == size) {
			size += size ? size : 1024;
			buf = realloc(buf, size+1);
			if (!buf) {
				perror("realloc");
				exit(1);
			}
		}
		got = read(0, buf+len, size-len);
		if (got < 0) {
			perror("read");
			exit(1);
		}
		len += got;
	}
	while (got);

	buf[len] = 0;

	return buf;
}


static void parse_only(const char *pgm)
{
	static struct patch patch;
	static struct compiler_sc sc = {
		.p = &patch,
	};
	struct parser_comm comm = {
		.u.sc = &sc,
		.assign_default = assign_default,
		.assign_per_frame = assign_per_frame,
		.assign_per_vertex = assign_per_vertex,
		.assign_image_name = assign_image_name,
	};
	int ok;

	symtab_init();
	ok = parse(pgm, TOK_START_ASSIGN, &comm);
	if (symbols) {
		const struct sym *sym;
		int user = 0;

		foreach_sym(sym) {
			if (!user && !(sym->flags & SF_SYSTEM)) {
				printf("\n");
				user = 1;
			}
			if (sym->flags & SF_CONST)
				printf("%s = %g\n", sym->fpvm_sym.name, sym->f);
			else
				printf("%s\n", sym->fpvm_sym.name);
		}
	}
	symtab_free();
	stim_db_free(); /* @@@ */
	stim_put(patch.stim);
	if (ok)
		return;
	fflush(stdout);
	fprintf(stderr, "%s\n", comm.msg);
	free((void *) comm.msg);
	exit(1);
}


/* ----- Compile the patch into PFPU code ---------------------------------- */


/*
 * "sym" and "field" are used to access a field chosen by the caller in the
 * "struct sym". For this, the caller provides a buffer (sym) and a pointer to
 * the respective field inside the buffer. This way, it's perfectly type-safe
 * and we don't need offsetof acrobatics.
 */

static const char *lookup_name(int idx, struct sym *sym, const int *field)
{
	const struct sym *walk;

	foreach_sym(walk) {
		*sym = *walk;
		if (*field == idx)
			return walk->fpvm_sym.name;
	}
	return NULL;
}


static void dump_regs(const int *alloc, struct sym *sym, const int *field,
    const float *values, int n)
{
	const char *mapped[n];
	int i;

	for (i = 0; i != n; i++)
		mapped[i] = NULL;
	for (i = 0; i != n; i++)
		if (alloc[i] != -1)
			mapped[alloc[i]] = lookup_name(i, sym, field);
	for (i = 0; i != n; i++) {
		if (!values[i] && !mapped[i])
			continue;
		printf("R%03d = %f", i, values[i]);
		if (mapped[i])
			printf(" %s", mapped[i]);
		printf("\n");
	}
}


static void show_patch(const struct patch *patch)
{
	int i;
	struct sym sym;

	printf("global:\n");
	for (i = 0; i != COMP_PFV_COUNT; i++)
		if (patch->pfv_initial[i])
			printf("R%03d = %f %s\n", i, patch->pfv_initial[i],
			    lookup_name(i, &sym, &sym.pfv_idx));
	printf("per-frame PFPU fragment:\n");
	dump_regs(patch->pfv_allocation, &sym, &sym.pfv_idx,
	    patch->perframe_regs, COMP_PFV_COUNT);
	pfpu_dump(patch->perframe_prog, patch->perframe_prog_length);
	printf("per-vertex PFPU fragment:\n");
	dump_regs(patch->pvv_allocation, &sym, &sym.pvv_idx,
	     patch->pervertex_regs, COMP_PVV_COUNT);
	pfpu_dump(patch->pervertex_prog, patch->pervertex_prog_length);
}


static struct midi {
	int chan, ctrl, value;
	struct midi *next;
} *midi = NULL, **last_midi = &midi;


static void add_midi(const char *s)
{
	int chan, ctrl, value;

	if (sscanf(s, "%d.%d=%d", &chan, &ctrl, &value) == 3);
	else if (sscanf(s, "%d=%d", &ctrl, &value) == 2)
		chan = 1;
	else {
		fprintf(stderr, "don't understand \"%s\"\n", s);
		exit(1);
	}
	*last_midi = malloc(sizeof(struct midi));
	if (!*last_midi) {
		perror("malloc");
		exit(1);
	}
	(*last_midi)->chan = chan;
	(*last_midi)->ctrl = ctrl;
	(*last_midi)->value = value;
	(*last_midi)->next = NULL;
	last_midi = &(*last_midi)->next;
}


static void play_midi(struct patch *patch)
{
	struct sym *sym;
	struct sym_stim *r;
	float f = 0;

	sym = unique(trace_var);
	if (!sym) {
		fprintf(stderr, "can't find trace variable \"%s\"\n",
		    trace_var);
		exit(1);
	}
	if (!sym->stim) {
		fprintf(stderr, "\"%s\" is not a control variable\n",
		    trace_var);
		exit(1);
	}
	for (r = sym->stim; r; r = r->next)
		r->regs->pfv = &f;

	while (midi) {
		stim_midi_ctrl(patch->stim,
		    midi->chan, midi->ctrl, midi->value);
		printf("%g\n", f);
		midi = midi->next;
	}
}


static void compile(const char *pgm, int framework)
{
	struct patch *patch;

	patch = patch_do_compile("/", pgm, report, framework);
	if (!patch) {
		symtab_free();
		exit(1);
	}
	if (!quiet)
		show_patch(patch);
	if (trace_var)
		play_midi(patch);
	symtab_free();
	stim_put(patch->stim);
	/*
	 * We can't use patch_free here because that function also accesses
	 * image data, which isn't available in standalone builds. A simple
	 * free(3) has the same effect in this case.
	 */
	free(patch);
}


/* ----- Compile the patch into FPVM code ---------------------------------- */


static const char *assign_raw(struct parser_comm *comm,
    struct sym *sym, struct ast_node *node)
{
	struct fpvm_fragment *frag = comm->u.fragment;

	if(fpvm_do_assign(frag, &sym->fpvm_sym, node))
		return NULL;
	else
		return strdup(fpvm_get_last_error(frag));
}


static const char *assign_raw_fail(struct parser_comm *comm,
    struct sym *sym, struct ast_node *node)
{
	return strdup("unsupported assignment mode");
}


static const char *assign_image_fail(struct parser_comm *comm,
    int number, const char *name)
{
	if (!quiet)
		printf("image %d = \"%s\"\n", number, name);
	return NULL;
}


static void compile_vm(const char *pgm)
{
	struct fpvm_fragment fragment;
	struct parser_comm comm = {
		.u.fragment = &fragment,
		.assign_default = assign_raw,
		.assign_per_frame = assign_raw,
		.assign_per_vertex = assign_raw_fail,
		.assign_image_name = assign_image_fail,
	};
	int ok;

	init_fpvm(&fragment, 0);
	fpvm_set_bind_mode(&fragment, FPVM_BIND_ALL);
	symtab_init();
	ok = parse(pgm, TOK_START_ASSIGN, &comm);

	if (ok)
		fpvm_dump(&fragment);
	symtab_free();
	if (ok)
		return;

	fflush(stdout);
	fprintf(stderr, "%s\n", comm.msg);
	free((void *) comm.msg);
	exit(1);
}


/* ----- Command-line processing ------------------------------------------- */


static void free_buffer(void)
{
	free((void *) buffer);
}


static void usage(const char *name)
{
	fprintf(stderr,
"usage: %s [-c [-c [-c]]|-f error] [-m [chan.]ctrl=value ...] [-n runs]\n"
"       %*s [-q] [-s] [-v var] [-Wwarning ...] [expr]\n\n"
"  -c        generate PFPU code and dump generated code (unless -q is set)\n"
"  -c -c     generate and dump VM code\n"
"  -c -c -c  generate and dump PFPU code (without patch framework)\n"
"  -f error  fail any assignment with specified error message\n"
"  -m [chan.]ctrl=value\n"
"            send a MIDI message to the stimuli subsystem\n"
"  -n runs   run compilation repeatedly (default: run only once)\n"
"  -q        quiet operation\n"
"  -s        dump symbol table after parsing (only if -c is not set)\n"
"  -v var    trace the specified variable (used with -m)\n"
"  -Wwarning enable compiler warning (one of: section, undefined)\n"
    , name, (int) strlen(name), "");
	exit(1);
}


int main(int argc, char **argv)
{
	int c;
	int codegen = 0;
	unsigned long repeat = 1;
	char *end;

	warn_section = 0;
	warn_undefined = 0;

	while ((c = getopt(argc, argv, "cf:m:n:qsv:W:")) != EOF)
		switch (c) {
		case 'c':
			codegen++;
			break;
		case 'f':
			fail = optarg;
			break;
		case 'm':
			add_midi(optarg);
			break;
		case 'n':
			repeat = strtoul(optarg, &end, 0);
			if (*end)
				usage(*argv);
			break;
		case 'q':
			quiet = 1;
			break;
		case 's':
			symbols = 1;
			break;
		case 'v':
			trace_var = optarg;
			break;
		case 'W':
			if (!strcmp(optarg, "section"))
				warn_section = 1;
			else if (!strcmp(optarg, "undefined"))
				warn_undefined = 1;
			else
				usage(*argv);
			break;
		default:
			usage(*argv);
		}

	if (codegen && (fail || symbols))
		usage(*argv);

	switch (argc-optind) {
	case 0:
		buffer = read_stdin();
		atexit(free_buffer);
		break;
	case 1:
		buffer = argv[optind];
		break;
	default:
		usage(*argv);
	}

	while (repeat--)
		switch (codegen) {
		case 0:
			parse_only(buffer);
			break;
		case 1:
			compile(buffer, 1);
			break;
		case 2:
			compile_vm(buffer);
			break;
		case 3:
			compile(buffer, 0);
			break;
		default:
			usage(*argv);
		}

	return 0;
}
