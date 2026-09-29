#ifdef __STANDALONE__

#ifndef XML_H
#define XML_H

#ifdef __WINDOWS__
#include "libxml/parser.h"
#include "libxml/tree.h"
#elif defined(__LINUX__)
#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/tree.h>
#endif

#include "core/synth.h"

/*
 * Saving a preset into an XML file :
 * - ADSR envelope parameters
 * - Filter ADSR envelope parameters and ON/OFF
 * - Filter cutoff
 * - Oscillators waveforms
 * - Detune effect
 * - Amplification
 */
int save_preset(
	synth_t synth,
	float attack, float decay,
	float sustain, float release,
	int wave_a, int wave_b, int wave_c,
	char *preset_filename, bool *saving_preset,
	bool distortion, bool overdrive, 
	float distortion_amount);

/*
 * Load a preset from an XML file to the application :
 * - ADSR envelope parameters
 * - Filter ADSR envelope parameters and ON/OFF
 * - Filter cutoff
 * - Oscillators waveforms
 * - Detune effect
 * - Amplification
 */
int load_preset(
	synth_t *synth,
	float *attack, float *decay,
	float *sustain, float *release,
	int *wave_a, int *wave_b, int *wave_c, 
	bool *distortion, bool *overdrive, 
	float *distortion_amount,
	bool *loading_preset);

int parse_filter(xmlNode *filter_node, 
				synth_t *synth);

int parse_effects(xmlNode *effects_node, synth_t *synth,
		bool *distortion, bool *overdrive, float *distortion_amount);

int parse_oscillators(xmlNode *osc_node, 
	int *wave_a, int *wave_b, int *wave_c);

int parse_lfo(xmlNode *lfo_node, synth_t *synth);

int parse_distortion(xmlNode *distortion_node, 
	bool *distortion, bool *overdrive, float *distortion_amount);

/* Parse an ADSR XML Node whether it's basic ADSR of filter ADSR */
int parse_adsr(
	xmlNode *adsr_root_node,
	synth_t *synth,
	float *attack, float *decay,
	float *sustain, float *release,
	bool filter);

#endif /* HEADER GUARD */
#endif /* __STANDALONE__ */