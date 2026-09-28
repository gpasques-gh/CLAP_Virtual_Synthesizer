
#ifdef __LINUX__
	#define _GNU_SOURCE
#else
	#define M_PI 3.14159265359
#endif

#include <math.h>

#include "defs.h"
#include "core/synth.h"

/*
 * Process a sample from the ADSR envelope
 * Returns the envelope amplification coeficient
 */
float adsr_process(adsr_t *adsr)
{
	switch (adsr->state)
	{
	case ENV_IDLE:
		adsr->output = 0.0f;
		break;
	case ENV_ATTACK:
		if (adsr->attack > 0.0f)
		{
			/* Increment the amplification by the attack amount */
			adsr->output += 1.0f / (adsr->attack * RATE);
			if (adsr->output >= 1.0f)
			{
				adsr->output = 1.0f;
				adsr->state = ENV_DECAY;
			}
		}
		else if (adsr->decay > 0.0f)
		{
			/* No attack and decay goes straight to decay */
			adsr->output = 1.0f;
			adsr->state = ENV_DECAY;
		}
		else
		{
			/* No attack and no decay goes straight to sustain */
			adsr->output = adsr->sustain;
			adsr->state = ENV_SUSTAIN;
		}
		break;
	case ENV_DECAY:
		if (adsr->decay > 0.0f)
		{
			/* Decrement the amplification by the decay amount relatively to the sustain amount */
			adsr->output -= (1.0f - adsr->sustain) / (adsr->decay * RATE);
			if (adsr->output <= adsr->sustain)
			{
				adsr->output = adsr->sustain;
				adsr->state = ENV_SUSTAIN;
			}
		}
		else
		{
			adsr->output = adsr->sustain;
			adsr->state = ENV_SUSTAIN;
		}
		break;
	case ENV_SUSTAIN:
		if (adsr->sustain > 0.0f)
		{
			/* Hold the amplification at the sustain amount */
			adsr->output = adsr->sustain;
		}
		else
		{
			/* Fall into release from the current level */
			adsr->state = ENV_RELEASE;
		}
		break;
	case ENV_RELEASE:
		if (adsr->release > 0.0f)
		{
			/* Release acts as a time constant */
			adsr->output -= adsr->output / (adsr->release * RATE);;
			if (adsr->output <= 0.001f)
			{
				adsr->output = 0.0f;
				adsr->state = ENV_IDLE;
			}
		}
		else
		{
			/* If no release, go in idle state */
			adsr->output = 0.0f;
			adsr->state = ENV_IDLE;
		}
		break;
	default:
		adsr->output = 0.0f;
		adsr->state = ENV_IDLE;
		break;
	}
	
	/* Return the amplification of the ADSR envelope */
	return adsr->output;
}


/* Process the synth voices into the sound buffer */
double process_voices(synth_t *synth)
{
	double mixed_voices = 0.0;

	for (int v = 0; v < VOICES; v++)
	{
		voice_t *voice = &synth->voices[v];
		if (synth->voices[v].adsr.state == ENV_IDLE)
		{
			continue;
		}

		if ((synth->arp && v == synth->active_arp) || !synth->arp)
		{
			float envelope = adsr_process(&synth->voices[v].adsr);
			double mixed_osc = 0.0;

			for (int o = 0; o < 3; o++)
			{
				osc_t *osc = &synth->voices[v].oscillators[o];
				double phase_inc = osc->freq / RATE;
				double sample;

				switch (osc->wave)
				{
				case SINE_WAVE:
					sample = sin(2.0 * M_PI * osc->phase);
					break;
				case SQUARE_WAVE:
					sample = (osc->phase < 0.5) ? 1.0 : -1.0;
					break;
				case TRIANGLE_WAVE:
					sample = 1.0 - 4.0 * fabs(osc->phase - 0.5);
					break;
				case SAWTOOTH_WAVE:
					sample = 2.0 * osc->phase - 1.0;
					break;
				default:
					sample = 0.0;
					break;
				}

				mixed_osc += sample;

				osc->phase += phase_inc;
				if (osc->phase >= 1.0)
				{
					osc->phase -= 1.0;
				}
			}

			/* Oscillator sound mix */
			mixed_osc /= 3.0;
			mixed_osc *= envelope;
			mixed_osc *= voice->velocity_amp;

			if (synth->lfo.mod_param == LFO_AMP)
			{
				mixed_osc *= synth->lfo_amp;
			}
			else
			{
				mixed_osc *= synth->amp;
			}

			mixed_voices += mixed_osc;
		}
	}
	
	return mixed_voices;
}

