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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <mtklib.h>
#include <rtems.h>
#include <bsp/milkymist_ac97.h>

#include "framedescriptor.h"
#include "analyzer.h"
#include "../osc.h"
#include "../config.h"
#include "../input.h"
#include "renderer.h"
#include "stimuli.h"
#include "sampler.h"

struct snd_history {
	float bass_att, mid_att, treb_att;
};

static void init_history(struct snd_history *history)
{
	history->bass_att = 0.0;
	history->mid_att = 0.0;
	history->treb_att = 0.0;
}

static void analyze_snd(struct frame_descriptor *frd, struct snd_history *history)
{
	struct analyzer_state analyzer;
	short *analyzer_buffer = (short *)frd->snd_buf->samples;
	int i;

	analyzer_init(&analyzer);
	for(i=0;i<frd->snd_buf->nsamples;i++)
		analyzer_put_sample(&analyzer, analyzer_buffer[2*i], analyzer_buffer[2*i+1]);

	frd->bass = ((float)analyzer.bass_acc)/1200000.0;
	frd->mid = ((float)analyzer.mid_acc)/400000.0;
	frd->treb = ((float)analyzer.treb_acc)/252000.0;

	history->treb_att = 0.6*history->treb_att + 0.4*frd->treb;
	history->mid_att = 0.6*history->mid_att + 0.4*frd->mid;
	history->bass_att = 0.6*history->bass_att + 0.4*frd->bass;
	frd->treb_att = history->treb_att;
	frd->mid_att = history->mid_att;
	frd->bass_att = history->bass_att;
}

static int idmx_map[IDMX_COUNT];

static void get_dmx_variables(int fd, float *out)
{
	int i;
	unsigned char val;

	for(i=0;i<IDMX_COUNT;i++) {
		lseek(fd, idmx_map[i], SEEK_SET);
		read(fd, &val, 1);
		out[i] = ((float)val)/255.0;
	}
}

static rtems_id returned_q;
static rtems_id sampler_terminated;

static rtems_task sampler_task(rtems_task_argument argument)
{
	struct frame_descriptor *frame_descriptors[FRD_COUNT];
	int i;
	int snd_fd, dmx_fd;
	struct snd_history history;
	frd_callback callback = (frd_callback)argument;
	rtems_event_set dummy;
	float time;
	int frame;

	snd_fd = open("/dev/snd", O_RDWR);
	if(snd_fd == -1) {
		perror("Unable to open audio device");
		goto end;
	}
	dmx_fd = open("/dev/dmx_in", O_RDONLY);
	if(dmx_fd == -1) {
		perror("Unable to open DMX input device");
		goto end0;
	}

	init_history(&history);

	for(i=0;i<FRD_COUNT;i++)
		frame_descriptors[i] = NULL;
	for(i=0;i<FRD_COUNT;i++) {
		frame_descriptors[i] = new_frame_descriptor();
		if(frame_descriptors[i] == NULL) {
			perror("new_frame_descriptor");
			goto end1;
		}
	}

	time = 0;
	frame = 0;

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
		recorded_descriptor->frame = frame++;
		recorded_descriptor->time = time;
		time += 1.0/FPS;
		/* Get DMX/OSC inputs */
		get_dmx_variables(dmx_fd, recorded_descriptor->idmx);
		get_osc_variables(recorded_descriptor->osc);
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

end1:
	close(dmx_fd);
	for(i=0;i<FRD_COUNT;i++)
		free_frame_descriptor(frame_descriptors[i]);

end0:
	close(snd_fd);
end:
	rtems_semaphore_release(sampler_terminated);
	rtems_task_delete(RTEMS_SELF);
}

static void midi_ctrl_event(struct patch *p, mtk_event *e)
{
	int chan, ctrl, value;

	chan = (e->press.code & 0x0f0000) >> 16;
	ctrl = (e->press.code & 0x7f00) >> 8;
	value = e->press.code & 0x7f;

	if(p)
		stim_midi_ctrl(p->stim, chan+1, ctrl, value);
}

/*
 * @@@ convert pitch events to stimuli later. Need to decide whether giving
 * them another stimulus type or using the control #128 hack.
 */

static void midi_pitch_event(mtk_event *e)
{
	int chan, value;

	chan = (e->press.code & 0x0f0000) >> 16;
	value = e->press.code & 0x7f;

	// TODO
}

static void event_callback(mtk_event *e, int count)
{
	struct patch *p;
	int i;

	renderer_lock_patch();
	p = renderer_get_patch(0);
	for(i=0;i<count;i++)
		switch(e[i].type) {
			case EVENT_TYPE_MIDI_CONTROLLER:
				midi_ctrl_event(p, e+i);
				break;
			case EVENT_TYPE_MIDI_PITCH:
				midi_pitch_event(e+i);
				break;
		}
	renderer_unlock_patch();
}

static rtems_id sampler_task_id;

void sampler_start(frd_callback callback)
{
	rtems_status_code sc;
	int i;
	char confname[12];

	for(i=0;i<IDMX_COUNT;i++) {
		sprintf(confname, "idmx%d", i+1);
		idmx_map[i] = config_read_int(confname, i+1)-1;
	}

	sc = rtems_message_queue_create(
		rtems_build_name('S', 'M', 'P', 'L'),
		FRD_COUNT,
		sizeof(void *),
		0,
		&returned_q);
	assert(sc == RTEMS_SUCCESSFUL);

	sc = rtems_semaphore_create(
		rtems_build_name('S', 'M', 'P', 'L'),
		0,
		RTEMS_SIMPLE_BINARY_SEMAPHORE,
		0,
		&sampler_terminated);
	assert(sc == RTEMS_SUCCESSFUL);

	sc = rtems_task_create(rtems_build_name('S', 'M', 'P', 'L'), 10, RTEMS_MINIMUM_STACK_SIZE,
		RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR,
		0, &sampler_task_id);
	assert(sc == RTEMS_SUCCESSFUL);
	sc = rtems_task_start(sampler_task_id, sampler_task, (rtems_task_argument)callback);
	assert(sc == RTEMS_SUCCESSFUL);
	input_add_callback(event_callback);
}

void sampler_return(struct frame_descriptor *frd)
{
	rtems_message_queue_send(returned_q, &frd, sizeof(void *));
}

void sampler_stop(void)
{
	input_delete_callback(event_callback);
	rtems_event_send(sampler_task_id, RTEMS_EVENT_0);

	rtems_semaphore_obtain(sampler_terminated, RTEMS_WAIT, RTEMS_NO_TIMEOUT);

	/* task self-deleted */
	rtems_semaphore_delete(sampler_terminated);
	rtems_message_queue_delete(returned_q);
}
