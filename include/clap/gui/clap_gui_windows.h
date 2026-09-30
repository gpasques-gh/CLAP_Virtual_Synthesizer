#ifdef __CLAP__
#ifdef _WIN32

#ifndef __CLAP_GUI_WIN32__
#define __CLAP_GUI_WIN32__

#define GUI_API CLAP_WINDOW_API_WIN32

#include <stdint.h>
#include <windows.h>
#include <windowsx.h>

#include "clap/gui/clap_gui.h"

typedef struct synth_plugin_s synth_plugin_t;

typedef struct
{
    HWND window;
    uint32_t *bits;
    gui_elements_t elements;
} clap_gui_t;

#include "clap/clap_plugin.h"

void gui_create(synth_plugin_t *plugin);
void gui_destroy(synth_plugin_t *plugin);
void gui_on_POSIX_fd(synth_plugin_t *plugin);

#define gui_set_parent(plugin, parent) SetParent((plugin)->gui->window, (HWND)(parent)->win32)
#define gui_set_visible(plugin, visible) ShowWindow((plugin)->gui->window, (visible) ? SW_SHOW : SW_HIDE)

#endif
#endif
#endif