/* Process the LFO modulation */
void process_lfo(synth_t *synth)
{
	if (synth->lfo.mod_param != LFO_OFF)
	{
		/* Processing the LFO */
		double phase_inc = synth->lfo.osc.freq / RATE;
		double automation;
		
		/* Calculating the wave from the LFO */
		switch (synth->lfo.osc.wave)
		{
		case SINE_WAVE:
			automation = fabs(sin(2.0 * M_PI * synth->lfo.osc.phase));
			break;
		case SQUARE_WAVE:
			automation = (synth->lfo.osc.phase < 0.5) ? 1.0 : 0.0;
			break;
		case TRIANGLE_WAVE:
			automation = fabs(1.0 - 4.0 * fabs(synth->lfo.osc.phase - 0.5));
			break;
		case SAWTOOTH_WAVE:
			automation = synth->lfo.osc.phase;
			break;
		default:
			automation = 0.0;
			break;
		}

		/* Applyging the LFO to the assigned parameter */
		switch (synth->lfo.mod_param)
		{
		case LFO_CUTOFF:
			synth->filter.lfo_cutoff = synth->filter.cutoff * automation;
			break;
		case LFO_DETUNE:
			synth->lfo_detune = synth->detune * automation;
			apply_detune_change(synth);
			break;
		case LFO_AMP:
			synth->lfo_amp = synth->amp * automation;
			break;
		default:
			break;
		}

		synth->lfo.osc.phase += phase_inc;
		if (synth->lfo.osc.phase >= 1.0)
		{
			synth->lfo.osc.phase -= 1.0;
		}
	}
}

/* Process the gain and low-pass filter onto the sound buffer */
double process_gain(synth_t *synth, double sample, int active_voices)
{
	/* No gain if arpeggio*/
	if (synth->arp)
	{
		return sample;
	}
	
	 /* Gain to stay at the same level despite the number of active voices */
	double gain = (active_voices > 0)
					  ? 1.0 / sqrt((double)active_voices)
					  : 0.0;

	/* Gain processing */
	double processed_sample = sample * gain;
	if (processed_sample > 1.0)
	{
		processed_sample = 1.0;
	}
	if (processed_sample < -1.0)
	{
		processed_sample = -1.0;
	}

	return processed_sample;
}

double process_filter(synth_t *synth, double sample)
{
	double cutoff = synth->filter.cutoff;
 
	if (synth->filter.env && synth->lfo.mod_param != LFO_CUTOFF)
	{
		cutoff = synth->filter.cutoff +
						adsr_process(&synth->filter.adsr) / 2;
		if (cutoff > 1.0)
		{
			cutoff = 1.0;
		}
		synth->filter.env_cutoff = cutoff;
	}

	/* Clipping */
	if (cutoff > 1.0f)
	{
		cutoff = 1.0f;
	}
	if (cutoff < 0.0f)
	{
		cutoff = 0.0f;
	}

	/* Calculating the filter amplification */
	float frequency = cutoff * (RATE / 16.0f);
	float omega = 2.0f * M_PI * frequency / RATE;
	float alpha = omega / (omega + 1.0f);
	float input_f = (float)sample;
	float output = alpha * input_f + (1.0f - alpha) * synth->filter.prev_output;

	/* Setting the previous output and input of the filter */
	synth->filter.prev_output = output;
	synth->filter.prev_input = input_f;

	return (double)output;
}

