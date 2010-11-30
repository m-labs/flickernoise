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

#ifndef __FLASHVALID_H
#define __FLASHVALID_H

enum {
	FLASHVALID_PASSED,
	FLASHVALID_ERROR_IO,
	FLASHVALID_ERROR_FORMAT
};

int flashvalid_bitstream(const char *filename);
int flashvalid_bios(const char *filename);
int flashvalid_application(const char *filename);

#endif /* __FLASHVALID_H */
