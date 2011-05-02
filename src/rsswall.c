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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>
#include <expat.h>

#include "rsswall.h"

void init_rsswall()
{
}

static size_t data_callback(void *ptr, size_t size, size_t nmemb, void *data)
{
	XML_Parser xml_handle = (XML_Parser)data;
	size_t realsize = size*nmemb;
	
	XML_Parse(xml_handle, (char *)ptr, realsize, 0);
//	fwrite(ptr, size, nmemb, stdout);
	return realsize;
}

static int feed_in_item;
static int feed_in_title;
static int feed_title_len;
static char feed_title[256];

static void feed_start_element(void *userData, const XML_Char *name, const XML_Char **atts)
{
	if((strcmp(name, "item") == 0) || (strcmp(name, "entry") == 0))
		feed_in_item = 1;
	else if(feed_in_item && (strcmp(name, "title") == 0)) {
		feed_title_len = 0;
		feed_in_title = 1;
	}
}

static void feed_end_element(void *userData, const XML_Char *name)
{
	if((strcmp(name, "item") == 0) || (strcmp(name, "entry") == 0))
		feed_in_item = 0;
	else if(feed_in_item && (strcmp(name, "title") == 0)) {
		feed_title[feed_title_len] = 0;
		printf("Item: '%s'\n", feed_title);
		feed_in_title = 0;
	}
}

static void feed_cdata(void *userData, const XML_Char *s, int len)
{
	int max;
	
	if(feed_in_title) {
		max = sizeof(feed_title) - feed_title_len - 1;
		if(len > max)
			len = max;
		memcpy(&feed_title[feed_title_len], s, len);
		feed_title_len += len;
	}
}

void open_rsswall_window()
{
	CURL *curl_handle;
	XML_Parser xml_handle;
	
	xml_handle = XML_ParserCreate("UTF-8");
	XML_SetElementHandler(xml_handle, feed_start_element, feed_end_element);
	XML_SetCharacterDataHandler(xml_handle, feed_cdata);
	
	curl_handle = curl_easy_init();
	curl_easy_setopt(curl_handle, CURLOPT_URL, "http://search.twitter.com/search.atom?q=milkymist");
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, data_callback);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)xml_handle);
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	curl_easy_perform(curl_handle);
	curl_easy_cleanup(curl_handle);
	
	XML_Parse(xml_handle, NULL, 0, 1);
	XML_ParserFree(xml_handle);
}
