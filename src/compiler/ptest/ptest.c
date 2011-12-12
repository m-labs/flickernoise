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

#include "../fpvm.h"
#include "../parser_helper.h"
#include "../parser.h"


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


const char *fpvm_get_last_error(struct fpvm_fragment *fragment)
{
	return fragment->last_error;
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


static void usage(const char *name)
{
	fprintf(stderr, "usage: %s [-f error] [-q] [expr]\n", name);
	exit(1);
}


int main(int argc, char **argv)
{
	int c;
	const char *buf;
	struct fpvm_fragment fragment;
	struct parser_comm comm = {
		.u.fragment = &fragment,
		.assign_default = assign_default,
		.assign_per_frame = assign_per_frame,
		.assign_per_vertex = assign_per_vertex,
		.assign_image_name = assign_image_name,
	 };
	const char *error;

	while ((c = getopt(argc, argv, "f:q")) != EOF)
		switch (c) {
		case 'f':
			fail = optarg;
			break;
		case 'q':
			quiet = 1;
			break;
		default:
			usage(*argv);
		}
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

	error = fpvm_parse(buf, TOK_START_ASSIGN, &comm);
	if (!error)
		return 0;
	fflush(stdout);
	fprintf(stderr, "%s\n", error);
	return 1;
}
