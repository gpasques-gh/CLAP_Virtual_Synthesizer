#ifdef __CLAP__

#ifndef __CLAP_GUI_H__
#define __CLAP_GUI_H__

#define GUI_WIDTH (780)
#define GUI_HEIGHT (400)

#define GRAY 0xC0C0C0
#define BLACK 0x0000000

#include <stdint.h>
#include <stdbool.h>

typedef struct synth_plugin_s synth_plugin_t;

typedef struct
{
	uint32_t left, right, top, bottom;
	uint32_t border_color, fill_color;
	uint32_t border_width;
} rectangle_t;

/* Slider structure */
/* The modulation will trigger process_events from clap_plugin.c */
typedef struct
{
	rectangle_t rec;
	rectangle_t rec_value;
	clap_id param_id;
	float param_value;
} slider_t;

/* Checkbox structure */
typedef struct
{
	rectangle_t rec;
	clap_id param_id;
	bool param_value;
} checkbox_t;

typedef struct
{
	rectangle_t rec;
	const char *name;
} menu_entry_t;

typedef struct
{
	rectangle_t base_rec;
	menu_entry_t *entries;
	bool entries_on;
	clap_id param_id;
	uint32_t param_value;
} menu_t;

typedef struct 
{
	slider_t volume_slider;
	slider_t adsr_sliders[4];
	
	slider_t cutoff_slider;
	slider_t filter_adsr_sliders[4];
	checkbox_t filter_env_on_box;

	menu_t waveforms;
} gui_elements_t;

typedef struct 
{
	bool mouse_dragging;
	uint32_t mouse_drag_og_x;
	uint32_t mouse_drag_og_y;
	float drag_param_og_val;
	uint32_t drag_param_id;
} mouse_t;

#ifdef __linux__
#include "clap/gui/clap_gui_linux.h"
#elif defined(_WIN32)
#include "clap/gui/clap_gui_windows.h"
#endif

#include "clap/clap_plugin.h"

void gui_create_elements(synth_plugin_t *plugin);
void plugin_paint(synth_plugin_t *plugin, uint32_t *bits);
void plugin_process_mouse_drag(synth_plugin_t *plugin, int x, int y);
void plugin_process_mouse_press(synth_plugin_t *plugin, int x, int y);
void plugin_process_mouse_release(synth_plugin_t *plugin);

#endif /* __CLAP_GUI_H__ */
#endif /* __CLAP__*/