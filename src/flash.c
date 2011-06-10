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

#include <assert.h>
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

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#include <mtklib.h>

#include "filedialog.h"
#include "messagebox.h"
#include "input.h"
#include "flashvalid.h"
#include "version.h"
#include "sysconfig.h"

#include "flash.h"

static int appid;
static struct filedialog *file_dlg;
static int current_file_to_choose;

enum {
	ARG_FILE_UPDATE = 0,
	ARG_GET_VERSIONS,
	ARG_WEB_UPDATE
};

enum {
	FLASH_STATE_READY = 0,
	FLASH_STATE_STARTING,

	FLASH_STATE_ERASE_BITSTREAM,
	FLASH_STATE_PROGRAM_BITSTREAM,
	FLASH_STATE_ERROR_BITSTREAM,

	FLASH_STATE_ERASE_BIOS,
	FLASH_STATE_PROGRAM_BIOS,
	FLASH_STATE_ERROR_BIOS,

	FLASH_STATE_ERASE_APP,
	FLASH_STATE_PROGRAM_APP,
	FLASH_STATE_ERROR_APP,

	DOWNLOAD_STATE_START_BITSTREAM,
	DOWNLOAD_STATE_ERROR_BITSTREAM,

	DOWNLOAD_STATE_START_BIOS,
	DOWNLOAD_STATE_ERROR_BIOS,

	DOWNLOAD_STATE_START_APP,
	DOWNLOAD_STATE_ERROR_APP,

	FLASH_STATE_SUCCESS
};

static int flash_state;
static int flash_progress;
static int flashvalid_val;

static char bitstream_name[384];
static char bios_name[384];
static char application_name[384];

static double *d_progress;
static int progress_callback(void *d_progress, double t, double d, double ultotal, double ulnow)
{
	flash_progress = (int) d/t*100;
	return 0;
}

static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
	size_t written;
	written = fwrite(ptr, size, nmemb, stream);
	return written;
}

static int download(const char *url, const char *filename)
{
	CURL *curl;
	FILE *fp;
	int ret = 0;

	fp = fopen(filename, "wb");
	if(fp == NULL) return 0;

	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 180);

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
		curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_callback);
		curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &d_progress);

		if(curl_easy_perform(curl) == 0)
			ret = 1;

		curl_easy_cleanup(curl);
	}

	fclose(fp);
	return ret;
}

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
	if(nblocks == 0) return 1;
	for(i=0;i<nblocks;i++) {
		r = ioctl(fd, FLASH_ERASE_BLOCK, i*blocksize);
		if(r == -1) return 0;
		flash_progress = 100*i/nblocks;
	}
	return 1;
}

static int flash_program(int flashfd, int srcfd, unsigned int len)
{
	int written;
	int r;
	unsigned char buf[1024];
	unsigned char buf2[1024];

	if(len == 0) return 1;

	written = 0;
	while(1) {
		r = read(srcfd, buf, sizeof(buf));
		if(r < 0) return 0;
		if(r == 0) break;
		written += r;
		if(r < sizeof(buf)) {
			/* Length must be a multiple of 2 */
			if(r & 1) r++;
		}
		write(flashfd, buf, r);
		flash_progress = 100*written/len;
	}
	if(written < len)
		return 0;

	/* Verify */
	lseek(flashfd, 0, SEEK_SET);
	lseek(srcfd, 0, SEEK_SET);
	while(1) {
		r = read(srcfd, buf, sizeof(buf));
		if(r < 0) return 0;
		if(r == 0) break;
		read(flashfd, buf2, sizeof(buf2));
		if(memcmp(buf, buf2, r) != 0)
			return 0; /* Verification failed */
	}

	return 1;
}

static int flash_file(const char *target, const char *file, int estate, int pstate)
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

	flash_state = estate;
	if(!flash_erase(flashfd, len)) {
		close(flashfd);
		close(srcfd);
		return 0;
	}
	flash_state = pstate;
	if(!flash_program(flashfd, srcfd, len)) {
		close(flashfd);
		close(srcfd);
		return 0;
	}

	close(flashfd);
	close(srcfd);

	return 1;
}

static void flash_terminate(int state)
{
	flash_state = state;
	rtems_task_delete(RTEMS_SELF);
}

