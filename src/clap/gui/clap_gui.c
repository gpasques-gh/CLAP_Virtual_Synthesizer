#ifdef __CLAP__

#include <string.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* MACROS */
#define IN_REC(x, y, rec) ((uint32_t)x >= (rec.left) && (uint32_t)x < (rec.right) && (uint32_t)y >= (rec.top) && (uint32_t)y < (rec.bottom))
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
	if (x < 0 || y < 0 || x >= GUI_WIDTH || y >= GUI_HEIGHT || alpha == 0)
		return;

	/* Get source and destination RGBs */
	uint32_t *dst = &bits[y * GUI_WIDTH + x];
	uint8_t sr = (uint8_t)(color >> 16);
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

static uint32_t get_text_width(const char *text, float px_size)
{
	if (!__font.loaded || !text)
		return 0;

	float scale = stbtt_ScaleForPixelHeight(&__font.info, px_size);

	int width = 0;

	for (const unsigned char *p = (const unsigned char *)text; *p; p++)
	{
		int advance_width;
		int left_side_bearing;

		stbtt_GetCodepointHMetrics(
			&__font.info,
			*p,
			&advance_width,
			&left_side_bearing);

		width += (int)(advance_width * scale);
	}

	return (uint32_t)width;
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
						(int)xpos + xoff + i,
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
			bits[y * GUI_WIDTH + x] = (y <= rec.top + rec.border_width - 1 ||
									   y >= rec.bottom - rec.border_width ||
									   x <= rec.left + rec.border_width - 1 ||
									   x >= rec.right - rec.border_width)
										  ? rec.border_color
										  : rec.fill_color;
		}
	}
}

/* PIANO VISUALIZER*/

/* Render the white keys onto the bitmap */
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
				.fill_color = WHITE,
				.border_width = 1};
		plugin_paint_rec(bits, key);
	}
}

/* Render the black keys onto the bitmap */
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
						.fill_color = BLACK,
						.border_width = 1};
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

/* Render a key onto the bitmap with the given MIDI note */
/* TODO: fix integer overflow for notes bigger than 127 */
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
	key.border_width = 1;

	plugin_paint_rec(bits, key);
}

int is_black_key(int midi_note)
{
	int note = midi_note % 12;
	return (note == 1 || note == 3 || note == 6 || note == 8 || note == 10);
}

/* Paint the piano keyboard to the bitmap */
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
	uint32_t x, uint32_t y)
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

	/* Filter envelope on checkbox */
	rectangle_t f_env_on =
		elements.filter_env_on_box.rec;

	rectangle_t detune =
		elements.detune_slider.rec_value;

	/* Return the parameter ID from XY position */
	if (elements.wave_a.entries_on)
	{
		if (IN_REC(x, y, elements.wave_a.entries[0].rec) ||
			IN_REC(x, y, elements.wave_a.entries[1].rec) ||
			IN_REC(x, y, elements.wave_a.entries[2].rec) ||
			IN_REC(x, y, elements.wave_a.entries[3].rec))
			return P_WAVE_A;
	}
	if (IN_REC(x, y, elements.wave_a.base_rec))
	{
		return P_WAVE_A;
	}

	if (elements.wave_b.entries_on)
	{
		if (IN_REC(x, y, elements.wave_b.entries[0].rec) ||
			IN_REC(x, y, elements.wave_b.entries[1].rec) ||
			IN_REC(x, y, elements.wave_b.entries[2].rec) ||
			IN_REC(x, y, elements.wave_b.entries[3].rec))
			return P_WAVE_B;
	}
	if (IN_REC(x, y, elements.wave_b.base_rec))
	{
		return P_WAVE_B;
	}

	if (elements.wave_c.entries_on)
	{
		if (IN_REC(x, y, elements.wave_c.entries[0].rec) ||
			IN_REC(x, y, elements.wave_c.entries[1].rec) ||
			IN_REC(x, y, elements.wave_c.entries[2].rec) ||
			IN_REC(x, y, elements.wave_c.entries[3].rec))
			return P_WAVE_C;
	}
	if (IN_REC(x, y, elements.wave_c.base_rec))
	{
		return P_WAVE_C;
	}

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
	if (IN_REC(x, y, detune))
		return P_DETUNE;

	/* If we are not on any square, send P_COUNT */
	return P_COUNT;
}

