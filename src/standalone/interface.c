#ifndef __CLAP__

#include <stdio.h>

#include "lib_raylib/src/raylib.h"
#include "lib_raygui/src/raygui.h"

#ifdef __WINDOWS__
#include "standalone/win_defs.h"
#include <windows.h>
#endif

#define __NO_RL__
#include "defs.h"
#undef __NO_RL__
#include "standalone/interface.h"
#include "core/synth.h"

/* Render the ADSR envelope sliders */
void render_adsr(
	float *attack, float *decay,
	float *sustain, float *release)
{
	/* ADSR envelope sliders */
	GuiGroupBox((Rectangle){30, 40, 550, 160}, "ADSR Envelope");

	GuiLabel((Rectangle){150, 50, 100, 20}, "Attack");
	GuiSlider((Rectangle){60, 70, 225, 40}, NULL, NULL,
			  attack, 0.0f, 2.0f);

	GuiLabel((Rectangle){150, 120, 100, 20}, "Decay");
	GuiSlider((Rectangle){60, 140, 225, 40}, NULL, NULL,
			  decay, 0.0f, 2.0f);

	GuiLabel((Rectangle){410, 50, 100, 20}, "Sustain");
	GuiSlider((Rectangle){320, 70, 225, 40}, NULL, NULL,
			  sustain, 0.0f, 1.0f);

	GuiLabel((Rectangle){410, 120, 100, 20}, "Release");
	GuiSlider((Rectangle){320, 140, 225, 40}, NULL, NULL,
			  release, 0.0f, 1.0f);
}

/* Render the filter ADSR envelope sliders */
void render_filter_adsr(synth_t *synth)
{
	/* Filter ADSR envelope sliders */
	GuiGroupBox((Rectangle){610, 40, 550, 160}, "Filter ADSR Envelope");

	GuiLabel((Rectangle){730, 50, 100, 20}, "Attack");
	GuiSlider((Rectangle){640, 70, 225, 40}, NULL, NULL,
			  &synth->filter.adsr.attack, 0.0f, 2.0f);

	GuiLabel((Rectangle){730, 120, 100, 20}, "Decay");
	GuiSlider((Rectangle){640, 140, 225, 40}, NULL, NULL,
			  &synth->filter.adsr.decay, 0.0f, 2.0f);

	GuiLabel((Rectangle){990, 50, 100, 20}, "Sustain");
	GuiSlider((Rectangle){900, 70, 225, 40}, NULL, NULL,
			  &synth->filter.adsr.sustain, 0.0f, 1.0f);

	GuiLabel((Rectangle){990, 120, 100, 20}, "Release");
	GuiSlider((Rectangle){900, 140, 225, 40}, NULL, NULL,
			  &synth->filter.adsr.release, 0.0f, 1.0f);
}

/* Render the oscillators waveforms dropdown menus*/
void render_osc_waveforms(
	int *wave_a, int *wave_b, int *wave_c,
	bool *ddm_a, bool *ddm_b, bool *ddm_c)
{
	/* Oscillators waveforms */
	GuiGroupBox((Rectangle){30, 230, 550, 160}, "Oscillators");

	GuiLabel((Rectangle){80, 265, 110, 20}, "Oscillator A");
	if (GuiDropdownBox((Rectangle){60, 285, 140, 40},
					   "#01#Sine;#02#Square;#03#Triangle;#04#Sawtooth",
					   wave_a, *ddm_a))
	{
		*ddm_a = !*ddm_a;
	}

	GuiLabel((Rectangle){250, 265, 110, 20}, "Oscillator B");
	if (GuiDropdownBox((Rectangle){230, 285, 140, 40},
					   "#01#Sine;#02#Square;#03#Triangle;#04#Sawtooth",
					   wave_b, *ddm_b))
	{
		*ddm_b = !*ddm_b;
	}

	GuiLabel((Rectangle){420, 265, 110, 20}, "Oscillator C");
	if (GuiDropdownBox((Rectangle){400, 285, 140, 40},
					   "#01#Sine;#02#Square;#03#Triangle;#04#Sawtooth",
					   wave_c, *ddm_c))
	{
		*ddm_c = !*ddm_c;
	}
}

