#ifdef __STANDALONE__

#include "defs.h"
#include "core/synth.h"
#include "standalone/keyboard.h"

/*
 * Get the keyboard input from the SDL key event and the keyboard layout (QWERTY or AZERTY)
 * Change the synth_t voices frequencies with the assigned note
 * Change the ADSR parameters when the assigned keys are being pressed
 * Change the cutoff, detune and amplification when the assigned keys are being pressed
 * Change the keyboard octave when UP or DOWN keys are being pressed
 */
void handle_input(synth_t *synth, int *octave, 
	float *attack, float *decay, float *sustain, float *release,
	int *osc_a_wave, int *osc_b_wave, int *osc_c_wave)
{
	int octave_length = *octave * 12;

	if (IsKeyPressed(kC))
		assign_note(synth, octave_length + nC);
	if (IsKeyPressed(kC_SHARP))
		assign_note(synth, octave_length + nC_SHARP);
	if (IsKeyPressed(kD))
		assign_note(synth, octave_length + nD);
	if (IsKeyPressed(kD_SHARP))
		assign_note(synth, octave_length + nD_SHARP);
	if (IsKeyPressed(kE))
		assign_note(synth, octave_length + nE);
	if (IsKeyPressed(kF))
		assign_note(synth, octave_length + nF);
	if (IsKeyPressed(kF_SHARP))
		assign_note(synth, octave_length + nF_SHARP);
	if (IsKeyPressed(kG))
		assign_note(synth, octave_length + nG);
	if (IsKeyPressed(kG_SHARP))
		assign_note(synth, octave_length + nG_SHARP);
	if (IsKeyPressed(kA))
		assign_note(synth, octave_length + nA);
	if (IsKeyPressed(kA_SHARP))
		assign_note(synth, octave_length + nA_SHARP);
	if (IsKeyPressed(kB))
		assign_note(synth, octave_length + nB);
	if (IsKeyPressed(KEY_UP))
	{
		(*octave)++;
		/* Releasing all of the voices so that some notes 
		don't get stucked when sustain is not at 0.0 */
		for (int v = 0; v < VOICES; v++)
		{
			synth->voices[v].adsr.state = ENV_IDLE;
			synth->voices[v].pressed = 0;
			synth->voices[v].note = -1;
		}
	}
	else if (IsKeyPressed(KEY_DOWN))
	{
		(*octave)--;
		/* Releasing all of the voices so that some notes 
		don't get stucked when sustain is not at 0.0 */
		for (int v = 0; v < VOICES; v++)
		{
			synth->voices[v].adsr.state = ENV_IDLE;
			synth->voices[v].pressed = 0;
			synth->voices[v].note = -1;
		}
			
	}
	else if (IsKeyDown(KEY_ADSR)) {
		if (IsKeyPressed(KEY_ATT))
			{
				*attack += 0.1f;
				if (*attack >= 2.0f)
					*attack = 0.0f;
			}
			else if (IsKeyPressed(KEY_DEC))
			{
				*decay += 0.1f;
				if (*decay >= 2.0f)
					*decay = 0.0f;
			}
			else if (IsKeyPressed(KEY_SUS))
			{
				*sustain += 0.1f;
				if (*sustain >= 1.0f)
					*sustain = 0.0f;
			}
			else if (IsKeyPressed(KEY_REL))
			{
				*release += 0.1f;
				if (*release >= 1.0f)
					*release = 0.0f;
			}
	}
			
	else if (IsKeyPressed(KEY_OSC_A))
	{
		(*osc_a_wave)++;
		*osc_a_wave %= 4;
	}
	else if (IsKeyPressed(KEY_OSC_B))
	{
		(*osc_b_wave)++;
		*osc_b_wave %= 4;
	}	
	else if (IsKeyPressed(KEY_OSC_C))
	{
		(*osc_c_wave)++;
		*osc_c_wave %= 4;
	}
		
}

/* Free the synth voices when their assigned note key are being released */
void handle_release(synth_t *synth, int octave)
{
	int octave_length = octave * 12;
	
	if (IsKeyReleased(kC))
		release_note(synth, octave_length + nC);
	if (IsKeyReleased(kC_SHARP))
		release_note(synth, octave_length + nC_SHARP);
	if (IsKeyReleased(kD))
		release_note(synth, octave_length + nD);
	if (IsKeyReleased(kD_SHARP))
		release_note(synth, octave_length + nD_SHARP);
	if (IsKeyReleased(kE))
		release_note(synth, octave_length + nE);
	if (IsKeyReleased(kF))
		release_note(synth, octave_length + nF);
	if (IsKeyReleased(kF_SHARP))
		release_note(synth, octave_length + nF_SHARP);
	if (IsKeyReleased(kG))
		release_note(synth, octave_length + nG);
	if (IsKeyReleased(kG_SHARP))
		release_note(synth, octave_length + nG_SHARP);
	if (IsKeyReleased(kA))
		release_note(synth, octave_length + nA);
	if (IsKeyReleased(kA_SHARP))
		release_note(synth, octave_length + nA_SHARP);
	if (IsKeyReleased(kB))
		release_note(synth, octave_length + nB);
}

/* Assign a note to a free synth voice */
void assign_note(synth_t *synth, int midi_note)
{
	if (midi_note != -1)
	{
		int pressed_voices = 0;
		for (int v = 0; v < VOICES; v++)
		{
			if (synth->voices[v].pressed)
			{
				pressed_voices++;
			}
				

			/* Cutting all the voices that are in ADSR release state to avoid blocking voices */
			if (synth->voices[v].adsr.state == ENV_RELEASE && !synth->arp)
			{
				synth->voices[v].adsr.state = ENV_IDLE;
			}
		}

		voice_t *free_voice = get_free_voice(synth);
		if (free_voice == NULL) 
		{
			return;
		}

		free_voice->pressed = 1;
		change_freq(free_voice, midi_note, 127, synth->detune);
		if (pressed_voices == 0 && synth->filter.env)
		{
			synth->filter.adsr.state = ENV_ATTACK;
		}
			

		if (synth->arp)
		{
			sort_synth_voices(synth);
			if (pressed_voices == 0)
			{
				synth->active_arp_float = 1.0;
			}
		}
			
		return;
	}
}

/* Release a note from a synth voice, does nothing if the note isn't pressed */
void release_note(synth_t *synth, int midi_note)
{   
	int pressed_voices = 0;

	for (int v = 0; v < VOICES; v++)
	{
		if (synth->voices[v].pressed)
		{
			pressed_voices++;
		}
	}
		
	for (int v = 0; v < VOICES; v++)
	{
		if (synth->voices[v].note == midi_note && 
			synth->voices[v].pressed == 1)
		{
			if (synth->arp && synth->voices[v].adsr.state != ENV_IDLE)
			{
				synth->voices[v].adsr.state = ENV_IDLE;
			}
			else if (
			   !synth->arp && 
				synth->voices[v].adsr.state != ENV_RELEASE &&
				synth->voices[v].adsr.state != ENV_IDLE)
			{
				synth->voices[v].adsr.state = ENV_RELEASE;
			}
				
			synth->voices[v].note = -1;
			synth->voices[v].pressed = 0;
				
			break;
		}
	}
		
	if (synth->arp)
	{
		sort_synth_voices(synth);
		if (pressed_voices == 2)
		{
			synth->active_arp_float = 1.0;
		}   
	}
}

#endif