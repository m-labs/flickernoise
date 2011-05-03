/*
 * Flickernoise
 * Copyright (C) 2011 Sebastien Bourdeauducq
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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>
#include <expat.h>
#include <mtklib.h>

#include "version.h"
#include "util.h"
#include "osd.h"
#include "cp.h"
#include "config.h"
#include "rsswall.h"

static int appid;
static int w_open;
static int rsswall_enable;

static void load_config()
{
	const char *url, *idle;
	
	rsswall_enable = config_read_int("rss_enable", 0);
	mtk_cmdf(appid, "b_enable.set(-state %s)", rsswall_enable ? "on" : "off");
	url = config_read_string("rss_url");
	/* Use http://search.twitter.com/search.atom?q=milkymist for Twitter */
	if(url == NULL)
		url = "http://identi.ca/api/search.atom?q=milkymist";
	idle = config_read_string("rss_idle");
	if(idle == NULL)
		idle = "Your message here? Hashtag #milkymist on identi.ca";
	mtk_cmdf(appid, "e_url.set(-text \"%s\")", url);
	mtk_cmdf(appid, "e_idle.set(-text \"%s\")", idle);
	mtk_cmdf(appid, "e_refresh.set(-text \"%d\")", config_read_int("rss_refresh", 7));
	mtk_cmdf(appid, "e_idlep.set(-text \"%d\")", config_read_int("rss_idlep", 5));
}

static void set_config()
{
	char url[512];
	char idle[512];
	
	mtk_req(appid, url, sizeof(url), "e_url.text");
	mtk_req(appid, idle, sizeof(idle), "e_idle.text");
	
	config_write_int("rss_enable", rsswall_enable);
	config_write_string("rss_url", url);
	config_write_string("rss_idle", idle);
	config_write_int("rss_refresh", mtk_req_i(appid, "e_refresh.text"));
	config_write_int("rss_idlep", mtk_req_i(appid, "e_idlep.text"));
}

static void close_window()
{
	mtk_cmd(appid, "w.close()");
	w_open = 0;
}

static void ok_callback(mtk_event *e, void *arg)
{
	set_config();
	cp_notify_changed();
	close_window();
}

static void cancel_callback(mtk_event *e, void *arg)
{
	close_window();
}

static void enable_callback(mtk_event *e, void *arg)
{
	rsswall_enable = !rsswall_enable;
	mtk_cmdf(appid, "b_enable.set(-state %s)", rsswall_enable ? "on" : "off");
}

static void clearurl_callback(mtk_event *e, void *arg)
{
	mtk_cmd(appid, "e_url.set(-text \"\")");
}

static void clearidle_callback(mtk_event *e, void *arg)
{
	mtk_cmd(appid, "e_idle.set(-text \"\")");
}