/* Paint a checkbox onto the bitmap */
static void paint_checkbox(uint32_t *bits, checkbox_t box)
{
	plugin_paint_rec(bits, box.rec);
	if (box.param_value)
	{
		rectangle_t inner_rec =
			{
				.left = box.rec.left + 5,
				.right = box.rec.right - 5,
				.top = box.rec.top + 5,
				.bottom = box.rec.bottom - 5,
				.fill_color = WHITE,
				.border_color = WHITE};

		plugin_paint_rec(bits, inner_rec);
		uint32_t txt_w = get_text_width("Filter env ON", 10);
		int txt_x = box.rec.left + (box.rec.right - box.rec.left) / 2 - txt_w / 2;
		int txt_y = box.rec.top - 10;
		plugin_paint_text(bits, txt_x, txt_y, "Filter env ON", 10, BLACK);
	}
	else
	{
		uint32_t txt_w = get_text_width("Filter env OFF", 10);
		int txt_x = box.rec.left + (box.rec.right - box.rec.left) / 2 - txt_w / 2;
		int txt_y = box.rec.top - 10;
		plugin_paint_text(bits, txt_x, txt_y, "Filter env OFF", 10, BLACK);
	}
}

/* Get slider rectangle from parameter ID */
static rectangle_t get_slider_rec(gui_elements_t elements, uint32_t param_id)
{
	switch (param_id)
	{
	case P_VOLUME:
		return elements.volume_slider.rec;
	case P_ATTACK:
		return elements.adsr_sliders[0].rec;
	case P_DECAY:
		return elements.adsr_sliders[1].rec;
	case P_SUSTAIN:
		return elements.adsr_sliders[2].rec;
	case P_RELEASE:
		return elements.adsr_sliders[3].rec;
	case P_FILTER_ATTACK:
		return elements.filter_adsr_sliders[0].rec;
	case P_FILTER_DECAY:
		return elements.filter_adsr_sliders[1].rec;
	case P_FILTER_SUSTAIN:
		return elements.filter_adsr_sliders[2].rec;
	case P_FILTER_RELEASE:
		return elements.filter_adsr_sliders[3].rec;
	case P_CUTOFF:
		return elements.cutoff_slider.rec;
	case P_DETUNE:
		return elements.detune_slider.rec;
	default:
		return (rectangle_t){0};
	}
}

/* SLIDERS IMPLEMENTATION */

/* Compute the horizontal slider cursor rectangle position */
static rectangle_t compute_horizontal_slider_rec(
	rectangle_t main_rec, uint32_t width,
	float value, float max)
{
	value = value / max;
	if (value < 0.0f)
		value = 0.0f;
	if (value > 1.0f)
		value = 1.0f;

	uint32_t travel = (main_rec.right - main_rec.left) - width;
	return (rectangle_t){
		.top = main_rec.top,
		.bottom = main_rec.bottom,
		.left = main_rec.left + (uint32_t)(travel * value),
		.right = main_rec.left + width + (uint32_t)(travel * value),
		.border_color = BLACK,
		.fill_color = GRAY,
		.border_width = 1};
}

/* Paint a slider and it's text name to the GUI bitmap */
static void plugin_paint_slider_name(
	uint32_t *bits,
	slider_t slider,
	const char *name,
	int txt_px_size)
{
	uint32_t txt_w = get_text_width(name, txt_px_size);
	int txt_x = slider.rec.left + (slider.rec.right - slider.rec.left) / 2 - txt_w / 2;
	int txt_y = slider.rec.top - txt_px_size;
	plugin_paint_text(bits, txt_x, txt_y, name, txt_px_size, BLACK);
	plugin_paint_rec(bits, slider.rec);
	plugin_paint_rec(bits, slider.rec_value);
}

