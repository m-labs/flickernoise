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
#include <setjmp.h>
#include <png.h>
#include <zlib.h>

#include "pixbuf.h"
#include "loaders.h"
#include "dither.h"

#ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
#warning Floating point PNG is slow
#endif

struct pixbuf *pixbuf_load_png(char *filename)
{
	struct pixbuf *ret;
	FILE *fd;
	unsigned char header[8];
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned int width, height;
	png_byte color_type;
	png_byte bit_depth;
	png_bytep *row_pointers;
	size_t rowbytes;
	int y;

	ret = NULL;
	fd = fopen(filename, "r");
	if(fd == NULL) goto free0;
	fread(header, 1, 8, fd);
	if(png_sig_cmp(header, 0, 8)) goto free1;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(png_ptr == NULL) goto free1;
	info_ptr = png_create_info_struct(png_ptr);
	if(info_ptr == NULL) goto free2;

	if(setjmp(png_jmpbuf(png_ptr))) goto free3;
	png_init_io(png_ptr, fd);
	png_set_sig_bytes(png_ptr, 8);
	png_read_info(png_ptr, info_ptr);

	width = png_get_image_width(png_ptr, info_ptr);
	height = png_get_image_height(png_ptr, info_ptr);
	color_type = png_get_color_type(png_ptr, info_ptr);
	bit_depth = png_get_bit_depth(png_ptr, info_ptr);

	if(color_type != PNG_COLOR_TYPE_RGB) goto free3;
	if(bit_depth != 8) goto free3;

	row_pointers = calloc(sizeof(png_bytep), height);
	if(row_pointers == NULL) goto free3;
	if(setjmp(png_jmpbuf(png_ptr))) goto free4;
	rowbytes = png_get_rowbytes(png_ptr, info_ptr);
	for(y=0;y<height;y++) {
		row_pointers[y] = malloc(rowbytes);
		if(row_pointers[y] == NULL)
			goto free4;
	}

	png_read_image(png_ptr, row_pointers);

	ret = pixbuf_new(width, height);
	if(ret == NULL) goto free4;
	ret->filename = strdup(filename);
	if(!pixbuf_dither(ret->pixels, row_pointers, width, height)) {
		pixbuf_dec_ref(ret);
		ret = NULL;
		goto free4;
	}

free4:
	for(y=0;y<height;y++)
		free(row_pointers[y]);
	free(row_pointers);
free3:
	png_destroy_info_struct(png_ptr, &info_ptr);
free2:
	png_destroy_read_struct(&png_ptr, NULL, NULL);
free1:
	fclose(fd);
free0:
	return ret;
}