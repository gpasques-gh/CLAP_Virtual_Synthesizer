#ifdef __CLAP__

#include <string.h>
#include <float.h> 
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* MACROS */
#define IN_REC(x, y, rec) (x >= (rec.left) && x < (rec.right) && y >= (rec.top) && y < (rec.bottom))
#define PARAM_IS_SLIDER(id) (id == P_ATTACK || id == P_DECAY || id == P_SUSTAIN || id == P_RELEASE || id == P_VOLUME || id == P_CUTOFF || id == P_FILTER_ATTACK || id == P_FILTER_DECAY || id == P_FILTER_SUSTAIN || id == P_FILTER_RELEASE || id == P_DETUNE)
#define PARAM_IS_CHECKBOX(id) (id == P_FILTER_ENV_ON)

#include "lib_clap/include/clap/clap.h"
#include "clap/clap_plugin.h"
#include "clap/gui/clap_gui.h"

/* Font headers */
#define STB_TRUETYPE_IMPLEMENTATION
#include "lib_stb/stb_truetype.h"
#include "clap_assets/regular_font.h"

#include "defs.h"

/* Font structure */
typedef struct 
{
	uint8_t *ttf_buffer;
	stbtt_fontinfo info;
	int loaded;
} gui_font_t;

static gui_font_t __font = {0};

/* Load font from asset header */
int gui_load_font_mem(const unsigned char *ttf_data)
{
	__font.ttf_buffer = NULL;
	__font.loaded = stbtt_InitFont(
		&__font.info, ttf_data,
		stbtt_GetFontOffsetForIndex(ttf_data, 0));
	return __font.loaded;
}

/* Free the font structure */
static void gui_font_free(void)
{
	free(__font.ttf_buffer);
	__font.ttf_buffer = NULL;
	__font.loaded = 0;
}

