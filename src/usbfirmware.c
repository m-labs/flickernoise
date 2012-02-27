/*
 * Flickernoise
 * Copyright (C) 2012 Sebastien Bourdeauducq
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
#include <assert.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <rtems.h>
#include <system_conf.h>
#include <bsp/milkymist_usbinput.h>

#include "usbfirmware.h"

static const unsigned char input_firmware[] = {
#include <softusb-input.h>
};

static struct usbinput_firmware_description firmware_desc = {
	.data = input_firmware,
	.length = sizeof(input_firmware)
};

static void do_load_usb_firmware(
    const struct usbinput_firmware_description *dsc)
{
	int fd;
	
	fd = open("/dev/usbinput", O_RDONLY);
	ioctl(fd, USBINPUT_LOAD_FIRMWARE, dsc);
	assert(fd != -1);
	close(fd);
}

void load_usb_firmware(void)
{
	do_load_usb_firmware(&firmware_desc);
}

int load_usb_firmware_file(const char *name)
{
	FILE *file;
	unsigned char buf[SOFTUSB_PMEM_SIZE];
	struct usbinput_firmware_description dsc = {
		.data = buf,
	};

	file = fopen(name, "r");
	if(!file)
		return 0;
	dsc.length = fread(buf, 1, sizeof(buf), file);
	fclose(file);
	if(dsc.length <= 0)
		return 0;
	do_load_usb_firmware(&dsc);
	return 1;
}
