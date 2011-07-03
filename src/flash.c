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
#include "performance.h"
#include "patchpool.h"

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
	
	DOWNLOAD_STATE_ERROR_GET_VERSIONS,

	DOWNLOAD_STATE_BITSTREAM,
	DOWNLOAD_STATE_ERROR_BITSTREAM,

	DOWNLOAD_STATE_BIOS,
	DOWNLOAD_STATE_ERROR_BIOS,

	DOWNLOAD_STATE_APP,
	DOWNLOAD_STATE_ERROR_APP,

	DOWNLOAD_STATE_PATCHES,

	FLASH_STATE_SUCCESS
};

static int flash_state;
static int flash_progress;
static int flashvalid_val;

static int installed_patches;
static char unknown_version[2] = "?";
static char available_socbios_buf[32];
static char available_application_buf[32];
static char *available_socbios;
static char *available_application;
static int available_patches;

static char bitstream_name[384];
static char bios_name[384];
static char application_name[384];

static double *d_progress;

static int progress_callback(void *d_progress, double t, double d, double ultotal, double ulnow)
{
	flash_progress = 100.0d*d/t;
	return 0;
}

static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t written;
	written = fwrite(ptr, size, nmemb, stream);
	return written;
}

static int download(const char *url, const char *filename, int progress)
{
	CURL *curl;
	FILE *fp;
	int ret = 0;
	long http_code;

	fp = fopen(filename, "wb");
	if(fp == NULL) return 0;

	curl = curl_easy_init();
	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 180);

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

		if(progress) {
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
			curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_callback);
			curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &d_progress);
		}

		if(curl_easy_perform(curl) == 0) {
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
			if(http_code == 200)
				ret = 1;
		}

		curl_easy_cleanup(curl);
	}

	fclose(fp);
	
	/* delete failed downloads */
	if(!ret)
		unlink(filename);
	
	return ret;
}

struct memory_struct {
	char *memory;
	size_t size;
};

static size_t write_memory_callback(void *ptr, size_t size, size_t nmemb, void *data)
{
	size_t realsize = size * nmemb;
	struct memory_struct *mem = (struct memory_struct *)data;

	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if(mem->memory == NULL) {
		printf("flash.c: write_memory_callback: not enough memory (realloc returned NULL)\n");
		return -1;
	}

	memcpy(&(mem->memory[mem->size]), ptr, realsize);
	mem->size += realsize;

	return realsize;
}

static char *download_mem(const char *url)
{
	CURL *curl;
	struct memory_struct chunk;
	long http_code;

	chunk.memory = malloc(1);
	chunk.size = 0;
	
	curl = curl_easy_init();
	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 180);

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

		if(curl_easy_perform(curl) != 0)
			chunk.size = 0;
		else {
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
			if(http_code != 200)
				chunk.size = 0;
		}

		curl_easy_cleanup(curl);
	}
	
	if(chunk.size == 0) {
		free(chunk.memory);
		return NULL;
	} else {
		chunk.memory[chunk.size] = 0;
		return chunk.memory;
	}
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

#define BASE_URL "http://www.milkymist.org/updates/current/"

static void get_versions(struct patchpool *local_patches, struct patchpool *remote_patches)
{
	char *b;
	char *c;
	
	flash_progress = 0;
	installed_patches = -1;
	available_socbios = unknown_version;
	available_application = unknown_version;
	available_patches = -1;

	patchpool_add_files(local_patches, SIMPLE_PATCHES_FOLDER, "fnp");
	installed_patches = patchpool_count(local_patches);

	flash_progress = 25;

	b = download_mem(BASE_URL "version-soc");
	if(b == NULL)
		flash_terminate(DOWNLOAD_STATE_ERROR_GET_VERSIONS);
	strncpy(available_socbios_buf, b, sizeof(available_socbios_buf));
	available_socbios_buf[sizeof(available_socbios_buf)-1] = 0;
	c = strchr(available_socbios_buf, '\n');
	if(c != NULL)
		*c = 0;
	available_socbios = available_socbios_buf; 
	free(b);
	
	flash_progress = 50;
	
	b = download_mem(BASE_URL "version-app");
	if(b == NULL)
		flash_terminate(DOWNLOAD_STATE_ERROR_GET_VERSIONS);
	strncpy(available_application_buf, b, sizeof(available_application_buf));
	available_application_buf[sizeof(available_application_buf)-1] = 0;
	c = strchr(available_application_buf, '\n');
	if(c != NULL)
		*c = 0;
	available_application = available_application_buf;
	free(b);
	
	flash_progress = 75;
	
	b = download_mem(BASE_URL "patchpool");
	if(b == NULL)
		flash_terminate(DOWNLOAD_STATE_ERROR_GET_VERSIONS);
	patchpool_add_multi(remote_patches, b);
	free(b);
	available_patches = patchpool_count(remote_patches);
	
	flash_progress = 100;
}

