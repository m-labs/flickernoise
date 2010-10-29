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
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <rtems.h>
#include <bsp/milkymist_usbinput.h>

#include <mtklib.h>
#include <keycodes.h>

#include "input.h"

/*
	  0,ESC,'1','2','3','4','5','6','7','8','9','0','-', '=', BS,TAB,
	'q','w','e','r','t','z','u','i','o','p', UE,'+', LF,  0,'a','s',
	'd','f','g','h','j','k','l',OE,  AE,'^',  0,'#','y','x','c','v',
	'b','n','m',',','.','-',  0,'*',  0,' ',  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,'7','8','9','-','4','5','6','+','1',
	'2','3','0',',',  0,  0,'<',  0,  0,  0,  0,  0,  0,  0,  0,  0,
	 LF,  0,'/',  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
*/

/* translation table for scancode set 2 to scancode set 1 conversion */
static int keyb_translation_table[256] = {
/*          0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f */
/* 00 */ 0xff, 0xff, 0xff, 0xff, 0x1e, 0x30, 0x2e, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26,
/* 10 */ 0x32, 0x31, 0x18, 0x19, 0x10, 0x13, 0x1f, 0x14, 0x16, 0x2f, 0x11, 0x2d, 0x15, 0x2c, 0x02, 0x03,
/* 20 */ 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x1c, 0x01, 0x0e, 0x0f, 0x39, 0xff, 0x2b, 0x1a,
/* 30 */ 0x1b, 0xff, 0x2b, 0x27, 0x28, 0xff, 0x33, 0x34, 0x35, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
/* 40 */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
/* 50 */ 0xff, 0xff, 0xff, 0xff, 0x62, 0xff, 0x4a, 0xff, 0x60, 0x4f, 0x50, 0x51, 0x4b, 0x4c, 0x4e, 0x47,
/* 60 */ 0x48, 0x49, 0x52, 0x53, 0x56, 0xff, 0xff, 0x49, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
/* 70 */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
/* 80 */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
/* 90 */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
/* a0 */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
/* b0 */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
/* c0 */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
/* d0 */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
/* e0 */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
/* f0 */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};


int input_fd;

#define MOUSE_LEFT       0x01000000
#define MOUSE_RIGHT      0x02000000
#define MOUSE_HOR_MASK   0x00ff0000
#define MOUSE_VER_MASK   0x0000ff00
#define MOUSE_HOR_SHIFT  16
#define MOUSE_VER_SHIFT  8
#define MOUSE_HOR_NEG    0x00800000
#define MOUSE_VER_NEG    0x00008000

static inline void write_event(mtk_event *e, int type, int code, int rel_x, int rel_y)
{
	e->type = type;
	switch(type) {
		case EVENT_TYPE_MOTION:
			e->motion.rel_x = rel_x;
			e->motion.rel_y = rel_y;
			break;
		case EVENT_TYPE_PRESS:
			e->press.code = code;
			break;
		case EVENT_TYPE_RELEASE:
			e->release.code = code;
			break;
	}
}

static unsigned int old_mstate;
static unsigned int new_mstate;

/**
 * Convert mouse event to mtk event
 */
