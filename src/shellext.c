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
#include <rtems/shell.h>
#include <bsp/milkymist_flash.h>
#include <bsp/milkymist_video.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "shellext.h"
#include "fbgrab.h"

static int main_viwrite(int argc, char **argv)
{
	unsigned int reg, val;
	int fd;
	unsigned int iov;
	
	if(argc != 3) {
		fprintf(stderr, "viwrite: you must specify register and value\n");
		return 1;
	}
	
	reg = strtoul(argv[1], NULL, 0);
	val = strtoul(argv[2], NULL, 0);
	iov = (reg << 16)|val;
	
	fd = open("/dev/video", O_RDWR);
	if(fd == -1) {
		perror("Unable to open video device");
		return 2;
	}
	
	ioctl(fd, VIDEO_SET_REGISTER, iov);
	
	close(fd);
	
	return 0;
}

static int main_viread(int argc, char **argv)
{
	int fd;
	unsigned int rv;
	
	if(argc != 2) {
		fprintf(stderr, "viread: you must specify register\n");
		return 1;
	}
	
	rv = strtoul(argv[1], NULL, 0);
	
	fd = open("/dev/video", O_RDWR);
	if(fd == -1) {
		perror("Unable to open video device");
		return 2;
	}
	
	ioctl(fd, VIDEO_GET_REGISTER, &rv);
	
	close(fd);
	
	printf("Value: %02x\n", rv);
	
	return 0;
}

static int main_erase(int argc, char **argv)
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

static int main_fbgrab(int argc, char **argv)
{
	int ret = 0;

	if(argc == 2)
		ret = fbgrab(argv[1]);
	else
		ret = fbgrab(NULL);

	return ret;
}

static rtems_shell_cmd_t shellext_viwrite = {
	"viwrite",			/* name */
	"viwrite register value",	/* usage */
	"flickernoise",			/* topic */
	main_viwrite,			/* command */
	NULL,				/* alias */
	NULL				/* next */
};

static rtems_shell_cmd_t shellext_viread = {
	"viread",			/* name */
	"viread register",		/* usage */
	"flickernoise",			/* topic */
	main_viread,			/* command */
	NULL,				/* alias */
	&shellext_viwrite		/* next */
};

static rtems_shell_cmd_t shellext_erase = {
	"erase",			/* name */
	"erase device",			/* usage */
	"flickernoise",			/* topic */
	main_erase,			/* command */
	NULL,				/* alias */
	&shellext_viread		/* next */
};

rtems_shell_cmd_t shellext = {
	"fbgrab",			/* name */
	"fbgrab file.png",		/* usage */
	"flickernoise",			/* topic */
	main_fbgrab,			/* command */
	NULL,				/* alias */
	&shellext_erase			/* next */
};
