/*
 * Flickernoise
 * Copyright (C) 2010 Sebastien Bourdeauducq
 * Copyright (C) 2002-2009 Norman Feske <norman.feske@genode-labs.com>
 * Genode Labs, Feske & Helmuth Systementwicklung GbR
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

#ifndef __FONT_H
#define __FONT_H

extern unsigned char vera20_tff[];

struct font_context {
	unsigned char *font;
	unsigned short *fb;
	int fb_w, fb_h;
};

void font_init_context(struct font_context *ctx, unsigned char *font, unsigned short *fb, int fb_w, int fb_h);
int font_get_height(struct font_context *ctx);
int font_draw_char(struct font_context *ctx, int x, int y, int r, unsigned char c);
void font_draw_string(struct font_context *ctx, int x, int y, int r, char *str);

#endif /* __FONT_H */

