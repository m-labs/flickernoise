/*
 * stimuli.h - Unified control input handling
 *
 * Copyright 2012 by Werner Almesberger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 */

#ifndef STIMULI_H
#define STIMULI_H

#include <stdint.h>


#define	MIDI_CHANS	16
#define	MIDI_CTRLS	128

enum stim_midi_dev_type {
	dt_range,	/* 0-127, holds value */
	dt_diff,	/* differential, 0 = 0, 1 = +1, 127 = -1, ... */
	dt_button,	/* button, 0 = up, 127 = down */
	dt_toggle,	/* toggle, 0 = off, 127 = on */
	dt_last
};

enum stim_midi_fn_type {
	ft_range,	/* [0, 1] */
	ft_unbounded,	/* additive, use with caution */
	ft_cyclic,	/* wraps around 0 <-> 1 */
	ft_button,	/* 0 = up, 1 = down */
	ft_toggle,	/* 0 = off, 1 = on */
	ft_last
	
};

struct stim_regs {
	float *pfv, *pvv;	/* NULL if unused */
};

struct s_midi_ctrl {
	void (*proc)(struct s_midi_ctrl *sct, int value);
	struct stim_regs regs;
	uint8_t last;		/* for midi_proc_accel */
};

struct s_midi_chan {
	struct s_midi_ctrl *ctrl[MIDI_CTRLS];
};

struct stimuli {
	struct s_midi_chan *midi[MIDI_CHANS+1];
	int ref;
	const void *target;	/* reference address for pointer relocation */
};

struct stim_db_midi_ctrl {
	const void *handle;
	int chan;
	int ctrl;
	enum stim_midi_dev_type type;
	struct stim_db_midi_ctrl *next;
};

struct stim_db_midi {
	const char *selector;
	struct stim_db_midi_ctrl *ctrls;
	struct stim_db_midi *next;
};

/*
 * Channel numbers are one-based. Channel 0 acts as a catch-all when used with
 * stim_add_midi_ctrl.
 */

/* Linear mapping [0, 127] -> [0, 1] */
void midi_proc_linear(struct s_midi_ctrl *ct, int value);

/* "Acceleration" (signed 7 bit delta value) with linear mapping */
void midi_proc_accel_cyclic(struct s_midi_ctrl *ct, int value);
void midi_proc_accel_unbounded(struct s_midi_ctrl *ct, int value);
void midi_proc_accel_linear(struct s_midi_ctrl *ct, int value);

void stim_midi_ctrl(struct stimuli *s, int chan, int ctrl, int value);
struct stim_regs *stim_add_midi_ctrl(struct stimuli *s, int chan, int ctrl,
    void (*proc)(struct s_midi_ctrl *ct, int value));
struct stimuli *stim_new(const void *target);
struct stimuli *stim_get(struct stimuli *s);
void stim_put(struct stimuli *s);
void stim_redirect(struct stimuli *s, void *new);

struct stim_db_midi *stim_db_midi(const char *selector);
int stim_db_midi_ctrl(struct stim_db_midi *dev, const void *handle,
    enum stim_midi_dev_type type, int chan, int ctrl);
struct stim_regs *stim_bind(struct stimuli *s, const void *handle,
    enum stim_midi_fn_type fn);

#endif /* STIMULI_H */
