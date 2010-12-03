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
	NULL, /* hostname */
	NULL, /* domainname */
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
	unsigned int dns1, dns2;
	char hostname[32];
	char domain[32];
	
	char login[32];
	char password[32];

	char autostart[256];
};

static int readconfig(const char *filename, struct sysconfig *out)
{
	FILE *fd;

	fd = fopen(filename, "rb");
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
	out->hostname[31] = 0;
	out->domain[31] = 0;
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

void sysconfig_load()
{
	struct sysconfig conf;

	if(readconfig("/flash/sysconfig", &conf))
		memcpy(&sysconfig, &conf, sizeof(struct sysconfig));

	if(sysconfig.dhcp_enable)
		rtems_bsdnet_config.bootp = rtems_bsdnet_do_dhcp;
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
	if(sysconfig.dns1) {
		format_ip(sysconfig.dns1, dns1_fmt);
		rtems_bsdnet_config.name_server[0] = dns1_fmt;
	} else
		rtems_bsdnet_config.name_server[0] = NULL;
	if(sysconfig.dns2) {
		format_ip(sysconfig.dns2, dns2_fmt);
		rtems_bsdnet_config.name_server[1] = dns2_fmt;
	} else
		rtems_bsdnet_config.name_server[1] = NULL;
	if(sysconfig.hostname[0])
		rtems_bsdnet_config.hostname = sysconfig.hostname;
	else
		rtems_bsdnet_config.hostname = NULL;
	if(sysconfig.domain[0])
		rtems_bsdnet_config.domainname = sysconfig.domain;
	else
		rtems_bsdnet_config.domainname = NULL;
	
}

void sysconfig_save()
{
}
