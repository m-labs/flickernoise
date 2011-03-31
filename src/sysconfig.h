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

#ifndef __SYSCONFIG_H
#define __SYSCONFIG_H

#include <stdbool.h>

enum {
	SC_RESOLUTION_640_480,
	SC_RESOLUTION_800_600,
	SC_RESOLUTION_1024_768,
};

enum {
	SC_LANGUAGE_ENGLISH,
	SC_LANGUAGE_FRENCH,
	SC_LANGUAGE_GERMAN
};

enum {
	SC_KEYBOARD_LAYOUT_US,
	SC_KEYBOARD_LAYOUT_FRENCH,
	SC_KEYBOARD_LAYOUT_GERMAN
};

void sysconfig_load();
void sysconfig_save();

int sysconfig_get_resolution();
void sysconfig_get_wallpaper(char *wallpaper);
int sysconfig_get_language();
int sysconfig_get_keyboard_layout();
void sysconfig_get_ipconfig(int *dhcp_enable, unsigned int *ip, unsigned int *netmask);
void sysconfig_get_credentials(char *login, char *password);
void sysconfig_get_autostart(char *autostart);

void sysconfig_set_resolution(int resolution);
void sysconfig_set_wallpaper(char *wallpaper);
void sysconfig_set_mtk_wallpaper();
void sysconfig_set_language(int language);
void sysconfig_set_keyboard_layout(int layout);
void sysconfig_set_ipconfig(int dhcp_enable, unsigned int ip, unsigned int netmask);
void sysconfig_set_credentials(char *login, char *password);
void sysconfig_set_autostart(char *autostart);

bool sysconfig_login_check(const char *user, const char *passphrase);

#endif /* __SYSCONFIG_H */