static void create_waveform_menu(
	synth_plugin_t *p,
	menu_t *menu,
	int l, int r,
	int t, int b,
	uint32_t bc,
	uint32_t fc,
	uint32_t param_id)
{
	/* Waveform drop down menu */
	rectangle_t waveform_base_rec = {
		.left = l, .right = r, .top = t, .bottom = b, .border_color = bc, .fill_color = fc, .border_width = 1};

	menu->base_rec = waveform_base_rec;
	menu->entries_on = false;
	menu->param_id = param_id;

	menu->entries[SINE_WAVE].val = SINE_WAVE;
	menu->entries[SINE_WAVE].name = "Sine";
	menu->entries[SINE_WAVE].rec = waveform_base_rec;

	menu->entries[SQUARE_WAVE].val = SQUARE_WAVE;
	menu->entries[SQUARE_WAVE].name = "Square";
	menu->entries[SQUARE_WAVE].rec = waveform_base_rec;
	menu->entries[SQUARE_WAVE].rec.top += 30;
	menu->entries[SQUARE_WAVE].rec.bottom += 30;

	menu->entries[TRIANGLE_WAVE].val = TRIANGLE_WAVE;
	menu->entries[TRIANGLE_WAVE].name = "Triangle";
	menu->entries[TRIANGLE_WAVE].rec = waveform_base_rec;
	menu->entries[TRIANGLE_WAVE].rec.top += 60;
	menu->entries[TRIANGLE_WAVE].rec.bottom += 60;

	menu->entries[SAWTOOTH_WAVE].val = SAWTOOTH_WAVE;
	menu->entries[SAWTOOTH_WAVE].name = "Sawtooth";
	menu->entries[SAWTOOTH_WAVE].rec = waveform_base_rec;
	menu->entries[SAWTOOTH_WAVE].rec.top += 90;
	menu->entries[SAWTOOTH_WAVE].rec.bottom += 90;

	uint8_t selected_wave = atomic_load(&p->params[param_id]);
	menu->selected = menu->entries[selected_wave];
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
	float detune = atomic_load(&plugin->params[P_DETUNE]);
	bool f_env_on = atomic_load(&plugin->params[P_FILTER_ENV_ON]);

	/* Attack slider */
	rectangle_t attack_rec = {
		.left = 24, .right = 153, .top = 24, .bottom = 59, .border_color = BLACK, .fill_color = GRAY, .border_width = 1};
	plugin->gui->elements.adsr_sliders[0].rec = attack_rec;
	plugin->gui->elements.adsr_sliders[0].rec_value =
		compute_horizontal_slider_rec(attack_rec, 20, attack, 2.0f);
	plugin->gui->elements.adsr_sliders[0].param_value = attack;

	/* Decay slider */
	rectangle_t decay_rec = {
		.left = 24, .right = 153, .top = 71, .bottom = 106, .border_color = BLACK, .fill_color = GRAY, .border_width = 1};
	plugin->gui->elements.adsr_sliders[1].rec = decay_rec;
	plugin->gui->elements.adsr_sliders[1].rec_value =
		compute_horizontal_slider_rec(decay_rec, 20, decay, 2.0f);
	plugin->gui->elements.adsr_sliders[1].param_value = decay;

	/* Sustain slider */
	rectangle_t sustain_rec = {
		.left = 165, .right = 294, .top = 24, .bottom = 59, .border_color = BLACK, .fill_color = GRAY, .border_width = 1};
	plugin->gui->elements.adsr_sliders[2].rec = sustain_rec;
	plugin->gui->elements.adsr_sliders[2].rec_value =
		compute_horizontal_slider_rec(sustain_rec, 20, sustain, 1.0f);
	plugin->gui->elements.adsr_sliders[2].param_value = sustain;

	/* Release slider */
	rectangle_t release_rec = {
		.left = 165, .right = 294, .top = 71, .bottom = 106, .border_color = BLACK, .fill_color = GRAY, .border_width = 1};
	plugin->gui->elements.adsr_sliders[3].rec = release_rec;
	plugin->gui->elements.adsr_sliders[3].rec_value =
		compute_horizontal_slider_rec(release_rec, 20, release, 2.0f);
	plugin->gui->elements.adsr_sliders[3].param_value = release;

	/* Filter Attack slider */
	rectangle_t f_attack_rec = {
		.left = 330, .right = 459, .top = 24, .bottom = 59, .border_color = BLACK, .fill_color = GRAY, .border_width = 1};
	plugin->gui->elements.filter_adsr_sliders[0].rec = f_attack_rec;
	plugin->gui->elements.filter_adsr_sliders[0].rec_value =
		compute_horizontal_slider_rec(f_attack_rec, 20, f_attack, 2.0f);
	plugin->gui->elements.filter_adsr_sliders[0].param_value = f_attack;

	/* Filter Decay slider */
	rectangle_t f_decay_rec = {
		.left = 330, .right = 459, .top = 71, .bottom = 106, .border_color = BLACK, .fill_color = GRAY, .border_width = 1};
	plugin->gui->elements.filter_adsr_sliders[1].rec = f_decay_rec;
	plugin->gui->elements.filter_adsr_sliders[1].rec_value =
		compute_horizontal_slider_rec(f_decay_rec, 20, f_decay, 2.0f);
	plugin->gui->elements.filter_adsr_sliders[1].param_value = f_decay;

	/* Filter Sustain slider */
	rectangle_t f_sustain_rec = {
		.left = 471, .right = 600, .top = 24, .bottom = 59, .border_color = BLACK, .fill_color = GRAY, .border_width = 1};
	plugin->gui->elements.filter_adsr_sliders[2].rec = f_sustain_rec;
	plugin->gui->elements.filter_adsr_sliders[2].rec_value =
		compute_horizontal_slider_rec(f_sustain_rec, 20, f_sustain, 1.0f);
	plugin->gui->elements.filter_adsr_sliders[2].param_value = f_sustain;

	/* Filter Release slider */
	rectangle_t f_release_rec = {
		.left = 471, .right = 600, .top = 71, .bottom = 106, .border_color = BLACK, .fill_color = GRAY, .border_width = 1};
	plugin->gui->elements.filter_adsr_sliders[3].rec = f_release_rec;
	plugin->gui->elements.filter_adsr_sliders[3].rec_value =
		compute_horizontal_slider_rec(f_release_rec, 20, f_release, 2.0f);
	plugin->gui->elements.filter_adsr_sliders[3].param_value = f_release;

	/* Amplification slider */
	rectangle_t amp_rec = {
		.left = 330, .right = 459, .top = 142, .bottom = 178, .border_color = BLACK, .fill_color = GRAY, .border_width = 1};
	plugin->gui->elements.volume_slider.rec = amp_rec;
	plugin->gui->elements.volume_slider.rec_value =
		compute_horizontal_slider_rec(amp_rec, 20, amp, 1.0f);
	plugin->gui->elements.volume_slider.param_value = amp;

	rectangle_t detune_rec = {
		.left = 330, .right = 459, .top = 190, .bottom = 226, .border_color = BLACK, .fill_color = GRAY, .border_width = 1};
	plugin->gui->elements.detune_slider.rec = detune_rec;
	plugin->gui->elements.detune_slider.rec_value =
		compute_horizontal_slider_rec(detune_rec, 20, detune, 5.0f);
	plugin->gui->elements.detune_slider.param_value = detune;

	/* Cutoff slider */
	rectangle_t cutoff_rec = {
		.left = 471, .right = 600, .top = 142, .bottom = 178, .border_color = BLACK, .fill_color = GRAY, .border_width = 1};
	plugin->gui->elements.cutoff_slider.rec = cutoff_rec;
	plugin->gui->elements.cutoff_slider.rec_value =
		compute_horizontal_slider_rec(cutoff_rec, 20, cutoff, 1.0f);
	plugin->gui->elements.cutoff_slider.param_value = cutoff;

	rectangle_t env_on_rec = {
		.left = 471, .right = 600, .top = 190, .bottom = 226, .border_color = BLACK, .fill_color = GRAY, .border_width = 1};
	plugin->gui->elements.filter_env_on_box.rec = env_on_rec;
	plugin->gui->elements.filter_env_on_box.param_value = f_env_on;

	create_waveform_menu(
		plugin, &plugin->gui->elements.wave_a,
		24, 106, 166, 202, BLACK, GRAY, P_WAVE_A);

	create_waveform_menu(
		plugin, &plugin->gui->elements.wave_b,
		118, 200, 166, 202, BLACK, GRAY, P_WAVE_B);

	create_waveform_menu(
		plugin, &plugin->gui->elements.wave_c,
		212, 294, 166, 202, BLACK, GRAY, P_WAVE_C);

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
	float detune = atomic_load(&p->params[P_DETUNE]);
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

	update_slider(&p->gui->elements.detune_slider, 20, detune, 5.0f);
}

