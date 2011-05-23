/*
 * Flickernoise
 * Copyright (C) 2011 Xiangfu Liu <xiangfu@sharism.cc>
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

/* http://ascii-table.com/keyboards.php */

#include "keymap.h"
#include "sysconfig.h"

#define DEG MTK_KEY_DEG
#define PAR MTK_KEY_PAR
#define ET  MTK_KEY_ET
#define UE  MTK_KEY_UE
#define OE  MTK_KEY_OE
#define AE  MTK_KEY_AE
#define UUE MTK_KEY_UUE
#define UOE MTK_KEY_UOE
#define UAE MTK_KEY_UAE
#define MU  MTK_KEY_MU
#define DI  MTK_KEY_DI
#define S2  MTK_KEY_S2

#define EA  MTK_KEY_EA
#define UEA MTK_KEY_UEA
#define DR  MTK_KEY_DR
#define DL  MTK_KEY_DL
#define PM  MTK_KEY_PM
#define SU  MTK_KEY_SU
#define AG6 MTK_KEY_AG6
#define CT  MTK_KEY_CT
#define PD  MTK_KEY_PD
#define HF  MTK_KEY_HF
#define QT  MTK_KEY_QT
#define PS  MTK_KEY_PS

#define F1  MTK_KEY_F1
#define F2  MTK_KEY_F2
#define F3  MTK_KEY_F3
#define F4  MTK_KEY_F4
#define F5  MTK_KEY_F5
#define F6  MTK_KEY_F6
#define F7  MTK_KEY_F7
#define F8  MTK_KEY_F8
#define F9  MTK_KEY_F9
#define F10 MTK_KEY_F10
#define F11 MTK_KEY_F11
#define F12 MTK_KEY_F12

#define ESC MTK_KEY_ESC
#define TAB MTK_KEY_TAB
#define BSP MTK_KEY_BACKSPACE

#define CLC MTK_KEY_CAPSLOCK
#define NLC MTK_KEY_NUMLOCK

#define SRQ MTK_KEY_SYSRQ
#define SLC MTK_KEY_SCROLLLOCK
#define PAU MTK_KEY_PAUSE

#define UP  MTK_KEY_UP
#define LFT MTK_KEY_LEFT
#define DWN MTK_KEY_DOWN
#define RIT MTK_KEY_RIGHT

#define PUP MTK_KEY_PAGEUP
#define PDW MTK_KEY_PAGEDOWN
#define HME MTK_KEY_HOME
#define END MTK_KEY_END
#define INT MTK_KEY_INSERT
#define DEL MTK_KEY_DELETE

#define NL  MTK_KEY_ENTER

#define NIL MTK_KEY_INVALID

static int keyb_translation_table[3][112] = {
/*        0    1    2    3    4    5    6    7    8    9    a    b    c    d    e    f */
	{
/* 00 */ NIL, NIL, NIL, NIL, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
/* 10 */ 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'z', 'y', '1', '2',
/* 20 */ '3', '4', '5', '6', '7', '8', '9', '0',  NL, ESC, BSP, TAB, ' ',  ET, NIL,  UE,
/* 30 */ '+', NIL, '#',  OE,  AE, '^', ',', '.', '-', CLC,  F1,  F2,  F3,  F4,  F5,  F6,
/* 40 */  F7,  F8,  F9, F10, F11, F12, SRQ, SLC, PAU, INT, HME, PUP, DEL, END, PDW, RIT,
/* 50 */ LFT, DWN,  UP, NLC,  DI, '*', '-', '+',  NL, '1', '2', '3', '4', '5', '6', '7',
/* 60 */ '8', '9', '0', '.', '<', NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL,
	},{			/* German layout 129 */
/* 00 */ NIL, NIL, NIL, NIL, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
/* 10 */ 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '1', '2',
/* 20 */ '3', '4', '5', '6', '7', '8', '9', '0',  NL, ESC, BSP, TAB, ' ', '-', '=', '^',
/* 30 */ NIL, '<', NIL, ';', '`', '#', ',', '.',  EA, CLC,  F1,  F2,  F3,  F4,  F5,  F6,
/* 40 */  F7,  F8,  F9, F10, F11, F12, SRQ, SLC, PAU, INT, HME, PUP, DEL, END, PDW, RIT,
/* 50 */ LFT, DWN,  UP, NLC,  DI, '*', '-', '+',  NL, '1', '2', '3', '4', '5', '6', '7',
/* 60 */ '8', '9', '0', '.',  DR, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL,
	},{			/* French layout 058 */
/* 00 */ NIL, NIL, NIL, NIL, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
/* 10 */ 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '1', '2',
/* 20 */ '3', '4', '5', '6', '7', '8', '9', '0',  NL, ESC, BSP, TAB, ' ', '-', '=', '[',
/* 30 */ ']','\\', NIL, ';','\'', '`', ',', '.', '/', CLC,  F1,  F2,  F3,  F4,  F5,  F6,
/* 40 */  F7,  F8,  F9, F10, F11, F12, SRQ, SLC, PAU, INT, HME, PUP, DEL, END, PDW, RIT,
/* 50 */ LFT, DWN,  UP, NLC, '/', '*', '-', '+',  NL, '1', '2', '3', '4', '5', '6', '7',
/* 60 */ '8', '9', '0', '.', NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL,
	}			/* US layout 103P */
};