static rtems_task flash_task(rtems_task_argument argument)
{
	const char *s_url = "http://www.milkymist.org/snapshots/latest/soc.fpg";
	const char *b_url = "http://www.milkymist.org/snapshots/latest/bios.bin";
	const char *f_url = "http://www.milkymist.org/snapshots/latest/flickernoise.fbi";
	const char *s = "/ramdisk/soc.fpg";
	const char *b = "/ramdisk/bios.bin";
	const char *f = "/ramdisk/flickernoise.fbi";

	int task = (int)argument;

	switch(task) {
		case ARG_WEB_UPDATE:
			flash_state = DOWNLOAD_STATE_START_BITSTREAM;
			if(!download(s_url, s))
				flash_terminate(DOWNLOAD_STATE_ERROR_BITSTREAM);
			mtk_cmdf(appid, "e1.set(-text \"%s\")", s);
			strcpy(bitstream_name, s);

			flash_state = DOWNLOAD_STATE_START_BIOS;
			if(!download(b_url, b))
				flash_terminate(DOWNLOAD_STATE_ERROR_BIOS);
			mtk_cmdf(appid, "e2.set(-text \"%s\")", b);
			strcpy(bios_name, b);

			flash_state = DOWNLOAD_STATE_START_APP;
			if(!download(f_url, f))
				flash_terminate(DOWNLOAD_STATE_ERROR_APP);
			mtk_cmdf(appid, "e3.set(-text \"%s\")", f);
			strcpy(application_name, f);
			/* fall through */
		case ARG_FILE_UPDATE:
			if(bitstream_name[0] != 0) {
				flashvalid_val = flashvalid_bitstream(bitstream_name);
				if(flashvalid_val != FLASHVALID_PASSED)
					flash_terminate(FLASH_STATE_ERROR_BITSTREAM);

				if(!flash_file("/dev/flash1", bitstream_name,
					       FLASH_STATE_ERASE_BITSTREAM, FLASH_STATE_PROGRAM_BITSTREAM))
					flash_terminate(FLASH_STATE_ERROR_BITSTREAM);
			}
			if(bios_name[0] != 0) {
				flashvalid_val = flashvalid_bios(bios_name);
				if(flashvalid_val != FLASHVALID_PASSED)
					flash_terminate(FLASH_STATE_ERROR_BIOS);

				if(!flash_file("/dev/flash2", bios_name,
					       FLASH_STATE_ERASE_BIOS, FLASH_STATE_PROGRAM_BIOS))
					flash_terminate(FLASH_STATE_ERROR_BIOS);
			}
			if(application_name[0] != 0) {
				flashvalid_val = flashvalid_application(application_name);
				if(flashvalid_val != FLASHVALID_PASSED)
					flash_terminate(FLASH_STATE_ERROR_APP);

				if(!flash_file("/dev/flash4", application_name,
					       FLASH_STATE_ERASE_APP, FLASH_STATE_PROGRAM_APP))
					flash_terminate(FLASH_STATE_ERROR_APP);
			}
			break;
	}

	flash_terminate(FLASH_STATE_SUCCESS);
}

static int flash_busy()
{
	switch(flash_state) {
		case FLASH_STATE_READY:
		case FLASH_STATE_ERROR_BITSTREAM:
		case FLASH_STATE_ERROR_BIOS:
		case FLASH_STATE_ERROR_APP:
		case DOWNLOAD_STATE_ERROR_BITSTREAM:
		case DOWNLOAD_STATE_ERROR_BIOS:
		case DOWNLOAD_STATE_ERROR_APP:
		case FLASH_STATE_SUCCESS:
			return 0;
		default:
			return 1;
	}
}

static void update_progress();

#define UPDATE_PERIOD 40
static rtems_interval next_update;

static void refresh_callback(mtk_event *e, int count)
{
	rtems_interval t;

	t = rtems_clock_get_ticks_since_boot();
	if(t >= next_update) {
		update_progress();
		next_update = t + UPDATE_PERIOD;
	}
}

static void display_flashvalid_message(const char *n)
{
	char buf[256];

	if(flashvalid_val == FLASHVALID_ERROR_IO) {
		sprintf(buf, "Unable to read the %s image.\nCheck that the file exists. Operation aborted.", n);
		messagebox("I/O error", buf);
	} else if(flashvalid_val == FLASHVALID_ERROR_FORMAT) {
		sprintf(buf, "The format of the %s image is not recognized.\nHave you selected the correct file? Operation aborted.", n);
		messagebox("Format error", buf);
	}
}

