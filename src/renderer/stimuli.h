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
	struct s_midi_chan *midi[MIDI_CHANS];
	int ref;
	const void *target;	/* reference address for pointer relocation */
};


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

#endif /* STIMULI_H */
