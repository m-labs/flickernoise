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
#define SYSCONFIG_VERSION 0

struct sysconfig {
	unsigned int magic;
	unsigned char version;
	
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
	.dhcp_enable = 1
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

void sysconfig_load()
{
	struct sysconfig conf;

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

void sysconfig_set_keyboard_layout(int layout)
{
	sysconfig.keyboard_layout = layout;
	/* TODO: notify MTK about the changed layout */
}

void sysconfig_set_ipconfig(int dhcp_enable, unsigned int ip, unsigned int netmask)
{
	if(dhcp_enable == -1)
		dhcp_enable = sysconfig.dhcp_enable;

	if(!dhcp_enable) {
		if((ip != 0) && (sysconfig.ip != ip))
			ifconfig_set_ip(SIOCSIFADDR, ip);
		if((netmask != 0) && (sysconfig.netmask != netmask))
			ifconfig_set_ip(SIOCSIFNETMASK, netmask);
		if(sysconfig.dhcp_enable)
			rtems_bsdnet_dhcp_down();
	} else if(!sysconfig.dhcp_enable) {
		ifconfig_set_ip(SIOCSIFADDR, 0);
		ifconfig_set_ip(SIOCSIFNETMASK, 0);
		rtems_bsdnet_do_dhcp_timeout(); // TODO: spawn task
	}

	sysconfig.dhcp_enable = dhcp_enable;
	if(ip != 0)
		sysconfig.ip = ip;
	if(netmask != 0)
		sysconfig.netmask = netmask;
}

void sysconfig_set_credentials(char *login, char *password)
{
	strcpy(sysconfig.login, login);
	strcpy(sysconfig.password, password);
}

void sysconfig_set_autostart(char *autostart)
{
	strcpy(sysconfig.autostart, autostart);
}

/* login callbacks */