void init_rsswall()
{
	appid = mtk_init_app("RSS wall");

	mtk_cmd_seq(appid,
		"g = new Grid()",

		"g_param = new Grid()",
		"l_enable = new Label(-text \"Wall:\")",
		"b_enable = new Button(-text \"Enable\")",
		"l_url = new Label(-text \"RSS/ATOM URL:\")",
		"e_url = new Entry()",
		"b_urlclear = new Button(-text \"Clear\")",
		"l_idle = new Label(-text \"Idle message:\")",
		"e_idle = new Entry()",
		"b_idleclear = new Button(-text \"Clear\")",
		"l_refresh = new Label(-text \"Refresh period:\")",
		"e_refresh = new Entry()",
		"l_refreshu = new Label(-text \"seconds\")",
		"l_idlep = new Label(-text \"Idle period:\")",
		"e_idlep = new Entry()",
		"l_idlepu = new Label(-text \"refreshes\")",
		"g_param.place(l_enable, -column 1 -row 1)",
		"g_param.place(b_enable, -column 2 -row 1)",
		"g_param.place(l_url, -column 1 -row 2)",
		"g_param.place(e_url, -column 2 -row 2)",
		"g_param.place(b_urlclear, -column 3 -row 2)",
		"g_param.place(l_idle, -column 1 -row 3)",
		"g_param.place(e_idle, -column 2 -row 3)",
		"g_param.place(b_idleclear, -column 3 -row 3)",
		"g_param.place(l_refresh, -column 1 -row 4)",
		"g_param.place(e_refresh, -column 2 -row 4)",
		"g_param.place(l_refreshu, -column 3 -row 4)",
		"g_param.place(l_idlep, -column 1 -row 5)",
		"g_param.place(e_idlep, -column 2 -row 5)",
		"g_param.place(l_idlepu, -column 3 -row 5)",
		"g_param.columnconfig(1, -size 0)",
		"g_param.columnconfig(2, -size 360)",
		"g_param.columnconfig(3, -size 0)",

		"g_btn = new Grid()",
		"b_ok = new Button(-text \"OK\")",
		"b_cancel = new Button(-text \"Cancel\")",
		"g_btn.columnconfig(1, -size 200)",
		"g_btn.place(b_ok, -column 2 -row 1)",
		"g_btn.place(b_cancel, -column 3 -row 1)",

		"g.place(g_param, -column 1 -row 1)",
		"g.rowconfig(2, -size 10)",
		"g.place(g_btn, -column 1 -row 3)",

		"w = new Window(-content g -title \"RSS/ATOM wall\")",
		0);

	mtk_bind(appid, "b_ok", "commit", ok_callback, NULL);
	mtk_bind(appid, "b_cancel", "commit", cancel_callback, NULL);
	
	mtk_bind(appid, "b_enable", "press", enable_callback, NULL);
	mtk_bind(appid, "b_urlclear", "commit", clearurl_callback, NULL);
	mtk_bind(appid, "b_idleclear", "commit", clearidle_callback, NULL);

	mtk_bind(appid, "w", "close", cancel_callback, NULL);
}

void open_rsswall_window()
{
	if(w_open) return;
	w_open = 1;
	load_config();
	mtk_cmd(appid, "w.open()");
}

static size_t data_callback(void *ptr, size_t size, size_t nmemb, void *data)
{
	XML_Parser xml_handle = (XML_Parser)data;
	size_t realsize = size*nmemb;
	int r;
	
	r = XML_Parse(xml_handle, (char *)ptr, realsize, 0);
	if(r == 0)
		return 0;
	return realsize;
}

struct feed_state {
	int got_first;
	int in_item;
	int in_title;
	int title_len;
	char title[256];
};

static void feed_start_element(void *userData, const XML_Char *name, const XML_Char **atts)
{
	struct feed_state *fs = userData;
	
	if(fs->got_first)
		return;

	if((strcmp(name, "item") == 0) || (strcmp(name, "entry") == 0))
		fs->in_item = 1;
	else if(fs->in_item && (strcmp(name, "title") == 0)) {
		fs->title_len = 0;
		fs->in_title = 1;
	}
}

static void feed_end_element(void *userData, const XML_Char *name)
{
	struct feed_state *fs = userData;
	
	if(fs->got_first)
		return;

	if((strcmp(name, "item") == 0) || (strcmp(name, "entry") == 0))
		fs->in_item = 0;
	else if(fs->in_item && (strcmp(name, "title") == 0)) {
		fs->title[fs->title_len] = 0;
		fs->got_first = 1;
		fs->in_title = 0;
	}
}

static void feed_cdata(void *userData, const XML_Char *s, int len)
{
	struct feed_state *fs = userData;
	int max;
	
	if(fs->got_first)
		return;
	
	if(fs->in_title) {
		max = sizeof(fs->title) - fs->title_len - 1;
		if(len > max)
			len = max;
		memcpy(&fs->title[fs->title_len], s, len);
		fs->title_len += len;
	}
}

