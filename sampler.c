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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <rtems.h>
#include <bsp/milkymist_ac97.h>

#include "framedescriptor.h"
#include "analyzer.h"
#include "sampler.h"

struct snd_history {
	float peak_bass, peak_mid, peak_treb;
	float bass_att, mid_att, treb_att;
};

static void init_history(struct snd_history *history)
{
	history->peak_bass = 240000.0;
	history->peak_mid = 200000.0;
	history->peak_treb = 180000.0;
	history->bass_att = 0.0;
	history->mid_att = 0.0;
	history->treb_att = 0.0;
}

static void analyze_snd(struct frame_descriptor *frd, struct snd_history *history)
{
	struct analyzer_state analyzer;
	short *analyzer_buffer = (short *)frd->snd_buf->samples;
	int i;
	float bass, mid, treb;

	analyzer_init(&analyzer);
	for(i=0;i<frd->snd_buf->nsamples;i++)
		analyzer_put_sample(&analyzer, analyzer_buffer[2*i], analyzer_buffer[2*i+1]);

	bass = (float)analyzer.bass_acc;
	mid = (float)analyzer.mid_acc;
	treb = (float)analyzer.treb_acc;
	if(bass > history->peak_bass)
		history->peak_bass = bass;
	if(mid > history->peak_mid)
		history->peak_mid = mid;
	if(treb > history->peak_treb)
		history->peak_treb = treb;

	frd->bass = bass/history->peak_bass;
	frd->mid = mid/history->peak_mid;
	frd->treb = treb/history->peak_treb;

	if(history->peak_bass > 120000.0)
		history->peak_bass -= 100.0;
	if(history->peak_mid > 100000.0)
		history->peak_mid -= 83.3;
	if(history->peak_treb > 90000.0)
		history->peak_treb -= 75.0;

	history->treb_att = 0.6*history->treb_att + 0.4*frd->treb;
	history->mid_att = 0.6*history->mid_att + 0.4*frd->mid;
	history->bass_att = 0.6*history->bass_att + 0.4*frd->bass;
	frd->treb_att = history->treb_att;
	frd->mid_att = history->mid_att;
	frd->bass_att = history->bass_att;
}

static rtems_id returned_q;
static rtems_id sampler_terminated;

