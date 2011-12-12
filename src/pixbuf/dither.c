/*
 * Flickernoise
 * Copyright (C) 2011 Sebastien Bourdeauducq
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

#include "../color.h"
#include "dither.h"

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

int pixbuf_dither(unsigned short *ret, unsigned char **row_pointers, int width, int height, int has_alpha)
{
	int x, y;
	unsigned char *row;
	int offset;
	int *pic;
	
	pic = malloc(width*height*3*sizeof(int));
	if(pic == NULL) return 0;
	
	if(has_alpha) {
		for(y=0;y<height;y++) {
			row = row_pointers[y];
			for(x=0;x<width;x++) {
				offset = 3*(width*y+x);
				pic[offset] = ((unsigned int)row[4*x]) << 16;
				pic[offset+1] = ((unsigned int)row[4*x+1]) << 16;
				pic[offset+2] = ((unsigned int)row[4*x+2]) << 16;
			}
		}
	} else {
		for(y=0;y<height;y++) {
			row = row_pointers[y];
			for(x=0;x<width;x++) {
				offset = 3*(width*y+x);
				pic[offset] = ((unsigned int)row[3*x]) << 16;
				pic[offset+1] = ((unsigned int)row[3*x+1]) << 16;
				pic[offset+2] = ((unsigned int)row[3*x+2]) << 16;
			}
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
