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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <rtems.h>
#include <bsp/milkymist_flash.h>

#include <mtklib.h>

#include "flash.h"
#include "filedialog.h"

static int appid;
static int file_dialog_id;
static int current_file_to_choose;

static void flash_filedialog_ok_callback()
{
	char filepath[384];

	get_filedialog_selection(file_dialog_id, filepath, sizeof(filepath));
	mtk_cmdf(appid, "e%d.set(-text \"%s\")", current_file_to_choose, filepath);
	close_filedialog(file_dialog_id);
}

static void flash_filedialog_cancel_callback()
{
	close_filedialog(file_dialog_id);
}

static void opendialog_callback(mtk_event *e, void *arg)
{
	current_file_to_choose = (int)arg;
	open_filedialog(file_dialog_id, "/");
}

static char bitstream_name[384];
static char bios_name[384];
static char application_name[384];

static int flash_erase(int fd, unsigned int len)
{
	int r;
	unsigned int size;
	unsigned int blocksize;
	unsigned int nblocks;
	unsigned int i;

	r = ioctl(fd, FLASH_GET_SIZE, &size);
	if(r == -1) return 0;
	r = ioctl(fd, FLASH_GET_BLOCKSIZE, &blocksize);
	if(r == -1) return 0;
	nblocks = (len + blocksize - 1)/blocksize;
	if(nblocks*blocksize > size) return 0;
	for(i=0;i<nblocks;i++) {
		printf("Erasing block %d\n", i);
		r = ioctl(fd, FLASH_ERASE_BLOCK, i*blocksize);
		if(r == -1) return 0;
	}
	printf("Erasure done\n");
	return 1;
}

static int flash_program(int flashfd, int srcfd, unsigned int len)
{
	int r;
	unsigned char buf[1024];
	unsigned char buf2[1024];
	unsigned int p;

	p = 0;
	while(1) {
		r = read(srcfd, buf, sizeof(buf));
		if(r < 0) return 0;
		if(r == 0) break;
		if(r < sizeof(buf)) {
			/* Length must be a multiple of 2 */
			if(r & 1) r++;
		}
		write(flashfd, buf, r);
		if(p++ == 10) {
			p = 0;
			printf(".");
		}
	}

	printf("\nVerifying\n");

	lseek(flashfd, 0, SEEK_SET);
	lseek(srcfd, 0, SEEK_SET);

	while(1) {
		r = read(srcfd, buf, sizeof(buf));
		if(r < 0) return 0;
		if(r == 0) break;
		read(flashfd, buf2, sizeof(buf2));
		if(memcmp(buf, buf2, r) != 0) {
			printf("Verify failed!\n");
			break;
		}
	}
	printf("Verify passed\n");

	return 1;
}

static int flash_file(const char *target, const char *file)
{
	int srcfd;
	int flashfd;
	off_t o;
	unsigned int len;

	srcfd = open(file, O_RDONLY);
	if(srcfd == -1) return 0;
	
	o = lseek(srcfd, 0, SEEK_END);
	if(o < 0) {
		close(srcfd);
		return 0;
	}
	len = o;
	o = lseek(srcfd, 0, SEEK_SET);
	if(o < 0) {
		close(srcfd);
		return 0;
	}

	flashfd = open(target, O_RDWR);
	if(flashfd == -1) {
		close(srcfd);
		return 0;
	}

	printf("Length = %d\n", len);

	if(!flash_erase(flashfd, len)) {
		close(flashfd);
		close(srcfd);
		return 0;
	}
	if(!flash_program(flashfd, srcfd, len)) {
		close(flashfd);
		close(srcfd);
		return 0;
	}

	close(flashfd);
	close(srcfd);

	return 1;
}

static void ok_callback(mtk_event *e, void *arg)
{
	mtk_req(appid, bitstream_name, sizeof(bitstream_name), "e1.text");
	mtk_req(appid, bios_name, sizeof(bios_name), "e2.text");
	mtk_req(appid, application_name, sizeof(application_name), "e3.text");

	flash_file("/dev/flash4", application_name);
}

static void cancel_callback(mtk_event *e, void *arg)
{
	close_filedialog(file_dialog_id);
	mtk_cmd(appid, "w.close()");
}

void init_flash()
{
	appid = mtk_init_app("Flash");

	mtk_cmd_seq(appid,
		"g = new Grid()",

		"l0 = new Label(-text \"Select images to flash.\nIf your board does not restart after flashing, don't panic!\nHold pushbutton #1 during power-up to enable rescue mode.\")",

		"g.place(l0, -column 1 -row 1 -align w)",

		"g2 = new Grid()",
		"g2.columnconfig(1, -size 0)",
		"g2.columnconfig(2, -size 250)",
		"g2.columnconfig(3, -size 0)",

		"l1 = new Label(-text \"Bitstream image (.BIT):\")",
		"l2 = new Label(-text \"BIOS image (.BIN):\")",
		"l3 = new Label(-text \"Application image (.FBI):\")",

		"e1 = new Entry()",
		"e2 = new Entry()",
		"e3 = new Entry()",

		"b_browse1 = new Button(-text \"Browse\")",
		"b_browse2 = new Button(-text \"Browse\")",
		"b_browse3 = new Button(-text \"Browse\")",

		"g2.place(l1, -column 1 -row 1 -align w)",
		"g2.place(l2, -column 1 -row 2 -align w)",
		"g2.place(l3, -column 1 -row 3 -align w)",

		"g2.place(e1, -column 2 -row 1)",
		"g2.place(e2, -column 2 -row 2)",
		"g2.place(e3, -column 2 -row 3)",

		"g2.place(b_browse1, -column 3 -row 1)",
		"g2.place(b_browse2, -column 3 -row 2)",
		"g2.place(b_browse3, -column 3 -row 3)",

		"g.place(g2, -column 1 -row 2)",

		"g3 = new Grid()",

		"b_ok = new Button(-text \"OK\")",
		"b_cancel = new Button(-text \"Cancel\")",

		"g3.place(b_ok, -column 1 -row 1)",
		"g3.place(b_cancel, -column 2 -row 1)",

		"g.place(g3, -column 1 -row 3)",

		"g.rowconfig(1, -size 0)",
		"g.rowconfig(2, -size 0)",
		"g.rowconfig(3, -size 0)",

		"w = new Window(-content g -title \"Flash upgrade\")",
		0);

	mtk_bind(appid, "b_browse1", "commit", opendialog_callback, (void *)1);
	mtk_bind(appid, "b_browse2", "commit", opendialog_callback, (void *)2);
	mtk_bind(appid, "b_browse3", "commit", opendialog_callback, (void *)3);
	mtk_bind(appid, "b_cancel", "commit", cancel_callback, NULL);
	mtk_bind(appid, "b_ok", "commit", ok_callback, NULL);

	mtk_bind(appid, "w", "close", cancel_callback, NULL);

	file_dialog_id = create_filedialog("Flashimg selection", 0, flash_filedialog_ok_callback, NULL, flash_filedialog_cancel_callback, NULL);
}

void open_flash_window()
{
	mtk_cmd(appid, "w.open()");
}
