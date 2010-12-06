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

#ifndef __SYSCONFIG_H
#define __SYSCONFIG_H

enum {
	SC_KEYBOARD_LAYOUT_GERMAN,
	SC_KEYBOARD_LAYOUT_FRENCH,
	SC_KEYBOARD_LAYOUT_US
};

void sysconfig_load();
void sysconfig_save();

int sysconfig_get_keyboard_layout();
void sysconfig_get_ipconfig(int *dhcp_enable, unsigned int *ip, unsigned int *netmask);
void sysconfig_get_credentials(char *login, char *password);
void sysconfig_get_autostart(char *autostart);

void sysconfig_set_keyboard_layout(int layout);
void sysconfig_set_ipconfig(int dhcp_enable, unsigned int ip, unsigned int netmask);
void sysconfig_set_credentials(char *login, char *password);
void sysconfig_set_autostart(char *autostart);

bool sysconfig_login_check(const char *user, const char *passphrase);

#endif /* __SYSCONFIG_H */
