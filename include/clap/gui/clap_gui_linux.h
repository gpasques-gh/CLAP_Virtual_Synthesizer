#ifdef __CLAP__
#ifdef __linux__

#ifndef __CLAP_GUI_LINUX__
#define __CLAP_GUI_LINUX__

#define GUI_API CLAP_WINDOW_API_X11

#include <stdint.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "clap/gui/clap_gui.h"

typedef struct synth_plugin_s synth_plugin_t;

typedef struct
{
    Display *display;
    Window window;
    XImage *image;
    uint32_t *bits;
    gui_elements_t elements;
} clap_gui_t;

#include "clap/clap_plugin.h"

void gui_create(synth_plugin_t *plugin);
void gui_destroy(synth_plugin_t *plugin);
void gui_set_parent(synth_plugin_t *plugin, const clap_window_t *window);
void gui_set_visible(synth_plugin_t *plugin, bool visible);
void gui_on_POSIX_fd(synth_plugin_t *plugin);

#endif
#endif
#endif
