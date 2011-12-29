/*
 * ptest.c - FNP parser tester
 *
 * Copyright 2011 by Werner Almesberger
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

#include "../parser_helper.h"
#include "../parser.h"
#include "../compiler.h"


static int quiet = 0;
static const char *fail = NULL;


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
		printf("%s", ast->label);
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
	case op_not:
		op("!", ast);
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
	default:
		abort();
	}
}


static const char *assign_default(struct parser_comm *comm,
    const char *label, struct ast_node *node)
{
	if (fail)
		return strdup(fail);
	if (!quiet) {
		printf("%s = ", label);
		dump_ast(node);
		putchar('\n');
	}
	return NULL;
}


static const char *assign_per_frame(struct parser_comm *comm,
    const char *label, struct ast_node *node)
{
	if (!quiet) {
		printf("per_frame = %s = ", label);
		dump_ast(node);
		putchar('\n');
	}
	return NULL;
}


static const char *assign_per_vertex(struct parser_comm *comm,
    const char *label, struct ast_node *node)
{
	if (!quiet) {
		printf("per_vertex = %s = ", label);
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
	exit(1);
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
	struct fpvm_fragment fragment;
	struct parser_comm comm = {
		.u.fragment = &fragment,
		.assign_default = assign_default,
		.assign_per_frame = assign_per_frame,
		.assign_per_vertex = assign_per_vertex,
		.assign_image_name = assign_image_name,
	 };
	const char *error;

	error = parse(pgm, TOK_START_ASSIGN, &comm);
	if (!error)
		return;
	fflush(stdout);
	fprintf(stderr, "%s\n", error);
	free((void *) error);
	exit(1);
}


static void dump_regs(const int *alloc, const char (*names)[FPVM_MAXSYMLEN],
    const float *values, int n)
{
	const char *mapped[n];
	int i;

	for (i = 0; i != n; i++)
		mapped[i] = NULL;
	for (i = 0; i != n; i++)
		if (alloc[i] != -1)
			mapped[alloc[i]] = names[i];
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

	printf("global:\n");
	for (i = 0; i != COMP_PFV_COUNT; i++)
		if (patch->pfv_initial[i])
			printf("R%03d = %f %s\n", i, patch->pfv_initial[i],
			    pfv_names[i]);
	printf("per-frame PFPU fragment:\n");
	dump_regs(patch->pfv_allocation, pfv_names, patch->perframe_regs,
	    COMP_PFV_COUNT);
	pfpu_dump(patch->perframe_prog, patch->perframe_prog_length);
	printf("per-vertex PFPU fragment:\n");
	dump_regs(patch->pvv_allocation, pvv_names, patch->pervertex_regs,
	    COMP_PVV_COUNT);
	pfpu_dump(patch->pervertex_prog, patch->pervertex_prog_length);
}


static void compile(const char *pgm)
{
	struct patch *patch;

	patch = patch_compile("/", pgm, report);
	if (!patch)
		exit(1);
	if (!quiet)
		show_patch(patch);
	/*
	 * We can't use patch_free here because that function also accesses
	 * image data, which isn't available in standalone builds. A simple
	 * free(3) has the same effect in this case.
	 */
	free(patch);
}


static void usage(const char *name)
{
	fprintf(stderr,
"usage: %s [-c|-f error] [-n runs] [-q] [expr]\n\n"
"  -c        generate code and dump generated code (unless -q is set)\n"
"  -f error  fail any assignment with specified error message\n"
"  -n runs   run compilation repeatedly (default: run only once)\n"
"  -q        quiet operation\n"
    , name);
	exit(1);
}


int main(int argc, char **argv)
{
	int c;
	const char *buf;
	int codegen = 0;
	unsigned long repeat = 1;
	char *end;

	while ((c = getopt(argc, argv, "cf:n:q")) != EOF)
		switch (c) {
		case 'c':
			codegen = 1;
			break;
		case 'f':
			fail = optarg;
			break;
		case 'n':
			repeat = strtoul(optarg, &end, 0);
			if (*end)
				usage(*argv);
			break;
		case 'q':
			quiet = 1;
			break;
		default:
			usage(*argv);
		}

	if (codegen && fail)
		usage(*argv);

	switch (argc-optind) {
	case 0:
		buf = read_stdin();
		break;
	case 1:
		buf = argv[optind];
		break;
	default:
		usage(*argv);
	}

	while (repeat--) {
		if (codegen)
			compile(buf);
		else
			parse_only(buf);
	}

	if (argc == optind)
		free((void *) buf);

	return 0;
}