/* Draw the drop down menus (enums) */
static void draw_waveforms_menu(synth_plugin_t *plugin, uint8_t wave)
{
	menu_t waveforms;
	if (wave == P_WAVE_A)
		waveforms = plugin->gui->elements.wave_a;
	else if (wave == P_WAVE_B)
		waveforms = plugin->gui->elements.wave_b;
	else if (wave == P_WAVE_C)
		waveforms = plugin->gui->elements.wave_c;
	else
		return;

	if (waveforms.entries_on)
	{
		for (int i = 0; i < 4; i++)
		{
			uint32_t txt_w = get_text_width(waveforms.entries[i].name, 10);
			int txt_x = waveforms.base_rec.left + (waveforms.base_rec.right - waveforms.base_rec.left) / 2 - txt_w / 2;
			int txt_y = waveforms.entries[i].rec.top + 10;

			plugin_paint_rec(plugin->gui->bits, waveforms.entries[i].rec);
			plugin_paint_text(
				plugin->gui->bits,
				txt_x, txt_y,
				waveforms.entries[i].name,
				10, BLACK);
		}
	}
	else
	{
		uint32_t txt_w = get_text_width(waveforms.selected.name, 10);
		int txt_x = waveforms.base_rec.left + (waveforms.base_rec.right - waveforms.base_rec.left) / 2 - txt_w / 2;
		int txt_y = waveforms.base_rec.top + 10;

		plugin_paint_rec(plugin->gui->bits, waveforms.base_rec);
		plugin_paint_text(
			plugin->gui->bits,
			txt_x, txt_y,
			waveforms.selected.name,
			10, BLACK);
	}

	if (wave == P_WAVE_A)
	{
		uint32_t txt_w = get_text_width("Osc A", 10);
		int txt_x = waveforms.base_rec.left + (waveforms.base_rec.right - waveforms.base_rec.left) / 2 - txt_w / 2;
		int txt_y = waveforms.base_rec.top - 10;
		plugin_paint_text(plugin->gui->bits,
						  txt_x, txt_y, "Osc A", 10, BLACK);
	}
	else if (wave == P_WAVE_B)
	{
		uint32_t txt_w = get_text_width("Osc B", 10);
		int txt_x = waveforms.base_rec.left + (waveforms.base_rec.right - waveforms.base_rec.left) / 2 - txt_w / 2;
		int txt_y = waveforms.base_rec.top - 10;
		plugin_paint_text(plugin->gui->bits,
						  txt_x, txt_y, "Osc B", 10, BLACK);
	}
	else if (wave == P_WAVE_C)
	{
		uint32_t txt_w = get_text_width("Osc C", 10);
		int txt_x = waveforms.base_rec.left + (waveforms.base_rec.right - waveforms.base_rec.left) / 2 - txt_w / 2;
		int txt_y = waveforms.base_rec.top - 10;
		plugin_paint_text(plugin->gui->bits,
						  txt_x, txt_y, "Osc C", 10, BLACK);
	}
}

