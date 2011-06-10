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

#include <bsp.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <mtklib.h>

#include "input.h"
#include "sysconfig.h"
#include "filedialog.h"
#include "sysettings.h"

static int appid;
static struct filedialog *browse_wallpaper_dlg;
static struct filedialog *browse_autostart_dlg;

static void close_sysettings_window(int save);
static void update_language();
static void update_layout();
static void update_network();
static void update_asmode();

static void resolution_callback(mtk_event *e, void *arg)
{
	int resolution = (int)arg;

	sysconfig_set_resolution(resolution);
	sysettings_update_resolution();
}

static void language_callback(mtk_event *e, void *arg)
{
	int language = (int)arg;
	
	sysconfig_set_language(language);
	update_language();
}

static void layout_callback(mtk_event *e, void *arg)
{
	int layout = (int)arg;

	sysconfig_set_keyboard_layout(layout);
	update_layout();
}

static void dhcp_callback(mtk_event *e, void *arg)
{
	int en;

	sysconfig_get_ipconfig(&en, NULL, NULL, NULL, NULL, NULL);
	sysconfig_set_ipconfig(!en, 0, 0, 0, 0, 0);
	update_network();
}

static int current_asmode;

static void asmode_callback(mtk_event *e, void *arg)
{
	int asmode = (int)arg;

	current_asmode = asmode;
	update_asmode();
}

static int current_asmode_dt;

static void asmode_dt_callback(mtk_event *e, void *arg)
{
	current_asmode_dt = !current_asmode_dt;
	update_asmode();
}

static int current_asmode_as;

static void asmode_as_callback(mtk_event *e, void *arg)
{
	current_asmode_as = !current_asmode_as;
	update_asmode();
}

static void ok_callback(mtk_event *e, void *arg)
{
	close_sysettings_window(1);
}

static void cancel_callback(mtk_event *e, void *arg)
{
	close_sysettings_window(0);
}

static void wallpaper_ok_callback()
{
	char filename[384];

	get_filedialog_selection(browse_wallpaper_dlg, filename, sizeof(filename));
	mtk_cmdf(appid, "e_wallpaper.set(-text \"%s\")", filename);
}

static void autostart_ok_callback()
{
	char filename[384];

	get_filedialog_selection(browse_autostart_dlg, filename, sizeof(filename));
	mtk_cmdf(appid, "e_autostart.set(-text \"%s\")", filename);
}

static void browse_wallpaper_callback(mtk_event *e, void *arg)
{
	open_filedialog(browse_wallpaper_dlg);
}

static void browse_autostart_callback(mtk_event *e, void *arg)
{
	open_filedialog(browse_autostart_dlg);
}

static void clear_wallpaper_callback(mtk_event *e, void *arg)
{
	mtk_cmd(appid, "e_wallpaper.set(-text \"\")");
}