/* Basic RGB blending function */
static void blend_pixel(
	uint32_t *bits, 
	int x, int y,
	uint32_t color, uint8_t alpha)
{
	if (x < 0 || y  < 0 || x >= GUI_WIDTH || y >= GUI_HEIGHT || alpha == 0)
		return;

	/* Get source and destination RGBs */
	uint32_t *dst = &bits[y * GUI_WIDTH + x];
	uint8_t sr = (uint8_t )(color >> 16);
	uint8_t sg = (uint8_t)(color >> 8);
	uint8_t sb = (uint8_t)(color);
	uint8_t dr = (uint8_t)(*dst >> 16);
	uint8_t dg = (uint8_t)(*dst >> 8);
	uint8_t db = (uint8_t)(*dst);

	/* Get blended RBG */
	uint8_t r = (uint8_t)((sr * alpha + dr * (255 - alpha)) / 255);
	uint8_t g = (uint8_t)((sg * alpha + dg * (255 - alpha)) / 255);
	uint8_t b = (uint8_t)((sb * alpha + db * (255 - alpha)) / 255);

	/* Blend the pixel */
	*dst = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

/* Paint text to the bitmap using STB TrueType library */
static void plugin_paint_text(
	uint32_t *bits, 
	int x, int y, 
	const char *text, 
	float px_size, 
	uint32_t color)
{
	if (!__font.loaded)
		return;

	float scale = 
		stbtt_ScaleForPixelHeight(&__font.info, px_size);
	int ascent;
	stbtt_GetFontVMetrics(&__font.info, &ascent, NULL, NULL);
	int baseline = (int)(ascent * scale);
	float xpos = (float)x;

	/* Loop through the text data */
	for (const char *p = text; *p; p++)
	{
		/* Get the local bitmap of the current letter */
		int w, h, xoff, yoff;
		unsigned char *bitmap = stbtt_GetCodepointBitmap(
			&__font.info, 0, scale, *p, &w, &h, &xoff, &yoff);
		
		if (bitmap)
		{
			/* Draw pixels on the GUI bitmap */
			for (int j = 0; j < h; j++)
			{
				for (int i = 0; i < w; i++)
				{
					/* Blend the bitmap pixel with the font bixel */
					uint8_t alpha = bitmap[j * w + i];
					blend_pixel(
						bits,
						(int)xpos + xoff +i,
						y + baseline + yoff + j,
						color, alpha);
				}
			}

			/* Free the local bitmap */
			stbtt_FreeBitmap(bitmap, NULL);
		}

		/* Advance to the next font letter */
		int advance;
		stbtt_GetCodepointHMetrics(&__font.info, *p, &advance, NULL);
		xpos += advance * scale;
		if (p[1])
			xpos += scale * 
				stbtt_GetCodepointKernAdvance(&__font.info, p[0], p[1]);
	}
}

/* Paint a rectangle to the bitmap */
static void plugin_paint_rec(uint32_t *bits, rectangle_t rec)
{
	for (uint32_t y = rec.top; y < rec.bottom; y++)
	{
		for (uint32_t x = rec.left; x < rec.right; x++)
		{
			bits[y * GUI_WIDTH + x] =  (
				y == rec.top || 
				y == rec.bottom - 1 || 
				x == rec.left ||
				x == rec.right - 1)
					? rec.border_color
					: rec.fill_color;
		}
	}
}

static void render_white_keys(uint32_t *bits)
{
	for (int i = 0; i < WHITE_KEYS; i++)
	{
		rectangle_t key = 
		{
			.left = (GUI_WIDTH / WHITE_KEYS) * i,
			.right = (GUI_WIDTH / WHITE_KEYS) * i + (GUI_WIDTH / WHITE_KEYS),
			.top = GUI_HEIGHT - GUI_HEIGHT / 6,
			.bottom = GUI_HEIGHT,
			.border_color = BLACK,
			.fill_color = 0xFFFFFF
		};
		plugin_paint_rec(bits, key);
	}
}

static void render_black_keys(uint32_t *bits)
{
	int black_keys_pattern[] = 
		{1, 1, 0, 1, 1, 1, 0, 0};
	int white_key_idx = 0;

	for (int octave = 0; octave <= (WHITE_KEYS / 7); octave++)
	{
		for (int i = 0; i < 7; i++)
		{
			if (black_keys_pattern[i])
			{
				int x = ((white_key_idx + 1) * 
					(GUI_WIDTH / WHITE_KEYS) - 
					(GUI_WIDTH / WHITE_KEYS / 4));
				rectangle_t key = 
				{
					.left = x,
					.right = x + (GUI_WIDTH / WHITE_KEYS / 2),
					.top = (GUI_HEIGHT - GUI_HEIGHT / 6),
					.bottom = ((GUI_HEIGHT - GUI_HEIGHT / 6) + GUI_HEIGHT / 10),
					.border_color = BLACK,
					.fill_color = BLACK 
				};
				plugin_paint_rec(bits, key);
				
			}

			white_key_idx++;
			if (white_key_idx >= WHITE_KEYS)
				break;
		}

		if (white_key_idx >= WHITE_KEYS)
			break;
	}
}

void render_key(uint32_t *bits, uint8_t midi_note)
{
	int note_in_octave = midi_note % 12;
	int octave = midi_note / 12;

	static const int black_keys[] = {0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0};
	bool is_black = black_keys[note_in_octave];

	static const int white_key_map[] = {0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6};
	int white_key_in_octave = white_key_map[note_in_octave];
	int white_key_index = (octave * 7) + white_key_in_octave;

	rectangle_t key = {0};
	key.top = GUI_HEIGHT - GUI_HEIGHT / 6;

	int white_key_width = (GUI_WIDTH / WHITE_KEYS);
	int black_key_width = white_key_width / 2;

	if (is_black)
	{
		key.left = (white_key_index * white_key_width) + white_key_width - (black_key_width / 2);
		key.right = key.left + black_key_width;
		key.bottom = key.top + (GUI_HEIGHT / 10);
	}
	else
	{
		key.left = white_key_index * white_key_width;
		key.right = key.left + white_key_width;
		key.bottom = GUI_HEIGHT;
	}

	key.fill_color = GRAY;
	key.border_color = BLACK;

	plugin_paint_rec(bits, key);
}

int is_black_key(int midi_note)
{
	int note = midi_note % 12;
	return (note == 1 || note == 3 || note == 6 || note == 8 || note == 10);
}


static void draw_piano_keyboard(synth_plugin_t *plugin)
{
	render_white_keys(plugin->gui->bits);
	for (int v = 0; v < VOICES; v++)
	{
		int note = atomic_load(&plugin->atomic_notes[v]);
		if (note != -1 && !is_black_key(note))
		render_key(plugin->gui->bits, note);
	}
	
	render_black_keys(plugin->gui->bits);
	for (int v = 0; v < VOICES; v++)
	{
		int note = atomic_load(&plugin->atomic_notes[v]);
		if (note != -1 && is_black_key(note))
			render_key(plugin->gui->bits, note);
	}
}

/* Send which param corresponds to a XY pos on the GUI */
static uint32_t get_param_gui(
	gui_elements_t elements, 
	int x, int y)
{
	/* SLIDERS */

	/* Amplification */
	rectangle_t amp = 
		elements.volume_slider.rec_value;
	
	/* ADSR */
	rectangle_t attack =
		elements.adsr_sliders[0].rec_value;
	rectangle_t decay =
		elements.adsr_sliders[1].rec_value;
	rectangle_t sustain =
		elements.adsr_sliders[2].rec_value;
	rectangle_t release =
		elements.adsr_sliders[3].rec_value;

	/* Filter ADSR */
	rectangle_t f_attack =
		elements.filter_adsr_sliders[0].rec_value;
	rectangle_t f_decay =
		elements.filter_adsr_sliders[1].rec_value;
	rectangle_t f_sustain =
		elements.filter_adsr_sliders[2].rec_value;
	rectangle_t f_release =
		elements.filter_adsr_sliders[3].rec_value;

	/* Filter cutoff */
	rectangle_t cutoff = 
		elements.cutoff_slider.rec_value;

	rectangle_t f_env_on =
		elements.filter_env_on_box.rec;

	rectangle_t menu =
		elements.waveforms.base_rec;
	rectangle_t sine =
		elements.waveforms.entries[0].rec;
	rectangle_t square =
		elements.waveforms.entries[1].rec;
	rectangle_t triangle =
		elements.waveforms.entries[2].rec;
	rectangle_t sawtooth =
		elements.waveforms.entries[3].rec;

	

	/* Return the parameter ID from XY position */

	if (elements.waveforms.entries_on)
	{
		if (IN_REC(x, y, sine) || 
			IN_REC(x, y, square) || 
			IN_REC(x, y, triangle) || 
			IN_REC(x, y, sawtooth))
				return P_WAVE_A;
	}
	if (IN_REC(x, y, menu))
		return P_WAVE_A;
	if (IN_REC(x, y, amp))
		return P_VOLUME;
	if (IN_REC(x, y, attack))
		return P_ATTACK;
	if (IN_REC(x, y, decay))
		return P_DECAY;
	if (IN_REC(x, y, sustain))
		return P_SUSTAIN;
	if (IN_REC(x, y, release))
		return P_RELEASE;
	if (IN_REC(x, y, f_attack))
		return P_FILTER_ATTACK;
	if (IN_REC(x, y, f_decay))
		return P_FILTER_DECAY;
	if (IN_REC(x, y, f_sustain))
		return P_FILTER_SUSTAIN;
	if (IN_REC(x, y, f_release))
		return P_FILTER_RELEASE;
	if (IN_REC(x, y, cutoff))
		return P_CUTOFF;
	if (IN_REC(x, y, f_env_on))
		return P_FILTER_ENV_ON;

	return P_COUNT;
}

static void paint_checkbox(uint32_t *bits, checkbox_t box)
{
	rectangle_t inner_rec = 
	{
		.left = box.rec.left + 5,
		.right = box.rec.right - 5,
		.top = box.rec.top + 5,
		.bottom = box.rec.bottom - 5,
		.fill_color = 0xFFFFF,
		.border_color = 0xFFFFF
	};

	plugin_paint_rec(bits, box.rec);
	if (box.param_value)
		plugin_paint_rec(bits, inner_rec);
}

/* Get slider rectangle from parameter ID */
static rectangle_t get_slider_rec(gui_elements_t elements, uint32_t param_id)
{
	switch (param_id)
	{
	case P_VOLUME: return elements.volume_slider.rec;
	case P_ATTACK: return elements.adsr_sliders[0].rec;
	case P_DECAY: return elements.adsr_sliders[1].rec;
	case P_SUSTAIN: return elements.adsr_sliders[2].rec;
	case P_RELEASE: return elements.adsr_sliders[3].rec;
	case P_FILTER_ATTACK: return elements.filter_adsr_sliders[0].rec;
	case P_FILTER_DECAY: return elements.filter_adsr_sliders[1].rec;
	case P_FILTER_SUSTAIN: return elements.filter_adsr_sliders[2].rec;
	case P_FILTER_RELEASE: return elements.filter_adsr_sliders[3].rec;
	case P_CUTOFF: return elements.cutoff_slider.rec;
	default: return (rectangle_t){0};
	}
}

/* SLIDERS IMPLEMENTATION */

/* Compute the horizontal slider cursor rectangle position */
static rectangle_t compute_horizontal_slider_rec(
	rectangle_t main_rec, uint32_t width,
	float value, float max)
{
	value = value / max;
	if (value < 0.0f) value = 0.0f;
	if (value > 1.0f) value = 1.0f;

	uint32_t travel = (main_rec.right - main_rec.left) - width;
	return (rectangle_t)
	{
		.top = main_rec.top,
		.bottom = main_rec.bottom,
		.left = main_rec.left + (uint32_t)(travel * value),
		.right = main_rec.left + width + (uint32_t)(travel * value),
		.border_color = BLACK, .fill_color = GRAY
	};
}

/* Paint a slider to the GUI bitmap */
static void plugin_paint_slider(uint32_t *bits, slider_t slider)
{
	plugin_paint_rec(bits, slider.rec);
	plugin_paint_rec(bits, slider.rec_value);
}

/* Create the elements of the GUI, called in gui_create */
void gui_create_elements(synth_plugin_t *plugin)
{
	/* Atomic read of the parameters */
	float amp = atomic_load(&plugin->params[P_VOLUME]);
	float attack = atomic_load(&plugin->params[P_ATTACK]);
	float decay = atomic_load(&plugin->params[P_DECAY]);
	float sustain = atomic_load(&plugin->params[P_SUSTAIN]);
	float release = atomic_load(&plugin->params[P_RELEASE]);
	float f_attack = atomic_load(&plugin->params[P_FILTER_ATTACK]);
	float f_decay = atomic_load(&plugin->params[P_FILTER_DECAY]);
	float f_sustain = atomic_load(&plugin->params[P_FILTER_SUSTAIN]);
	float f_release = atomic_load(&plugin->params[P_FILTER_RELEASE]);
	float cutoff = atomic_load(&plugin->params[P_CUTOFF]);
	bool f_env_on = atomic_load(&plugin->params[P_FILTER_ENV_ON]);

	/* Amplification slider */
	rectangle_t amp_rec = { 10, 110, 10, 50, BLACK, GRAY};
	plugin->gui->elements.volume_slider.rec = amp_rec;
	plugin->gui->elements.volume_slider.rec_value = 
		compute_horizontal_slider_rec(amp_rec, 20, amp, 1.0f);
	plugin->gui->elements.volume_slider.param_value = amp;

	/* Attack slider */
	rectangle_t attack_rec = { 10, 110, 80, 120, BLACK, GRAY};
	plugin->gui->elements.adsr_sliders[0].rec = attack_rec;
	plugin->gui->elements.adsr_sliders[0].rec_value = 
		compute_horizontal_slider_rec(attack_rec, 20, attack, 2.0f);
	plugin->gui->elements.adsr_sliders[0].param_value = attack;

	/* Decay slider */
	rectangle_t decay_rec = { 10, 110, 140, 180, BLACK, GRAY};
	plugin->gui->elements.adsr_sliders[1].rec = decay_rec;
	plugin->gui->elements.adsr_sliders[1].rec_value = 
		compute_horizontal_slider_rec(decay_rec, 20, decay, 2.0f);
	plugin->gui->elements.adsr_sliders[1].param_value = decay;

	/* Sustain slider */
	rectangle_t sustain_rec = { 10, 110, 200, 240, BLACK, GRAY};
	plugin->gui->elements.adsr_sliders[2].rec = sustain_rec;
	plugin->gui->elements.adsr_sliders[2].rec_value = 
		compute_horizontal_slider_rec(sustain_rec, 20, sustain, 1.0f);
	plugin->gui->elements.adsr_sliders[2].param_value = sustain;

	/* Release slider */
	rectangle_t release_rec = { 10, 110, 260, 300, BLACK, GRAY};
	plugin->gui->elements.adsr_sliders[3].rec = release_rec;
	plugin->gui->elements.adsr_sliders[3].rec_value = 
		compute_horizontal_slider_rec(release_rec, 20, release, 2.0f);
	plugin->gui->elements.adsr_sliders[3].param_value = release;

	/* Filter Attack slider */
	rectangle_t f_attack_rec = { 140, 240, 80, 120, BLACK, GRAY};
	plugin->gui->elements.filter_adsr_sliders[0].rec = f_attack_rec;
	plugin->gui->elements.filter_adsr_sliders[0].rec_value = 
		compute_horizontal_slider_rec(f_attack_rec, 20, f_attack, 2.0f);
	plugin->gui->elements.filter_adsr_sliders[0].param_value = f_attack;

	/* Filter Decay slider */
	rectangle_t f_decay_rec = { 140, 240, 140, 180, BLACK, GRAY};
	plugin->gui->elements.filter_adsr_sliders[1].rec = f_decay_rec;
	plugin->gui->elements.filter_adsr_sliders[1].rec_value = 
		compute_horizontal_slider_rec(f_decay_rec, 20, f_decay, 2.0f);
	plugin->gui->elements.filter_adsr_sliders[1].param_value = f_decay;

	/* Filter Sustain slider */
	rectangle_t f_sustain_rec = { 140, 240, 200, 240, BLACK, GRAY};
	plugin->gui->elements.filter_adsr_sliders[2].rec = f_sustain_rec;
	plugin->gui->elements.filter_adsr_sliders[2].rec_value = 
		compute_horizontal_slider_rec(f_sustain_rec, 20, f_sustain, 1.0f);
	plugin->gui->elements.filter_adsr_sliders[2].param_value = f_sustain;

	/* Filter Release slider */
	rectangle_t f_release_rec = { 140, 240, 260, 300, BLACK, GRAY};
	plugin->gui->elements.filter_adsr_sliders[3].rec = f_release_rec;
	plugin->gui->elements.filter_adsr_sliders[3].rec_value = 
		compute_horizontal_slider_rec(f_release_rec, 20, f_release, 2.0f);
	plugin->gui->elements.filter_adsr_sliders[3].param_value = f_release;

	/* Cutoff slider */
	rectangle_t cutoff_rec = { 140, 240, 320, 360, BLACK, GRAY};
	plugin->gui->elements.cutoff_slider.rec = cutoff_rec;
	plugin->gui->elements.cutoff_slider.rec_value = 
		compute_horizontal_slider_rec(cutoff_rec, 20, cutoff, 1.0f);
	plugin->gui->elements.cutoff_slider.param_value = cutoff;

	rectangle_t env_on_rec = {300, 340, 100, 140, GRAY, GRAY};
	plugin->gui->elements.filter_env_on_box.rec = env_on_rec;
	plugin->gui->elements.filter_env_on_box.param_value = f_env_on;

	rectangle_t waveform_base_rec = {300, 400, 10, 40, BLACK, GRAY};
	plugin->gui->elements.waveforms.base_rec = waveform_base_rec;
	plugin->gui->elements.waveforms.entries_on = false;
	plugin->gui->elements.waveforms.param_id = P_WAVE_A;

	plugin->gui->elements.waveforms.entries[SINE_WAVE].val = SINE_WAVE;
	plugin->gui->elements.waveforms.entries[SINE_WAVE].name = "Sine wave";
	plugin->gui->elements.waveforms.entries[SINE_WAVE].rec = waveform_base_rec;

	plugin->gui->elements.waveforms.entries[SQUARE_WAVE].val = SQUARE_WAVE;
	plugin->gui->elements.waveforms.entries[SQUARE_WAVE].name = "Square wave";
	plugin->gui->elements.waveforms.entries[SQUARE_WAVE].rec = waveform_base_rec;
	plugin->gui->elements.waveforms.entries[SQUARE_WAVE].rec.top += 40;
	plugin->gui->elements.waveforms.entries[SQUARE_WAVE].rec.bottom += 40;
	
	plugin->gui->elements.waveforms.entries[TRIANGLE_WAVE].val = TRIANGLE_WAVE;
	plugin->gui->elements.waveforms.entries[TRIANGLE_WAVE].name = "Triangle wave";
	plugin->gui->elements.waveforms.entries[TRIANGLE_WAVE].rec = waveform_base_rec;
	plugin->gui->elements.waveforms.entries[TRIANGLE_WAVE].rec.top += 80;
	plugin->gui->elements.waveforms.entries[TRIANGLE_WAVE].rec.bottom += 80;

	plugin->gui->elements.waveforms.entries[SAWTOOTH_WAVE].val = SAWTOOTH_WAVE;
	plugin->gui->elements.waveforms.entries[SAWTOOTH_WAVE].name = "Sawtooth wave";
	plugin->gui->elements.waveforms.entries[SAWTOOTH_WAVE].rec = waveform_base_rec;
	plugin->gui->elements.waveforms.entries[SAWTOOTH_WAVE].rec.top += 120;
	plugin->gui->elements.waveforms.entries[SAWTOOTH_WAVE].rec.bottom += 120;

	plugin->gui->elements.waveforms.selected = plugin->gui->elements.waveforms.entries[0];
	
	/* Load the font from the asset header */
	gui_load_font_mem(__embedded_font);
}

/* Update slider position with the new  parameter data */
static void update_slider(slider_t *slider, uint32_t width, float new_val, float val_max)
{
	if (slider->param_value != new_val)
	{
		slider->param_value = new_val;
		slider->rec_value = 
			compute_horizontal_slider_rec(
				slider->rec, width, new_val, val_max);
	}
}

/* Update slider position with the parameter data */
/* Updates even if the data is changed from 
the parameter view of the host and not the GUI view*/
static void update_sliders(synth_plugin_t *p)
{
	/* Get the new data*/
	float amp = atomic_load(&p->params[P_VOLUME]);
	float attack = atomic_load(&p->params[P_ATTACK]);
	float decay = atomic_load(&p->params[P_DECAY]);
	float sustain = atomic_load(&p->params[P_SUSTAIN]);
	float release = atomic_load(&p->params[P_RELEASE]);
	float f_attack = atomic_load(&p->params[P_FILTER_ATTACK]);
	float f_decay = atomic_load(&p->params[P_FILTER_DECAY]);
	float f_sustain = atomic_load(&p->params[P_FILTER_SUSTAIN]);
	float f_release = atomic_load(&p->params[P_FILTER_RELEASE]);
	float cutoff = atomic_load(&p->params[P_CUTOFF]);

	/* Update the volume slider */
	update_slider(&p->gui->elements.volume_slider, 20, amp, 1.0f);

	/* Update ADSR envelope sliders */
	update_slider(&p->gui->elements.adsr_sliders[0], 20, attack, 2.0f);
	update_slider(&p->gui->elements.adsr_sliders[1], 20, decay, 2.0f);
	update_slider(&p->gui->elements.adsr_sliders[2], 20, sustain, 1.0f);
	update_slider(&p->gui->elements.adsr_sliders[3], 20, release, 2.0f);

	/* Update filter parameters sliders */
	update_slider(&p->gui->elements.filter_adsr_sliders[0], 20, f_attack, 2.0f);
	update_slider(&p->gui->elements.filter_adsr_sliders[1], 20, f_decay, 2.0f);
	update_slider(&p->gui->elements.filter_adsr_sliders[2], 20, f_sustain, 1.0f);
	update_slider(&p->gui->elements.filter_adsr_sliders[3], 20, f_release, 2.0f);
	update_slider(&p->gui->elements.cutoff_slider, 20, cutoff, 1.0f);
}

/* Draw the drop down menus (enums) */
static void draw_waveforms_menus(synth_plugin_t *plugin)
{
	menu_t waveforms = plugin->gui->elements.waveforms;
	if (waveforms.entries_on)
	{
		for (int i = 0; i < 4; i++)
		{
			int text_x = waveforms.entries[i].rec.left + 10;
			int text_y = waveforms.entries[i].rec.top + 10;

			plugin_paint_rec(plugin->gui->bits, waveforms.entries[i].rec);
			plugin_paint_text(
				plugin->gui->bits,
				text_x, text_y, 
				waveforms.entries[i].name,
				10, BLACK);
		}
	}
	else 
	{
		plugin_paint_rec(plugin->gui->bits, waveforms.base_rec);
		int text_x = waveforms.base_rec.left + 10;
		int text_y = waveforms.base_rec.top + 10;

		plugin_paint_rec(plugin->gui->bits, waveforms.base_rec);
		plugin_paint_text(
			plugin->gui->bits,
			text_x, text_y, 
			waveforms.selected.name,
			10, BLACK);
	}
}

void plugin_paint(synth_plugin_t *plugin, uint32_t *bits) 
{
	rectangle_t background = 
	{
		.left = 0, .right = GUI_WIDTH,
		.top = 0, .bottom = GUI_HEIGHT,
		.border_color = BLACK, .fill_color = BLACK
	};
	
	plugin_paint_rec(bits, background);
	
	update_sliders(plugin);

	/* Painting amplification slider */
	if (__font.loaded)
		plugin_paint_text(plugin->gui->bits, 400, 300, "Volume", 10, GRAY);
	plugin_paint_slider(bits, plugin->gui->elements.volume_slider);

	/* Painting ADSR sliders */
	plugin_paint_slider(bits, plugin->gui->elements.adsr_sliders[0]);
	plugin_paint_slider(bits, plugin->gui->elements.adsr_sliders[1]);
	plugin_paint_slider(bits, plugin->gui->elements.adsr_sliders[2]);
	plugin_paint_slider(bits, plugin->gui->elements.adsr_sliders[3]);

	/* Painting filter ADSR sliders and cutoff */
	plugin_paint_slider(bits, plugin->gui->elements.filter_adsr_sliders[0]);
	plugin_paint_slider(bits, plugin->gui->elements.filter_adsr_sliders[1]);
	plugin_paint_slider(bits, plugin->gui->elements.filter_adsr_sliders[2]);
	plugin_paint_slider(bits, plugin->gui->elements.filter_adsr_sliders[3]);
	plugin_paint_slider(bits, plugin->gui->elements.cutoff_slider);

	paint_checkbox(bits, plugin->gui->elements.filter_env_on_box);

	draw_waveforms_menus(plugin);

	draw_piano_keyboard(plugin);
}

/* MOUSE GESTURES */

/* Mouse drag function, used for sliders */
void plugin_process_mouse_drag(synth_plugin_t *plugin, int x, int y)
{
	(void)y;
	if (plugin->mouse.mouse_dragging)
	{
		/* Rectangle data */
		uint32_t id = plugin->mouse.drag_param_id;
		rectangle_t slider = get_slider_rec(plugin->gui->elements, id);
		const uint32_t handle_width = 20;

		/* Calculate the new value from the drag */
		float travel = (float)((slider.right - slider.left) - handle_width);
		float new_val = (x - (float)slider.left) / travel;
		if (new_val < 0.0f) new_val = 0.0f;
		if (new_val > 1.0f) new_val = 1.0f;

		/* Double the value if slider is 0.0 to 2.0 range */
		bool is_2_range =
			id == P_ATTACK || id == P_DECAY || id == P_RELEASE ||
			id == P_FILTER_ATTACK || id == P_FILTER_DECAY || id == P_FILTER_RELEASE;
		new_val *= is_2_range ? 2.0f : 1.0f;

		atomic_store(&plugin->params[plugin->mouse.drag_param_id], new_val);
		atomic_store(&plugin->params_dirty[plugin->mouse.drag_param_id], true);

		if (plugin->host_params && plugin->host_params->request_flush)
			plugin->host_params->request_flush(plugin->host);
	}
}

/* Mouse press handling, starting drag if we are on a slider */
void plugin_process_mouse_press(synth_plugin_t *plugin, int x, int y)
{
	uint32_t param_id = get_param_gui(plugin->gui->elements, x, y);
	if (param_id >= 0 && param_id < P_COUNT)
	{
		if (param_id == P_WAVE_A && 
			!plugin->gui->elements.waveforms.entries_on)
		{
			plugin->mouse.mouse_dragging = false;
			plugin->gui->elements.waveforms.entries_on = true;
		}
		else if (param_id == P_WAVE_A)
		{
			plugin->mouse.mouse_dragging = false;
			rectangle_t sine =
				plugin->gui->elements.waveforms.entries[0].rec;
			rectangle_t square =
				plugin->gui->elements.waveforms.entries[1].rec;
			rectangle_t triangle =
				plugin->gui->elements.waveforms.entries[2].rec;
			rectangle_t sawtooth =
				plugin->gui->elements.waveforms.entries[3].rec;

			uint8_t wave = SINE_WAVE;

			if (IN_REC(x, y, square))
				wave = SQUARE_WAVE;
			else if (IN_REC(x, y, triangle))
				wave = TRIANGLE_WAVE;
			else if (IN_REC(x, y, sawtooth))
				wave = SAWTOOTH_WAVE;

			atomic_store(&plugin->params[P_WAVE_A], wave);
			atomic_store(&plugin->params_dirty[P_WAVE_A], true);
			atomic_store(&plugin->params[P_WAVE_B], wave);
			atomic_store(&plugin->params_dirty[P_WAVE_B], true);
			atomic_store(&plugin->params[P_WAVE_C], wave);
			atomic_store(&plugin->params_dirty[P_WAVE_C], true);

			plugin->gui->elements.waveforms.entries_on = false;
			plugin->gui->elements.waveforms.selected =
					plugin->gui->elements.waveforms.entries[wave];
		}
		else if (PARAM_IS_SLIDER(param_id))
		{
			plugin->mouse.mouse_dragging = true;
			plugin->mouse.drag_param_id = param_id;
			plugin->mouse.mouse_drag_og_x = x;
			plugin->mouse.mouse_drag_og_y = y;
			plugin->mouse.drag_param_og_val = atomic_load(&plugin->params[plugin->mouse.drag_param_id]);
			atomic_store(&plugin->gestures_start[plugin->mouse.drag_param_id], true);
		}
		else if (PARAM_IS_CHECKBOX(param_id))
		{
			plugin->mouse.mouse_dragging = false;
			bool param = atomic_load(&plugin->params[param_id]);
			atomic_store(&plugin->params_dirty[param_id], true);
			atomic_store(&plugin->params[param_id], !param);
			plugin->gui->elements.filter_env_on_box.param_value = !param;
		}

		if (plugin->host_params && plugin->host_params->request_flush)
			plugin->host_params->request_flush(plugin->host);
	}
	else if (param_id > P_COUNT)
	{
		if (plugin->gui->elements.waveforms.entries_on)
		{
			plugin->gui->elements.waveforms.entries_on = false;
		}
	}
}

/* Mouse release handling */
void plugin_process_mouse_release(synth_plugin_t *plugin)
{
	if (plugin->mouse.mouse_dragging)
	{
		atomic_store(&plugin->gestures_end[plugin->mouse.drag_param_id], true);
		if (plugin->host_params && plugin->host_params->request_flush)
			plugin->host_params->request_flush(plugin->host);
		plugin->mouse.mouse_dragging = false;
	}
}

/* EXTENSIONS FUNCTIONS */

/* Check wether current API is supported */
bool is_api_supported(
	const clap_plugin_t *plugin, 
	const char *api, 
	bool is_floating)
{
	return !strcmp(api, GUI_API) && !is_floating;
}

/* Get the prefered API */
bool get_prefered_api(
	const clap_plugin_t *plugin, 
	const char **api, 
	bool *is_floating)
{
	(void)plugin;
	*api = GUI_API;
	*is_floating = false;
	return true;
}

/* Create the GUI with the OS specific creation function */
bool create(const clap_plugin_t *plugin, const char *api, bool is_floating)
{
	if (!is_api_supported(plugin, api, is_floating))
		return false;
	gui_create(plugin->plugin_data);
	return true;
}

void destroy(const clap_plugin_t *plugin)
{
	gui_destroy((synth_plugin_t *)plugin->plugin_data);
}

bool set_scale(const clap_plugin_t *plugin, double scale)
{
	(void)plugin; (void)scale;
	return false;
}

bool get_size(
	const clap_plugin_t *plugin, 
	uint32_t *w, uint32_t *h)
{
	(void)plugin;
	*w = GUI_WIDTH;
	*h = GUI_HEIGHT;
	return true;
}

bool can_resize(const clap_plugin_t *plugin)
{
	(void)plugin;
	return false;
}

bool get_resize_hints(
	const clap_plugin_t *plugin, 
	clap_gui_resize_hints_t *hints)
{
	(void)plugin; (void)hints;
	return false;
}

bool adjust_size(
	const clap_plugin_t *plugin, 
	uint32_t *w, uint32_t *h)
{
	return get_size(plugin, w, h);
}

bool set_size(
	const clap_plugin_t *plugin, 
	uint32_t w, uint32_t h)
{
	(void)plugin; (void)w; (void)h; 
	return true;
}

bool set_parent(
	const clap_plugin_t *plugin, 
	const clap_window_t *window)
{
	gui_set_parent((synth_plugin_t *)plugin->plugin_data, window);
	return true;
}

bool set_transient(
	const clap_plugin_t *plugin, 
	const clap_window_t *window)
{
	(void)plugin; (void)window;
	return false;
}

void suggest_title(const clap_plugin_t *plugin, const char *title) { }

bool show(const clap_plugin_t *plugin)
{
	gui_set_visible((synth_plugin_t *)plugin->plugin_data, true);
	return true;
}

bool hide(const clap_plugin_t *plugin)
{
	gui_set_visible((synth_plugin_t *)plugin->plugin_data, false);
	return true;
}

/* CLAP GUI extension */
const clap_plugin_gui_t gui_ext =
{
	.is_api_supported = is_api_supported,
	.get_preferred_api = get_prefered_api,
	.create = create,
	.destroy = destroy,
	.set_scale = set_scale,
	.get_size = get_size,
	.can_resize = can_resize,
	.get_resize_hints = get_resize_hints,
	.adjust_size = adjust_size,
	.set_size = set_size,
	.set_parent = set_parent,
	.set_transient = set_transient,
	.suggest_title = suggest_title,
	.show = show,
	.hide = hide
};

#endif 