void plugin_paint(synth_plugin_t *plugin, uint32_t *bits)
{
	/* Update the sliders cursors positions */
	update_sliders(plugin);

	/* Clear the bitmap */
	rectangle_t background =
		{.left = 0, .right = GUI_WIDTH, .top = 0, .bottom = GUI_HEIGHT, .border_color = BLACK, .fill_color = WHITE, .border_width = 1};
	plugin_paint_rec(bits, background);

	/* Draw the piano visualizer */
	draw_piano_keyboard(plugin);

	/* Painting ADSR sliders */
	rectangle_t adsr_rec =
		{.left = 12, .right = 306, .top = 12, .bottom = 119, .border_color = BLACK, .fill_color = WHITE, .border_width = 1};
	plugin_paint_rec(bits, adsr_rec);
	plugin_paint_slider_name(bits, plugin->gui->elements.adsr_sliders[0], "Attack", 10);
	plugin_paint_slider_name(bits, plugin->gui->elements.adsr_sliders[1], "Decay", 10);
	plugin_paint_slider_name(bits, plugin->gui->elements.adsr_sliders[2], "Sustain", 10);
	plugin_paint_slider_name(bits, plugin->gui->elements.adsr_sliders[3], "Release", 10);

	/* Painting filter ADSR sliders and cutoff */
	rectangle_t filter_adsr_rec =
		{.left = 318, .right = 612, .top = 12, .bottom = 119, .border_color = BLACK, .fill_color = WHITE, .border_width = 1};
	plugin_paint_rec(bits, filter_adsr_rec);
	plugin_paint_slider_name(bits, plugin->gui->elements.filter_adsr_sliders[0], "Attack", 10);
	plugin_paint_slider_name(bits, plugin->gui->elements.filter_adsr_sliders[1], "Decay", 10);
	plugin_paint_slider_name(bits, plugin->gui->elements.filter_adsr_sliders[2], "Sustain", 10);
	plugin_paint_slider_name(bits, plugin->gui->elements.filter_adsr_sliders[3], "Release", 10);

	/* Miscellanous synthesizer parameters */
	rectangle_t params_rec =
		{.left = 318, .right = 612, .top = 131, .bottom = 238, .border_color = BLACK, .fill_color = WHITE, .border_width = 1};
	plugin_paint_rec(bits, params_rec);
	plugin_paint_slider_name(bits, plugin->gui->elements.volume_slider, "Volume", 10);
	plugin_paint_slider_name(bits, plugin->gui->elements.detune_slider, "Detune", 10);
	plugin_paint_slider_name(bits, plugin->gui->elements.cutoff_slider, "Cutoff", 10);
	paint_checkbox(bits, plugin->gui->elements.filter_env_on_box);

	/* Waveforms menus */
	rectangle_t wave_rec =
		{.left = 12, .right = 306, .top = 131, .bottom = 238, .border_color = BLACK, .fill_color = WHITE, .border_width = 1};
	plugin_paint_rec(bits, wave_rec);
	draw_waveforms_menu(plugin, P_WAVE_A);
	draw_waveforms_menu(plugin, P_WAVE_B);
	draw_waveforms_menu(plugin, P_WAVE_C);
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
		if (new_val < 0.0f)
			new_val = 0.0f;
		if (new_val > 1.0f)
			new_val = 1.0f;

		/* Double the value if slider is 0.0 to 2.0 range */
		bool is_2_range =
			id == P_ATTACK || id == P_DECAY || id == P_RELEASE ||
			id == P_FILTER_ATTACK || id == P_FILTER_DECAY || id == P_FILTER_RELEASE;

		if (is_2_range)
			new_val *= 2.0f;
		else if (id == P_DETUNE)
			new_val *= 5.0f;

		atomic_store(&plugin->params[plugin->mouse.drag_param_id], new_val);
		atomic_store(&plugin->params_dirty[plugin->mouse.drag_param_id], true);

		if (plugin->host_params && plugin->host_params->request_flush)
			plugin->host_params->request_flush(plugin->host);
	}
}