static void download_images()
{
	if(strcmp(available_socbios, soc) != 0) {
		strcpy(bitstream_name, "/ramdisk/soc.fpg");
		strcpy(bios_name, "/ramdisk/bios.bin");
	} else {
		bitstream_name[0] = 0;
		bios_name[0] = 0;
	}
	if(strcmp(available_application, VERSION) != 0)
		strcpy(application_name, "/ramdisk/flickernoise.fbi");
	else
		application_name[0] = 0;

	if(bitstream_name[0] != 0) {
		flash_state = DOWNLOAD_STATE_BITSTREAM;
		if(!download(BASE_URL "soc.fpg", bitstream_name, 1))
			flash_terminate(DOWNLOAD_STATE_ERROR_BITSTREAM);
	}

	if(bios_name[0] != 0) {
		flash_state = DOWNLOAD_STATE_BIOS;
		if(!download(BASE_URL "bios.bin", bios_name, 1))
			flash_terminate(DOWNLOAD_STATE_ERROR_BIOS);
	}

	if(application_name[0] != 0) {
		flash_state = DOWNLOAD_STATE_APP;
		if(!download(BASE_URL "flickernoise.fbi", application_name, 1))
			flash_terminate(DOWNLOAD_STATE_ERROR_APP);
	}
}

/* As often, standard adoration does a lot of damage and prevents people from
 * writing software that just works by default.
 * (http://curl.haxx.se/mail/lib-2009-08/0176.html)
 */
static char *curl_author_needs_to_take_a_dump(char *s)
{
	CURL *curl;
	char *ret, *ret2;
	
	curl = curl_easy_init();
	if(!curl) return NULL;
	ret = curl_easy_escape(curl, s, 0);
	if(!ret) {
		curl_easy_cleanup(curl);
		return NULL;
	}
	/* I'm not sure if the pointer is still valid after curl_easy_cleanup(),
	 * and the curl doc doesn't say that.
	 */
	ret2 = strdup(ret);
	curl_free(ret);
	if(!ret2) {
		curl_easy_cleanup(curl);
		return NULL;
	}
	curl_easy_cleanup(curl);
	return ret2;
}

static void download_patches(struct patchpool *pp)
{
	int done, total;
	int i;
	char url[384];
	char target[384];
	char *encoded;
	
	/* TODO: ensure that the patch pool directory exists. Issue #25 precludes a simple mkdir(). */
	
	done = 0;
	total = patchpool_count(pp);
	if(total == 0)
		return;
	flash_state = DOWNLOAD_STATE_PATCHES;
	for(i=0;i<pp->alloc_size;i++) {
		if(pp->names[i] != NULL) {
			encoded = curl_author_needs_to_take_a_dump(pp->names[i]);
			snprintf(url, sizeof(url), BASE_URL "patches/%s", encoded);
			free(encoded);
			snprintf(target, sizeof(target), SIMPLE_PATCHES_FOLDER "%s", pp->names[i]);
			download(url, target, 0);
			done++;
		}
		flash_progress = 100*done/total;
	}
}

static void flash_images()
{
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
}

