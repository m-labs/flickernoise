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

#include "color.h"
#include "png.h"

#ifdef PNG_FLOATING_ARITHMETIC_SUPPORTED
#warning Floating point PNG is slow
#endif

int png_write(unsigned char *outbuffer, const char *filename,
	     int width, int height, int interlace)
{
	int i;
	png_bytep row_pointers[height];
	png_structp png_ptr;
	png_infop info_ptr;
	FILE *outfile;

	for(i=0; i<height; i++)
		row_pointers[i] = outbuffer + i * width * 3;

	outfile = fopen(filename, "w");
	if(outfile == NULL) {
		perror("Error: Couldn't fopen");
		return -1;
	}

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!png_ptr) {
		fprintf(stderr, "Error: Couldn't create PNG write struct\n");
		goto free0;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr) {
		fprintf(stderr, "Error: Couldn't create PNG info struct\n");
		goto free1;
	}

	if(setjmp(png_jmpbuf(png_ptr))) goto free1;

	png_init_io(png_ptr, outfile);

	png_set_compression_level(png_ptr, Z_DEFAULT_COMPRESSION);
	png_set_bgr(png_ptr);
	png_set_IHDR(png_ptr, info_ptr, width, height,
		     8, PNG_COLOR_TYPE_RGB, interlace,
		     PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	png_write_info(png_ptr, info_ptr);
	png_write_image(png_ptr, row_pointers);
	png_write_end(png_ptr, info_ptr);

free1:
	png_destroy_write_struct(&png_ptr, &info_ptr);

free0:
	fclose(outfile);

	return 0;
}
