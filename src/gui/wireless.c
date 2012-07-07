/*
 * Flickernoise
 * Copyright (C) 2012 Xiangfu Liu
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

#include <bsp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <assert.h>
#include <mtklib.h>
#include <curl/curl.h>
#include <jansson.h>

#include "../util.h"
#include "../input.h"
#include "../config.h"
#include "cp.h"
#include "messagebox.h"

#include "wireless.h"

static int appid;
static int w_open;

static char *wireless[20][3];
static char wireless_buf[32768];
static int wireless_index;

struct data_buffer {
	void		*buf;
	size_t		len;
};

struct upload_buffer {
	const void	*buf;
	size_t		len;
};

static void databuf_free(struct data_buffer *db)
{
	if(!db)
		return;

	free(db->buf);

	memset(db, 0, sizeof(*db));
}

static size_t all_data_cb(const void *ptr, size_t size, size_t nmemb,
			  void *user_data)
{
	struct data_buffer *db = user_data;
	size_t len = size * nmemb;
	size_t oldlen, newlen;
	void *newmem;
	static const unsigned char zero = 0;

	oldlen = db->len;
	newlen = oldlen + len;

	newmem = realloc(db->buf, newlen + 1);
	if(!newmem)
		return 0;

	db->buf = newmem;
	db->len = newlen;
	memcpy(db->buf + oldlen, ptr, len);
	memcpy(db->buf + newlen, &zero, 1);	/* null terminate */

	return len;
}

static size_t upload_data_cb(void *ptr, size_t size, size_t nmemb,
			     void *user_data)
{
	struct upload_buffer *ub = user_data;
	unsigned int len = size * nmemb;

	if(len > ub->len)
		len = ub->len;

	if(len) {
		memcpy(ptr, ub->buf, len);
		ub->buf += len;
		ub->len -= len;
	}

	return len;
}

static json_t *json_rpc_call(CURL *curl, const char *url,
		     const char *userpass, const char *rpc_req, int timeout)
{
	json_t *val, *err_val, *res_val;
	int rc;
	struct data_buffer all_data = {NULL, 0};
	struct upload_buffer upload_data;
	json_error_t err;
	struct curl_slist *headers = NULL;
	char len_hdr[64];
	char curl_err_str[CURL_ERROR_SIZE];
	char *s;

	memset(&err, 0, sizeof(err));

	/* it is assumed that 'curl' is freshly [re]initialized at this pt */
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);

#if 0 /* Disable curl debugging since it spews to stderr */
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
#endif
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_ENCODING, "");
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
	curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "");

	/* Shares are staggered already and delays in submission can be costly
	 * so do not delay them */
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, all_data_cb);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &all_data);
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, upload_data_cb);
	curl_easy_setopt(curl, CURLOPT_READDATA, &upload_data);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_err_str);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_TRY);
	if(userpass) {
		curl_easy_setopt(curl, CURLOPT_USERPWD, userpass);
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
	}
	curl_easy_setopt(curl, CURLOPT_POST, 1);

	upload_data.buf = rpc_req;
	upload_data.len = strlen(rpc_req);

	sprintf(len_hdr, "Content-Length: %lu",
		(unsigned long) upload_data.len);

	headers = curl_slist_append(headers,
		"Content-type: application/json");
	headers = curl_slist_append(headers, len_hdr);
	headers = curl_slist_append(headers, "Expect:");/* disable Expect hdr*/

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	rc = curl_easy_perform(curl);
	if(rc) {
		printf("HTTP request failed: %s\n", curl_err_str);
		messagebox("Wireless settings", "Cannot reach wireless gadget");
		goto err_out;
	}

	if(!all_data.buf) {
		printf("Empty data received in json_rpc_call.");
		goto err_out;
	}

	val = json_loads(all_data.buf, 0, &err);
	if(!val) {
		printf("JSON decode failed(%d): %s", err.line, err.text);
		printf("JSON protocol response:\n%s", (char *)all_data.buf);
		goto err_out;
	}

	/* JSON-RPC valid response returns a non-null 'result',
	 * and a null 'error'.
	 */
	res_val = json_object_get(val, "result");
	err_val = json_object_get(val, "error");

	if(!res_val || json_is_null(res_val) ||
	    (err_val && !json_is_null(err_val))) {
		if(err_val)
			s = json_dumps(err_val, JSON_INDENT(3));
		else
			s = strdup("(unknown reason)");

		printf("JSON-RPC call failed: %s\n", s);
		free(s);

		goto err_out;
	}

	databuf_free(&all_data);
	curl_slist_free_all(headers);
	curl_easy_reset(curl);

	return val;

