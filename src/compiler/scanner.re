/*
 * Milkymist SoC (Software)
 * Copyright (C) 2007, 2008, 2009, 2010 Sebastien Bourdeauducq
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "symtab.h"
#include "scanner.h"

#define YYCTYPE     unsigned char
#define YYCURSOR    s->cursor
#define YYLIMIT     s->limit

/* refilling not supported */
#define YYMARKER s->marker
#define YYFILL(n)

#define	YYCONDTYPE		enum scanner_cond
#define	YYGETCONDITION()	s->cond
#define	YYSETCONDITION(c)	s->cond = (c)

struct scanner *new_scanner(unsigned char *input)
{
	struct scanner *s;

	s = malloc(sizeof(struct scanner));
	if(s == NULL) return NULL;

	s->cond = yycN;
	s->marker = input;
	s->old_cursor = input;
	s->cursor = input;
	s->limit = input + strlen((char *)input);
	s->lineno = 1;

	return s;
}

void delete_scanner(struct scanner *s)
{
	free(s);
}

static int nls(const unsigned char *s, const unsigned char *end)
{
	int n = 0;

	while(s != end)
		if(*s++ == '\n')
			n++;
	return n;
}

/*
 * Regular expression for C-style comments by Stephen Ostermiller, from
 * http://ostermiller.org/findcomment.html
 */

int scan(struct scanner *s)
{
	std:
	if(s->cursor == s->limit) return TOK_EOF;
	s->old_cursor = s->cursor;

	/*!re2c
		<*>[\x00]		{ abort(); }

		<*>[\x20\r\t]		{ goto std; }
		<*>"\n"			{ s->lineno++;
					  YYSETCONDITION(yycN);
					  goto std; }

		<N>"//"[^\n\x00]*	{ goto std; }
		<N>"/*"([^*\x00]|("*"+([^*/\x00])))*"*"+"/"
					{ s->lineno += nls(s->old_cursor,
					      s->cursor);
					  goto std; }

		<N>"[preset00]"		{ goto std; }

		<N>[0-9]+		{ return TOK_CONSTANT; }
		<N>[0-9]+ "." [0-9]*	{ return TOK_CONSTANT; }
		<N>[0-9]* "." [0-9]+	{ return TOK_CONSTANT; }

		<N>"above"		{ return TOK_ABOVE; }
		<N>"abs"		{ return TOK_ABS; }
		<N>"below"		{ return TOK_BELOW; }
		<N>"cos"		{ return TOK_COS; }
		<N>"equal"		{ return TOK_EQUAL; }
		<N>"f2i"		{ return TOK_F2I; }
		<N>"icos"		{ return TOK_ICOS; }
		<N>"i2f"		{ return TOK_I2F; }
		<N>"if"			{ return TOK_IF; }
		<N>"int"		{ return TOK_INT; }
		<N>"invsqrt"		{ return TOK_INVSQRT; }
		<N>"isin"		{ return TOK_ISIN; }
		<N>"max"		{ return TOK_MAX; }
		<N>"min"		{ return TOK_MIN; }
		<N>"quake"		{ return TOK_QUAKE; }
		<N>"sin"		{ return TOK_SIN; }
		<N>"sqr"		{ return TOK_SQR; }
		<N>"sqrt"		{ return TOK_SQRT; }
		<N>"tsign"		{ return TOK_TSIGN; }

		<N>"per_frame"		{ return TOK_PER_FRAME; }
		<N>"per_vertex"		{ return TOK_PER_VERTEX; }
		<N>"per_frame"[a-z_0-9]+
					{ return TOK_PER_FRAME_UGLY; }
		<N>"per_vertex"[a-z_0-9]+
					{ return TOK_PER_VERTEX_UGLY; }
		<N>"per_pixel"[a-z_0-9]*
					{ return TOK_PER_PIXEL_UGLY; }

		<N>"imagefile"[1-9]	{ YYSETCONDITION(yycFNAME1);
					  return TOK_IMAGEFILE; }

		<N>[a-zA-Z_0-9]+	{ return TOK_IDENT; }

		<N>"+"			{ return TOK_PLUS; }
		<N>"-"			{ return TOK_MINUS; }
		<N>"*"			{ return TOK_MULTIPLY; }
		<N>"/"			{ return TOK_DIVIDE; }
		<N>"%"			{ return TOK_PERCENT; }
		<N>"("			{ return TOK_LPAREN; }
		<N>")"			{ return TOK_RPAREN; }
		<N>","			{ return TOK_COMMA; }
		<N>"?"			{ return TOK_QUESTION; }
		<N>":"			{ return TOK_COLON; }
		<N>"!"			{ return TOK_NOT; }
		<N>"=="			{ return TOK_EQ; }
		<N>"!="			{ return TOK_NE; }
		<N>"<"			{ return TOK_LT; }
		<N>">"			{ return TOK_GT; }
		<N>"<="			{ return TOK_LE; }
		<N>">="			{ return TOK_GE; }

		<N,FNAME1>"="		{ if (YYGETCONDITION() == yycFNAME1)
						YYSETCONDITION(yycFNAME2);
					  return TOK_ASSIGN; }
		<N>";"			{ return TOK_SEMI; }

		<FNAME2>[^ \x00\n\r\t]|[^ \x00\n\r\t][^\n\x00]*[^ \x00\n\r\t]
					{ return TOK_FNAME; }

		<*>[\x01-\xff]		{ return TOK_ERROR; }
	*/
}

struct sym *get_symbol(struct scanner *s)
{
	return unique_n((const char *) s->old_cursor,
	    s->cursor - s->old_cursor);
}

const char *get_name(struct scanner *s)
{
	char *buf;
	int n;

	n = s->cursor - s->old_cursor;
	buf = malloc(n+1);
	memcpy(buf, s->old_cursor, n);
	buf[n] = 0;
	return buf;
}

float get_constant(struct scanner *s)
{
	const unsigned char *p;
	float v = 0;
	float m = 1;

	for(p = s->old_cursor; p != s->cursor; p++) {
		if(*p == '.')
			goto dot;
		v = v*10+(*p-'0');
	}
	return v;

dot:
	for(p++; p != s->cursor; p++) {
		m /= 10;
		v += m*(*p-'0');
	}
	return v;
}