static rtems_task sampler_task(rtems_task_argument argument)
{
	struct frame_descriptor *frame_descriptors[FRD_COUNT];
	int i;
	int snd_fd;
	struct snd_history history;
	frd_callback callback = (frd_callback)argument;
	rtems_event_set dummy;
	float time;

	snd_fd = open("/dev/snd", O_RDWR);
	if(snd_fd == -1) {
		perror("Unable to open audio device");
		goto end;
	}

	init_history(&history);

	for(i=0;i<FRD_COUNT;i++) {
		frame_descriptors[i] = new_frame_descriptor();
		if(frame_descriptors[i] == NULL) {
			perror("new_frame_descriptor");
			close(snd_fd);
			goto end;
		}
	}

	time = 0.0;

	while(rtems_event_receive(RTEMS_EVENT_0, RTEMS_NO_WAIT, RTEMS_NO_TIMEOUT, &dummy) != RTEMS_SUCCESSFUL) {
		struct snd_buffer *recorded_buf;
		struct frame_descriptor *recorded_descriptor;
		bool pending_downstream;

		/*
		 * Recycle any returned frame descriptor.
		 * Block if all frame descriptors have status >= FRD_STATUS_SAMPLED
		 * (i.e. all frame descriptors are pending processing by downstream)
		 */
		pending_downstream = true;
		for(i=0;i<FRD_COUNT;i++) {
			if(frame_descriptors[i]->status < FRD_STATUS_SAMPLED) {
				pending_downstream = false;
				break;
			}
		}
		if(pending_downstream) {
			size_t s;
			struct frame_descriptor *returned_descriptor;

			rtems_message_queue_receive(
				returned_q,
				&returned_descriptor,
				&s,
				RTEMS_WAIT,
				RTEMS_NO_TIMEOUT
			);
			returned_descriptor->status = FRD_STATUS_NEW;
		}
		while(1) {
			size_t s;
			struct frame_descriptor *returned_descriptor;
			rtems_status_code sc;

			sc = rtems_message_queue_receive(
				returned_q,
				&returned_descriptor,
				&s,
				RTEMS_NO_WAIT,
				RTEMS_NO_TIMEOUT
			);
			if(sc != RTEMS_SUCCESSFUL)
				break;
			returned_descriptor->status = FRD_STATUS_NEW;
		}
		/* Refill AC97 driver with record buffers */
		for(i=0;i<FRD_COUNT;i++) {
			if(frame_descriptors[i]->status == FRD_STATUS_NEW) {
				ioctl(snd_fd, SOUND_SND_SUBMIT_RECORD, frame_descriptors[i]->snd_buf);
				frame_descriptors[i]->status = FRD_STATUS_SAMPLING;
			}
		}
		/* Wait for some sound to be recorded */
		ioctl(snd_fd, SOUND_SND_COLLECT_RECORD, &recorded_buf);
		recorded_descriptor = (struct frame_descriptor *)recorded_buf->user;
		/* Analyze */
		analyze_snd(recorded_descriptor, &history);
		recorded_descriptor->time = time;
		time += 1.0/FPS;
		/* TODO: collect DMX and MIDI info */
		recorded_descriptor->idmx1 = 0.0;
		recorded_descriptor->idmx2 = 0.0;
		recorded_descriptor->idmx3 = 0.0;
		recorded_descriptor->idmx4 = 0.0;
		/* Update status and send downstream */
		recorded_descriptor->status = FRD_STATUS_SAMPLED;
		callback(recorded_descriptor);
	}

	/* Drain buffers from the sound driver */
	while(1) {
		struct snd_buffer *recorded_buf;
		struct frame_descriptor *recorded_descriptor;
		bool recording;

		recording = false;
		for(i=0;i<FRD_COUNT;i++) {
			if(frame_descriptors[i]->status == FRD_STATUS_SAMPLING) {
				recording = true;
				break;
			}
		}
		if(!recording)
			break;

		ioctl(snd_fd, SOUND_SND_COLLECT_RECORD, &recorded_buf);
		recorded_descriptor = (struct frame_descriptor *)recorded_buf->user;
		recorded_descriptor->status = FRD_STATUS_NEW;
	}

	/* Wait for all frame descriptors to be returned */
	while(1) {
		bool all_returned;
		size_t s;
		struct frame_descriptor *returned_descriptor;

		all_returned = true;
		for(i=0;i<FRD_COUNT;i++) {
			if(frame_descriptors[i]->status >= FRD_STATUS_SAMPLED) {
				all_returned = false;
				break;
			}
		}
		if(all_returned)
			break;

		rtems_message_queue_receive(
			returned_q,
			&returned_descriptor,
			&s,
			RTEMS_WAIT,
			RTEMS_NO_TIMEOUT
		);
		returned_descriptor->status = FRD_STATUS_NEW;
	}

	for(i=0;i<FRD_COUNT;i++)
		free_frame_descriptor(frame_descriptors[i]);

	close(snd_fd);

end:
	rtems_semaphore_release(sampler_terminated);
	rtems_task_delete(RTEMS_SELF);
}

static rtems_id sampler_task_id;

void sampler_start(frd_callback callback)
{
	assert(rtems_message_queue_create(
		rtems_build_name('S', 'M', 'P', 'L'),
		FRD_COUNT,
		sizeof(void *),
		0,
		&returned_q) == RTEMS_SUCCESSFUL);

	assert(rtems_semaphore_create(
		rtems_build_name('S', 'M', 'P', 'L'),
		0,
		RTEMS_SIMPLE_BINARY_SEMAPHORE,
		0,
		&sampler_terminated) == RTEMS_SUCCESSFUL);

	assert(rtems_task_create(rtems_build_name('S', 'M', 'P', 'L'), 10, RTEMS_MINIMUM_STACK_SIZE,
		RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR,
		0, &sampler_task_id) == RTEMS_SUCCESSFUL);
	assert(rtems_task_start(sampler_task_id, sampler_task, (rtems_task_argument)callback) == RTEMS_SUCCESSFUL);
}

void sampler_return(struct frame_descriptor *frd)
{
	rtems_message_queue_send(returned_q, &frd, sizeof(void *));
}

void sampler_stop()
{
	rtems_event_send(sampler_task_id, RTEMS_EVENT_0);

	rtems_semaphore_obtain(sampler_terminated, RTEMS_WAIT, RTEMS_NO_TIMEOUT);

	/* task self-deleted */
	rtems_semaphore_delete(sampler_terminated);
	rtems_message_queue_delete(returned_q);
}
