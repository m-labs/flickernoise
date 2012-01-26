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

#include <rtems.h>
#include <rtems/rtems_bsdnet.h>
#include <rtems/dhcp.h>
#include <rtems/bspcmdline.h>
#include <rtems/bsdnet/servers.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/route.h>
#include <bsp.h>
#include <mtklib.h>
#include <mtki18n.h>

#include "version.h"
#include "pixbuf/pixbuf.h"
#include "fb.h"
#include "translations/languages.h"
#include "sysconfig.h"

static struct rtems_bsdnet_ifconfig netdriver_config = {
	RTEMS_BSP_NETWORK_DRIVER_NAME,
	RTEMS_BSP_NETWORK_DRIVER_ATTACH,
	NULL,
	NULL, /* ip_address */
	NULL, /* ip_netmask */
	NULL,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	NULL
};

struct rtems_bsdnet_config rtems_bsdnet_config = {
	&netdriver_config,
	NULL, /* bootp */
	5,
	0,
	0,
	"milkymist", /* hostname */
	"local", /* domainname */
	NULL, /* gateway */
	NULL, /* log_host */
	{ NULL, NULL, NULL }, /* name_server[3] */
	{ NULL }, /* ntp_server[3] */
	0,
	0,
	0,
	0,
	0
};

#define SYSCONFIG_MAGIC 0xda81d4cb
#define SYSCONFIG_VERSION 3

struct sysconfig {
	unsigned int magic;
	unsigned char version;
	
	unsigned char resolution;
	char wallpaper[256];
	
	unsigned char language;
	unsigned char keyboard_layout;
	
	unsigned char dhcp_enable;
	unsigned int ip;
	unsigned int netmask;
	unsigned int gateway;
	unsigned int dns1, dns2;
	
	char login[32];
	char password[32];

	int autostart_mode;
	int autostart_dt;
	int autostart_as;
	char autostart[256];
};

static int readconfig(const char *filename, struct sysconfig *out)
{
	FILE *fd;

	fd = fopen(filename, "r");
	if(fd == NULL)
		return 0;
	if(fread(out, sizeof(struct sysconfig), 1, fd) != 1) {
		fclose(fd);
		return 0;
	}
	fclose(fd);

	if(out->magic != SYSCONFIG_MAGIC)
		return 0;
	if(out->version != SYSCONFIG_VERSION)
		return 0;

	/* make sure strings are terminated */
	out->login[31] = 0;
	out->password[31] = 0;
	out->autostart[255] = 0;
	
	return 1;
}

/* initialize with default config */
static struct sysconfig sysconfig = {
	.magic = SYSCONFIG_MAGIC,
	.version = SYSCONFIG_VERSION,
	.resolution = SC_RESOLUTION_640_480,
	.language = SC_LANGUAGE_ENGLISH,
	.keyboard_layout = SC_KEYBOARD_LAYOUT_US,
	.dhcp_enable = 1,
	.ip = 0xc0a8002a,	/* 192.168.0.42 */
	.netmask = 0xffffff00,	/* 255.255.255.0 */
	.gateway = 0xc0a80001,	/* 192.168.0.1 */
	.dns1 = 0xd043dede,	/* 208.67.222.222 */
	.dns2 = 0xd043dedc,	/* 208.67.222.220 */
	.autostart_as = 1
};

#define FMT_IP_LEN (4*3+3*1+1)

static char ip_fmt[FMT_IP_LEN];
static char netmask_fmt[FMT_IP_LEN];
static char gateway_fmt[FMT_IP_LEN];
static char dns1_fmt[FMT_IP_LEN];
static char dns2_fmt[FMT_IP_LEN];

static void format_ip(unsigned int ip, char *out)
{
	sprintf(out, "%d.%d.%d.%d",
		(ip & 0xff000000) >> 24,
		(ip & 0x00ff0000) >> 16,
		(ip & 0x0000ff00) >> 8,
		ip & 0x000000ff);
}

#define SYSCONFIG_FILE "/ssd/sysconfig.bin"

static void sysconfig_credentials_lock_init(void);
static void sysconfig_credentials_lock(void);
static void sysconfig_credentials_unlock(void);

int sysconfig_is_rescue(void)
{
	const char *bsp_cmdline;
	
	bsp_cmdline = rtems_bsp_cmdline_get();
	if(bsp_cmdline == NULL)
		return 0;
	return strcmp(bsp_cmdline, "rescue") == 0;
}

static void start_dhcp_task(void);

