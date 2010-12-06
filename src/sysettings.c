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

#include <bsp.h>
#include <stdio.h>
#include <mtklib.h>

#include "input.h"
#include "sysconfig.h"
#include "filedialog.h"
#include "sysettings.h"

static int appid;
static int browse_appid;

static void close_sysettings_window(int save);
static void update_layout();
static void update_network();

static void layout_callback(mtk_event *e, void *arg)
{
	int layout = (int)arg;

	sysconfig_set_keyboard_layout(layout);
	update_layout();
}

static void dhcp_callback(mtk_event *e, void *arg)
{
	int en;

	sysconfig_get_ipconfig(&en, NULL, NULL);
	sysconfig_set_ipconfig(!en, 0, 0);
	update_network();
}

static void ok_callback(mtk_event *e, void *arg)
{
	close_sysettings_window(1);
}

static void cancel_callback(mtk_event *e, void *arg)
{
	close_sysettings_window(0);
}

static void browse_ok_callback()
{
	char filename[384];

	get_filedialog_selection(browse_appid, filename, sizeof(filename));
	close_filedialog(browse_appid);
	mtk_cmdf(appid, "e_autostart.set(-text \"%s\")", filename);
}

static void browse_cancel_callback()
{
	close_filedialog(browse_appid);
}

static void browse_callback(mtk_event *e, void *arg)
{
	open_filedialog(browse_appid, "/");
}