err_out:
	databuf_free(&all_data);
	curl_slist_free_all(headers);
	curl_easy_reset(curl);
	curl_easy_setopt(curl, CURLOPT_FRESH_CONNECT, 1);

	return NULL;
}

static char *jget_key(const json_t *obj, const char *key, int required)
{
	char *str;
	json_t *tmp;

	tmp = json_object_get(obj, key);
	if(!tmp) {
		if(required)
			printf("JSON key '%s' not found", key);
		return NULL;
	}

	str = (char *)json_string_value(tmp);
	if(!str) {
		printf("JSON key '%s' is not a string", key);
		return NULL;
	}

	return str;
}

static int update_list(const char *str)
{
	char lst[32768];
	char *p, *pch;
	char *wireless_l[20];
	int i, j, l;

	memset(wireless_buf, 0, sizeof(wireless_buf));
	strcpy(wireless_buf, str);
	l = strlen(wireless_buf) - 1;
	if(l < 2) {
		mtk_cmdf(appid, "lst_wireless.set(-text \"%s\" -selection 0)", "No wireless found");
		return 0;
	}
	while(wireless_buf[l] == '\n' || wireless_buf[l] == ' ')
		wireless_buf[l--] = 0;

	i = 0;
	pch = strtok(wireless_buf, "\n");
	while(pch) {
		wireless_l[i] = pch;
		pch = strtok(NULL, "\n");
		i++;
	}
	wireless_l[i] = NULL;

	i = 0;
	while(wireless_l[i]) {
		j = 0;
		pch = strtok(wireless_l[i], ",");
		while(pch) {
			wireless[i][j] = pch;
			pch = strtok(NULL, ",");
			j++;
		}
		i++;
	}

	i = 0;
	p = lst;
	while(wireless_l[i]) {
		p += sprintf(p, "%s    %s\n", wireless[i][0], wireless[i][1]);
		i++;
	}

	if(p != lst) {
		p--;
		*p = 0;
	}

	mtk_cmdf(appid, "lst_wireless.set(-text \"%s\" -selection 0)", lst);
	wireless_index = 0;

	return 0;
}

static void selchange_callback(mtk_event *e, void *arg)
{
	wireless_index = mtk_req_i(appid, "lst_wireless.selection");
}

static char *rpc_req_login = "{"
	"\"jsonrpc\":\"1.0\","
	"\"method\" :\"login\","
	"\"params\" :[\"root\", \"\"],"
	"\"id\":0"
	"}";
static char *rpc_req_scan =
	"{"
	"\"jsonrpc\":\"1.0\","
	"\"method\" :\"exec\","
	"\"params\" :[\"/opt/m1-wireless.sh get\"],"
	"\"id\"     :0"
	"}";
static char *rpc_req_status =
	"{"
	"\"jsonrpc\":\"1.0\","
	"\"method\" :\"exec\","
	"\"params\" :[\"/opt/m1-wireless.sh status\"],"
	"\"id\"     :0"
	"}";

#define URL_703N "http://192.168.42.1/cgi-bin/luci/rpc"

static void scan_callback(mtk_event *e, void *arg)
{
	const char *url = URL_703N"/auth";
	const char *userpass = NULL;

	CURL *curl;
	json_t *val;
	char *key, *url_token;

	curl = curl_easy_init();

	val = json_rpc_call(curl, url, userpass, rpc_req_login, 5);
	if(!val) {
		curl_easy_cleanup(curl);
		return;
	}

	key = jget_key(val, "result", 0);
	if(!key) {
		json_decref(val);
		curl_easy_cleanup(curl);
		return;
	}
		
	url_token = malloc(100);
	if(!url_token) {
		json_decref(val);
		curl_easy_cleanup(curl);
		messagebox("Error", "Out of memory");
		return;
	}

	strcpy(url_token, URL_703N"/sys?auth=\0");
	url_token = strcat(url_token, key);

	json_decref(val);

	val = json_rpc_call(curl, (char *)url_token, userpass, rpc_req_scan, 10);
	if(!val) {
		free(url_token);
		curl_easy_cleanup(curl);
		return;
	}

	key = jget_key(val, "result", 0);
	if(!key) {
		free(url_token);
		json_decref(val);
		curl_easy_cleanup(curl);
		return;
	}

	update_list(key);

	free(url_token);
	json_decref(val);
	curl_easy_cleanup(curl);
}

