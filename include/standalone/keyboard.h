#ifdef __STANDALONE__

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "core/synth.h"

/*
 * Get the keyboard input from the SDL key event and the keyboard layout (QWERTY or AZERTY)
 * Change the synth_t voices frequencies with the assigned note
 * Change the keyboard octave when UP or DOWN keys are being pressed
 */
void handle_input(synth_t *synth, int *octave, 
	float *attack, float *decay, float *sustain, float *release,
	int *osc_a_wave, int *osc_b_wave, int *osc_c_wave);

/* Free the synth voices when their assigned note key are being released */
void handle_release(synth_t *synth, int octave);

/* Assign a note to a free synth voice */
void assign_note(synth_t *synth, int midi_note);

/* Release a note from a synth voice, does nothing if note is not pressed */
void release_note(synth_t *synth, int midi_note);

#endif /* HEADER GUARD */
#endif /* __STANDALONE__ */