static int handle_mouse_event(mtk_event *e)
{
	/* left mouse button pressed */
	if (!(old_mstate & MOUSE_LEFT) && (new_mstate & MOUSE_LEFT)) {
		write_event(e, EVENT_TYPE_PRESS, MTK_BTN_LEFT, 0, 0);
		old_mstate |= MOUSE_LEFT;
		return 1;
	}

	/* left mouse button released */
	if ((old_mstate & MOUSE_LEFT) && !(new_mstate & MOUSE_LEFT)) {
		write_event(e, EVENT_TYPE_RELEASE, MTK_BTN_LEFT, 0, 0);
		old_mstate &= ~MOUSE_LEFT;
		return 1;
	}

	/* right mouse button pressed */
	if (!(old_mstate & MOUSE_RIGHT) && (new_mstate & MOUSE_RIGHT)) {
		write_event(e, EVENT_TYPE_PRESS, MTK_BTN_RIGHT, 0, 0);
		old_mstate |= MOUSE_RIGHT;
		return 1;
	}

	/* right mouse button released */
	if ((old_mstate & MOUSE_RIGHT) && !(new_mstate & MOUSE_RIGHT)) {
		write_event(e, EVENT_TYPE_RELEASE, MTK_BTN_RIGHT, 0, 0);
		old_mstate &= ~MOUSE_RIGHT;
		return 1;
	}

	/* check for mouse motion */
	if ((new_mstate & MOUSE_HOR_MASK) || (new_mstate & MOUSE_VER_MASK)) {
		write_event(e, EVENT_TYPE_MOTION, 0,
		      (new_mstate & MOUSE_HOR_NEG) ? (((new_mstate & MOUSE_HOR_MASK) >> MOUSE_HOR_SHIFT) | 0xFFFFFF00) : (new_mstate & MOUSE_HOR_MASK) >> MOUSE_HOR_SHIFT,
		      (new_mstate & MOUSE_VER_NEG) ?  (int)((new_mstate & MOUSE_VER_MASK) >> MOUSE_VER_SHIFT | 0xFFFFFF00) : (int)((new_mstate & MOUSE_VER_MASK) >> MOUSE_VER_SHIFT) );
		/* other events (buttons) have already been processed */
		old_mstate = new_mstate;
		return 1;
	}

	/* no events for us, update new_mstate so we read from the USB controller again */
	old_mstate = new_mstate;
	return 0;
}

/**
 * Convert keyboard event to mtk event
 */

int pending_scancode_state;
int pending_scancode;
static int handle_keybd_event(mtk_event *e, unsigned int keystate)
{
	/* translate hardware scancode set 2 to software scancode set 1 */
	pending_scancode = keyb_translation_table[keystate & 0xFF];

	if(keystate & 0x2200) {
		write_event(e, EVENT_TYPE_PRESS, 42, 0, 0);
		pending_scancode_state = 4;
	} else {
		write_event(e, EVENT_TYPE_PRESS, pending_scancode, 0, 0);
		pending_scancode_state = 1;
	}

	//printf("keystate: %04x\n", keystate);
	return 1;
}

static int get_event(mtk_event *e)
{
	unsigned int data;

	if(pending_scancode_state == 4) {
		write_event(e, EVENT_TYPE_PRESS, pending_scancode, 0, 0);
		pending_scancode_state = 3;
		return 1;
	} else if(pending_scancode_state == 3) {
		write_event(e, EVENT_TYPE_RELEASE, pending_scancode, 0, 0);
		pending_scancode_state = 2;
		return 1;
	} else if(pending_scancode_state == 2) {
		write_event(e, EVENT_TYPE_RELEASE, 42, 0, 0);
		pending_scancode_state = 0;
		return 1;
	} else if(pending_scancode_state == 1) {
		write_event(e, EVENT_TYPE_RELEASE, pending_scancode, 0, 0);
		pending_scancode_state = 0;
		return 1;
	} if(new_mstate != old_mstate)
		return handle_mouse_event(e);
	else {
		int r;

		r = read(input_fd, &data, 4);
		if(r <= 0)
			return 0;
		else if(r == 2)
			return handle_keybd_event(e, data >> 16);
		else {
			new_mstate = data;
			return handle_mouse_event(e);
		}
	}
}

void init_input()
{
	input_fd = open("/dev/usbinput", O_RDONLY);
	assert(input_fd != -1);
	ioctl(input_fd, USBINPUT_SETTIMEOUT, 1);
	old_mstate = 0;
	new_mstate = 0;
	pending_scancode_state = 0;
}

void input_eventloop()
{
	mtk_event e[MAX_EVENTS];
	int i;

	while(1) {
		i = 0;
		while(i < MAX_EVENTS) {
			if(!get_event(&e[i])) break;
			i++;
		}
		mtk_input(e, i);
	}
}