static rtems_task flash_task(rtems_task_argument argument)
{
	int task = (int)argument;
	struct patchpool local_patches;
	struct patchpool remote_patches;

	switch(task) {
		case ARG_GET_VERSIONS:
		case ARG_WEB_UPDATE:
			patchpool_init(&local_patches);
			patchpool_init(&remote_patches);
			get_versions(&local_patches, &remote_patches);
			if(task == ARG_GET_VERSIONS) {
				patchpool_free(&local_patches);
				patchpool_free(&remote_patches);
				break;
			}
			download_images();
			patchpool_diff(&remote_patches, &local_patches);
			download_patches(&remote_patches);
			patchpool_free(&local_patches);
			patchpool_free(&remote_patches);
			/* fall through */
		case ARG_FILE_UPDATE:
			flash_images();
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
		case DOWNLOAD_STATE_ERROR_GET_VERSIONS:
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

static void update_done()
{
	input_delete_callback(refresh_callback);
}

static void update_progress()
{
	if(installed_patches < 0)
		mtk_cmd(appid, "l_patchpool_i.set(-text \"\e?\")");
	else
		mtk_cmdf(appid, "l_patchpool_i.set(-text \"\e%d\")", installed_patches);
	mtk_cmdf(appid, "l_socbios_a.set(-text \"\e%s\")", available_socbios);
	mtk_cmdf(appid, "l_flickernoise_a.set(-text \"\e%s\")", available_application);
	if(available_patches < 0)
		mtk_cmd(appid, "l_patchpool_a.set(-text \"\e?\")");
	else
		mtk_cmdf(appid, "l_patchpool_a.set(-text \"\e%d\")", available_patches);

	switch(flash_state) {
		case FLASH_STATE_READY:
			mtk_cmd(appid, "l_stat.set(-text \"Ready.\")");
			break;
		case FLASH_STATE_STARTING:
			mtk_cmd(appid, "l_stat.set(-text \"Working...\")");
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
			update_done();
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
			update_done();
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
			update_done();
			break;
		case DOWNLOAD_STATE_ERROR_GET_VERSIONS:
			mtk_cmd(appid, "l_stat.set(-text \"Failed to download version information.\")");
			update_done();
			break;
		case DOWNLOAD_STATE_BITSTREAM:
			mtk_cmd(appid, "l_stat.set(-text \"Downloading bitstream...\")");
			break;
		case DOWNLOAD_STATE_ERROR_BITSTREAM:
			mtk_cmd(appid, "l_stat.set(-text \"Failed to download bitstream.\")");
			update_done();
			break;
		case DOWNLOAD_STATE_BIOS:
			mtk_cmd(appid, "l_stat.set(-text \"Downloading BIOS...\")");
			break;
		case DOWNLOAD_STATE_ERROR_BIOS:
			mtk_cmd(appid, "l_stat.set(-text \"Failed to download BIOS.\")");
			update_done();
			break;
		case DOWNLOAD_STATE_APP:
			mtk_cmd(appid, "l_stat.set(-text \"Downloading application...\")");
			break;
		case DOWNLOAD_STATE_ERROR_APP:
			mtk_cmd(appid, "l_stat.set(-text \"Failed to download application.\")");
			update_done();
			break;
		case DOWNLOAD_STATE_PATCHES:
			mtk_cmd(appid, "l_stat.set(-text \"Downloading patches...\")");
			break;
		case FLASH_STATE_SUCCESS:
			mtk_cmd(appid, "l_stat.set(-text \"Completed successfully.\")");
			update_done();
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
		if((bitstream_name[0] == 0) && (bios_name[0] == 0) && (application_name[0] == 0)) {
			messagebox("Nothing to do!", "No flash images are specified.");
			return;
		}
	}

	flash_state = FLASH_STATE_STARTING; /* < this state is here for race condition prevention purposes */
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
		"l_socbios = new Label(-text \"\eSoC/BIOS:\")",
		"l_flickernoise = new Label(-text \"\eFlickernoise:\")",
		"l_patchpool = new Label(-text \"Patch pool:\")",
		"l_installed = new Label(-text \"Installed\")",
		"l_available = new Label(-text \"Available\")",
		0);
		
	mtk_cmdf(appid, "l_socbios_i = new Label(-text \"\e%s\")", soc);
	mtk_cmd_seq(appid,
		"l_socbios_a = new Label(-text \"\e?\")",
		"l_flickernoise_i = new Label(-text \"\e"VERSION"\")",
		"l_flickernoise_a = new Label(-text \"\e?\")",
		"l_patchpool_i = new Label(-text \"\e?\")",
		"l_patchpool_a = new Label(-text \"\e?\")",
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
	installed_patches = -1;
	available_socbios = unknown_version;
	available_application = unknown_version;
	available_patches = -1;
	update_progress();
	mtk_cmd(appid, "w.open()");
	if(automatic)
		run_callback(NULL, (void *)ARG_WEB_UPDATE);
}