static int status(void)
{
	const char *url = URL_703N"/auth";
	const char *userpass = NULL;

	CURL *curl;
	json_t *val;
	char *key, *url_token;
	char *essid, *ip, *tmp;
	int ret = 1;

	mtk_cmd(appid, "l_essid.set(-text \"Disconnect\")");
	mtk_cmd(appid, "l_ip.set(-text \"0.0.0.0\")");

	curl = curl_easy_init();

	val = json_rpc_call(curl, (char *)url, userpass, rpc_req_login, 5);
	if(!val) {
		curl_easy_cleanup(curl);
		return ret;
	}

	url_token = malloc(100);
	if(!url_token) {
		json_decref(val);
		curl_easy_cleanup(curl);
		messagebox("Error", "Out of memory");
		return ret;
	}
	strcpy(url_token, URL_703N"/sys?auth=");

	key = jget_key(val, "result", 0);
	url_token = strcat(url_token, key);

	json_decref(val);

	val = json_rpc_call(curl, (char *)url_token, userpass, rpc_req_status, 30);
	if(!val) {
		curl_easy_cleanup(curl);
		free(url_token);
		return ret;
	}

	key = jget_key(val, "result", 0);

	essid = strdup(key);
	if(!essid) {
		free(url_token);
		json_decref(val);
		curl_easy_cleanup(curl);
		messagebox("Error", "Out of memory");
		return ret;
	}

	ip = strchr(essid, '\n');
	if(ip) *ip = 0;
	ip++;

	tmp = strchr(ip, '\n');
	if(tmp) *tmp = 0;

	mtk_cmdf(appid, "l_essid.set(-text \"%s\")", essid);
	mtk_cmdf(appid, "l_ip.set(-text \"%s\")", ip);

	free(essid);
	free(url_token);
	json_decref(val);
	curl_easy_cleanup(curl);

	return 0;
}

static void status_callback(mtk_event *e, void *arg)
{
	status();
}

static void connect_callback(mtk_event *e, void *arg)
{
	const char *url = URL_703N"/auth";
	const char *userpass = NULL;
	char rpc_req_set[256];

	CURL *curl;
	json_t *val;
	char *key, *url_token;

	char passwd[384];

	if(!wireless[wireless_index][1] || !wireless[wireless_index][2]) {
		messagebox("Error", "No network select");
		return;
	}

	if(!strcmp(wireless[wireless_index][2], "none")) {
		strcpy(passwd, "none");
	} else {
		mtk_req(appid, passwd, sizeof(passwd), "e_password.text");
		if(passwd[0] == 0) {
			messagebox("Error", "Not OPEN wireless, input password");
			return;
		}
	}

	curl = curl_easy_init();

	val = json_rpc_call(curl, (char *)url, userpass, rpc_req_login, 5);
	if(!val) {
		curl_easy_cleanup(curl);
		messagebox("Error", "Cannot reach wireless gadget");
		return;
	}

	url_token = malloc(100);
	if(!url_token) {
		json_decref(val);
		curl_easy_cleanup(curl);
		messagebox("Error", "Out of memory");
		return;
	}

	key = jget_key(val, "result", 0);
	strcpy(url_token, URL_703N"/sys?auth=\0");
	url_token = strcat(url_token, key);

	json_decref(val);

	sprintf(rpc_req_set, "{"
		"\"jsonrpc\":\"1.0\","
		"\"method\" :\"exec\","
		"\"params\" :[\"/opt/m1-wireless.sh set %s %s %s\"],"
		"\"id\"     :0"
		"}",
		wireless[wireless_index][1], passwd,
		wireless[wireless_index][2]);

	val = json_rpc_call(curl, (char *)url_token, userpass, rpc_req_set, 30);
	if(!val) {
		free(url_token);
		curl_easy_cleanup(curl);
		messagebox("Error", "Cannot reach wireless gadget");
		return;
	}
	json_decref(val);

	status();

	free(url_token);
	curl_easy_cleanup(curl);
}

static void close_window(void)
{
	mtk_cmd(appid, "w.close()");
	w_open = 0;
}

static void ok_callback(mtk_event *e, void *arg)
{
	close_window();
}

static void cancel_callback(mtk_event *e, void *arg)
{
	close_window();
}

