#ifdef __LV2__

#ifndef __LV2_URIS__
#define __LV2_URIS__

#include "lv2/atom/atom.h"
#include "lv2/parameters/parameters.h"
#include "lv2/urid/urid.h"

#define SYNTH_URI "https://github.com/gpasques-gh/CLAP_Virtual_Synthesizer.git"

typedef struct 
{
	/* URIs defined in LV2 specifications */
	LV2_URID atom_Vector;
	LV2_URID atomFloat;
	LV2_URID atom_Int;
	LV2_URID atom_envent_transfer;

	/* URIs defined for this plugin */
	LV2_URID raw_audio;
	LV2_URID channel_id;
	LV2_URID audio_data;
	LV2_URID ui_on;
	LV2_URID ui_off;
	LV2_URID ui_state;
} SynthLV2URIs;

#endif 

#endif 