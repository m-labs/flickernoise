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

#ifndef __INPUT_H
#define __INPUT_H

#include <mtklib.h>

#define EVENT_TYPE_IR              (EVENT_TYPE_USER_BASE)
#define EVENT_TYPE_MIDI_NOTEON     (EVENT_TYPE_USER_BASE+1)
#define EVENT_TYPE_MIDI_NOTEOFF    (EVENT_TYPE_USER_BASE+2)
#define EVENT_TYPE_MIDI_CONTROLLER (EVENT_TYPE_USER_BASE+3)
#define EVENT_TYPE_MIDI_PITCH      (EVENT_TYPE_USER_BASE+4)
#define EVENT_TYPE_OSC             (EVENT_TYPE_USER_BASE+5)

typedef void (*input_callback)(mtk_event *e, int count);

void init_input();
void input_add_callback(input_callback cb);
void input_delete_callback(input_callback cb);
void input_eventloop();

void input_inject_midi(const unsigned char *msg);
void input_inject_osc(unsigned char msg);

#endif