static int keyb_shift_table[3][112] = {
/*        0    1    2    3    4    5    6    7    8    9    a    b    c    d    e    f */
	{
/* 00 */ NIL, NIL, NIL, NIL, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
/* 10 */ 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Z', 'Y', '!','\"',
/* 20 */ PAR, '$', '%', '&', '/', '(', ')', '=', NIL, NIL, NIL, NIL, NIL, '?', '`', UUE,
/* 30 */ '*', NIL,'\'', UOE, UAE, DEG, ';', ':', '_', NIL, NIL, NIL, NIL, NIL, NIL, NIL,
/* 40 */ NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL,
/* 50 */ NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL,
/* 60 */ NIL, NIL, NIL, NIL, '>', NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL,
	},{
/* 00 */ NIL, NIL, NIL, NIL, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
/* 10 */ 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '!','\"',
/* 20 */ '/', '$', '%', '?', '&', '*', '(', ')', NIL, NIL, NIL, NIL, NIL, '_', '+', NIL,
/* 30 */ NIL, '>', NIL, ':', NIL, '|','\'', NIL, UEA, NIL, NIL, NIL, NIL, NIL, NIL, NIL,
/* 40 */ NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL,
/* 50 */ NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL,
/* 60 */ NIL, NIL, NIL, NIL,  DL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL,
	},{
/* 00 */ NIL, NIL, NIL, NIL, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
/* 10 */ 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '!', '@',
/* 20 */ '#', '$', '%', '^', '&', '*', '(', ')', NIL, NIL, NIL, NIL, NIL, '_', '+', '{',
/* 30 */ '}', '|', NIL, ':','\"', '~', '<', '>', '?', NIL, NIL, NIL, NIL, NIL, NIL, NIL,
/* 40 */ NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL,
/* 50 */ NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL,
/* 60 */ NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL,
	}
};

static int keyb_altgr_table[2][112] = {
/*        0    1    2    3    4    5    6    7    8    9    a    b    c    d    e    f */
	{
/* 00 */ NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL,
/* 10 */  MU, NIL, NIL, NIL, '@', NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL,  S2,
/* 20 */ NIL, NIL, NIL, NIL, '{', '[', ']', '}', NIL, NIL, NIL, NIL, NIL,'\\', NIL, NIL,
/* 30 */ '~', NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL,
/* 40 */ NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL,
/* 50 */ NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL,
/* 60 */ NIL, NIL, NIL, NIL, '|', NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL,
	},{
/* 00 */ NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL,
/* 10 */  MU, NIL, PAR,  PS, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL,  PM, '@',
/* 20 */  PD,  CT,  SU, AG6, '|',  S2, NIL,  QT, NIL, NIL, NIL, NIL, NIL,  HF, NIL, '[',
/* 30 */ ']', '}', NIL, '~', '{','\\', NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL,
/* 40 */ NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL,
/* 50 */ NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL,
/* 60 */ NIL, NIL, NIL, NIL, DEG, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL,
	}
};

int get_keycode(unsigned char modifiers, int scan_code)
{
	unsigned int curr_layout;
	int k = NIL;

	if(scan_code>112) return NIL;

	curr_layout = sysconfig_get_keyboard_layout();

	if(modifiers & 0x20 || modifiers & 0x02)
		k = keyb_shift_table[curr_layout][scan_code];
	else if((modifiers & 0x40) &&
		(curr_layout == SC_KEYBOARD_LAYOUT_GERMAN || curr_layout == SC_KEYBOARD_LAYOUT_FRENCH))
			k = keyb_altgr_table[curr_layout][scan_code];
	else
		k = keyb_translation_table[curr_layout][scan_code];

	return k;
}