static void change_waveform(synth_plugin_t *plugin, uint8_t p_wave, int x, int y)
{
	menu_t *waveforms;
	if (p_wave == P_WAVE_A)
		waveforms = &plugin->gui->elements.wave_a;
	else if (p_wave == P_WAVE_B)
		waveforms = &plugin->gui->elements.wave_b;
	else if (p_wave == P_WAVE_C)
		waveforms = &plugin->gui->elements.wave_c;
	else
		return;

	rectangle_t sine =
		waveforms->entries[0].rec;
	rectangle_t square =
		waveforms->entries[1].rec;
	rectangle_t triangle =
		waveforms->entries[2].rec;
	rectangle_t sawtooth =
		waveforms->entries[3].rec;

	uint8_t wave = UINT8_MAX;

	if (IN_REC(x, y, sine))
		wave = SINE_WAVE;
	if (IN_REC(x, y, square))
		wave = SQUARE_WAVE;
	else if (IN_REC(x, y, triangle))
		wave = TRIANGLE_WAVE;
	else if (IN_REC(x, y, sawtooth))
		wave = SAWTOOTH_WAVE;

	if (wave > SAWTOOTH_WAVE)
		return;

	atomic_store(&plugin->params[p_wave], wave);
	atomic_store(&plugin->params_dirty[p_wave], true);

	waveforms->entries_on = false;
	waveforms->selected =
		waveforms->entries[wave];
}