static void update_progress()
{
	switch(flash_state) {
		case FLASH_STATE_READY:
			mtk_cmd(appid, "l_stat.set(-text \"Ready.\")");
			break;
		case FLASH_STATE_STARTING:
			mtk_cmd(appid, "l_stat.set(-text \"Starting...\")");
			break;
		case FLASH_STATE_ERASE_BITSTREAM:
			mtk_cmd(appid, "l_stat.set(-text \"Erasing bitstream...\")");
			break;
		case FLASH_STATE_PROGRAM_BITSTREAM:
			mtk_cmd(appid, "l_stat.set(-text \"Programming bitstream...\")");
			break;
		case FLASH_STATE_ERROR_BITSTREAM:
			mtk_cmd(appid, "l_stat.set(-text \"Failed to program bitstream.\")");
			display_flashvalid_message("bitstream");
			input_delete_callback(refresh_callback);
			break;
		case FLASH_STATE_ERASE_BIOS:
			mtk_cmd(appid, "l_stat.set(-text \"Erasing BIOS...\")");
			break;
		case FLASH_STATE_PROGRAM_BIOS:
			mtk_cmd(appid, "l_stat.set(-text \"Programming BIOS...\")");
			break;
		case FLASH_STATE_ERROR_BIOS:
			mtk_cmd(appid, "l_stat.set(-text \"Failed to program BIOS.\")");
			display_flashvalid_message("BIOS");
			input_delete_callback(refresh_callback);
			break;
		case FLASH_STATE_ERASE_APP:
			mtk_cmd(appid, "l_stat.set(-text \"Erasing application...\")");
			break;
		case FLASH_STATE_PROGRAM_APP:
			mtk_cmd(appid, "l_stat.set(-text \"Programming application...\")");
			break;
		case FLASH_STATE_ERROR_APP:
			mtk_cmd(appid, "l_stat.set(-text \"Failed to program application.\")");
			display_flashvalid_message("application");
			input_delete_callback(refresh_callback);
			break;
		case DOWNLOAD_STATE_START_BITSTREAM:
			mtk_cmd(appid, "l_stat.set(-text \"Downloading bitstream...\")");
			break;
		case DOWNLOAD_STATE_ERROR_BITSTREAM:
			mtk_cmd(appid, "l_stat.set(-text \"Failed to download bitstream .\")");
			input_delete_callback(refresh_callback);
			break;
		case DOWNLOAD_STATE_START_BIOS:
			mtk_cmd(appid, "l_stat.set(-text \"Downloading BIOS...\")");
			break;
		case DOWNLOAD_STATE_ERROR_BIOS:
			mtk_cmd(appid, "l_stat.set(-text \"Failed to download bitstream.\")");
			input_delete_callback(refresh_callback);
			break;
		case DOWNLOAD_STATE_START_APP:
			mtk_cmd(appid, "l_stat.set(-text \"Downloading application...\")");
			break;
		case DOWNLOAD_STATE_ERROR_APP:
			mtk_cmd(appid, "l_stat.set(-text \"Failed to download application .\")");
			input_delete_callback(refresh_callback);
			break;
		case FLASH_STATE_SUCCESS:
			mtk_cmd(appid, "l_stat.set(-text \"Completed successfully.\")");
			input_delete_callback(refresh_callback);
 			break;
	}
	mtk_cmdf(appid, "p_stat.barconfig(load, -value %d)", flash_progress);
}

static rtems_id flash_task_id;

static void run_callback(mtk_event *e, void *arg)
{
	rtems_status_code sc;
	
	if(flash_busy()) return;

	if((int)arg == ARG_FILE_UPDATE) {
		mtk_req(appid, bitstream_name, sizeof(bitstream_name), "e1.text");
		mtk_req(appid, bios_name, sizeof(bios_name), "e2.text");
		mtk_req(appid, application_name, sizeof(application_name), "e3.text");

		/* Sanity checks */
		if((bitstream_name[0] == 0) && (bios_name[0] == 0) &&(application_name[0] == 0)) {
			messagebox("Nothing to do!", "No flash images are specified.");
			return;
		}
		/* prevent a race condition that could cause two flash tasks to start
		 * if this function is called twice quickly.
		 */
		flash_state = FLASH_STATE_STARTING;
	}

	flash_progress = 0;

	/* start flashing in the background */
	sc = rtems_task_create(rtems_build_name('F', 'L', 'S', 'H'), 100, 24*1024,
			       RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR,
			       0, &flash_task_id);
	assert(sc == RTEMS_SUCCESSFUL);

	sc = rtems_task_start(flash_task_id, flash_task, (int)arg);
	assert(sc == RTEMS_SUCCESSFUL);

	/* display update */
	next_update = rtems_clock_get_ticks_since_boot() + UPDATE_PERIOD;
	input_add_callback(refresh_callback);
}

