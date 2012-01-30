/*
 * stimuli.c - Unified control input handling
 *
 * Copyright 2012 by Werner Almesberger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "stimuli.h"


static void midi_add(struct s_midi_ctrl *ct, int value)
{
	float f;

	f = (float) value/127.0;
	if(ct->regs.pfv)
		*ct->regs.pfv += f;
	if(ct->regs.pvv)
		*ct->regs.pvv += f;
}

void midi_proc_linear(struct s_midi_ctrl *ct, int value)
{
	float f;

	f = (float) value/127.0;
	if(ct->regs.pfv)
		*ct->regs.pfv = f;
	if(ct->regs.pvv)
		*ct->regs.pvv = f;
}

void midi_proc_accel_cyclic(struct s_midi_ctrl *ct, int value)
{
	if(value < 64)
		ct->last += value;
	else
		ct->last -= 128-value;
	midi_proc_linear(ct, ct->last & 0x7f);
}

void midi_proc_accel_unbounded(struct s_midi_ctrl *ct, int value)
{
	midi_add(ct, value < 64 ? value : value-128);
}

void midi_proc_accel_linear(struct s_midi_ctrl *ct, int value)
{
	if(value < 64) {
		ct->last += value;
		if(ct->last > 127)
			ct->last = 127;
	} else {
		ct->last -= 128-value;
		if(ct->last & 0x80)
			ct->last = 0;
	}
	midi_proc_linear(ct, ct->last);
}


void stim_midi_ctrl(struct stimuli *s, int chan, int ctrl, int value)
{
	struct s_midi_ctrl *ct;

	if(s && s->midi[chan]) {
		ct = s->midi[chan]->ctrl[ctrl];
		if(ct)
			ct->proc(ct, value);
	}
}

struct stim_regs *stim_add_midi_ctrl(struct stimuli *s, int chan, int ctrl,
    void (*proc)(struct s_midi_ctrl *ct, int value))
{
	struct s_midi_chan *ch;
	struct s_midi_ctrl *ct;

	if(chan < 0 || chan > 15 || ctrl < 0 || ctrl > 127)
		return NULL;
	if(!s->midi[chan]) {
		s->midi[chan] = calloc(1, sizeof(struct s_midi_chan));
		if(!s->midi[chan])
			return NULL;
	}
	ch = s->midi[chan];

	if(!ch->ctrl[ctrl]) {
		ch->ctrl[ctrl] = calloc(1, sizeof(struct s_midi_ctrl));
		if(!ch->ctrl[ctrl])
			return NULL;
	}
	ct = ch->ctrl[ctrl];

	ct->proc = proc;
	ct->regs.pfv = ct->regs.pvv = NULL;

	return &ct->regs;
}

struct stimuli *stim_new(const void *target)
{
	struct stimuli *s;

	s = calloc(1, sizeof(struct stimuli));
	if(s) {
		s->ref = 1;
		s->target = target;
	}
	return s;
}

struct stimuli *stim_get(struct stimuli *s)
{
	if(s)
		s->ref++;
	return s;
}

void stim_put(struct stimuli *s)
{
	int i, j;

	if(!s)
		return;
	if(--s->ref)
		return;
	for(i = 0; i != MIDI_CHANS; i++)
		if(s->midi[i]) {
			for(j = 0; j != MIDI_CTRLS; j++)
				free(s->midi[i]->ctrl[j]);
			free(s->midi[i]);
		}
	free(s);
}

void stim_redirect(struct stimuli *s, void *new)
{
	int i, j;
	struct s_midi_ctrl *ct;
	ptrdiff_t d = new-s->target;

	if(!s)
		return;
	for(i = 0; i != MIDI_CHANS; i++) {
		if(!s->midi[i])
			continue;
		for(j = 0; j != MIDI_CTRLS; j++) {
			ct = s->midi[i]->ctrl[j];
			if (!ct)
				continue;
			if(ct->regs.pfv)
				ct->regs.pfv = (void *) ct->regs.pfv+d;
			if(ct->regs.pvv)
				ct->regs.pvv = (void *) ct->regs.pvv+d;
		}
	}
	s->target = new;
}
