#ifndef __CLAP_PLUGIN_H__
#define __CLAP_PLUGIN_H__

#ifdef __CLAP__

#ifdef _WIN32
#include <windows.h>

typedef HANDLE mutex;
#define mutex_acquire(mutex) WaitForSingleObject(mutex, INFINITE)
#define mutex_release(mutex) ReleaseMutex(mutex)
#define mutex_init(mutex) (mutex = CreateMutex(NULL, FALSE, NULL))
#define mutex_destroy(mutex) CloseHandle(mutex)

#elif defined(__linux__)

#include <pthread.h>
#define mutex_acquire(mutex) pthread_mutex_lock(&(mutex))
#define mutex_release(mutex) pthread_mutex_unlock(&(mutex))
#define mutex_init(mutex) pthread_mutex_init(&(mutex), NULL)
#define mutex_destroy(mutex) pthread_mutex_destroy(&(mutex))

#endif

#include "lib_clap/include/clap/clap.h"
#include "clap/clap_params.h"
#include "clap/gui/clap_gui.h"
#include "core/synth.h"
#include "defs.h"

#include <stdatomic.h>

extern const char *__features[];
extern const clap_plugin_descriptor_t __descriptor;

/* Synth CLAP plugin structure */
typedef struct synth_plugin_s
{
	/* CLAP Variables */
	clap_plugin_t plugin;
	const clap_host_t *host;
	double sample_rate;
	
	/* Synthesizer*/
	synth_t synth;

	/* Parameters */
	_Atomic float params[P_COUNT];
	atomic_bool params_dirty[P_COUNT];
	atomic_bool gestures_start[P_COUNT], gestures_end[P_COUNT];
	_Atomic int atomic_notes[VOICES];
	const clap_host_params_t *host_params;
	
	/* Graphical User Interface */
	clap_gui_t *gui;
	mouse_t mouse;
	const clap_host_posix_fd_support_t
		*host_POSIX_support;

	/* Timer support */
	const clap_host_timer_support_t *host_timer_support;
	clap_id timer_id;
} synth_plugin_t;

void process_event(
	synth_plugin_t *p,
	const clap_event_header_t *hdr);

clap_process_status plugin_process(
	const clap_plugin_t *plugin,
	const clap_process_t *process);

/* Plugin functions */
bool plugin_init(const clap_plugin_t *plugin);
void plugin_destroy(const clap_plugin_t *plugin);
bool plugin_activate(
	const clap_plugin_t *plugin, 
	double sample_rate,
	uint32_t min_frames, 
	uint32_t max_frames);
void plugin_deactivate(const clap_plugin_t *plugin);
bool plugin_start_processing(const clap_plugin_t *plugin);
void plugin_stop_processing(const clap_plugin_t *plugin);
void plugin_reset(const clap_plugin_t *plugin);
void plugin_on_main_thread(const clap_plugin_t *plugin);
const void *plugin_get_extension(
	const clap_plugin_t *plugin, const char *id);

#endif /* __CLAP__ */
#endif /* __CLAP_PLUGIN_H__ */