/*
 * stimuli.c - Unified control input handling
 *
 * Copyright 2012 by Werner Almesberger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 */

#include <stdlib.h>
#include <string.h>

#include "stimuli.h"


void midi_proc_lin(struct s_midi_ctrl *ct, int value)
{
	*ct->var = (float) value/127.0;
}


void midi_proc_accel(struct s_midi_ctrl *ct, int value)
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
	*ct->var = (float) ct->last/127.0;
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


int stim_add(struct stimuli *s, int chan, int ctrl, float *var,
    void (*proc)(struct s_midi_ctrl *ct, int value))
{
	struct s_midi_chan *ch;
	struct s_midi_ctrl *ct;

	if(!s->midi[chan]) {
		s->midi[chan] = calloc(1, sizeof(struct s_midi_chan));
		if(!s->midi[chan])
			return 0;
	}
	ch = s->midi[chan];

	if(!ch->ctrl[ctrl]) {
		ch->ctrl[ctrl] = calloc(1, sizeof(struct s_midi_ctrl));
		if(!ch->ctrl[ctrl])
			return 0;
	}
	ct = ch->ctrl[ctrl];

	ct->proc = proc;
	ct->var = var;

	return 1;
}


struct stimuli *stim_new(void)
{
	struct stimuli *s;

	s = calloc(1, sizeof(struct stimuli));
	if(s)
		s->ref = 1;
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
