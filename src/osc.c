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
#include <assert.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <lop/lop_lowlevel.h>

#include "input.h"

#include "osc.h"

static rtems_task osc_task(rtems_task_argument argument)
{
	struct sockaddr_in local;
	struct sockaddr_in remote;
	int r;
	int s;
	socklen_t slen;
	char buf[1024];

	s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(s == -1) {
		printf("Unable to create socket for OSC server\n");
		return;
	}

	memset((char *) &local, 0, sizeof(struct sockaddr_in));
	local.sin_family = AF_INET;
	local.sin_port = htons(7777);
	local.sin_addr.s_addr = htonl(INADDR_ANY);
	r = bind(s, (struct sockaddr *)&local, sizeof(struct sockaddr_in));
	if(r == -1) {
		printf("Unable to bind socket for OSC server\n");
		close(s);
		return;
	}

	while(1) {
		slen = sizeof(struct sockaddr_in);
		r = recvfrom(s, buf, 1024, 0, (struct sockaddr *)&remote, &slen);
		if(r > 0)
			printf("Received packet from %s:%d\n",
			       inet_ntoa(remote.sin_addr), ntohs(remote.sin_port));
	}
}

static rtems_id osc_task_id;

void init_osc()
{
	rtems_status_code sc;

	sc = rtems_task_create(rtems_build_name('O', 'S', 'C', ' '), 15, 32*1024,
		RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR,
		0, &osc_task_id);
	assert(sc == RTEMS_SUCCESSFUL);
	sc = rtems_task_start(osc_task_id, osc_task, 0);
	assert(sc == RTEMS_SUCCESSFUL);
}
