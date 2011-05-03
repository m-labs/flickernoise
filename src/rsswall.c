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

#include "version.h"
#include "osd.h"
#include "rsswall.h"

void init_rsswall()
{
}

void open_rsswall_window()
{
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

static rtems_id rsswall_task_id;
static rtems_id rsswall_terminated;

static rtems_task rsswall_task(rtems_task_argument argument)
{
	char *previous_entry;
	char *last_entry;
	rtems_event_set events;
	
	previous_entry = NULL;
	while(rtems_event_receive(RTEMS_EVENT_1,
	  RTEMS_EVENT_ANY | RTEMS_WAIT, 
	  500, &events) == RTEMS_TIMEOUT) {
		last_entry = feed_get_last("http://search.twitter.com/search.atom?q=milkymist");
		if(last_entry == NULL) continue;
		printf("fetched: '%s'\n", last_entry);
		if((previous_entry == NULL) || (strcmp(previous_entry, last_entry) != 0)) {
			osd_event(last_entry);
			free(previous_entry);
			previous_entry = last_entry;
		} else
			free(last_entry);
	}
	free(previous_entry);
	rtems_semaphore_release(rsswall_terminated);
	rtems_task_delete(RTEMS_SELF);
}

void rsswall_start()
{
	rtems_status_code sc;
	
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
}

void rsswall_stop()
{
	rtems_event_send(rsswall_task_id, RTEMS_EVENT_1);
	rtems_semaphore_obtain(rsswall_terminated, RTEMS_WAIT, RTEMS_NO_TIMEOUT);
	/* task self-deleted  */
	rtems_semaphore_delete(rsswall_terminated);
}
