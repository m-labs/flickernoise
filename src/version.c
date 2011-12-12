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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

char soc[13];
char pcb[3];
char pcb_rev[2];

static void read_dev(const char *dev, char *buf, unsigned int len)
{
	int fd;
	int rl;

	buf[0] = '?';
	buf[1] = 0;
	fd = open(dev, O_RDONLY);
	if(fd == -1) return;
	rl = read(fd, buf, len-1);
	if(rl <= 0) {
		close(fd);
		return;
	}
	buf[rl] = 0;
	close(fd);
}

void init_version(void)
{
	read_dev("/dev/soc", soc, sizeof(soc));
	read_dev("/dev/pcb", pcb, sizeof(pcb));
	read_dev("/dev/pcb_rev", pcb_rev, sizeof(pcb_rev));
}
