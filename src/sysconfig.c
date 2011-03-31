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

#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <rtems.h>
#include <rtems/rtems_bsdnet.h>
#include <rtems/dhcp.h>
#include <bsp.h>

#include "version.h"
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
	{ NULL }, /* name_server[3] */
	{ NULL }, /* ntp_server[3] */
	0,
	0,
	0,
	0,
	0
};

#define SYSCONFIG_MAGIC 0xda81d4cb
#define SYSCONFIG_VERSION 1

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
	
	char login[32];
	char password[32];

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
	.resolution = SC_RESOLUTION_1024_768,
	.wallpaper = "/flash/comet.png",
	.language = SC_LANGUAGE_ENGLISH,
	.keyboard_layout = SC_KEYBOARD_LAYOUT_GERMAN,
	.dhcp_enable = 0,
	.ip = 0xc0a8002a,
	.netmask = 0xffffff00
};

#define FMT_IP_LEN (4*3+3*1+1)

static char ip_fmt[FMT_IP_LEN];
static char netmask_fmt[FMT_IP_LEN];

static void format_ip(unsigned int ip, char *out)
{
	sprintf(out, "%d.%d.%d.%d",
		(ip & 0xff000000) >> 24,
		(ip & 0x00ff0000) >> 16,
		(ip & 0x0000ff00) >> 8,
		ip & 0x000000ff);
}

#define SYSCONFIG_FILE "/flash/sysconfig.bin"

static void my_dhcp()
{
	rtems_bsdnet_do_dhcp_timeout();
}

static void sysconfig_credentials_lock_init();
static void sysconfig_credentials_lock();
static void sysconfig_credentials_unlock();

void sysconfig_load()
{
	struct sysconfig conf;

	sysconfig_credentials_lock_init();
	
	if(readconfig(SYSCONFIG_FILE, &conf))
		memcpy(&sysconfig, &conf, sizeof(struct sysconfig));

	if(sysconfig.dhcp_enable)
		rtems_bsdnet_config.bootp = my_dhcp;
	else
		rtems_bsdnet_config.bootp = NULL;
	if(sysconfig.ip) {
		format_ip(sysconfig.ip, ip_fmt);
		netdriver_config.ip_address = ip_fmt;
	} else
		netdriver_config.ip_address = NULL;
	if(sysconfig.netmask) {
		format_ip(sysconfig.netmask, netmask_fmt);
		netdriver_config.ip_netmask = netmask_fmt;
	} else
		netdriver_config.ip_netmask = NULL;
}

void sysconfig_save()
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

/* get */

int sysconfig_get_resolution()
{
	return sysconfig.resolution;
}

void sysconfig_get_wallpaper(char *wallpaper)
{
	strcpy(wallpaper, sysconfig.wallpaper);
}

int sysconfig_get_language()
{
	return sysconfig.language;
}

int sysconfig_get_keyboard_layout()
{
	return sysconfig.keyboard_layout;
}

void sysconfig_get_ipconfig(int *dhcp_enable, unsigned int *ip, unsigned int *netmask)
{
	*dhcp_enable = sysconfig.dhcp_enable;

	if(ip != NULL)
		*ip = ifconfig_get_ip(SIOCGIFADDR);
	if(netmask != NULL)
		*netmask = ifconfig_get_ip(SIOCGIFNETMASK);
}

void sysconfig_get_credentials(char *login, char *password)
{
	strcpy(login, sysconfig.login);
	strcpy(password, sysconfig.password);
}

void sysconfig_get_autostart(char *autostart)
{
	strcpy(autostart, sysconfig.autostart);
}

/* set */

void sysconfig_set_resolution(int resolution)
{
	sysconfig.resolution = resolution;
	/* TODO: switch video mode and redraw screen */
}

void sysconfig_set_wallpaper(char *wallpaper)
{
	strcpy(sysconfig.wallpaper, wallpaper);
	/* TODO: display new wallpaper */
}

void sysconfig_set_language(int language)
{
	sysconfig.language = language;
}

void sysconfig_set_keyboard_layout(int layout)
{
	sysconfig.keyboard_layout = layout;
	/* TODO: notify MTK about the changed layout */
}

static int dhcp_task_running;

static rtems_task dhcp_task(rtems_task_argument argument)
{
	rtems_bsdnet_do_dhcp_timeout();
	dhcp_task_running = 0;
	rtems_task_delete(RTEMS_SELF);
}

static void start_dhcp_task()
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

void sysconfig_set_ipconfig(int dhcp_enable, unsigned int ip, unsigned int netmask)
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
}

void sysconfig_set_credentials(char *login, char *password)
{
	sysconfig_credentials_lock();
	strcpy(sysconfig.login, login);
	strcpy(sysconfig.password, password);
	sysconfig_credentials_unlock();
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

static void sysconfig_credentials_lock_init()
{
	rtems_semaphore_create(
		rtems_build_name('C', 'R', 'E', 'D'),
		1,
		RTEMS_SIMPLE_BINARY_SEMAPHORE,
		0,
		&credentials_lock);
}

static void sysconfig_credentials_lock()
{
	rtems_semaphore_obtain(credentials_lock, RTEMS_WAIT, RTEMS_NO_TIMEOUT);
}

static void sysconfig_credentials_unlock()
{
	rtems_semaphore_release(credentials_lock);
}