void init_sysettings()
{
	appid = mtk_init_app("System settings");

	mtk_cmd_seq(appid,
		"g = new Grid()",

		"g_desktop0 = new Grid()",
		"l_desktop = new Label(-text \"Desktop\" -font \"title\")",
		"s_desktop1 = new Separator(-vertical no)",
		"s_desktop2 = new Separator(-vertical no)",
		"g_desktop0.place(s_desktop1, -column 1 -row 1)",
		"g_desktop0.place(l_desktop, -column 2 -row 1)",
		"g_desktop0.place(s_desktop2, -column 3 -row 1)",
		"g_desktop1 = new Grid()",
		"l_resolution = new Label(-text \"Resolution:\")",
		"b_res_640 = new Button(-text \"\e640x480\")",
		"b_res_800 = new Button(-text \"\e800x600\")",
		"b_res_1024 = new Button(-text \"\e1024x768\")",
		"g_desktop1.place(l_resolution, -column 1 -row 1)",
		"g_desktop1.place(b_res_640, -column 2 -row 1)",
		"g_desktop1.place(b_res_800, -column 3 -row 1)",
		"g_desktop1.place(b_res_1024, -column 4 -row 1)",
		"g_desktop2 = new Grid()",
		"l_wallpaper = new Label(-text \"Wallpaper:\")",
		"e_wallpaper = new Entry()",
		"b_wallpaper = new Button(-text \"Browse\")",
		"b_wallpaper_clr = new Button(-text \"Clear\")",
		"g_desktop2.place(l_wallpaper, -column 1 -row 1)",
		"g_desktop2.place(e_wallpaper, -column 2 -row 1)",
		"g_desktop2.place(b_wallpaper, -column 3 -row 1)",
		"g_desktop2.place(b_wallpaper_clr, -column 4 -row 1)",
		"g_desktop2.columnconfig(3, -size 0)",
		"g_desktop2.columnconfig(4, -size 0)",

		"g_localization0 = new Grid()",
		"l_localization = new Label(-text \"Localization\" -font \"title\")",
		"s_localization1 = new Separator(-vertical no)",
		"s_localization2 = new Separator(-vertical no)",
		"g_localization0.place(s_localization1, -column 1 -row 1)",
		"g_localization0.place(l_localization, -column 2 -row 1)",
		"g_localization0.place(s_localization2, -column 3 -row 1)",
		"g_localization = new Grid()",
		"l_language = new Label(-text \"Language:\")",
		"b_lang_english = new Button(-text \"English\")",
		"b_lang_french = new Button(-text \"French\")",
		"b_lang_german = new Button(-text \"German\")",
		"l_layout = new Label(-text \"Keyboard layout:\")",
		"b_kbd_us = new Button(-text \"US\")",
		"b_kbd_french = new Button(-text \"French\")",
		"b_kbd_german = new Button(-text \"German\")",
		"g_localization.place(l_language, -column 1 -row 1)",
		"g_localization.place(b_lang_english, -column 2 -row 1)",
		"g_localization.place(b_lang_french, -column 3 -row 1)",
		"g_localization.place(b_lang_german, -column 4 -row 1)",
		"g_localization.place(l_layout, -column 1 -row 2)",
		"g_localization.place(b_kbd_us, -column 2 -row 2)",
		"g_localization.place(b_kbd_french, -column 3 -row 2)",
		"g_localization.place(b_kbd_german, -column 4 -row 2)",

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
		"g_network1.place(l_dhcp, -column 1 -row 1)",
		"g_network1.place(b_dhcp, -column 2 -row 1)",
		"g_network2 = new Grid()",
		"l_ip = new Label(-text \"IP address:\")",
		"e_ip = new Entry()",
		"l_netmask = new Label(-text \"Netmask:\")",
		"e_netmask = new Entry()",
		"l_gateway = new Label(-text \"Gateway:\")",
		"e_gateway = new Entry()",
		"l_dns = new Label(-text \"DNS:\")",
		"e_dns = new Entry()",
		"g_network2.place(l_ip, -column 1 -row 1)",
		"g_network2.place(e_ip, -column 2 -row 1)",
		"g_network2.place(l_netmask, -column 3 -row 1)",
		"g_network2.place(e_netmask, -column 4 -row 1)",
		"g_network2.place(l_gateway, -column 1 -row 2)",
		"g_network2.place(e_gateway, -column 2 -row 2)",
		"g_network2.place(l_dns, -column 3 -row 2)",
		"g_network2.place(e_dns, -column 4 -row 2)",

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
		"l_password = new Label(-text \"Pass:\")",
		"e_password = new Entry(-blind yes)",
		"g_remote1.place(l_login, -column 1 -row 1)",
		"g_remote1.place(e_login, -column 2 -row 1)",
		"g_remote1.place(l_password, -column 3 -row 1)",
		"g_remote1.place(e_password, -column 4 -row 1)",

		"g_autostart0 = new Grid()",
		"l_autostart = new Label(-text \"Autostart\" -font \"title\")",
		"s_autostart1 = new Separator(-vertical no)",
		"s_autostart2 = new Separator(-vertical no)",
		"g_autostart0.place(s_autostart1, -column 1 -row 1)",
		"g_autostart0.place(l_autostart, -column 2 -row 1)",
		"g_autostart0.place(s_autostart2, -column 3 -row 1)",
		"g_autostart1 = new Grid()",
		"l_asmode = new Label(-text \"Mode:\")",
		"b_asmode_none = new Button(-text \"None\")",
		"b_asmode_simple = new Button(-text \"Simple:\")",
		"b_asmode_file = new Button(-text \"Configured:\")",
		"g_autostart1.place(l_asmode, -column 1 -row 1)",
		"g_autostart1.place(b_asmode_none, -column 2 -row 1)",
		"g_autostart1.place(b_asmode_simple, -column 2 -row 2)",
		"g_autostart1.place(b_asmode_file, -column 2 -row 3)",
		"g_autostart2 = new Grid()",
		"b_asmode_dt = new Button(-text \"Display titles\")",
		"b_asmode_as = new Button(-text \"Auto switch\")",
		"g_autostart2.place(b_asmode_dt, -column 1 -row 1)",
		"g_autostart2.place(b_asmode_as, -column 2 -row 1)",
		"g_autostart1.place(g_autostart2, -column 3 -row 2)",
		"g_autostart3 = new Grid()",
		"e_autostart = new Entry()",
		"b_autostart = new Button(-text \"Browse\")",
		"g_autostart3.place(e_autostart, -column 1 -row 1)",
		"g_autostart3.place(b_autostart, -column 2 -row 1)",
		"g_autostart3.columnconfig(1, -size 220)",
		"g_autostart3.columnconfig(2, -size 0)",
		"g_autostart1.place(g_autostart3, -column 3 -row 3)",

		"g_btn = new Grid()",
		"b_ok = new Button(-text \"OK\")",
		"b_cancel = new Button(-text \"Cancel\")",
		"g_btn.columnconfig(1, -size 280)",
		"g_btn.place(b_ok, -column 2 -row 1)",
		"g_btn.place(b_cancel, -column 3 -row 1)",

		"g.place(g_desktop0, -column 1 -row 1)",
		"g.place(g_desktop1, -column 1 -row 2)",
		"g.place(g_desktop2, -column 1 -row 3)",
		"g.place(g_localization0, -column 1 -row 4)",
		"g.place(g_localization, -column 1 -row 5)",
		"g.place(g_network0, -column 1 -row 6)",
		"g.place(g_network1, -column 1 -row 7)",
		"g.place(g_network2, -column 1 -row 8)",
		"g.place(g_remote0, -column 1 -row 9)",
		"g.place(g_remote1, -column 1 -row 10)",
		"g.place(g_autostart0, -column 1 -row 11)",
		"g.place(g_autostart1, -column 1 -row 12)",
		"g.place(g_btn, -column 1 -row 14)",

		"w = new Window(-content g -title \"System settings\" -y 0)",
		0);

	mtk_bind(appid, "b_res_640", "press", resolution_callback, (void *)SC_RESOLUTION_640_480);
	mtk_bind(appid, "b_res_800", "press", resolution_callback, (void *)SC_RESOLUTION_800_600);
	mtk_bind(appid, "b_res_1024", "press", resolution_callback, (void *)SC_RESOLUTION_1024_768);

	mtk_bind(appid, "b_wallpaper", "commit", browse_wallpaper_callback, NULL);
	mtk_bind(appid, "b_wallpaper_clr", "commit", clear_wallpaper_callback, NULL);

	mtk_bind(appid, "b_lang_english", "press", language_callback, (void *)SC_LANGUAGE_ENGLISH);
	mtk_bind(appid, "b_lang_french", "press", language_callback, (void *)SC_LANGUAGE_FRENCH);
	mtk_bind(appid, "b_lang_german", "press", language_callback, (void *)SC_LANGUAGE_GERMAN);

	mtk_bind(appid, "b_kbd_us", "press", layout_callback, (void *)SC_KEYBOARD_LAYOUT_US);
	mtk_bind(appid, "b_kbd_french", "press", layout_callback, (void *)SC_KEYBOARD_LAYOUT_FRENCH);
	mtk_bind(appid, "b_kbd_german", "press", layout_callback, (void *)SC_KEYBOARD_LAYOUT_GERMAN);

	mtk_bind(appid, "b_dhcp", "press", dhcp_callback, NULL);

	mtk_bind(appid, "b_asmode_none", "press", asmode_callback, (void *)SC_AUTOSTART_NONE);
	mtk_bind(appid, "b_asmode_simple", "press", asmode_callback, (void *)SC_AUTOSTART_SIMPLE);
	mtk_bind(appid, "b_asmode_file", "press", asmode_callback, (void *)SC_AUTOSTART_FILE);
	mtk_bind(appid, "b_asmode_dt", "press", asmode_dt_callback, NULL);
	mtk_bind(appid, "b_asmode_as", "press", asmode_as_callback, NULL);
	mtk_bind(appid, "b_autostart", "commit", browse_autostart_callback, NULL);
	
	mtk_bind(appid, "b_ok", "commit", ok_callback, NULL);
	mtk_bind(appid, "b_cancel", "commit", cancel_callback, NULL);

	mtk_bind(appid, "w", "close", cancel_callback, NULL);

	browse_wallpaper_dlg = create_filedialog("Select wallpaper", 0, "png", wallpaper_ok_callback, NULL, NULL, NULL);
	browse_autostart_dlg = create_filedialog("Select autostart performance", 0, "per", autostart_ok_callback, NULL, NULL, NULL);
}