/* Process the arpeggiator */
void process_arpeggiator(synth_t *synth, int active_voices)
{
	if (synth->arp)
	{
		float bpm_increment = 1.0 / (60.0 / (float)synth->bpm * RATE);
		synth->active_arp_float += bpm_increment;

		if (synth->active_arp_float >= 1.0)
		{
			synth->active_arp++;
			if (synth->active_arp >= active_voices)
			{
				synth->active_arp = 0;
			}
			synth->active_arp_float = 0.0;
			
			/* Reseting ADSR envelope */
			if (synth->voices[synth->active_arp].pressed)
			{
				synth->voices[synth->active_arp].adsr.state = ENV_ATTACK;
				if (synth->filter.env)
				{
					synth->filter.adsr.state = ENV_ATTACK;
				}
			}
		}
	}
}

/*
 * Change the frequency of a voice_t oscillators with the given MIDI note and velocity
 * Multiplied by the synth_t detune coefficient
 */
void change_freq(voice_t *voice, int note,
				 int velocity, double detune)
{
	int a4_diff = note - A4_POSITION;

	/* Activating the voice */
	voice->note = note;
	voice->adsr.output = 0.001;
	voice->adsr.state = ENV_ATTACK;
	voice->velocity_amp = velocity / MIDI_MAX_VALUE;

	/* Applying the frequency and detune effect to the oscillators */
	voice->oscillators[0].freq = A_4 * pow(2, a4_diff / 12.0);
	voice->oscillators[0].phase = 0.0;
	voice->oscillators[1].freq = A_4 * pow(2, a4_diff / 12.0) + (5 * detune);
	voice->oscillators[1].phase = 0.0;
	voice->oscillators[2].freq = A_4 * pow(2, a4_diff / 12.0) - (5 * detune);
	voice->oscillators[2].phase = 0.0;
}

/* Apply the detune change to the voices oscillators */
void apply_detune_change(synth_t *synth)
{
	float detune;
	if (synth->lfo.mod_param == LFO_DETUNE)
	{
		detune = synth->lfo_detune;
	}
	else 
	{
		detune = synth->detune;
	}
	
	for (int v = 0; v < VOICES; v++)
	{
		int a4_diff = synth->voices[v].note - A4_POSITION;
		synth->voices[v].oscillators[1].freq = 
			A_4 * pow(2, a4_diff / 12.0) + (5 * detune);
		synth->voices[v].oscillators[2].freq = 
			A_4 * pow(2, a4_diff / 12.0) - (5 * detune);
	}
}

/* Get the literal name of a given waveform */
const char *get_wave_name(int wave)
{
	switch (wave)
	{
	case SINE_WAVE:
		return "Sine wave";
	case SQUARE_WAVE:
		return "Square wave";
	case TRIANGLE_WAVE:
		return "Triangle wave";
	case SAWTOOTH_WAVE:
		return "Sawtooth wave";
	default:
		return "Unknown wave";
	}
}

/*
 * Returns the first free voice from the synth_t
 * Used to assign a note send by MIDI or keyboard to the first free voice
 */
voice_t *get_free_voice(synth_t *synth)
{
	for (int i = 0; i < VOICES; i++)
	{
		if (!synth->arp && synth->voices[i].adsr.state == ENV_IDLE)
		{
			return &synth->voices[i];
		}
		else if (synth->arp && !synth->voices[i].pressed)
		{
			return &synth->voices[i];
		}
	}
	return NULL;
}