void init_sysettings()
{
	appid = mtk_init_app("System settings");

	mtk_cmd_seq(appid,
		"g = new Grid()",

		"g_keyboard0 = new Grid()",
		"l_keyboard = new Label(-text \"Keyboard\" -font \"title\")",
		"s_keyboard1 = new Separator(-vertical no)",
		"s_keyboard2 = new Separator(-vertical no)",
		"g_keyboard0.place(s_keyboard1, -column 1 -row 1)",
		"g_keyboard0.place(l_keyboard, -column 2 -row 1)",
		"g_keyboard0.place(s_keyboard2, -column 3 -row 1)",
		"g_keyboard1 = new Grid()",
		"l_layout = new Label(-text \"Layout:\")",
		"b_german = new Button(-text \"German\")",
		"b_french = new Button(-text \"French\")",
		"b_us = new Button(-text \"US\")",
		"g_keyboard1.place(l_layout, -column 1 -row 1)",
		"g_keyboard1.place(b_german, -column 2 -row 1)",
		"g_keyboard1.place(b_french, -column 3 -row 1)",
		"g_keyboard1.place(b_us, -column 4 -row 1)",

		"g_network0 = new Grid()",
		"l_network = new Label(-text \"Network\" -font \"title\")",
		"s_network1 = new Separator(-vertical no)",
		"s_network2 = new Separator(-vertical no)",
		"g_network0.place(s_network1, -column 1 -row 1)",
		"g_network0.place(l_network, -column 2 -row 1)",
		"g_network0.place(s_network2, -column 3 -row 1)",
		"g_network1 = new Grid()",
		"l_dhcp = new Label(-text \"DHCP client:\")",
		"b_dhcp = new Button(-text \"Enable\")",
		"l_ip = new Label(-text \"IP address:\")",
		"e_ip = new Entry()",
		"l_netmask = new Label(-text \"Netmask:\")",
		"e_netmask = new Entry()",
		"g_network1.place(l_dhcp, -column 1 -row 1)",
		"g_network1.place(b_dhcp, -column 2 -row 1)",
		"g_network1.place(l_ip, -column 1 -row 2)",
		"g_network1.place(e_ip, -column 2 -row 2)",
		"g_network1.place(l_netmask, -column 1 -row 3)",
		"g_network1.place(e_netmask, -column 2 -row 3)",

		"g_remote0 = new Grid()",
		"l_remote = new Label(-text \"Remote access\" -font \"title\")",
		"s_remote1 = new Separator(-vertical no)",
		"s_remote2 = new Separator(-vertical no)",
		"g_remote0.place(s_remote1, -column 1 -row 1)",
		"g_remote0.place(l_remote, -column 2 -row 1)",
		"g_remote0.place(s_remote2, -column 3 -row 1)",
		"g_remote1 = new Grid()",
		"l_login = new Label(-text \"Login:\")",
		"e_login = new Entry()",
		"l_password = new Label(-text \"Password:\")",
		"e_password = new Entry(-blind yes)",
		"g_remote1.place(l_login, -column 1 -row 1)",
		"g_remote1.place(e_login, -column 2 -row 1)",
		"g_remote1.place(l_password, -column 1 -row 2)",
		"g_remote1.place(e_password, -column 2 -row 2)",

		"g_autostart0 = new Grid()",
		"l_autostart = new Label(-text \"Autostart\" -font \"title\")",
		"s_autostart1 = new Separator(-vertical no)",
		"s_autostart2 = new Separator(-vertical no)",
		"g_autostart0.place(s_autostart1, -column 1 -row 1)",
		"g_autostart0.place(l_autostart, -column 2 -row 1)",
		"g_autostart0.place(s_autostart2, -column 3 -row 1)",
		"g_autostart1 = new Grid()",
		"l_autostart = new Label(-text \"File:\")",
		"e_autostart = new Entry()",
		"b_autostart = new Button(-text \"Browse\")",
		"g_autostart1.place(l_autostart, -column 1 -row 1)",
		"g_autostart1.place(e_autostart, -column 2 -row 1)",
		"g_autostart1.place(b_autostart, -column 3 -row 1)",
		"g_autostart1.columnconfig(3, -size 0)",

		"g_btn = new Grid()",
		"b_ok = new Button(-text \"OK\")",
		"b_cancel = new Button(-text \"Cancel\")",
		"g_btn.columnconfig(1, -size 200)",
		"g_btn.place(b_ok, -column 2 -row 1)",
		"g_btn.place(b_cancel, -column 3 -row 1)",

		"g.place(g_keyboard0, -column 1 -row 1)",
		"g.place(g_keyboard1, -column 1 -row 2)",
		"g.place(g_network0, -column 1 -row 3)",
		"g.place(g_network1, -column 1 -row 4)",
		"g.place(g_remote0, -column 1 -row 5)",
		"g.place(g_remote1, -column 1 -row 6)",
		"g.place(g_autostart0, -column 1 -row 7)",
		"g.place(g_autostart1, -column 1 -row 8)",
		"g.rowconfig(9, -size 10)",
		"g.place(g_btn, -column 1 -row 10)",

		"w = new Window(-content g -title \"System settings\")",
		0);

	mtk_bind(appid, "b_german", "press", layout_callback, (void *)SC_KEYBOARD_LAYOUT_GERMAN);
	mtk_bind(appid, "b_french", "press", layout_callback, (void *)SC_KEYBOARD_LAYOUT_FRENCH);
	mtk_bind(appid, "b_us", "press", layout_callback, (void *)SC_KEYBOARD_LAYOUT_US);

	mtk_bind(appid, "b_dhcp", "press", dhcp_callback, NULL);

	mtk_bind(appid, "b_autostart", "commit", browse_callback, NULL);
	
	mtk_bind(appid, "b_ok", "commit", ok_callback, NULL);
	mtk_bind(appid, "b_cancel", "commit", cancel_callback, NULL);

	mtk_bind(appid, "w", "close", cancel_callback, NULL);

	browse_appid = create_filedialog("Autostart select", 0, browse_ok_callback, NULL, browse_cancel_callback, NULL);
}

static void update_layout()
{
	int layout;

	layout = sysconfig_get_keyboard_layout();

	mtk_cmdf(appid, "b_german.set(-state %s)", layout == SC_KEYBOARD_LAYOUT_GERMAN ? "on" : "off");
	mtk_cmdf(appid, "b_french.set(-state %s)", layout == SC_KEYBOARD_LAYOUT_FRENCH ? "on" : "off");
	mtk_cmdf(appid, "b_us.set(-state %s)", layout == SC_KEYBOARD_LAYOUT_US ? "on" : "off");
}

