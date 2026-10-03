#ifdef _WIN32

#include "lib_clap/include/clap/clap.h"
#include "clap/gui/clap_gui.h"
#include "clap/gui/clap_gui_windows.h"
#include "clap/clap_plugin.h"

static int global_open_gui_count = 0;

/* Paint the GUI */
void gui_paint(synth_plugin_t *plugin, bool internal)
{
	if (internal)
		plugin_paint(plugin, plugin->gui->bits);
	RedrawWindow(plugin->gui->window, 0, 0, RDW_INVALIDATE);
}

/* Window procedure callback */
LRESULT CALLBACK gui_window_procedure(
	HWND window,
	UINT message,
	WPARAM wparam,
	LPARAM lparam)
{
	synth_plugin_t *plugin =
		(synth_plugin_t *)GetWindowLongPtr(window, 0);
	if (!plugin)
		return DefWindowProc(window, message, wparam, lparam);

	switch (message)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT paint;
		HDC dc = BeginPaint(window, &paint);
		BITMAPINFO info =
			{
				.bmiHeader =
					{
						.biSize = sizeof(BITMAPINFOHEADER),
						.biWidth = GUI_WIDTH,
						.biHeight = -GUI_HEIGHT,
						.biPlanes = 1,
						.biBitCount = 32,
						.biCompression = BI_RGB,
					}};
		StretchDIBits(dc, 0, 0, GUI_WIDTH, GUI_HEIGHT, 0, 0, GUI_WIDTH, GUI_HEIGHT, plugin->gui->bits, &info, DIB_RGB_COLORS, SRCCOPY);
		EndPaint(window, &paint);
		break;
	}
	case WM_MOUSEMOVE: /* Mouse move for dragging */
	{
		plugin_process_mouse_drag(plugin, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
		gui_paint(plugin, true);
		break;
	}
	case WM_LBUTTONDOWN: /* Mouse click */
		SetCapture(window);
		plugin_process_mouse_press(plugin, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
		gui_paint(plugin, true);
		break;
	case WM_LBUTTONUP: /* Mouse release */
		ReleaseCapture();
		plugin_process_mouse_release(plugin);
		gui_paint(plugin, true);
		break;
	default:
		return DefWindowProc(window, message, wparam, lparam);
	}

	return 0;
}

/* Win32 window alocating function */
void gui_create(synth_plugin_t *plugin)
{
	plugin->gui = (clap_gui_t *)calloc(1, sizeof(clap_gui_t));
	if (!plugin->gui)
		return;

	gui_create_elements(plugin);

	if (global_open_gui_count == 0)
	{
		WNDCLASS window_class = {0};
		window_class.lpfnWndProc = gui_window_procedure;
		window_class.cbWndExtra = sizeof(synth_plugin_t *);
		window_class.lpszClassName = __descriptor.id;
		window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
		window_class.style = CS_DBLCLKS;
		RegisterClass(&window_class);
	}

	global_open_gui_count++;

	plugin->gui->window = CreateWindow(__descriptor.id, __descriptor.name, WS_CHILDWINDOW | WS_CLIPSIBLINGS, CW_USEDEFAULT, 0, GUI_WIDTH, GUI_HEIGHT, GetDesktopWindow(), NULL, NULL, NULL);
	if (!plugin->gui->window)
		return;
	plugin->gui->bits = (uint32_t *)calloc(1, GUI_WIDTH * GUI_HEIGHT * 4);
	if (!plugin->gui->bits)
		return;
	SetWindowLongPtr(plugin->gui->window, 0, (LONG_PTR)plugin);
	plugin_paint(plugin, plugin->gui->bits);
}

/* Win32 windows destory function */
void gui_destroy(synth_plugin_t *plugin)
{
	DestroyWindow(plugin->gui->window);
	free(plugin->gui->bits);
	free(plugin->gui);
	plugin->gui = NULL;

	global_open_gui_count--;
	if (global_open_gui_count == 0)
		UnregisterClass(__descriptor.id, NULL);
}

/* Empty POSIX FD function */
void gui_on_POSIX_fd(synth_plugin_t *plugin) { (void)plugin; }

#endif
