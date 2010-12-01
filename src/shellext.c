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
	"flash",			/* topic */
	main_erase,			/* command */
	NULL,				/* alias */
	NULL				/* next */
};