static int w_open;

static void close_callback(mtk_event *e, void *arg)
{
	if(flash_busy()) return;
	close_filedialog(file_dlg);
	mtk_cmd(appid, "w.close()");
	mtk_cmd(appid, "w_files.close()");
	w_open = 0;
}
static void openfiles_callback(mtk_event *e, void *arg)
{
	mtk_cmd(appid, "w_files.open()");
}

static void closefiles_callback(mtk_event *e, void *arg)
{
	mtk_cmd(appid, "w_files.close()");
}

static void flash_filedialog_ok_callback()
{
	char filepath[384];

	get_filedialog_selection(file_dlg, filepath, sizeof(filepath));
	mtk_cmdf(appid, "e%d.set(-text \"%s\")", current_file_to_choose, filepath);
	close_filedialog(file_dlg);
}

static void opendialog_callback(mtk_event *e, void *arg)
{
	if(flash_busy()) return;
	current_file_to_choose = (int)arg;
	open_filedialog(file_dlg);
}

void init_flash()
{
	appid = mtk_init_app("Flash");

	mtk_cmd(appid, "g = new Grid()");

	if(sysconfig_is_rescue())
		mtk_cmd(appid, "l0 = new Label(-text \"Click the 'Update from web' button to begin.\nSince you are in rescue mode, the new software will always be reinstalled,\neven if you already have the latest version.\")");
	else
		mtk_cmd(appid, "l0 = new Label(-text \"Click the 'Update from web' button to begin.\nIf your synthesizer does not restart after the update, don't panic!\nHold right (R) pushbutton during power-up to enable rescue mode.\")");

	mtk_cmd_seq(appid,
		"g.place(l0, -column 1 -row 1 -align w)",

		"g2 = new Grid()",
		"l_socbios = new Label(-text \"SoC/BIOS:\")",
		"l_flickernoise = new Label(-text \"Flickernoise:\")",
		"l_patchpool = new Label(-text \"Patch pool:\")",
		"l_installed = new Label(-text \"Installed\")",
		"l_available = new Label(-text \"Available\")",
		0);
		
	mtk_cmdf(appid, "l_socbios_i = new Label(-text \"%s\")", soc);
	mtk_cmd_seq(appid,
		"l_socbios_a = new Label(-text \"?\")",
		"l_flickernoise_i = new Label(-text \""VERSION"\")",
		"l_flickernoise_a = new Label(-text \"?\")",
		"l_patchpool_i = new Label(-text \"?\")",
		"l_patchpool_a = new Label(-text \"?\")",
		"g2.place(l_socbios, -column 1 -row 2)",
		"g2.place(l_flickernoise, -column 1 -row 3)",
		"g2.place(l_patchpool, -column 1 -row 4)",
		"g2.place(l_installed, -column 2 -row 1)",
		"g2.place(l_available, -column 3 -row 1)",
		"g2.place(l_socbios_i, -column 2 -row 2)",
		"g2.place(l_socbios_a, -column 3 -row 2)",
		"g2.place(l_flickernoise_i, -column 2 -row 3)",
		"g2.place(l_flickernoise_a, -column 3 -row 3)",
		"g2.place(l_patchpool_i, -column 2 -row 4)",
		"g2.place(l_patchpool_a, -column 3 -row 4)",

		"g.place(g2, -column 1 -row 2)",

		"sep1 = new Separator(-vertical no)",
		"l_stat = new Label()",
		"p_stat = new LoadDisplay()",
		"sep2 = new Separator(-vertical no)",
		"g.place(sep1, -column 1 -row 3)",
		"g.place(l_stat, -column 1 -row 4)",
		"g.place(p_stat, -column 1 -row 5)",
		"g.place(sep2, -column 1 -row 6)",

		"g3 = new Grid()",

		"b_webupdate = new Button(-text \"Update from web\")",
		"b_checkversions = new Button(-text \"Check versions\")",
		"b_files = new Button(-text \"Update from files\")",
		"b_close = new Button(-text \"Close\")",

		"g3.place(b_checkversions, -column 1 -row 1)",
		"g3.place(b_files, -column 2 -row 1)",
		"g3.place(b_close, -column 3 -row 1)",

		"g.rowconfig(7, -size 10)",
		"g.place(b_webupdate, -column 1 -row 8)",
		"g.place(g3, -column 1 -row 9)",

		"g.rowconfig(1, -size 0)",
		"g.rowconfig(2, -size 0)",
		"g.rowconfig(3, -size 0)",

		"w = new Window(-content g -title \"Update\")",
		
		"gfiles = new Grid()",
		
		"gfiles_sel = new Grid()",
		"gfiles_sel.columnconfig(1, -size 0)",
		"gfiles_sel.columnconfig(2, -size 250)",
		"gfiles_sel.columnconfig(3, -size 0)",
		
		"l1 = new Label(-text \"Bitstream image (.FPG):\")",
		"l2 = new Label(-text \"BIOS image (.BIN):\")",
		"l3 = new Label(-text \"Application image (.FBI):\")",

		"e1 = new Entry()",
		"e2 = new Entry()",
		"e3 = new Entry()",

		"b_browse1 = new Button(-text \"Browse\")",
		"b_browse2 = new Button(-text \"Browse\")",
		"b_browse3 = new Button(-text \"Browse\")",

		"gfiles_sel.place(l1, -column 1 -row 1 -align w)",
		"gfiles_sel.place(l2, -column 1 -row 2 -align w)",
		"gfiles_sel.place(l3, -column 1 -row 3 -align w)",

		"gfiles_sel.place(e1, -column 2 -row 1)",
		"gfiles_sel.place(e2, -column 2 -row 2)",
		"gfiles_sel.place(e3, -column 2 -row 3)",

		"gfiles_sel.place(b_browse1, -column 3 -row 1)",
		"gfiles_sel.place(b_browse2, -column 3 -row 2)",
		"gfiles_sel.place(b_browse3, -column 3 -row 3)",
		
		"gfiles.place(gfiles_sel, -column 1 -row 1)",
		
		"gfiles_btn = new Grid()",
		
		"b_filesupdate = new Button(-text \"Flash\")",
		"b_closefiles = new Button(-text \"Close\")",
		
		"gfiles_btn.columnconfig(1, -size 100)",
		"gfiles_btn.place(b_filesupdate, -column 2 -row 1)",
		"gfiles_btn.place(b_closefiles, -column 3 -row 1)",
		
		"gfiles.place(gfiles_btn, -column 1 -row 2)",
		
		"w_files = new Window(-content gfiles -title \"Update from files\")",
		0);

	mtk_bind(appid, "b_webupdate", "commit", run_callback, (void *)ARG_WEB_UPDATE);
	mtk_bind(appid, "b_checkversions", "commit", run_callback, (void *)ARG_GET_VERSIONS);
	mtk_bind(appid, "b_files", "commit", openfiles_callback, NULL);
	mtk_bind(appid, "b_close", "commit", close_callback, NULL);

	mtk_bind(appid, "w", "close", close_callback, NULL);
	
	mtk_bind(appid, "b_browse1", "commit", opendialog_callback, (void *)1);
	mtk_bind(appid, "b_browse2", "commit", opendialog_callback, (void *)2);
	mtk_bind(appid, "b_browse3", "commit", opendialog_callback, (void *)3);

	mtk_bind(appid, "b_filesupdate", "commit", run_callback, (void *)ARG_FILE_UPDATE);
	mtk_bind(appid, "b_closefiles", "commit", closefiles_callback, NULL);

	mtk_bind(appid, "w_files", "close", closefiles_callback, NULL);

	file_dlg = create_filedialog("Open flash image", 0, "", flash_filedialog_ok_callback, NULL, NULL, NULL);
}

void open_flash_window(int automatic)
{
	if(w_open) return;
	w_open = 1;
	flash_state = FLASH_STATE_READY;
	flash_progress = 0;
	update_progress();
	mtk_cmd(appid, "w.open()");
	if(automatic)
		run_callback(NULL, (void *)ARG_WEB_UPDATE);
}
