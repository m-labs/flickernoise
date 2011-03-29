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

#include "color.h"
#include "pngload.h"

static void floyd_steinberg(int *pic, int width, int height)
{
	int x, y;
	int offset;
	int old_r, old_g, old_b;
	int new_r, new_g, new_b;
	int qe_r, qe_g, qe_b;
	
	for(y=0;y<height;y++)
		for(x=0;x<width;x++) {
			offset = 3*(width*y+x);
			old_r = pic[offset];
			old_g = pic[offset+1];
			old_b = pic[offset+2];
			if(old_r > 0x00f80000)
				new_r = 0x00f80000;
			else if(old_r > 0)
				new_r = old_r & 0x00f80000;
			else
				new_r = 0;
			if(old_g > 0x00fc0000)
				new_g = 0x00fc0000;
			else if(old_g > 0)
				new_g = old_g & 0x00fc0000;
			else
				new_g = 0;
			if(old_b > 0x00f80000)
				new_b = 0x00f80000;
			else if(old_b > 0)
				new_b = old_b & 0x00f80000;
			else
				new_b = 0;
			pic[offset] = new_r;
			pic[offset+1] = new_g;
			pic[offset+2] = new_b;
			qe_r = old_r - new_r;
			qe_g = old_g - new_g;
			qe_b = old_b - new_b;
			
			if((x+1) < width) {
				pic[offset+3] += (qe_r*7) >> 4;
				pic[offset+3+1] += (qe_g*7) >> 4;
				pic[offset+3+2] += (qe_b*7) >> 4;
			}
			if((y+1) < height) {
				offset += 3*width;
				if(x > 0) {
					pic[offset-3] += (qe_r*3) >> 4;
					pic[offset-3+1] += (qe_g*3) >> 4;
					pic[offset-3+2] += (qe_b*3) >> 4;
				}
				pic[offset] += (qe_r*5) >> 4;
				pic[offset+1] += (qe_g*5) >> 4;
				pic[offset+2] += (qe_b*5) >> 4;
				if((x+1) < width) {
					pic[offset+3] += qe_r >> 4;
					pic[offset+3+1] += qe_g >> 4;
					pic[offset+3+2] += qe_b >> 4;
				}
			}
		}
}

static int dither(unsigned short *ret, png_bytep *row_pointers, int width, int height)
{
	int x, y;
	unsigned char *row;
	int offset;
	int *pic;
	
	pic = malloc(width*height*3*sizeof(int));
	if(pic == NULL) return 0;
	
	for(y=0;y<height;y++) {
		row = (unsigned char *)row_pointers[y];
		for(x=0;x<width;x++) {
			offset = 3*(width*y+x);
			pic[offset] = ((unsigned int)row[3*x]) << 16;
			pic[offset+1] = ((unsigned int)row[3*x+1]) << 16;
			pic[offset+2] = ((unsigned int)row[3*x+2]) << 16;
		}
	}
	
	floyd_steinberg(pic, width, height);
	
	for(y=0;y<height;y++) {
		for(x=0;x<width;x++) {
			offset = 3*(width*y+x);
			ret[width*y+x] = MAKERGB565N(pic[offset] >> 16,
				pic[offset+1] >> 16,
				pic[offset+2] >> 16);
		}
	}
	
	free(pic);
	
	return 1;
}

unsigned short *png_load(const char *filename, unsigned int *w, unsigned int *h)
{
	unsigned short *ret;
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

	ret = malloc(width*height*2);
	if(ret == NULL) goto free4;
	if(!dither(ret, row_pointers, width, height)) {
		free(ret);
		ret = NULL;
		goto free4;
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