static void update_network()
{
	int dhcp_enable;
	unsigned int ip;
	unsigned int netmask;

	// TODO: enable/disable text entries with DHCP
	
	sysconfig_get_ipconfig(&dhcp_enable, &ip, &netmask);
	mtk_cmdf(appid, "b_dhcp.set(-state %s)", dhcp_enable ? "on" : "off");
	mtk_cmdf(appid, "e_ip.set(-text \"%u.%u.%u.%u\")",
		(ip & 0xff000000) >> 24,
		(ip & 0x00ff0000) >> 16,
		(ip & 0x0000ff00) >> 8,
		ip & 0x000000ff);
	mtk_cmdf(appid, "e_netmask.set(-text \"%u.%u.%u.%u\")",
		(netmask & 0xff000000) >> 24,
		(netmask & 0x00ff0000) >> 16,
		(netmask & 0x0000ff00) >> 8,
		netmask & 0x000000ff);
}

static void update_credentials()
{
	char login[32];
	char password[32];

	sysconfig_get_credentials(login, password);
	mtk_cmdf(appid, "e_login.set(-text \"%s\")", login);
	mtk_cmdf(appid, "e_password.set(-text \"%s\")", password);
}

static void update_autostart()
{
	char autostart[256];

	sysconfig_get_autostart(autostart);
	mtk_cmdf(appid, "e_autostart.set(-text \"%s\")", autostart);
}

#define UPDATE_PERIOD 30
static rtems_interval next_update;

static void ip_update(mtk_event *e, int count)
{
	rtems_interval t;
	int dhcp_en;

	t = rtems_clock_get_ticks_since_boot();
	if(t >= next_update) {
		sysconfig_get_ipconfig(&dhcp_en, NULL, NULL);
		if(dhcp_en)
			update_network();
		next_update = t + UPDATE_PERIOD;
	}
}

static int w_open;

static int previous_layout;
static int previous_dhcp;
static unsigned int previous_ip, previous_netmask;

void open_sysettings_window()
{
	if(w_open) return;
	w_open = 1;

	update_layout();
	update_network();
	update_credentials();
	update_autostart();

	previous_layout = sysconfig_get_keyboard_layout();
	sysconfig_get_ipconfig(&previous_dhcp, &previous_ip, &previous_netmask);
	
	mtk_cmd(appid, "w.open()");

	next_update = rtems_clock_get_ticks_since_boot() + UPDATE_PERIOD;
	input_add_callback(ip_update);
}

static unsigned int parse_ip(const char *ip)
{
	unsigned int i[4];

	if(sscanf(ip, "%u.%u.%u.%u", &i[0], &i[1], &i[2], &i[3]) != 4)
		return 0;
	return (i[0] << 24)|(i[1] << 16)|(i[2] << 8)|i[3];
}

static void close_sysettings_window(int save)
{
	mtk_cmd(appid, "w.close()");
	w_open = 0;

	input_delete_callback(ip_update);
	close_filedialog(browse_appid);
	
	if(save) {
		char ip_txt[16];
		char netmask_txt[16];
		char login[32];
		char password[32];
		char autostart[256];

		mtk_req(appid, ip_txt, sizeof(ip_txt), "e_ip.text");
		mtk_req(appid, netmask_txt, sizeof(netmask_txt), "e_netmask.text");
		mtk_req(appid, login, sizeof(login), "e_login.text");
		mtk_req(appid, password, sizeof(password), "e_password.text");
		mtk_req(appid, autostart, sizeof(autostart), "e_autostart.text");

		sysconfig_set_ipconfig(-1, parse_ip(ip_txt), parse_ip(netmask_txt));
		sysconfig_set_credentials(login, password);
		sysconfig_set_autostart(autostart);
		
		sysconfig_save();
	} else {
		sysconfig_set_keyboard_layout(previous_layout);
		sysconfig_set_ipconfig(previous_dhcp, previous_ip, previous_netmask);
	}
}