void sysconfig_load(void)
{
	struct sysconfig conf;

	sysconfig_credentials_lock_init();
	
	if(!sysconfig_is_rescue()) {
		sysconfig.autostart_mode = SC_AUTOSTART_SIMPLE;
		if(readconfig(SYSCONFIG_FILE, &conf))
			memcpy(&sysconfig, &conf, sizeof(struct sysconfig));
	}

	if(sysconfig.dhcp_enable)
		rtems_bsdnet_config.bootp = start_dhcp_task;
	else {
		if(sysconfig.ip) {
			format_ip(sysconfig.ip, ip_fmt);
			netdriver_config.ip_address = ip_fmt;
		}
		if(sysconfig.netmask) {
			format_ip(sysconfig.netmask, netmask_fmt);
			netdriver_config.ip_netmask = netmask_fmt;
		}
		if(sysconfig.gateway) {
			format_ip(sysconfig.gateway, gateway_fmt);
			rtems_bsdnet_config.gateway = gateway_fmt;
		}
		if(sysconfig.dns1) {
			format_ip(sysconfig.dns1, dns1_fmt);
			rtems_bsdnet_config.name_server[0] = dns1_fmt;
		}
		if(sysconfig.dns2) {
			format_ip(sysconfig.dns2, dns2_fmt);
			rtems_bsdnet_config.name_server[1] = dns2_fmt;
		}
	}
}

void sysconfig_save(void)
{
	FILE *fd;

	fd = fopen(SYSCONFIG_FILE, "w");
	if(fd == NULL) return;
	fwrite(&sysconfig, sizeof(struct sysconfig), 1, fd);
	fclose(fd);
}

#define NETWORK_INTERFACE "minimac0"

static unsigned int ifconfig_get_ip(uint32_t cmd)
{
	struct sockaddr_in ipaddr;

	memset(&ipaddr, 0, sizeof(ipaddr));
	ipaddr.sin_len = sizeof(ipaddr);
	ipaddr.sin_family = AF_INET;
	rtems_bsdnet_ifconfig(NETWORK_INTERFACE, cmd, &ipaddr);
	return ipaddr.sin_addr.s_addr;
}

static void ifconfig_set_ip(uint32_t cmd, unsigned int ip)
{
	struct sockaddr_in ipaddr;

	memset(&ipaddr, 0, sizeof(ipaddr));
	ipaddr.sin_len = sizeof(ipaddr);
	ipaddr.sin_family = AF_INET;
	ipaddr.sin_addr.s_addr = ip;
	rtems_bsdnet_ifconfig(NETWORK_INTERFACE, cmd, &ipaddr);
}

/* HACK: we are not meant to use those from the application,
 * but RTEMS provides no other way to read the routing table.
 * (In its defense, the BSD API is about as crappy)
 */
extern struct radix_node_head *rt_tables[AF_MAX+1];
void rtems_bsdnet_semaphore_obtain(void);
void rtems_bsdnet_semaphore_release(void);

static int retrieve_gateway(struct radix_node *rn, void *vw)
{
	unsigned int *r = (unsigned int *)vw;
	struct rtentry *rt = (struct rtentry *)rn;
	struct sockaddr_in *dst;
	struct sockaddr_in *gateway;
	
	dst = (struct sockaddr_in *)rt_key(rt);
	gateway = (struct sockaddr_in *)rt->rt_gateway;
	if((dst != NULL) && (dst->sin_addr.s_addr == INADDR_ANY)
	  && (rt->rt_flags & RTF_GATEWAY) && (gateway != NULL))
		*r = gateway->sin_addr.s_addr;
	
	return 0;
}

static unsigned int route_get_gateway(void)
{
	struct radix_node_head *rnh;
	int error;
	unsigned int r;
	
	rnh = rt_tables[AF_INET];
	if(!rnh)
		return 0;
	r = 0;
	rtems_bsdnet_semaphore_obtain();
	error = rnh->rnh_walktree(rnh, retrieve_gateway, &r);
	rtems_bsdnet_semaphore_release();
	if(error)
		return 0;
	return r;
}