static char *feed_get_last(const char *url)
{
	CURL *curl_handle;
	XML_Parser xml_handle;
	struct feed_state fs;
	int r;
	CURLcode res;
	
	xml_handle = XML_ParserCreate("UTF-8");
	if(xml_handle == NULL) return NULL;
	fs.got_first = 0;
	fs.in_item = 0;
	fs.in_title = 0;
	fs.title_len = 0;
	XML_SetUserData(xml_handle, &fs);
	XML_SetElementHandler(xml_handle, feed_start_element, feed_end_element);
	XML_SetCharacterDataHandler(xml_handle, feed_cdata);

	curl_handle = curl_easy_init();
	if(curl_handle == NULL) {
		XML_ParserFree(xml_handle);
		return NULL;
	}
	curl_easy_setopt(curl_handle, CURLOPT_URL, url);
	/* FIXME: libcurl timeouts do not work in RTEMS */
	// maybe needed? curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 5);
	curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 5);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, data_callback);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)xml_handle);
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "flickernoise/"VERSION);
	res = curl_easy_perform(curl_handle);
	curl_easy_cleanup(curl_handle);
	
	if(res != 0) {
		XML_ParserFree(xml_handle);
		return NULL;
	}
	
	r = XML_Parse(xml_handle, NULL, 0, 1);
	if(r == 0) {
		XML_ParserFree(xml_handle);
		return NULL;
	}
	XML_ParserFree(xml_handle);
	
	if(fs.title_len == 0)
		return NULL;
	else
		return strdup(fs.title);
}

static int rsswall_running;
static rtems_id rsswall_task_id;
static rtems_id rsswall_terminated;
static const char *rsswall_url;
static const char *rsswall_idle;
static int rsswall_refresh;
static int rsswall_idlep;

static rtems_task rsswall_task(rtems_task_argument argument)
{
	char *previous_entry;
	char *last_entry;
	rtems_event_set events;
	int empty_count;
	
	previous_entry = NULL;
	empty_count = 0;
	while(rtems_event_receive(RTEMS_EVENT_1,
	  RTEMS_EVENT_ANY | RTEMS_WAIT, 
	  rsswall_refresh, &events) == RTEMS_TIMEOUT) {
		last_entry = feed_get_last(rsswall_url);
		if((last_entry != NULL) && ((previous_entry == NULL) || (strcmp(previous_entry, last_entry) != 0))) {
			empty_count = 0;
			osd_event(last_entry);
			free(previous_entry);
			previous_entry = last_entry;
		} else {
			free(last_entry);
			empty_count++;
			if(empty_count == rsswall_idlep) {
				if((rsswall_idle != NULL) && (rsswall_idle[0] != 0))
					osd_event(rsswall_idle);
				empty_count = 0;
			}
		}
	}
	free(previous_entry);
	rtems_semaphore_release(rsswall_terminated);
	rtems_task_delete(RTEMS_SELF);
}

void rsswall_start()
{
	rtems_status_code sc;
	
	if(!rsswall_enable || rsswall_running) return;
	
	rsswall_url = config_read_string("rss_url");
	if((rsswall_url == NULL) || (rsswall_url[0] == 0)) return;
	rsswall_idle = config_read_string("rss_idle");
	rsswall_refresh = config_read_int("rss_refresh", 7)*100;
	if(rsswall_refresh < 100)
		rsswall_refresh = 100;
	rsswall_idlep = config_read_int("rss_idlep", 5);
	if(rsswall_idlep < 2)
		rsswall_idlep = 2;
	
	sc = rtems_semaphore_create(
		rtems_build_name('R', 'S', 'S', 'W'),
		0,
		RTEMS_SIMPLE_BINARY_SEMAPHORE,
		0,
		&rsswall_terminated);
	assert(sc == RTEMS_SUCCESSFUL);
	
	sc = rtems_task_create(rtems_build_name('R', 'S', 'S', 'W'), 15, 128*1024,
		RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR,
		0, &rsswall_task_id);
	assert(sc == RTEMS_SUCCESSFUL);
	sc = rtems_task_start(rsswall_task_id, rsswall_task, 0);
	assert(sc == RTEMS_SUCCESSFUL);
	
	rsswall_running = 1;
}

void rsswall_stop()
{
	if(!rsswall_running) return;
	rtems_event_send(rsswall_task_id, RTEMS_EVENT_1);
	rtems_semaphore_obtain(rsswall_terminated, RTEMS_WAIT, RTEMS_NO_TIMEOUT);
	/* task self-deleted */
	rtems_semaphore_delete(rsswall_terminated);
	rsswall_running = 0;
}
