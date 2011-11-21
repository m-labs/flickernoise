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
#include <stdio.h>
#include <jpeglib.h>

#include "pixbuf.h"
#include "dither.h"
#include "loaders.h"

struct pixbuf *pixbuf_load_jpeg(char *filename)
{
	struct pixbuf *ret;
	FILE *fd;
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	unsigned char *pixels;
	int i;
	unsigned char **row_pointers;
	
	ret = NULL;
	fd = fopen(filename, "rb");
	if(fd == NULL) goto free0;
	
	cinfo.err = jpeg_std_error(&jerr); // TODO: get rid of the exit() code
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, fd);
	jpeg_read_header(&cinfo, TRUE);
		
	cinfo.out_color_space = JCS_RGB;
	cinfo.out_color_components = 3;
	cinfo.dither_mode = JDITHER_NONE;

	pixels = malloc(3*cinfo.image_width*cinfo.image_height);
	if(pixels == NULL) goto free2;
	row_pointers = malloc(sizeof(char *)*cinfo.image_height);
	if(row_pointers == NULL) goto free3;
	for(i=0;i<cinfo.image_height;i++)
		row_pointers[i] = &pixels[i*3*cinfo.image_width];

	jpeg_start_decompress(&cinfo);
	while(cinfo.output_scanline < cinfo.output_height)
		jpeg_read_scanlines(&cinfo, &row_pointers[cinfo.output_scanline], 1);
	ret = pixbuf_new(cinfo.image_width, cinfo.image_height);
	if(ret == NULL) goto free5;
	
	if(!pixbuf_dither(ret->pixels, row_pointers, cinfo.image_width, cinfo.image_height)) {
		pixbuf_dec_ref(ret);
		ret = NULL;
		goto free5;
	}
	
free5:
	jpeg_finish_decompress(&cinfo);
free4:
	free(row_pointers);
free3:
	free(pixels);
free2:
	jpeg_destroy_decompress(&cinfo);
free1:
	fclose(fd);
free0:
	return ret;
}