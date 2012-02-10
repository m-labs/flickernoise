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


static void midi_set(struct s_midi_ctrl *ct, float f)
{
	if(ct->regs.pfv)
		*ct->regs.pfv = f;
	if(ct->regs.pvv)
		*ct->regs.pvv = f;
}

static void midi_add(struct s_midi_ctrl *ct, int value)
{
	float f;

	f = (float) value/127.0;
	if(ct->regs.pfv)
		*ct->regs.pfv += f;
	if(ct->regs.pvv)
		*ct->regs.pvv += f;
}

/* Linear mapping [0, 127] -> [0, 1] */

static void midi_proc_linear(struct s_midi_ctrl *ct, int value)
{
	midi_set(ct, (float) value/127.0);
}

/* Differential (signed 7 bit delta value) with linear mapping */

static void midi_proc_diff_cyclic(struct s_midi_ctrl *ct, int value)
{
	if(value < 64)
		ct->last += value;
	else
		ct->last -= 128-value;
	midi_proc_linear(ct, ct->last & 0x7f);
}

static void midi_proc_diff_unbounded(struct s_midi_ctrl *ct, int value)
{
	midi_add(ct, value < 64 ? value : value-128);
}

static void midi_proc_diff_linear(struct s_midi_ctrl *ct, int value)
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

static void midi_proc_range_button(struct s_midi_ctrl *ct, int value)
{
	midi_set(ct, !!value);
}

static void midi_proc_diff_button(struct s_midi_ctrl *ct, int value)
{
	if(value & 0x40)
		midi_set(ct, 0);
	else
		midi_set(ct, 1);
}

static void midi_proc_button_toggle(struct s_midi_ctrl *ct, int value)
{
	if(!value)
		return;
	ct->last = !ct->last;
	midi_set(ct, ct->last);
}

void stim_midi_ctrl(struct stimuli *s, int chan, int ctrl, int value)
{
	struct s_midi_ctrl *ct;

	if(!s)
		return;

	if(s->midi[chan]) {
		ct = s->midi[chan]->ctrl[ctrl];
		if(ct) {
			ct->proc(ct, value);
			return;
		}
	}

	if(s->midi[0]) {
		ct = s->midi[0]->ctrl[ctrl];
		if(ct)
			ct->proc(ct, value);
	}
}

static struct stim_regs *stim_add_midi_ctrl(struct stimuli *s, int chan,
    int ctrl, void (*proc)(struct s_midi_ctrl *ct, int value))
{
	struct s_midi_chan *ch;
	struct s_midi_ctrl *ct;

	if(chan < 0 || chan > MIDI_CHANS || ctrl < 0 || ctrl > 127)
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
	for(i = 0; i != MIDI_CHANS+1; i++)
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
	for(i = 0; i != MIDI_CHANS+1; i++) {
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

/* ----- Input device database --------------------------------------------- */

static void (*map[dt_last][ft_last])(struct s_midi_ctrl *sct, int value) = {
	[dt_range] = {
		[ft_range] =		midi_proc_linear,
		[ft_unbounded] =	midi_proc_linear,
		[ft_cyclic] =		midi_proc_linear,
		[ft_button] =		midi_proc_range_button,
		[ft_toggle] =		midi_proc_range_button,
	},
	[dt_diff] = {
		[ft_range] =		midi_proc_diff_linear,
		[ft_unbounded] =	midi_proc_diff_unbounded,
		[ft_cyclic] =		midi_proc_diff_cyclic,
		[ft_button] =		midi_proc_diff_button,
		[ft_toggle] =		midi_proc_diff_button,
	},
	[dt_button] = {
		[ft_range] =		midi_proc_linear,
		[ft_unbounded] =	midi_proc_linear,
		[ft_cyclic] =		midi_proc_linear,
		[ft_button] =		midi_proc_linear,
		[ft_toggle] =		midi_proc_button_toggle,
	},
	[dt_toggle] = {
		[ft_range] =		midi_proc_linear,
		[ft_unbounded] =	midi_proc_linear,
		[ft_cyclic] =		midi_proc_linear,
		[ft_button] =		NULL,
		[ft_toggle] =		midi_proc_linear,
	},
};

static struct stim_db_midi *db = NULL, **last = &db;

struct stim_db_midi *stim_db_midi(const char *selector)
{
	struct stim_db_midi *dev;

	dev = malloc(sizeof(struct stim_db_midi));
	if(!dev)
		return NULL;
	dev->selector = selector;
	dev->ctrls = NULL;
	dev->next = NULL;
	*last = dev;
	last = &dev->next;
	return dev;
}

int stim_db_midi_ctrl(struct stim_db_midi *dev, const void *handle,
    enum stim_midi_dev_type type, int chan, int ctrl)
{
	struct stim_db_midi_ctrl **p;

	for(p = &dev->ctrls; *p; p= &(*p)->next)
		if((*p)->handle == handle)
			return 0;
	*p = malloc(sizeof(struct stim_db_midi_ctrl));
	if(!*p)
		return 0;
	(*p)->handle = handle;
	(*p)->chan = chan;
	(*p)->ctrl = ctrl;
	(*p)->type = type;
	(*p)->next = NULL;
	return 1;
}

static struct stim_regs *do_bind(struct stimuli *s,
    const struct stim_db_midi_ctrl *ctrl, enum stim_midi_fn_type fn)
{
	return stim_add_midi_ctrl(s, ctrl->chan, ctrl->ctrl,
	    map[ctrl->type][fn]);
}

struct stim_regs *stim_bind(struct stimuli *s, const void *handle,
    enum stim_midi_fn_type fn)
{
	const struct stim_db_midi *dev;
	const struct stim_db_midi_ctrl *ctrl;

	for(dev = db; dev; dev = dev->next)
		for(ctrl = db->ctrls; ctrl; ctrl = ctrl->next)
			if(ctrl->handle == handle)
				return do_bind(s, ctrl, fn);
	return NULL;
}