void init_wireless(void)
{
	appid = mtk_init_app("WIRELESS");

	mtk_cmd_seq(appid,
		"g = new Grid()",

		"g_status = new Grid()",
		"l_status = new Label(-text \"Status\" -font \"title\")",
		"s_status1 = new Separator(-vertical no)",
		"s_status2 = new Separator(-vertical no)",
		"g_status.place(s_status1, -column 1 -row 1)",
		"g_status.place(l_status, -column 2 -row 1)",
		"g_status.place(s_status2, -column 3 -row 1)",

		"g_status1 = new Grid()",
		"l_ESSID = new Label(-text \"ESSID:\")",
		"l_IP    = new Label(-text \"IP:\")",
		"l_essid = new Label(-text \"\")",
		"l_ip    = new Label(-text \"\")",
		"g_status1.place(l_ESSID, -column 1 -row 1)",
		"g_status1.place(l_essid, -column 2 -row 1)",
		"g_status1.place(l_IP, -column 1 -row 2)",
		"g_status1.place(l_ip, -column 2 -row 2)",
		"g_status1.columnconfig(1, -size 100)",

		"g_wirelesslist = new Grid()",
		"l_wireless = new Label(-text \"Wireless networks\" -font \"title\")",
		"s_wireless1 = new Separator(-vertical no)",
		"s_wireless2 = new Separator(-vertical no)",
		"g_wirelesslist.place(s_wireless1, -column 1 -row 1)",
		"g_wirelesslist.place(l_wireless, -column 2 -row 1)",
		"g_wirelesslist.place(s_wireless2, -column 3 -row 1)",
		"lst_wireless = new List()",
		"lst_wirelessf = new Frame(-content lst_wireless -scrollx no -scrolly yes)",

		"g_connect = new Grid()",
		"s_connects = new Separator(-vertical no)",
		"g_connect.place(s_connects, -column 1 -row 1)",

		"g_connect1 = new Grid()",
		"l_password = new Label(-text \"Password:\")",
		"e_password = new Entry()",
		"b_connect  = new Button(-text \"Connect\")",
		"g_connect1.place(l_password, -column 1 -row 2)",
		"g_connect1.place(e_password, -column 2 -row 2)",
		"g_connect1.place(b_connect,  -column 3 -row 2)",
		"g_connect1.columnconfig(2, -size 150)",

		"b_scan = new Button(-text \"Scan\")",
		"b_status = new Button(-text \"Status\")",

		"g.place(g_status, -column 1 -row 1)",
		"g.place(g_status1, -column 1 -row 2)",
		"g.place(b_status, -column 1 -row 3)",

		"g.place(g_wirelesslist, -column 1 -row 4)",
		"g.place(lst_wirelessf, -column 1 -row 5)",
		"g.rowconfig(5, -size 130)",
		"g.place(g_connect, -column 1 -row 6)",
		"g.place(b_scan, -column 1 -row 7)",
		"g.place(g_connect1, -column 1 -row 8)",

		"g_btn = new Grid()",
		"b_ok = new Button(-text \"OK\")",
		"b_cancel = new Button(-text \"Cancel\")",
		"g_btn.columnconfig(1, -size 200)",
		"g_btn.place(b_ok, -column 2 -row 1)",
		"g_btn.place(b_cancel, -column 3 -row 1)",

		"g.rowconfig(9, -size 10)",
		"g.place(g_btn, -column 1 -row 10)",

		"w = new Window(-content g -title \"Wireless settings\" -workx 10)",
		0);

	mtk_bind(appid, "lst_wireless", "selchange", selchange_callback, NULL);
	mtk_bind(appid, "lst_wireless", "selcommit", selchange_callback, NULL);

	mtk_bind(appid, "b_scan", "commit", scan_callback, NULL);
	mtk_bind(appid, "b_connect", "commit", connect_callback, NULL);
	mtk_bind(appid, "b_status", "commit", status_callback, NULL);
	mtk_bind(appid, "b_ok", "commit", ok_callback, NULL);
	mtk_bind(appid, "b_cancel", "commit", cancel_callback, NULL);

	mtk_bind(appid, "w", "close", cancel_callback, NULL);
}

void open_wireless_window(void)
{
	if(w_open) return;
	if(status()) return;

	mtk_cmd(appid, "lst_wireless.set(-text \"\" -selection 0)");
	wireless[0][0] = wireless[0][1] = wireless[0][2] = 0;
	wireless_index = 0;

	w_open = 1;
	mtk_cmd(appid, "w.open()");
}