void sysettings_update_resolution()
{
	int resolution;

	resolution = sysconfig_get_resolution();

	mtk_cmdf(appid, "b_res_640.set(-state %s)", resolution == SC_RESOLUTION_640_480 ? "on" : "off");
	mtk_cmdf(appid, "b_res_800.set(-state %s)", resolution == SC_RESOLUTION_800_600 ? "on" : "off");
	mtk_cmdf(appid, "b_res_1024.set(-state %s)", resolution == SC_RESOLUTION_1024_768 ? "on" : "off");
}

static void update_wallpaper()
{
	char wallpaper[256];

	sysconfig_get_wallpaper(wallpaper);
	mtk_cmdf(appid, "e_wallpaper.set(-text \"%s\")", wallpaper);
}

static void update_language()
{
	int language;
	
	language = sysconfig_get_language();
	
	mtk_cmdf(appid, "b_lang_english.set(-state %s)", language == SC_LANGUAGE_ENGLISH ? "on" : "off");
	mtk_cmdf(appid, "b_lang_french.set(-state %s)", language == SC_LANGUAGE_FRENCH ? "on" : "off");
	mtk_cmdf(appid, "b_lang_german.set(-state %s)", language == SC_LANGUAGE_GERMAN ? "on" : "off");
}

static void update_layout()
{
	int layout;

	layout = sysconfig_get_keyboard_layout();

	mtk_cmdf(appid, "b_kbd_us.set(-state %s)", layout == SC_KEYBOARD_LAYOUT_US ? "on" : "off");
	mtk_cmdf(appid, "b_kbd_french.set(-state %s)", layout == SC_KEYBOARD_LAYOUT_FRENCH ? "on" : "off");
	mtk_cmdf(appid, "b_kbd_german.set(-state %s)", layout == SC_KEYBOARD_LAYOUT_GERMAN ? "on" : "off");
}