/* Insertion sort algorithm for the voices of a synth_t, used for arpeggiator */
void sort_synth_voices(synth_t *synth)
{
	/* First sort the voices */
	for (int v = 1; v < VOICES; v++)
	{
		voice_t current = synth->voices[v];
		int i = v - 1;
		
		while (i >= 0 && synth->voices[i].note > current.note)
		{
			synth->voices[i + 1] = synth->voices[i];
			i--;
		}

		synth->voices[i + 1] = current;
	}

	/* Then move the voices with empty notes to the end */
	for (int v = 1; v < VOICES; v++)
	{
		voice_t current = synth->voices[v];
		int i = v - 1;
		
		while (i >= 0 && synth->voices[i].note == -1)
		{
			synth->voices[i + 1] = synth->voices[i];
			i--;
		}

		synth->voices[i + 1] = current;
	}
}

void voice_on(synth_t *synth, int key, int vel)
{
	/* Count the currently pressed voices */
	int pressed_voices = 0;
	for (int v = 0; v < VOICES; v++)
	{   
		if (synth->voices[v].pressed)
			pressed_voices++;
		if (synth->voices[v].adsr.state == ENV_RELEASE && !synth->arp)
			synth->voices[v].adsr.state = ENV_IDLE;
	}

	/* Get the first free voice */
	voice_t *free_voice = get_free_voice(synth);
	if (free_voice == NULL) return;

	/* Press the voice and activate it */
	free_voice->pressed = 1;
	change_freq(free_voice, key, vel, synth->detune);
	if (pressed_voices == 0 && synth->filter.env)
			synth->filter.adsr.state = ENV_ATTACK;

	/* If the arpeggiator is on */
	if (synth->arp)
	{
		/* Sort the synthesizer voices by MIDI note */
		sort_synth_voices(synth);
		if (pressed_voices == 0)
			synth->active_arp_float = 1.0;
	}
}

void voice_off(synth_t *synth, int key)
{
	/* Count the currently pressed voices */
	int pressed_voices = 0;
	for (int v = 0; v < VOICES; v++)
		if (synth->voices[v].pressed)
			pressed_voices++;
	
	/* Loop through the voices to deactivate 
	the one of which MIDI note has been released */
	for (int v = 0; v < VOICES; v++)
	{
		if (synth->voices[v].note == key && 
			synth->voices[v].pressed)
		{
			if (synth->arp && synth->voices[v].adsr.state != ENV_IDLE)
			{
				synth->voices[v].adsr.state = ENV_IDLE;
			}
			else if (!synth->arp &&
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

	/* If the arpeggiator is on */
	if (synth->arp)
	{
		/* Sort the voices by MIDI note */
		sort_synth_voices(synth);
		if (pressed_voices == 2)
		{
			synth->active_arp_float = 1.0;
		}
	}
}

void update_filter_params(
	synth_t *synth,
	float cutoff,
	float a, float d, float s, float r,
	bool env)
{
	synth->filter.cutoff = cutoff;
	synth->filter.adsr.attack = a;
	synth->filter.adsr.decay = d;
	synth->filter.adsr.sustain = s;
	synth->filter.adsr.release = r;
	synth->filter.env = env;
}

void update_lfo_params(
	synth_t *synth,
	int waveform,
	int param)
{
	synth->lfo.osc.wave = waveform;
	synth->lfo.mod_param = param;
}

void update_synth_envelope(
	synth_t *synth,
	float a, float d, float s, float r)
{
	for (int v = 0; v < VOICES; v++)
	{
		synth->voices[v].adsr.attack = a;
		synth->voices[v].adsr.decay = d;
		synth->voices[v].adsr.sustain = s;
		synth->voices[v].adsr.release = r;
	}
}

void update_synth_oscillators(
	synth_t *synth, 
	int w_a, int w_b, int w_c)
{
	for (int v = 0; v < VOICES; v++)
	{
		synth->voices[v].oscillators[0].wave = w_a;
		synth->voices[v].oscillators[1].wave = w_b;
		synth->voices[v].oscillators[2].wave = w_c;
	}
}
