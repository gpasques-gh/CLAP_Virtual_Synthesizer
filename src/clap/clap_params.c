#ifdef __CLAP__

#include "clap/clap_plugin.h"
#include "clap/clap_params.h"
#include "defs.h"

const param_desc_t PARAMS[P_COUNT] =
{
	PARAM_VOLUME,
	PARAM_WAVE_A,
	PARAM_WAVE_B,
	PARAM_WAVE_C,
	PARAM_DETUNE,
	PARAM_ATTACK,
	PARAM_DECAY,
	PARAM_SUSTAIN,
	PARAM_RELEASE,
	PARAM_CUTOFF,
	PARAM_FILTER_ATTACK,
	PARAM_FILTER_DECAY,
	PARAM_FILTER_SUSTAIN,
	PARAM_FILTER_RELEASE,
	PARAM_FILTER_ENV_ON,
};

/* Change the oscillators waveforms from the DAW */
static void __apply_wave_change_to_osc(synth_t *synth, int osc, int wave)
{
	if (osc < 0 || osc > 2 || wave < SINE_WAVE || wave > SAWTOOTH_WAVE)
		return;
	for (int v = 0; v < VOICES; v++)
		synth->voices[v].oscillators[osc].wave = wave;
}

/* Change the ADSR envelope parameters from the DAW */
static void __apply_adsr_change(synth_t *synth, int param, float value)
{
	if (param < 0 || param > 4)
		return;
	
	for (int v = 0; v < VOICES; v++)
	{
		switch (param)
		{
		case 0: /* Attack */
			synth->voices[v].adsr.attack = value;
			break;
		case 1: /* Decay */
			synth->voices[v].adsr.decay = value;
			break;
		case 2: /* Sustain */
			synth->voices[v].adsr.sustain = value;
			break;
		case 3: /* Release */
			synth->voices[v].adsr.release = value;
			break;
		}
	}
}

void apply_param_to_engine(
	synth_plugin_t *p, 
	clap_id id, 
	float value)
{
	switch(id)
	{
	case P_VOLUME: 
		p->synth.amp = (float)value; 
		break;
	case P_DETUNE: 
		p->synth.detune = (float)value; 
		apply_detune_change(&p->synth);
		break;
	case P_WAVE_A:
		__apply_wave_change_to_osc(&p->synth, 0, (int)value);
		break;
	case P_WAVE_B:
		__apply_wave_change_to_osc(&p->synth, 1, (int)value);
		break;
	case P_WAVE_C:
		__apply_wave_change_to_osc(&p->synth, 2, (int)value);
		break;
	case P_ATTACK:
		__apply_adsr_change(&p->synth, 0, (float)value);
		break;
	case P_DECAY:
		__apply_adsr_change(&p->synth, 1, (float)value);
		break;
	case P_SUSTAIN:
		__apply_adsr_change(&p->synth, 2, (float)value);
		break;
	case P_RELEASE:
		__apply_adsr_change(&p->synth, 3, (float)value);
		break;
	case P_CUTOFF:
		p->synth.filter.cutoff = (float)value;
		break;
	case P_FILTER_ATTACK:
		p->synth.filter.adsr.attack = (float)value;
		break;
	case P_FILTER_DECAY:
		p->synth.filter.adsr.decay = (float)value;
		break;
	case P_FILTER_SUSTAIN:
		p->synth.filter.adsr.sustain = (float)value;
		break;
	case P_FILTER_RELEASE:
		p->synth.filter.adsr.release = (float)value;
		break;
	case P_FILTER_ENV_ON:
		p->synth.filter.env = (bool)(int)(value);
		break;
	default:
		break;
	}
}

void flush_gui_params(synth_plugin_t *p, clap_output_events_t *out)
{
	for (clap_id id = 0; id < P_COUNT; id++)
	{
		if (!atomic_exchange(&p->params_dirty[id], false))
			continue;
		
		float value = atomic_load(&p->params[id]);
		apply_param_to_engine(p, id, value);

		clap_event_param_value_t ev = {0};
		ev.header.size = sizeof(ev);
		ev.header.time = 0;
		ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
		ev.header.type = CLAP_EVENT_PARAM_VALUE;
		ev.param_id = id;
		ev.cookie = NULL;
		ev.note_id = -1;
		ev.port_index = -1;
		ev.channel = -1;
		ev.key = -1;
		ev.value = value;
		out->try_push(out, &ev.header);
	}
}

