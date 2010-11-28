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

#include "sysettings.h"

static int appid;

static int w_open;

static void ok_callback(mtk_event *e, void *arg)
{
	mtk_cmd(appid, "w.close()");
	w_open = 0;
}

static void cancel_callback(mtk_event *e, void *arg)
{
	mtk_cmd(appid, "w.close()");
	w_open = 0;
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
		"l_dns1 = new Label(-text \"Primary DNS:\")",
		"e_dns1 = new Entry()",
		"l_dns2 = new Label(-text \"Secondary DNS:\")",
		"e_dns2 = new Entry()",
		"l_hostname = new Label(-text \"Hostname:\")",
		"e_hostname = new Entry()",
		"l_domain = new Label(-text \"Domain:\")",
		"e_domain = new Entry()",
		"g_network1.place(l_dhcp, -column 1 -row 1)",
		"g_network1.place(b_dhcp, -column 2 -row 1)",
		"g_network1.place(l_ip, -column 1 -row 2)",
		"g_network1.place(e_ip, -column 2 -row 2)",
		"g_network1.place(l_netmask, -column 1 -row 3)",
		"g_network1.place(e_netmask, -column 2 -row 3)",
		"g_network1.place(l_dns1, -column 1 -row 4)",
		"g_network1.place(e_dns1, -column 2 -row 4)",
		"g_network1.place(l_dns2, -column 1 -row 5)",
		"g_network1.place(e_dns2, -column 2 -row 5)",
		"g_network1.place(l_hostname, -column 1 -row 6)",
		"g_network1.place(e_hostname, -column 2 -row 6)",
		"g_network1.place(l_domain, -column 1 -row 7)",
		"g_network1.place(e_domain, -column 2 -row 7)",

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
		"b_save = new Button(-text \"Save\")",
		"s_save = new Separator(-vertical yes)",
		"b_ok = new Button(-text \"OK\")",
		"b_cancel = new Button(-text \"Cancel\")",
		"g_btn.columnconfig(1, -size 200)",
		"g_btn.place(b_save, -column 2 -row 1)",
		"g_btn.place(s_save, -column 3 -row 1)",
		"g_btn.place(b_ok, -column 4 -row 1)",
		"g_btn.place(b_cancel, -column 5 -row 1)",

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


	mtk_bind(appid, "b_ok", "commit", ok_callback, NULL);
	mtk_bind(appid, "b_cancel", "commit", cancel_callback, NULL);

	/* TODO: only german layout for now */
	mtk_cmd(appid, "b_german.set(-state on)");

	mtk_bind(appid, "w", "close", cancel_callback, NULL);
}

void open_sysettings_window()
{
	if(w_open) return;
	w_open = 1;
	mtk_cmd(appid, "w.open()");
}
