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

#include <rtems.h>
#include <mtklib.h>
#include <keycodes.h>

#include "input.h"

#include "reboot.h"

static int ctrl, alt;

static void callback(mtk_event *e, int count)
{
	int i;

	for(i=0;i<count;i++) {
		if(e[i].type == EVENT_TYPE_PRESS) {
			if(e[i].press.code == MTK_KEY_LEFTCTRL)
				ctrl = 1;
			else if(e[i].press.code == MTK_KEY_LEFTALT)
				alt = 1;
			else if(ctrl && alt && (e[i].press.code == MTK_KEY_DELETE))
				rtems_shutdown_executive(0);
		} else if (e[i].type == EVENT_TYPE_RELEASE) {
			if(e[i].release.code == MTK_KEY_LEFTCTRL)
				ctrl = 0;
			else if(e[i].release.code == MTK_KEY_LEFTALT)
				alt = 0;
		}
	}
}

void init_reboot()
{
	input_add_callback(callback);
}
