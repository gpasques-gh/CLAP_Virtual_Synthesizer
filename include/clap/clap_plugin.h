#ifdef __CLAP__

#ifndef __CLAP_PLUGIN_H__
#define __CLAP_PLUGIN_H__

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
	const clap_host_params_t *host_params;

	/* Graphical User Interface */
	clap_gui_t *gui;
	mouse_t mouse;
	const clap_host_posix_fd_support_t *host_POSIX_support;
	_Atomic int atomic_notes[VOICES];

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

#endif /* __CLAP_PLUGIN_H__ */
#endif /* __CLAP__ */