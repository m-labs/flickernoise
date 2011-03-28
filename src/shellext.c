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

#include <rtems.h>
#include <rtems/shell.h>
#include <bsp/milkymist_flash.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <png.h>

#include "shellext.h"

int main_erase(int argc, char **argv)
{
	int fd;
	int r;
	unsigned int size;
	unsigned int blocksize;
	unsigned int nblocks;
	int i;
	
	if(argc != 2) {
		fprintf(stderr, "erase: you must specify a flash device\n");
		return 1;
	}

	fd = open(argv[1], O_RDWR);
	if(fd == -1) {
		perror("Unable to open flash device");
		return 2;
	}
	
	r = ioctl(fd, FLASH_GET_SIZE, &size);
	if(r == -1) {
		perror("Unable to get flash partition size");
		close(fd);
		return 2;
	}
	r = ioctl(fd, FLASH_GET_BLOCKSIZE, &blocksize);
	if(r == -1) {
		perror("Unable to get flash block size");
		close(fd);
		return 2;
	}
	nblocks = size/blocksize;

	printf("About to erase %d blocks...\n", nblocks);
	for(i=0;i<nblocks;i++) {
		printf("%d ", i);
		fflush(stdout);
		r = ioctl(fd, FLASH_ERASE_BLOCK, i*blocksize);
		if(r == -1) {
			perror("Erase failed");
			close(fd);
			return 2;
		}
	}
	
	close(fd);
	
	return 0;
}

rtems_shell_cmd_t shell_usercmd = {
	"erase",			/* name */
	"erase device",			/* usage */
	"flickernoise",			/* topic */
	main_erase,			/* command */
	NULL,				/* alias */
	NULL				/* next */
};

static int write_PNG(unsigned char *outbuffer, char *filename,
		     int width, int height, int interlace)
{
	int i;
	int bit_depth = 0, color_type;
	png_bytep row_pointers[height];
	png_structp png_ptr;
	png_infop info_ptr;
	FILE *outfile;

	for (i=0; i<height; i++)
		row_pointers[i] = outbuffer + i * 4 * width;

	outfile = fopen(filename, "w");
	if (outfile == NULL) {
		perror("Error: Couldn't fopen");
		return -1;
	}

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr) {
		fprintf(stderr, "Error: Couldn't create PNG write struct");
		return -1;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
		fprintf(stderr, "Error: Couldn't create PNG info struct");
		return -1;
	}

	png_init_io(png_ptr, outfile);

	png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);

	bit_depth = 8;
	color_type = PNG_COLOR_TYPE_RGB_ALPHA;
	png_set_invert_alpha(png_ptr);
	png_set_bgr(png_ptr);

	png_set_IHDR(png_ptr, info_ptr, width, height,
		     bit_depth, color_type, interlace,
		     PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	png_write_info(png_ptr, info_ptr);

	printf("Now writing PNG file...\n");

	png_write_image(png_ptr, row_pointers);

	png_write_end(png_ptr, info_ptr);
	/* puh, done, now freeing memory... */
	png_destroy_write_struct(&png_ptr, &info_ptr);

	if (outfile != NULL)
		fclose(outfile);

	return 0;
}

static void convert565to32(int width, int height,
			   unsigned char *inbuffer,
			   unsigned char *outbuffer)
{
	unsigned int i;

	for (i=0; i < height*width*2; i+=2)
	{
		/* BLUE  = 0 */
		outbuffer[(i<<1)+0] = (inbuffer[i+1] & 0x1f) << 3;
		/* GREEN = 1 */
		outbuffer[(i<<1)+1] = 
			(((inbuffer[i] & 0x7) << 3) | (inbuffer[i+1] & 0xE0) >> 5) 
			<< 2;
		/* RED   = 2 */
		outbuffer[(i<<1)+2] = (inbuffer[i] & 0xF8);
		/* ALPHA = 3 */
		outbuffer[(i<<1)+3] = '\0';
	}
}

static int convert_and_write(unsigned char *inbuffer, char *filename,
			     int width, int height, int bits, int interlace)
{
	int ret = 0;

	size_t bufsize = width * height * 4;

	unsigned char *outbuffer = malloc(bufsize);
	if (outbuffer == NULL) {
		fprintf(stderr, "Not enough memory");
		return -1;
	}

	memset(outbuffer, 0, bufsize);

	printf("Converting image from %i\n", bits);

	switch (bits) {
	case 16:
		convert565to32(width, height, inbuffer, outbuffer);
		ret = write_PNG(outbuffer, filename, width, height, interlace);
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

int main_fbgrab(int argc, char **argv)
{
	int fd;
	unsigned char *buf_p;
	int ret = 0;

	unsigned int bitdepth = 16;
	size_t width = 640;
	size_t height = 480;
	size_t buf_size = width * height * 2;
	int interlace = PNG_INTERLACE_NONE;

	char *device = "/dev/fb";
	char *outfile = NULL;

	if(argc != 2) {
		fprintf(stderr, "%s: you must specify a file name\n", argv[0]);
		return 1;
	}
	outfile = argv[1];

	fd = open(device, O_RDONLY);
	if(fd == -1) {
		perror("Unable to open /dev/fb");
		return 2;
	}

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
			  width, height, bitdepth,
			  interlace);
free0:
	free(buf_p);
close0:
	close(fd);

	return ret;
}

rtems_shell_cmd_t framebuffer_grab = {
	"fbgrab",			/* name */
	"fbgrab file.png",		/* usage */
	"flickernoise",			/* topic */
	main_fbgrab,			/* command */
	NULL,				/* alias */
	NULL				/* next */
};