const param_desc_t *param_desc_from_id(clap_id id)
{
	return id < P_COUNT ? &PARAMS[id] : NULL;
}

static bool valid_param(clap_id id)
{
	return id < P_COUNT;
}

static uint32_t params_count(const clap_plugin_t *plugin)
{
	(void)plugin;
	return P_COUNT;
}

static bool params_get_info(
	const clap_plugin_t *plugin,
	uint32_t index,
	clap_param_info_t *info)
{
	(void)plugin;

	if (index >= P_COUNT || !info)
		return false;

	const param_desc_t *src = &PARAMS[index];
	memset(info, 0, sizeof(*info));

	info->id = src->id;
	info->flags = src->flags;
	info->cookie = NULL;
	snprintf(info->name, sizeof(info->name), "%s", src->name);
	snprintf(info->module, sizeof(info->module), "%s", src->module);
	info->min_value = src->min;
	info->max_value = src->max;
	info->default_value = src->def;
	return true;
}

static bool params_get_value(
	const clap_plugin_t *plugin,
	clap_id id,
	double *out_value)
{
	if (!valid_param(id) || !out_value)
		return false;

	synth_plugin_t *p = plugin->plugin_data;
	*out_value = atomic_load(&p->params[id]);
	return true;
}

/* Get the parameter text from its ID */
static bool params_value_to_text(
	const clap_plugin_t *plugin,
	clap_id id,
	double value,
	char *out,
	uint32_t capacity)
{
	(void)plugin;

	if (!valid_param(id) || !out || capacity == 0)
		return false;
	
	if (id >= P_WAVE_A && id <= P_WAVE_C)
		snprintf(out, capacity, "%s", get_wave_name((int)value));
	else if (id == P_ATTACK || id == P_DECAY || id == P_RELEASE)
		snprintf(out, capacity, "%.3f s", value);
	else if (id == P_FILTER_ENV_ON)
		if ((int)value)
			snprintf(out, capacity, "%s", "Filter Env ON");
		else
			snprintf(out, capacity, "%s", "Filter Env OFF");
	else
		snprintf(out, capacity, "%.2f", value);
	
	return true;
}

/* Get the parameter value from its text */
static bool params_text_to_value(
	const clap_plugin_t *plugin,
	clap_id id,
	const char *text,
	double *out_value)
{
	(void)plugin;

	if (!valid_param(id) || !text || !out_value)
		return false;

	/* Waveforms */
	if (!strcmp(text, "Sine"))     { *out_value = SINE_WAVE; return true; }
    if (!strcmp(text, "Square"))   { *out_value = SQUARE_WAVE; return true; }
    if (!strcmp(text, "Triangle")) { *out_value = TRIANGLE_WAVE; return true; }
    if (!strcmp(text, "Sawtooth")) { *out_value = SAWTOOTH_WAVE; return true; }
	if (!strcmp(text, "Filter Env ON")) { *out_value = 1; return true; }
	if (!strcmp(text, "Filter Env OFF")) { *out_value = 0; return true; }
	
	/* Parse the value from string to double */
	char *end = NULL;
	double value = strtod(text, &end);
	if (end == text)
		return false;

	/* Clamping */
	if (value < PARAMS[id].min) value = PARAMS[id].min;
	if (value > PARAMS[id].max) value = PARAMS[id].max;

	*out_value = value;
	return true;
}

/* Send the parameters to the audio thread */
static void params_flush(
	const clap_plugin_t *plugin,
	const clap_input_events_t *in,
	const clap_output_events_t *out)
{
	(void)out;

	synth_plugin_t *p = plugin->plugin_data;
	uint32_t count = in->size(in);

	for (uint32_t i = 0; i < count; i++)
	{
		const clap_event_header_t *event = in->get(in, i);
		if (event->space_id == CLAP_CORE_EVENT_SPACE_ID && 
			event->type == CLAP_EVENT_PARAM_VALUE)
				process_event(p, event);
	}

	flush_gui_params(p, out);
}

/* Parameters extension */
const clap_plugin_params_t params_ext =
{
	.count = params_count,
	.get_info = params_get_info,
	.get_value = params_get_value,
	.value_to_text = params_value_to_text,
	.text_to_value = params_text_to_value,
	.flush = params_flush,
};

#endif 