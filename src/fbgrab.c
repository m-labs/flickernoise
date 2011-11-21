/*
 * Flickernoise
 * Copyright (C) 2010, 2011 Sebastien Bourdeauducq
 * Copyright (C) 2011 Xiangfu Liu
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

#include <rtems.h>
#include <rtems/fb.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <png.h>

#include "pngwrite.h"
#include "fbgrab.h"

static char *filename = "/ssd/Screenshot-00.png";

static char *get_name()
{
	struct stat st;
	int i, result;
	for(i=0;i<100;i++) {
		result = stat(filename, &st);
		if (result < 0) {
			if (errno == ENOENT)
				break;
		} else
			sprintf(filename + strlen(filename) - 6, "%02d.png", i);
	}

	return filename;
}

static void convert565to24(int width, int height,
			   unsigned char *inbuffer,
			   unsigned char *outbuffer)
{
	unsigned int i;

	for (i=0; i < height*width*2; i+=2) {
		/* BLUE  = 0 */
		outbuffer[(i/2*3)+0] = (inbuffer[i+1] & 0x1f) << 3;
		/* GREEN = 1 */
		outbuffer[(i/2*3)+1] =
			(((inbuffer[i] & 0x7) << 3) | (inbuffer[i+1] & 0xE0) >> 5) 
			<< 2;
		/* RED   = 2 */
		outbuffer[(i/2*3)+2] = (inbuffer[i] & 0xF8);
	}
}

static int convert_and_write(unsigned char *inbuffer, char *filename,
			     int width, int height, int bits, int interlace)
{
	int ret = 0;
	size_t bufsize = width * height * 3;

	unsigned char *outbuffer = malloc(bufsize);
	if (outbuffer == NULL) {
		fprintf(stderr, "Not enough memory");
		return -1;
	}
	memset(outbuffer, 0, bufsize);

	printf("grab screen to %s\n", filename);

	switch (bits) {
	case 16:
		convert565to24(width, height, inbuffer, outbuffer);
		ret = png_write(outbuffer, filename, width, height, interlace);
		break;
	case 15:
	case 24:
	case 32:
	default:
		fprintf(stderr, "%d bits per pixel are not supported! ", bits);
	}

	free(outbuffer);

	return ret;
}

int fbgrab(char *fn)
{
	int fd;

	struct fb_var_screeninfo fb_var;
	size_t width;
	size_t height;
	size_t buf_size;
	unsigned char *buf_p;

	char *device = "/dev/fb";
	char *outfile = NULL;
	int ret = 0;

	if(fn == NULL)
		outfile = get_name();
	else
		outfile = fn;

	fd = open(device, O_RDONLY);
	if(fd == -1) {
		perror("Unable to open /dev/fb");
		return 2;
	}
	
	ioctl(fd, FBIOGET_VSCREENINFO, &fb_var);
	width = fb_var.xres;
	height = fb_var.yres;
	buf_size = width * height * 2;

	buf_p = malloc(buf_size);
	if(buf_p == NULL) {
		fprintf(stderr, "Unable to get %d bytes memory\n", buf_size);
		ret = 3;
		goto close0;
	}

	if(read(fd, buf_p, buf_size) != buf_size) {
		perror("Unable to read device");
		ret = 4;
		goto free0;
	}

	ret = convert_and_write(buf_p, outfile,
				width, height, 16, /* bit depth */
				PNG_INTERLACE_NONE);

free0:
	free(buf_p);
close0:
	close(fd);

	return ret;
}
