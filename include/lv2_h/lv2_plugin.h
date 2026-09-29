#ifdef __LV2__

#ifndef __LV2_PLUGIN__
#define __LV2_PLUGIN__

#include "lv2/atom/atom.h"
#include "lv2/atom/forge.h"
#include "lv2/atom/util.h"
#include "lv2/core/lv2.h"
#include "lv2/core/lv2_util.h"
#include "lv2/log/log.h"
#include "lv2/log/logger.h"
#include "lv2/state/state.h"
#include "lv2/urid/urid.h"

#include "lv2_h/lv2_uris.h"
#include "core/synth.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Port indexes */
typedef enum 
{
	SCO_CONTROL = 0, /* Event input */
	SCO_NOTIFY 	= 1, /* Event output */
	SCO_INPUT0 	= 2, /* Audio input 0 */
	SCO_OUTPUT0 = 3, /* Audio output 0 */
	SCO_INPUT1 	= 4, /* Audio input 1 (stereo variant) */
	SCO_OUTPUT1 = 5, /* Audio input 2 (stereo variant)*/
} port_idx_t;

/* Plugin structure for LV2 synth */
typedef struct
{
	/* Port buffers */
	float *input[2];
	float *output[2];
	const LV2_Atom_Sequence *control;
	LV2_Atom_Sequence *notify;

	/* Atom forge and URI mapping */
	LV2_URID_Map *map;
	SynthLV2URIs uris;
	LV2_Atom_Forge forge;
	LV2_Atom_Forge_Frame frame;

	/* Log feature and convenience API */
	LV2_Log_Logger logger;

	/* Instanciation settings */
	uint32_t n_channels;
	double rate;

	/* UI state */
	bool ui_active;
	bool send_settings_to_ui;
	
	/* Synthesizer */
	synth_t synth;
} synth_plugin_t;

#endif 
#endif 