static void update_network()
{
	int dhcp_enable;
	unsigned int ip;
	unsigned int netmask;
	unsigned int gateway;
	unsigned int dns1, dns2;
	
	sysconfig_get_ipconfig(&dhcp_enable, &ip, &netmask, &gateway, &dns1, &dns2);
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
	if(gateway == 0)
		mtk_cmd(appid, "e_gateway.set(-text \"\")");
	else
		mtk_cmdf(appid, "e_gateway.set(-text \"%u.%u.%u.%u\")",
			(gateway & 0xff000000) >> 24,
			(gateway & 0x00ff0000) >> 16,
			(gateway & 0x0000ff00) >> 8,
			gateway & 0x000000ff);
	if(dns1 == 0)
		mtk_cmd(appid, "e_dns.set(-text \"\")");
	else if(dns2 == 0)
		mtk_cmdf(appid, "e_dns.set(-text \"%u.%u.%u.%u\")",
			(dns1 & 0xff000000) >> 24,
			(dns1 & 0x00ff0000) >> 16,
			(dns1 & 0x0000ff00) >> 8,
			dns1 & 0x000000ff);
	else
		mtk_cmdf(appid, "e_dns.set(-text \"%u.%u.%u.%u,%u.%u.%u.%u\")",
			(dns1 & 0xff000000) >> 24,
			(dns1 & 0x00ff0000) >> 16,
			(dns1 & 0x0000ff00) >> 8,
			dns1 & 0x000000ff,
			(dns2 & 0xff000000) >> 24,
			(dns2 & 0x00ff0000) >> 16,
			(dns2 & 0x0000ff00) >> 8,
			dns2 & 0x000000ff);
	mtk_cmdf(appid, "e_ip.set(-readonly %s)", dhcp_enable ? "yes" : "no");
	mtk_cmdf(appid, "e_netmask.set(-readonly %s)", dhcp_enable ? "yes" : "no");
	mtk_cmdf(appid, "e_gateway.set(-readonly %s)", dhcp_enable ? "yes" : "no");
	mtk_cmdf(appid, "e_dns.set(-readonly %s)", dhcp_enable ? "yes" : "no");
}

static void update_credentials()
{
	char login[32];
	char password[32];

	sysconfig_get_credentials(login, password);
	mtk_cmdf(appid, "e_login.set(-text \"%s\")", login);
	mtk_cmdf(appid, "e_password.set(-text \"%s\")", password);
}

