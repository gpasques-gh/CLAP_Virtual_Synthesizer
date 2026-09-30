#ifdef __linux__

#include <stdlib.h>

#include "clap/clap_plugin.h"
#include "clap/gui/clap_gui_linux.h"

#define INPUTS (SubstructureNotifyMask | ExposureMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask | EnterWindowMask | LeaveWindowMask | ButtonMotionMask | KeymapStateMask | FocusChangeMask | PropertyChangeMask)

void gui_paint(synth_plugin_t *plugin, bool internal)
{
    if (internal)
        plugin_paint(plugin, plugin->gui->bits);
    XPutImage(
        plugin->gui->display,
        plugin->gui->window,
        DefaultGC(plugin->gui->display, 0),
        plugin->gui->image,
        0, 0, 0, 0,
        GUI_WIDTH, GUI_HEIGHT);
}

static void gui_X11_process_event(synth_plugin_t *plugin, XEvent *event)
{
    if (event->type == Expose)
    {
        if (event->xexpose.window == plugin->gui->window)
            gui_paint(plugin, false);
    }
    else if (event->type == MotionNotify)
    {
        if (event->xmotion.window == plugin->gui->window)
            plugin_process_mouse_drag(plugin, event->xmotion.x, event->xmotion.y);
    }
    else if (event->type == ButtonPress)
    {
        if (event->xbutton.window == plugin->gui->window && event->xbutton.button == 1)
        {
            plugin_process_mouse_press(plugin, event->xbutton.x, event->xbutton.y);
            XGrabPointer(
                plugin->gui->display,
                plugin->gui->window,
                False,
                PointerMotionMask | ButtonReleaseMask,
                GrabModeAsync, GrabModeAsync,
                None, None,
                CurrentTime);
        }
    }
    else if (event->type == ButtonRelease)
    {
        if (event->xbutton.window == plugin->gui->window && event->xbutton.button == 1)
        {
            plugin_process_mouse_release(plugin);
            XUngrabPointer(plugin->gui->display, CurrentTime);
        }
    }
}

void gui_create(synth_plugin_t *plugin)
{
    /* Allocate the GUI */
    plugin->gui = (clap_gui_t *)calloc(1, sizeof(clap_gui_t));

    gui_create_elements(plugin);

    /* Open the X11 display and window */
    plugin->gui->display = XOpenDisplay(NULL);
    XSetWindowAttributes attributes = {0};
    plugin->gui->window = XCreateWindow(
        plugin->gui->display,
        DefaultRootWindow(plugin->gui->display),
        0, 0, GUI_WIDTH, GUI_HEIGHT, 0, 0,
        InputOutput, CopyFromParent, CWOverrideRedirect, &attributes);
    XStoreName(plugin->gui->display, plugin->gui->window, __descriptor.name);

    /* Set embed information for the window */
    Atom embed_info_atom = XInternAtom(plugin->gui->display, "_XEMBED_INFO", 0);
    uint32_t embed_info_data[2] = {0 /* version */, 0 /* not mapped */};
    XChangeProperty(
        plugin->gui->display, plugin->gui->window,
        embed_info_atom, embed_info_atom,
        32, PropModeReplace,
        (uint8_t *)embed_info_data, 2);

    /* Set the size of the window */
    XSizeHints *size_hints = XAllocSizeHints();
    size_hints->flags = PMinSize | PMaxSize;
    size_hints->min_width = size_hints->max_width = GUI_WIDTH;
    size_hints->min_height = size_hints->max_height = GUI_HEIGHT;
    XSetWMNormalHints(plugin->gui->display, plugin->gui->window, size_hints);
    XFree(size_hints);

    /* Select the events the window will receive */
    XSelectInput(plugin->gui->display, plugin->gui->window, INPUTS);

    /* Create the bitmap */
    plugin->gui->image = XCreateImage(
        plugin->gui->display,
        DefaultVisual(plugin->gui->display, 0),
        24, ZPixmap, 0, NULL,
        10, 10, 32, 0);
    plugin->gui->bits = (uint32_t *)calloc(1, GUI_WIDTH * GUI_HEIGHT * 4);
    plugin->gui->image->width = GUI_WIDTH;
    plugin->gui->image->height = GUI_HEIGHT;
    plugin->gui->image->bytes_per_line = GUI_WIDTH * 4;
    plugin->gui->image->data = (char *)plugin->gui->bits;

    /* Register the file descripter we'll receive events from */
    if (plugin->host_POSIX_support && plugin->host_POSIX_support->register_fd)
    {
        plugin->host_POSIX_support->register_fd(
            plugin->host,
            ConnectionNumber(plugin->gui->display),
            CLAP_POSIX_FD_READ);
    }
}

void gui_destroy(synth_plugin_t *plugin)
{
    if (plugin->host_POSIX_support && plugin->host_POSIX_support->unregister_fd)
        plugin->host_POSIX_support->unregister_fd(plugin->host, ConnectionNumber(plugin->gui->display));

    /* Free the bitmap, window and display */
    free(plugin->gui->bits);
    plugin->gui->image->data = NULL;
    XDestroyImage(plugin->gui->image);
    XDestroyWindow(plugin->gui->display, plugin->gui->window);
    XCloseDisplay(plugin->gui->display);

    /* Free the GUI structure */
    free(plugin->gui);
    plugin->gui = NULL;
}

void gui_set_parent(synth_plugin_t *plugin, const clap_window_t *window)
{
    XReparentWindow(plugin->gui->display, plugin->gui->window, (Window)window->x11, 0, 0);
    XFlush(plugin->gui->display);
}

void gui_set_visible(synth_plugin_t *plugin, bool visible)
{
    if (visible)
        XMapRaised(plugin->gui->display, plugin->gui->window);
    else
        XUnmapWindow(plugin->gui->display, plugin->gui->window);
    XFlush(plugin->gui->display);
}

void gui_on_POSIX_fd(synth_plugin_t *plugin)
{
    XFlush(plugin->gui->display);

    if (XPending(plugin->gui->display))
    {
        XEvent event;
        XNextEvent(plugin->gui->display, &event);

        while (XPending(plugin->gui->display))
        {
            XEvent event0;
            XNextEvent(plugin->gui->display, &event0);

            if (event.type == MotionNotify && event0.type == MotionNotify)
            {
            }
            else
            {
                gui_X11_process_event(plugin, &event);
                XFlush(plugin->gui->display);
            }

            event = event0;
        }

        gui_X11_process_event(plugin, &event);
        XFlush(plugin->gui->display);
        gui_paint(plugin, true);
    }

    XFlush(plugin->gui->display);
}

#endif