/* Render the synthesizer parameters */
void render_synth_params(synth_t *synth)
{
	/* Synth parameters */
	GuiGroupBox((Rectangle){610, 230, 550, 160}, "Synth parameters");

	GuiLabel((Rectangle){730, 240, 100, 20}, "Amp");
	GuiSlider((Rectangle){640, 260, 225, 40}, NULL, NULL,
			  &synth->amp, 0.0f, 1.0f);
	if (synth->lfo.mod_param == LFO_AMP)
	{
		DrawRectangle(640, 260, 225 * synth->lfo_amp, 40, GRAY);
	}

	GuiLabel((Rectangle){730, 310, 100, 20}, "Cutoff");
	GuiSlider((Rectangle){640, 330, 225, 40}, NULL, NULL,
			  &synth->filter.cutoff, 0.025f, 1.0f); /* 2.5% to 100% to avoid muting */
	if (synth->lfo.mod_param == LFO_CUTOFF)
	{
		DrawRectangle(640, 330, 225 * synth->filter.lfo_cutoff, 40, GRAY);
	}

	GuiLabel((Rectangle){990, 240, 100, 20}, "Detune");
	if (GuiSlider((Rectangle){900, 260, 225, 40}, NULL, NULL,
				  &synth->detune, 0.0f, 1.0f))
	{
		apply_detune_change(synth);
	}

	if (synth->lfo.mod_param == LFO_DETUNE)
	{
		DrawRectangle(900, 260, 225 * synth->lfo_detune, 40, GRAY);
	}

	GuiLabel((Rectangle){900, 310, 150, 20}, "Filter env amount");
	GuiSlider((Rectangle){900, 330, 225, 40}, NULL, NULL, &synth->filter.env_wet, 0.0f, 1.0f);
}

/* Render the options menu */
void render_options(
	synth_t *synth,
	char *audio_filename,
	bool *saving_preset, bool *loading_preset,
	bool *saving_audio_file, bool *recording)
{
	/* Options */
	GuiGroupBox((Rectangle){1190, 230, 554, 160}, "Options");

	if (GuiButton((Rectangle){1210, 240, 120, 40}, "Save preset"))
	{
		*saving_preset = true;
	}

	if (GuiButton((Rectangle){1210, 290, 120, 40}, "Load preset"))
	{
		*loading_preset = true;
	}

	int record_button = GuiButton((Rectangle){1210, 340, 120, 40}, "Record");

	if (record_button && !*recording)
	{
		*saving_audio_file = true;
	}
	else if (record_button && *recording)
	{
		*recording = false;
	}

	if (*saving_audio_file)
	{
		int res = GuiTextInputBox((Rectangle){WIDTH / 2 - 100, HEIGHT / 2 - 50, 200, 100}, "Audio file name :", "", audio_filename, 20, "Start recording", (int *)saving_audio_file, false);
		if (res == 1)
		{
			*recording = true;
			*saving_audio_file = false;
		}
	}

	/* Drawing a little rectangle that shows we are recording */
	if (*recording)
	{
		DrawRectangleRounded((Rectangle){1340, 340, 5, 40}, 0.2, 10, RED);
	}

	if (GuiCheckBox((Rectangle){1350, 240, 40, 40}, "Arpeggiator", &synth->arp))
	{
		for (int v = 0; v < VOICES; v++)
		{
			synth->voices[v].adsr.state = ENV_IDLE;
			synth->voices[v].pressed = 0;
		}
	}

	GuiLabel((Rectangle){1400, 290, 100, 20}, "BPM");
	GuiSlider((Rectangle){1350, 310, 225, 40}, NULL, NULL, &synth->bpm, 0.0, 250.0);
}

/* Render the effects parameters */
void render_effects(
	synth_t *synth,
	bool *lfo_wave_ddm, bool *lfo_params_ddm,
	bool *distortion, bool *overdrive,
	float *distortion_amount)
{
	/* Effects */
	GuiGroupBox((Rectangle){1190, 40, 554, 160}, "Effects");

	GuiLabel((Rectangle){1210 + 265 / 2 - 120 / 2, 120, 120, 20}, "LFO frequency");
	GuiSlider((Rectangle){1210, 140, 265, 40}, NULL, NULL,
			  &synth->lfo.osc.freq, 0.0f, 1.0f);

	GuiLabel((Rectangle){1210 + 130 / 2 - 80 / 2, 50, 80, 20}, "LFO wave");
	if (GuiDropdownBox((Rectangle){1210, 70, 130, 40},
					   "#01#Sine;#02#Square;#03#Triangle;#04#Sawtooth",
					   &synth->lfo.osc.wave, *lfo_wave_ddm))
	{
		*lfo_wave_ddm = !*lfo_wave_ddm;
	}

	GuiLabel((Rectangle){1345 + 130 / 2 - 100 / 2, 50, 100, 20}, "LFO param");
	if (GuiDropdownBox((Rectangle){1345, 70, 130, 40},
					   "#01#Off;#02#Cutoff;#03#Detune;#04#Amp",
					   &synth->lfo.mod_param, *lfo_params_ddm))
	{
		*lfo_params_ddm = !*lfo_params_ddm;
	}

	/* Distortion */
	GuiLabel((Rectangle){1540 - 25, 50, 100, 20}, "Distortion");
	GuiCheckBox((Rectangle){1540, 70, 40, 40}, NULL, distortion);

	GuiLabel((Rectangle){1650 - 25, 50, 100, 20}, "Overdrive");
	GuiCheckBox((Rectangle){1650, 70, 40, 40}, NULL, overdrive);

	GuiLabel((Rectangle){1500 + 225 / 2 - 160 / 2, 120, 160, 20}, "Distortion amount");
	GuiSlider((Rectangle){1500, 140, 225, 40}, NULL, NULL,
			  distortion_amount, 0.0f, 0.99f);
}

