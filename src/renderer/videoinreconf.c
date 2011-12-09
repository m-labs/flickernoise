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

#include <assert.h>
#include <rtems.h>
#include <sys/ioctl.h>

#include "videoinreconf.h"

struct reconf_request {
	int cmd;
	int value;
};

static rtems_id request_q;

void init_videoinreconf(void)
{
	rtems_status_code sc;
	
	sc = rtems_message_queue_create(
		rtems_build_name('V', 'I', 'N', 'R'),
		2,
		sizeof(struct reconf_request),
		0,
		&request_q);
	assert(sc == RTEMS_SUCCESSFUL);
}

void videoinreconf_request(int cmd, int value)
{
	struct reconf_request req;
	
	req.cmd = cmd;
	req.value = value;
	rtems_message_queue_send(request_q, &req, sizeof(struct reconf_request));
}

void videoinreconf_do(int fd)
{
	rtems_status_code sc;
	struct reconf_request req;
	size_t s;
	
	sc = rtems_message_queue_receive(
		request_q,
		&req,
		&s,
		RTEMS_NO_WAIT,
		RTEMS_NO_TIMEOUT
	);
	if(sc == RTEMS_SUCCESSFUL)
		ioctl(fd, req.cmd, req.value);
}