static void update_asmode()
{
	mtk_cmdf(appid, "b_asmode_none.set(-state %s)", current_asmode == SC_AUTOSTART_NONE ? "on" : "off");
	mtk_cmdf(appid, "b_asmode_simple.set(-state %s)", current_asmode == SC_AUTOSTART_SIMPLE ? "on" : "off");
	mtk_cmdf(appid, "b_asmode_file.set(-state %s)", current_asmode == SC_AUTOSTART_FILE ? "on" : "off");
	mtk_cmdf(appid, "b_asmode_dt.set(-state %s)", current_asmode_dt ? "on" : "off");
	mtk_cmdf(appid, "b_asmode_as.set(-state %s)", current_asmode_as ? "on" : "off");
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
		sysconfig_get_ipconfig(&dhcp_en, NULL, NULL, NULL, NULL, NULL);
		if(dhcp_en)
			update_network();
		next_update = t + UPDATE_PERIOD;
	}
}

static int w_open;

static int previous_resolution;
static int previous_language;
static int previous_layout;
static int previous_dhcp;
static unsigned int previous_ip, previous_netmask, previous_gateway, previous_dns1, previous_dns2;

void open_sysettings_window()
{
	if(w_open) return;
	w_open = 1;

	previous_resolution = sysconfig_get_resolution();
	previous_language = sysconfig_get_language();
	current_asmode = sysconfig_get_autostart_mode();
	sysconfig_get_autostart_mode_simple(&current_asmode_dt, &current_asmode_as);
	
	sysettings_update_resolution();
	update_wallpaper();
	update_language();
	update_layout();
	update_network();
	update_credentials();
	update_asmode();
	update_autostart();

	previous_layout = sysconfig_get_keyboard_layout();
	sysconfig_get_ipconfig(&previous_dhcp, &previous_ip, &previous_netmask,
		&previous_gateway, &previous_dns1, &previous_dns2);
	
	mtk_cmd(appid, "w.open()");

	next_update = rtems_clock_get_ticks_since_boot() + UPDATE_PERIOD;
	input_add_callback(ip_update);
}

static void close_sysettings_window(int save)
{
	mtk_cmd(appid, "w.close()");
	w_open = 0;

	input_delete_callback(ip_update);
	close_filedialog(browse_wallpaper_dlg);
	close_filedialog(browse_autostart_dlg);
	
	if(save) {
		char wallpaper[256];
		char ip_txt[16];
		char netmask_txt[16];
		char gateway_txt[16];
		char dns_txt[33];
		char *c;
		unsigned int dns1, dns2;
		char login[32];
		char password[32];
		char autostart[256];

		mtk_req(appid, wallpaper, sizeof(wallpaper), "e_wallpaper.text");
		mtk_req(appid, ip_txt, sizeof(ip_txt), "e_ip.text");
		mtk_req(appid, netmask_txt, sizeof(netmask_txt), "e_netmask.text");
		mtk_req(appid, gateway_txt, sizeof(gateway_txt), "e_gateway.text");
		mtk_req(appid, dns_txt, sizeof(dns_txt), "e_dns.text");
		mtk_req(appid, login, sizeof(login), "e_login.text");
		mtk_req(appid, password, sizeof(password), "e_password.text");
		mtk_req(appid, autostart, sizeof(autostart), "e_autostart.text");
		
		dns1 = dns2 = 0;
		if(dns_txt[0] != 0) {
			c = strchr(dns_txt, ',');
			if(c != NULL) {
				*c = 0;
				c++;
				dns2 = inet_addr(c);
			}
			dns1 = inet_addr(dns_txt);
		}

		sysconfig_set_wallpaper(wallpaper);
		sysconfig_set_ipconfig(-1, inet_addr(ip_txt), inet_addr(netmask_txt),
			inet_addr(gateway_txt), dns1, dns2);
		sysconfig_set_credentials(login, password);
		sysconfig_set_autostart_mode(current_asmode);
		sysconfig_set_autostart_mode_simple(current_asmode_dt, current_asmode_as);
		sysconfig_set_autostart(autostart);
		
		sysconfig_save();
	} else {
		sysconfig_set_resolution(previous_resolution);
		sysconfig_set_language(previous_language);
		sysconfig_set_keyboard_layout(previous_layout);
		sysconfig_set_ipconfig(previous_dhcp, previous_ip, previous_netmask,
			previous_gateway, previous_dns1, previous_dns2);
	}
}
