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
#include <setjmp.h>
#include <jpeglib.h>

#include "pixbuf.h"
#include "dither.h"
#include "loaders.h"

struct my_error_mgr {
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
};

static void my_error_exit(j_common_ptr cinfo)
{
	struct my_error_mgr * myerr = (struct my_error_mgr *)cinfo->err;
	(*cinfo->err->output_message) (cinfo);
	longjmp(myerr->setjmp_buffer, 1);
}

struct pixbuf *pixbuf_load_jpeg(char *filename)
{
	struct pixbuf *ret;
	FILE *fd;
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	unsigned char *pixels;
	int i;
	unsigned char **row_pointers;
	
	ret = NULL;
	fd = fopen(filename, "rb");
	if(fd == NULL) goto free0;
	
	cinfo.err = jpeg_std_error((struct jpeg_error_mgr *)&jerr);
	jerr.pub.error_exit = my_error_exit;
	if(setjmp(jerr.setjmp_buffer)) goto free2;
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

	if(setjmp(jerr.setjmp_buffer)) goto free4;
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
	fclose(fd);
free0:
	return ret;
}