/* Renders the waveform generated by the render_synth function */
void render_waveform(short *buffer)
{
	GuiGroupBox((Rectangle){30, 420, WIDTH - 55, 160}, "Waveform");

	int mid_y = HEIGHT / 4 + 35;
	int y = HEIGHT / 3 + mid_y;

	int step = 1;

	/* Looping onto the frames of the buffer,
	the i = 18 and FRAMES - 15 is because the waveform would go horizontally past the GuiGroupBox */
	for (int i = 18; i < FRAMES - 15; i += step)
	{

		int x1 = (i * WIDTH) / FRAMES;
		int x2 = ((i + step) * WIDTH) / FRAMES;

		int y1 = y - ((buffer[i] * mid_y) / 32768);
		int y2 = y - ((buffer[i + step] * mid_y) / 32768);

		/* Preventing the waveforum going vertically past the GuiGroupBox */
		if (y1 < 420)
		{
			y1 = 420;
		}
		if (y2 < 420)
		{
			y2 = 420;
		}
		if (y1 > 580)
		{
			y1 = 580;
		}
		if (y2 > 580)
		{
			y2 = 580;
		}

		DrawLine(x1, y1, x2, y2, BLACK);
	}
}

/* Render the white keys from the MIDI piano visualizer */
void render_white_keys()
{
	for (int i = 0; i < WHITE_KEYS; i++)
	{
		DrawRectangleLines(i * WHITE_KEYS_WIDTH, HEIGHT - WHITE_KEYS_HEIGHT,
						   WHITE_KEYS_WIDTH + 1, WHITE_KEYS_HEIGHT, BLACK);
	}
}

/* Render the black keys from the MIDI piano visualizer */
void render_black_keys()
{
	char black_keys_pattern[] = {1, 1, 0, 1, 1, 1, 0, 0};
	int white_key_index = 0;

	for (int octave = 0; octave <= (WHITE_KEYS / 7); octave++)
	{
		for (int i = 0; i < 7; i++)
		{
			if (black_keys_pattern[i])
			{
				int x = (white_key_index * WHITE_KEYS_WIDTH) + WHITE_KEYS_WIDTH - (BLACK_KEYS_WIDTH / 2);
				DrawRectangle(x, HEIGHT - WHITE_KEYS_HEIGHT,
							  BLACK_KEYS_WIDTH, BLACK_KEYS_HEIGHT, BLACK);
			}
			white_key_index++;
			if (white_key_index >= WHITE_KEYS)
			{
				break;
			}
		}
		if (white_key_index >= WHITE_KEYS)
		{
			break;
		}
	}
}

/* Renders given note into a pressed key in the MIDI piano visualizer
Render the key in a different color if it's the current arpeggio key */
void render_key(int midi_note, bool arp)
{
	if (midi_note < 0)
	{
		return;
	}

	int width = 0, height = 0, x = 0, y = 0, is_black = 0;
	get_key_position(midi_note, &x, &y, &width, &height, &is_black);

	if (arp)
	{
		DrawRectangle(x, y, width, height, SKYBLUE);
	}
	else
	{
		DrawRectangle(x, y, width, height, (Color){151, 232, 255, 255});
	}

	DrawRectangleLines(x, y, width, height, BLACK);

	/* Avoid double thick line when pressing a white key */
	if (!is_black)
	{
		if (arp)
		{
			DrawLine(x + width, y + 1, x + width, y + height, SKYBLUE);
		}
		else
		{
			DrawLine(x + width, y + 1, x + width, y + height, (Color){151, 232, 255, 255});
		}
		DrawLine(x, y + height, x + width, y + height, BLACK);
	}
}

/* Outputs a given MIDI note rectangle parameters (x, y, width and height) */
void get_key_position(int midi_note, int *x, int *y,
					  int *width, int *height, int *is_black)
{
	int note_in_octave = midi_note % 12;
	int octave = midi_note / 12;

	static const int black_keys[] = {0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0};
	*is_black = black_keys[note_in_octave];

	static const int white_key_map[] = {0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6};
	int white_key_in_octave = white_key_map[note_in_octave];

	int white_key_index = (octave * 7) + white_key_in_octave;

	if (*is_black)
	{
		*width = WHITE_KEYS_WIDTH / 2;
		*height = (WHITE_KEYS_HEIGHT * 2) / 3;
		*x = (white_key_index * WHITE_KEYS_WIDTH) + WHITE_KEYS_WIDTH - (*width / 2);
		*y = HEIGHT - WHITE_KEYS_HEIGHT;
	}
	else
	{
		*width = WHITE_KEYS_WIDTH;
		*height = WHITE_KEYS_HEIGHT;
		*x = white_key_index * WHITE_KEYS_WIDTH;
		*y = HEIGHT - WHITE_KEYS_HEIGHT;
	}
}

/* Returns if a MIDI note is a assigned to a black key or not */
int is_black_key(int midi_note)
{
	int note = midi_note % 12;
	return (note == 1 || note == 3 || note == 6 || note == 8 || note == 10);
}

#endif