/* Mouse press handling, starting drag if we are on a slider */
void plugin_process_mouse_press(synth_plugin_t *plugin, int x, int y)
{
	uint32_t param_id = get_param_gui(plugin->gui->elements, (uint32_t)x, (uint32_t)y);
	if (param_id < P_COUNT)
	{
		if (param_id == P_WAVE_A &&
			!plugin->gui->elements.wave_a.entries_on)
		{
			plugin->mouse.mouse_dragging = false;
			plugin->gui->elements.wave_a.entries_on = true;
			plugin->gui->elements.wave_b.entries_on = false;
			plugin->gui->elements.wave_c.entries_on = false;
		}
		else if (param_id == P_WAVE_A)
		{
			plugin->mouse.mouse_dragging = false;
			change_waveform(plugin, P_WAVE_A, x, y);
		}
		if (param_id == P_WAVE_B &&
			!plugin->gui->elements.wave_b.entries_on)
		{
			plugin->mouse.mouse_dragging = false;
			plugin->gui->elements.wave_a.entries_on = false;
			plugin->gui->elements.wave_b.entries_on = true;
			plugin->gui->elements.wave_c.entries_on = false;
		}
		else if (param_id == P_WAVE_B)
		{
			plugin->mouse.mouse_dragging = false;
			change_waveform(plugin, P_WAVE_B, x, y);
		}
		if (param_id == P_WAVE_C &&
			!plugin->gui->elements.wave_c.entries_on)
		{
			plugin->mouse.mouse_dragging = false;
			plugin->gui->elements.wave_a.entries_on = false;
			plugin->gui->elements.wave_b.entries_on = false;
			plugin->gui->elements.wave_c.entries_on = true;
		}
		else if (param_id == P_WAVE_C)
		{
			plugin->mouse.mouse_dragging = false;
			change_waveform(plugin, P_WAVE_C, x, y);
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
	else
	{
		if (plugin->gui->elements.wave_a.entries_on)
			plugin->gui->elements.wave_a.entries_on = false;

		if (plugin->gui->elements.wave_b.entries_on)
			plugin->gui->elements.wave_b.entries_on = false;

		if (plugin->gui->elements.wave_c.entries_on)
			plugin->gui->elements.wave_c.entries_on = false;
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
	(void)plugin;
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
	gui_font_free();
}

bool set_scale(const clap_plugin_t *plugin, double scale)
{
	(void)plugin;
	(void)scale;
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
	fprintf(stderr, "can_resize called\n");
	return false;
}

bool get_resize_hints(
	const clap_plugin_t *plugin,
	clap_gui_resize_hints_t *hints)
{
	(void)plugin;
	(void)hints;
	hints->can_resize_horizontally = false;
	hints->can_resize_vertically = false;
	hints->preserve_aspect_ratio = false;
	hints->aspect_ratio_width = GUI_WIDTH;
	hints->aspect_ratio_height = GUI_HEIGHT;
	return false;
}

bool adjust_size(
	const clap_plugin_t *plugin,
	uint32_t *w, uint32_t *h)
{
	(void)plugin;
	*w = GUI_WIDTH;
	*h = GUI_HEIGHT;
	return true;
}

bool set_size(
	const clap_plugin_t *plugin,
	uint32_t w, uint32_t h)
{
	(void)plugin;
	return w == GUI_WIDTH && h == GUI_HEIGHT;
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
	(void)plugin;
	(void)window;
	return false;
}

void suggest_title(const clap_plugin_t *plugin, const char *title)
{
	(void)plugin;
	(void)title;
}

bool show(const clap_plugin_t *plugin)
{
	synth_plugin_t *p = (synth_plugin_t *)plugin->plugin_data;
	gui_set_visible(p, true);
	if (p->host_timer_support && p->host_timer_support->register_timer && p->timer_id != CLAP_INVALID_ID)
		p->host_timer_support->register_timer(p->host, 16, &p->timer_id);
	return true;
}

bool hide(const clap_plugin_t *plugin)
{
	synth_plugin_t *p = (synth_plugin_t *)plugin->plugin_data;
	gui_set_visible(p, false);
	if (p->host_timer_support && p->host_timer_support->unregister_timer && p->timer_id != CLAP_INVALID_ID)
		p->host_timer_support->unregister_timer(p->host, p->timer_id);
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
		.hide = hide};

#endif