static void route_set_gateway(unsigned int ip)
{
	struct sockaddr_in oldgw;
	struct sockaddr_in olddst;
	struct sockaddr_in oldmask;
	struct sockaddr_in gw;

	memset(&olddst, 0, sizeof(olddst));
	olddst.sin_len = sizeof(olddst);
	olddst.sin_family = AF_INET;
	olddst.sin_addr.s_addr = INADDR_ANY;

	memset(&oldgw, 0, sizeof(oldgw));
	oldgw.sin_len = sizeof(oldgw);
	oldgw.sin_family = AF_INET;
	oldgw.sin_addr.s_addr = INADDR_ANY;

	memset(&oldmask, 0, sizeof(oldmask));
	oldmask.sin_len = sizeof(oldmask);
	oldmask.sin_family = AF_INET;
	oldmask.sin_addr.s_addr = INADDR_ANY;
	
	memset(&gw, 0, sizeof(gw));
	gw.sin_len = sizeof(gw);
	gw.sin_family = AF_INET;
	gw.sin_addr.s_addr = ip;

	rtems_bsdnet_rtrequest(RTM_DELETE,
		(struct sockaddr *)&olddst,
		(struct sockaddr *)&oldgw,
		(struct sockaddr *)&oldmask, 
		(RTF_UP | RTF_STATIC), NULL);

	rtems_bsdnet_rtrequest(RTM_ADD,
		(struct sockaddr *)&olddst,
		(struct sockaddr *)&gw,
		(struct sockaddr *)&oldmask,
		(RTF_UP | RTF_GATEWAY | RTF_STATIC), NULL);
}

/* get */

int sysconfig_get_resolution(void)
{
	return sysconfig.resolution;
}

void sysconfig_get_wallpaper(char *wallpaper)
{
	strcpy(wallpaper, sysconfig.wallpaper);
}

int sysconfig_get_language(void)
{
	return sysconfig.language;
}

int sysconfig_get_keyboard_layout(void)
{
	return sysconfig.keyboard_layout;
}

void sysconfig_get_ipconfig(int *dhcp_enable, unsigned int *ip, unsigned int *netmask, unsigned int *gateway, unsigned int *dns1, unsigned int *dns2)
{
	*dhcp_enable = sysconfig.dhcp_enable;

	if(ip != NULL)
		*ip = ifconfig_get_ip(SIOCGIFADDR);
	if(netmask != NULL)
		*netmask = ifconfig_get_ip(SIOCGIFNETMASK);
	if(gateway != NULL)
		*gateway = route_get_gateway();
	if(dns1 != NULL)
		*dns1 = rtems_bsdnet_nameserver_count > 0 ? rtems_bsdnet_nameserver[0].s_addr : 0;
	if(dns2 != NULL)
		*dns2 = rtems_bsdnet_nameserver_count > 1 ? rtems_bsdnet_nameserver[1].s_addr : 0;
}

void sysconfig_get_credentials(char *login, char *password)
{
	strcpy(login, sysconfig.login);
	strcpy(password, sysconfig.password);
}

int sysconfig_get_autostart_mode(void)
{
	return sysconfig.autostart_mode;
}

void sysconfig_get_autostart_mode_simple(int *dt, int *as)
{
	if(dt != NULL)
		*dt = sysconfig.autostart_dt;
	if(as != NULL)
		*as = sysconfig.autostart_as;
}

void sysconfig_get_autostart(char *autostart)
{
	strcpy(autostart, sysconfig.autostart);
}

/* set */

void sysconfig_set_resolution(int resolution)
{
	int update;
	update = sysconfig.resolution != resolution;
	sysconfig.resolution = resolution;
	if(update)
		fb_resize_gui();
}

void sysconfig_set_mtk_wallpaper(void)
{
	struct pixbuf *p;
	
	if(strcmp(sysconfig.wallpaper, "") != 0) {
		p = pixbuf_get(sysconfig.wallpaper);
		if(p == NULL) {
			mtk_config_set_wallpaper(NULL, 0, 0);
			return;
		}
		mtk_config_set_wallpaper(p->pixels, p->width, p->height);
		pixbuf_dec_ref(p);
	} else
		mtk_config_set_wallpaper(NULL, 0, 0);
}

void sysconfig_set_wallpaper(char *wallpaper)
{
	int update;
	
	update = strcmp(sysconfig.wallpaper, wallpaper) != 0;
	strcpy(sysconfig.wallpaper, wallpaper);
	if(update) {
		sysconfig_set_mtk_wallpaper();
		mtk_cmd(1, "screen.refresh()");
	}
}

void sysconfig_set_language(int language)
{
	sysconfig.language = language;
	sysconfig_set_mtk_language();
}

void sysconfig_set_mtk_language(void)
{
	switch(sysconfig.language) {
		case SC_LANGUAGE_FRENCH:
			mtk_set_language(translation_french);
			break;
		case SC_LANGUAGE_GERMAN:
			mtk_set_language(translation_german);
			break;
		default:
			mtk_set_language(NULL);
			break;
	}
}

