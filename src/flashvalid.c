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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <zlib.h>

#include "flashvalid.h"

#define BITSTREAM_HEADER_SIZE 20

static const char bitstream_header[BITSTREAM_HEADER_SIZE] =
	{0xff, 0xff, 0xff, 0xff,
	 0xff, 0xff, 0xff, 0xff,
	 0xff, 0xff, 0xff, 0xff,
	 0xff, 0xff, 0xff, 0xff,
	 0x55, 0x99, 0xaa, 0x66};

int flashvalid_bitstream(const char *filename)
{
	int fd;
	char buf[BITSTREAM_HEADER_SIZE];

	fd = open(filename, O_RDONLY);
	if(fd == -1) return FLASHVALID_ERROR_IO;
	if(read(fd, buf, BITSTREAM_HEADER_SIZE) != BITSTREAM_HEADER_SIZE) {
		close(fd);
		return FLASHVALID_ERROR_IO;
	}
	close(fd);

	if(memcmp(buf, bitstream_header, BITSTREAM_HEADER_SIZE) != 0)
		return FLASHVALID_ERROR_FORMAT;
	
	return FLASHVALID_PASSED;
}

int flashvalid_bios(const char *filename)
{
	int fd;
	unsigned int op;

	op = 0xffffffff;
	fd = open(filename, O_RDONLY);
	if(fd == -1) return FLASHVALID_ERROR_IO;
	if(read(fd, &op, 4) != 4) {
		close(fd);
		return FLASHVALID_ERROR_IO;
	}
	close(fd);

	/* First instruction must be xor r0,r0,r0 */
	if(op != 0x98000000)
		return FLASHVALID_ERROR_FORMAT;
	
	return FLASHVALID_PASSED;
}

int flashvalid_application(const char *filename)
{
	int fd;
	unsigned int e_length;
	unsigned int e_crc;
	unsigned int length;
	unsigned int crc;
	unsigned char buf[1024];
	int r;

	fd = open(filename, O_RDONLY);
	if(fd == -1) return FLASHVALID_ERROR_IO;
	if(read(fd, &e_length, 4) != 4) {
		close(fd);
		return FLASHVALID_ERROR_IO;
	}
	if((e_length < 32) || (e_length > 4*1024*1024)) {
		close(fd);
		return FLASHVALID_ERROR_FORMAT;
	}
	if(read(fd, &e_crc, 4) != 4) {
		close(fd);
		return FLASHVALID_ERROR_IO;
	}

	length = 0;
	crc = crc32(0L, Z_NULL, 0);

	while(1) {
		r = read(fd, buf, sizeof(buf));
		if(r < 0) {
			close(fd);
			return FLASHVALID_ERROR_IO;
		}
		if(r == 0) break;
		crc = crc32(crc, buf, r);
		length += r;
	}
	
	close(fd);

	if((e_length != length) || (e_crc != crc))
		return FLASHVALID_ERROR_FORMAT;
	
	return FLASHVALID_PASSED;
}
