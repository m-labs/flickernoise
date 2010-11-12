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

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <png.h>

#include "color.h"
#include "pngload.h"

unsigned short *png_load(const char *filename, int *w, int *h)
{
	unsigned short *ret;
	FILE *fd;
	unsigned char header[8];
	png_structp png_ptr;
	png_infop info_ptr;
	int width, height;
	png_byte color_type;
	png_byte bit_depth;
	png_bytep *row_pointers;
	size_t rowbytes;
	int x, y;

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

	ret = malloc(width*height*2);
	if(ret == NULL) goto free4;

	for(y=0;y<height;y++) {
		unsigned char *row;
		row = (unsigned char *)row_pointers[y];
		for(x=0;x<width;x++)
			ret[width*y+x] = MAKERGB565N(row[3*x], row[3*x+1], row[3*x+2]);
	}

	*w = width;
	*h = height;

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