void sysconfig_set_keyboard_layout(int layout)
{
	sysconfig.keyboard_layout = layout;
}

static int dhcp_task_running;

static rtems_task dhcp_task(rtems_task_argument argument)
{
	int r;
	
	while(sysconfig.dhcp_enable) {
		r = rtems_bsdnet_do_dhcp_timeout();
		if(r >= 0)
			break; /* success */
	}
	dhcp_task_running = 0;
	rtems_task_delete(RTEMS_SELF);
}

static void start_dhcp_task(void)
{
	rtems_id task_id;
	rtems_status_code sc;
	
	sc = rtems_task_create(rtems_build_name('A', 'D', 'H', 'C'), 150, 64*1024,
		RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR,
		0, &task_id);
	if(sc != RTEMS_SUCCESSFUL) return;
	sc = rtems_task_start(task_id, dhcp_task, 0);
	if(sc != RTEMS_SUCCESSFUL) {
		rtems_task_delete(task_id);
		return;
	}
	dhcp_task_running = 1;
}

void sysconfig_set_ipconfig(int dhcp_enable, unsigned int ip, unsigned int netmask, unsigned int gateway, unsigned int dns1, unsigned int dns2)
{
	if(dhcp_enable == -1)
		dhcp_enable = sysconfig.dhcp_enable;

	if(!dhcp_task_running) {
		if(!dhcp_enable) {
			if(sysconfig.dhcp_enable)
				rtems_bsdnet_dhcp_down();
			if(ip != 0)
				ifconfig_set_ip(SIOCSIFADDR, ip);
			if(netmask != 0)
				ifconfig_set_ip(SIOCSIFNETMASK, netmask);
			if(gateway != 0)
				route_set_gateway(gateway);
			if(dns1 != 0) {
				rtems_bsdnet_nameserver_count = 1;
				rtems_bsdnet_nameserver[0].s_addr = dns1;
				if(dns2 != 0) {
					rtems_bsdnet_nameserver_count = 2;
					rtems_bsdnet_nameserver[1].s_addr = dns2;
				}
			}
			/* TODO: run res_init() ? */
		} else if(!sysconfig.dhcp_enable) {
			ifconfig_set_ip(SIOCSIFADDR, 0);
			ifconfig_set_ip(SIOCSIFNETMASK, 0);
			start_dhcp_task();
		}
	}

	sysconfig.dhcp_enable = dhcp_enable;
	if(ip != 0)
		sysconfig.ip = ip;
	if(netmask != 0)
		sysconfig.netmask = netmask;
	if(gateway != 0)
		sysconfig.gateway = gateway;
	if(dns1 != 0)
		sysconfig.dns1 = dns1;
	if(dns2 != 0)
		sysconfig.dns2 = dns2;
}

void sysconfig_set_credentials(char *login, char *password)
{
	sysconfig_credentials_lock();
	strcpy(sysconfig.login, login);
	strcpy(sysconfig.password, password);
	sysconfig_credentials_unlock();
}

void sysconfig_set_autostart_mode(int autostart_mode)
{
	sysconfig.autostart_mode = autostart_mode;
}

void sysconfig_set_autostart_mode_simple(int dt, int as)
{
	sysconfig.autostart_dt = dt;
	sysconfig.autostart_as = as;
}

void sysconfig_set_autostart(char *autostart)
{
	strcpy(sysconfig.autostart, autostart);
}

/* login callbacks */

bool sysconfig_login_check(const char *user, const char *passphrase)
{
	bool r;

	if(user == NULL) user = "";
	if(passphrase == NULL) passphrase = "";

	sysconfig_credentials_lock();
	r = ((sysconfig.login[0] != 0) || (sysconfig.password[0] != 0))
	&& (strcmp(user, sysconfig.login) == 0) && (strcmp(passphrase, sysconfig.password) == 0);
	sysconfig_credentials_unlock();

	return r;
}

static rtems_id credentials_lock;

static void sysconfig_credentials_lock_init(void)
{
	rtems_semaphore_create(
		rtems_build_name('C', 'R', 'E', 'D'),
		1,
		RTEMS_SIMPLE_BINARY_SEMAPHORE,
		0,
		&credentials_lock);
}

static void sysconfig_credentials_lock(void)
{
	rtems_semaphore_obtain(credentials_lock, RTEMS_WAIT, RTEMS_NO_TIMEOUT);
}

static void sysconfig_credentials_unlock(void)
{
	rtems_semaphore_release(credentials_lock);
}
