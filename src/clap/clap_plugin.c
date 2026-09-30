#ifdef __CLAP__

#include "lib_clap/include/clap/clap.h"
#include "clap/clap_plugin.h"
#include "clap/clap_audio_ports.h"
#include "clap/clap_note_ports.h"
#include "clap/clap_params.h"
#include "clap/gui/clap_gui.h"

#include "defs.h"
#include "core/synth.h"
#include "core/effects.h"

#define STATE_MAGIC 0x53594E54u /* "SYNT" */
#define STATE_VERSION 1u

/* Clamp the parameter value by its minimum and maximum */
static double clamp_param_value(clap_id id, double value)
{
	const param_desc_t *desc = param_desc_from_id(id);
	if (!desc) return value;
	
	if (value < desc->min) value = desc->min;
	if (value > desc->max) value = desc->max;

	if (desc->flags & CLAP_PARAM_IS_STEPPED)
		value = (double)(int)value;

	return value;
}

/* STATE */
static bool stream_write_all(const clap_ostream_t *s, const void *buf, uint64_t size)
{
	const char *ptr = (const char *)buf;
	while (size > 0)
	{
		int64_t n = s->write(s, ptr, size);
		if (n <= 0) return false;
		ptr += n; size -= (uint64_t)n;
	}
	return true;
}

static bool stream_read_all(const clap_istream_t *s, void *buf, uint64_t size)
{
	char *ptr = (char *)buf;
	while (size > 0)
	{
		int64_t n = s->read(s, ptr, size);
		if (n <= 0) return false;
		ptr += n; size -= (uint64_t)n;
	}
	return true;
}

static bool state_save(const clap_plugin_t *plugin, const clap_ostream_t *stream)
{
	synth_plugin_t *p = (synth_plugin_t *)plugin->plugin_data;

	/* Save the state headers */
	uint32_t header[3] = { STATE_MAGIC, STATE_VERSION, P_COUNT };
	if (!stream_write_all(stream, header, sizeof(header)))
		return false;
	
	/* Save all parameters data */
	for (uint32_t i = 0; i < P_COUNT; i++)
	{
		float value = atomic_load(&p->params[i]);
		if (!stream_write_all(stream, &value, sizeof(value)))
			return false;
	}
	return true;
}

static bool state_load(const clap_plugin_t *plugin, const clap_istream_t *stream)
{
	synth_plugin_t *p = (synth_plugin_t *)plugin->plugin_data;

	/* Read the state headers */
	uint32_t header[3];
	if (!stream_read_all(stream, header, sizeof(header)))
		return false;
	if (header[0] != STATE_MAGIC || header[1] != STATE_VERSION)
		return false;

	/* Tolerate a version with a higher parameter count */
	uint32_t count = header[2];
	if (count > P_COUNT)
		count = P_COUNT;
	
	/* Read all the parameters */
	for (uint32_t i = 0; i < count; i++)
	{
		float value;
		if (!stream_read_all(stream, &value, sizeof(value)))
			return false;

		/* Store the parameter value and make it dirty to flush the GUI */
		value = (float)clamp_param_value(i, value);
		atomic_store(&p->params[i], value);
		atomic_store(&p->params_dirty[i], true);
	}

	/* Request host to flush parameters even if audio thread isn't running */
	if (p->host_params && p->host_params->request_flush)
		p->host_params->request_flush(p->host);

	return true;
}

static const clap_plugin_state_t state_ext =
{
	.save = state_save,
	.load = state_load,
};

static const clap_plugin_audio_ports_t audio_ports_ext =
{
	.count = audio_ports_count,
	.get = audio_ports_get
};

static const clap_plugin_note_ports_t note_ports_ext =
{
	.count = note_ports_count,
	.get = note_ports_get
};

/* Synth CLAP plugin features */
const char *__features[] =
{
	CLAP_PLUGIN_FEATURE_INSTRUMENT,
	CLAP_PLUGIN_FEATURE_SYNTHESIZER,
	NULL
};

/* Synth CLAP plugin descriptors */
const clap_plugin_descriptor_t __descriptor =
{
	.clap_version = CLAP_VERSION_INIT,
	.id = "com.example.midi-synth",
	.name = "Raygui Synth - CLAP Version",
	.vendor = "gpasques-gh",
	.url = "github.com/gpasques-gh/ALSA_raygui_Synthesizer.git",
	.manual_url = "",
	.support_url = "",
	.version = "1.0.0",
	.description = "Minimal CLAP MIDI Synth",
	.features = __features
};

/* Free the synthesizer */
static void synth_free(const clap_plugin_t *plugin)
{
	synth_plugin_t *p = (synth_plugin_t *)plugin->plugin_data;
	if (!p || !p->synth.voices)
		return;

	for (int i = 0; i < VOICES; i++)
	{
		free(p->synth.voices[i].oscillators);
		p->synth.voices[i].oscillators = NULL;
	}

	free(p->synth.voices);
	p->synth.voices = NULL;
}

/* Allocate the synthesizer from the plugin */
static int synth_alocate(const clap_plugin_t *plugin)
{
	synth_plugin_t *p = (synth_plugin_t *)plugin->plugin_data;

	/* Polyphonic Synthesizer */
	p->synth.voices = malloc(sizeof(voice_t) * VOICES);
	if (!p->synth.voices)
		return 1;

	/* Initialize the POSIX and HostParams extensions */
	p->host_params = (const clap_host_params_t *)
		p->host->get_extension(p->host, CLAP_EXT_PARAMS);
	p->host_POSIX_support = (const clap_host_posix_fd_support_t *)
		p->host->get_extension(p->host, CLAP_EXT_POSIX_FD_SUPPORT);
	p->host_timer_support = (const clap_host_timer_support_t *)
		p->host->get_extension(p->host, CLAP_EXT_TIMER_SUPPORT);

	/* Initializing CLAP parameters */
	for (uint32_t i = 0; i < P_COUNT; i++)
	{
		atomic_init(&p->params[i], PARAMS[i].def);
		atomic_init(&p->gestures_start[i], false);
		atomic_init(&p->gestures_end[i], false);
		atomic_init(&p->params_dirty[i], false);
	}

	/* Low-Pass Filter */
	p->synth.filter.cutoff = 0.5;
	p->synth.filter.prev_input = 0.0;
	p->synth.filter.prev_output = 0.0;
	p->synth.filter.env = false;
	p->synth.filter.adsr.attack = 0.0;
	p->synth.filter.adsr.decay = 0.0;
	p->synth.filter.adsr.sustain = 0.0;
	p->synth.filter.adsr.release = 0.0;
	p->synth.filter.adsr.output = 0.0;
	p->synth.filter.adsr.state = ENV_IDLE;
	p->synth.filter.adsr.type = ENV_TYPE_FILTER;

	/* Miscellanous synthesizer parameters */
	p->synth.amp = DEFAULT_AMPLITUDE;
	p->synth.detune = 0.0;
	p->synth.arp = false;
	p->synth.active_arp = 0;
	p->synth.active_arp_float = 1.0;
	p->synth.bpm = 150.0;

	/* Low Frequency Oscillator */
	p->synth.lfo.osc.freq = 0.5;
	p->synth.lfo.osc.phase = 0.0;
	p->synth.lfo.osc.wave = SINE_WAVE;
	p->synth.lfo.mod_param = LFO_OFF;

	/* Create the synthesizer voices */
	for (int i = 0; i < VOICES; i++)
	{
		/* Synthesizer ADSR envelope */
		p->synth.voices[i].adsr.attack = 0.2;
		p->synth.voices[i].adsr.decay = 0.3;
		p->synth.voices[i].adsr.sustain = 0.7;
		p->synth.voices[i].adsr.release = 0.2;
		p->synth.voices[i].adsr.state = ENV_IDLE;
		p->synth.voices[i].adsr.type = ENV_TYPE_SYNTH;
		p->synth.voices[i].adsr.output = 0.0;

		p->synth.voices[i].note = -1;
		p->synth.voices[i].velocity_amp = 0.0;
		p->synth.voices[i].pressed = 0;

		/* Allocating the oscillators */
		p->synth.voices[i].oscillators = malloc(sizeof(osc_t) * 3);
		if (p->synth.voices[i].oscillators == NULL)
			return 1;

		for (int j = 0; j < 3; j++)
		{
			p->synth.voices[i].oscillators[j].freq = 0.0;
			p->synth.voices[i].oscillators[j].phase = 0.0;
		}

		p->synth.voices[i].oscillators[0].wave = SINE_WAVE;
		p->synth.voices[i].oscillators[1].wave = SINE_WAVE;
		p->synth.voices[i].oscillators[2].wave = SINE_WAVE;

		atomic_init(&p->atomic_notes[i], -1);
	}

	return 0;
}

/* Process a given CLAP event */
/* Events can be NOTE_ON, NOTE_OFF, MIDI, or PARAM_VALUE */
void process_event(
	synth_plugin_t *p,
	const clap_event_header_t *hdr)
{
	if (hdr->space_id != CLAP_CORE_EVENT_SPACE_ID)
		return;
	
	switch(hdr->type)
	{
	case CLAP_EVENT_NOTE_ON:
	{
		/* Activate the first free voice */
		const clap_event_note_t *ev = 
			(const clap_event_note_t *)hdr;
		voice_on(&p->synth, ev->key, (int)(ev->velocity * 127.0f));
		break;
	}
	case CLAP_EVENT_NOTE_OFF:
	{
		/* Deactivate the given pressed voice */
		const clap_event_note_t *ev = 
			(const clap_event_note_t *)hdr;
		voice_off(&p->synth, ev->key);
		break;
	}
	case CLAP_EVENT_MIDI:
	{
		const clap_event_midi_t *ev =
			(const clap_event_midi_t *)hdr;
		
		/* Getting the MIDI information from the header */
		uint8_t status = ev->data[0] & PRESSED;
		uint8_t key = ev->data[1];
		uint8_t vel_raw = ev->data[2];

		/* NOTE_ON */
		if (status == 0x90 && vel_raw > 0)
			voice_on(&p->synth, key, vel_raw);
		/* NOTE_OFF */
		else if (status == 0x80 || (status == 0x90 && vel_raw == 0))
			voice_off(&p->synth, key);
		break;
	}
	case CLAP_EVENT_PARAM_VALUE:
	{
		const clap_event_param_value_t *ev =
			(const clap_event_param_value_t *)hdr;
	
		/* Getting the parameters, event ID and value */
		if (ev->param_id < P_COUNT)
		{
			double value = clamp_param_value(ev->param_id, ev->value);
			atomic_store(&p->params[ev->param_id], (float)value);
			apply_param_to_engine(p, ev->param_id, (float)value);
		}
		break;
	}
	default:
		break;
	}
}

/* Apply gestures events */
static void apply_gestures_events(synth_plugin_t *p, const clap_output_events_t *out)
{
	for (uint32_t i = 0; i < P_COUNT; i++)
	{
		/* Sending gestures start events */
		if (atomic_exchange(&p->gestures_start[i], false))
		{
			clap_event_param_gesture_t ev = {0};
			ev.header.size = sizeof(ev);
			ev.header.time = 0;
			ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
			ev.header.type = CLAP_EVENT_PARAM_GESTURE_BEGIN;
			ev.header.flags = 0;
			ev.param_id = i;
			out->try_push(out, &ev.header);
		}
		
		/* Sending gestures end events */
		if (atomic_exchange(&p->gestures_end[i], false))
		{
			clap_event_param_gesture_t ev = {0};
			ev.header.size = sizeof(ev);
			ev.header.time = 0;
			ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
			ev.header.type = CLAP_EVENT_PARAM_GESTURE_END;
			ev.header.flags = 0;
			ev.param_id = i;
			out->try_push(out, &ev.header);
		}
	}
}

/* Main audio thread function, process the synthesizer 
sound data into the CLAP host audio output */
clap_process_status plugin_process(
	const clap_plugin_t *plugin,
	const clap_process_t *process)
{
	synth_plugin_t *p = (synth_plugin_t *)plugin->plugin_data;
	
	/* Frame iteration variables */
	const uint32_t frame_count = process->frames_count;
	const uint32_t event_count = process->in_events->size(process->in_events);
	uint32_t event_index = 0;
	uint32_t next_event_frame = event_count ? 0 : frame_count;

	/* Get CLAP audio buffers */
	clap_audio_buffer_t *out = &process->audio_outputs[0];
	float *out_l = out->data32[0];
	float *out_r = out->data32[1];

	/* Get the GUI events */
	flush_gui_params(p, process->out_events);
	apply_gestures_events(p, process->out_events);

	uint32_t frame = 0;

	while (frame < frame_count)
	{
		/* Process incoming CLAP events */
		while (event_index < event_count)
		{
			const clap_event_header_t *hdr =
				process->in_events->get(process->in_events, event_index);
			if (hdr->time != frame)
				break;
			process_event(p, hdr);
			event_index++;
		}

		/* Increment the event frame */
		next_event_frame = (event_index < event_count) 
			? process->in_events->get(process->in_events, event_index)->time
			: frame_count;

		/* Count the number of active voices */
		int active_voices = 0;
		for (int v = 0; v < VOICES; v++)
		{
			if (p->synth.voices[v].adsr.state != ENV_IDLE)
			{
				active_voices++;
			}

			if (p->synth.voices[v].pressed)
			{
				atomic_store(&p->atomic_notes[v], p->synth.voices[v].note);
			}
			else
			{
				atomic_store(&p->atomic_notes[v], -1);
			}
		}

		/* Render the synthesizer sound data */
		while (frame < next_event_frame)
		{
			process_lfo(&p->synth);
			double sample = process_voices(&p->synth);
			sample = process_gain(&p->synth, sample, active_voices);
			sample = process_filter(&p->synth, sample);
			out_l[frame] = (float)sample;
			out_r[frame] = (float)sample;
			frame++;
		}
	}

	return CLAP_PROCESS_CONTINUE;
}

/* Initialize the plugin */
bool plugin_init(const clap_plugin_t *plugin) 
{
	int res = synth_alocate(plugin);
	if (res == 0)
		return true;
	else
		return false;
}

/* Destroy the plugin */
void plugin_destroy(const clap_plugin_t *plugin) 
{
	synth_free(plugin);
	synth_plugin_t *p = (synth_plugin_t *)plugin->plugin_data;
	free(p);
}

/* Activate the plugin at a given sample rate */
bool plugin_activate(
	const clap_plugin_t *plugin, 
	double sample_rate,
	uint32_t min_frames, uint32_t max_frames) 
{
	(void)min_frames; (void)max_frames;
	((synth_plugin_t *)plugin->plugin_data)->sample_rate = sample_rate;
	return true;
}

/* Deactivate the plugin (does nothing) */
void plugin_deactivate(const clap_plugin_t *plugin)
{
	(void)plugin;
}

/* Start the plugin processing (does nothing) */
bool plugin_start_processing(const clap_plugin_t *plugin)
{
	(void)plugin; 
	return true;
}

/* Stop the plugin processing (does nothing) */
void plugin_stop_processing(const clap_plugin_t *plugin) 
{
	(void)plugin;
}

/* Reset the plugin */
void plugin_reset(const clap_plugin_t *plugin)
{
	synth_plugin_t *p = plugin->plugin_data;

	p->synth.filter.prev_input = 0.0f;
	p->synth.filter.prev_output = 0.0f;
	p->synth.lfo.osc.phase = 0.0f;

	for (int v = 0; v < VOICES; v++)
	{
		voice_t *voice = &p->synth.voices[v];
		voice->adsr.state = ENV_IDLE;
		voice->adsr.output = 0.0f;
		voice->note = -1;
		voice->pressed = 0;

		for (int osc = 0; osc < 3; osc++)
			voice->oscillators[osc].phase = 0.0f;
	}
}

/* Launch the plugin on main thread (does nothing) */
void plugin_on_main_thread(const clap_plugin_t *plugin)
{
	(void)plugin;
}

void posix_on_fd(const clap_plugin_t *plugin, int fd, clap_posix_fd_flags_t flags)
{
	(void)flags; (void)fd;
	synth_plugin_t *p = (synth_plugin_t *)plugin->plugin_data;
	gui_on_POSIX_fd(p);
}

static const clap_plugin_posix_fd_support_t posix_fd_support_ext =
{
	.on_fd = posix_on_fd,
};

static void on_timer( const clap_plugin_t *plugin, clap_id timer_id)
{
	synth_plugin_t *p = (synth_plugin_t *)plugin->plugin_data;
	if (p->gui && timer_id == p->timer_id)
	{
		gui_on_POSIX_fd(p);
		gui_paint(p, true);
	}
}

static const clap_plugin_timer_support_t timer_support_ext =
{
	.on_timer = on_timer,
};

/* Get all of the plugin extensions (PARAMS, NOTE_PORTS & AUDIO_PORTS) */
const void *plugin_get_extension(const clap_plugin_t *plugin, const char *id)
{
    (void)plugin;

	extern const clap_plugin_params_t params_ext;
	extern const clap_plugin_gui_t gui_ext;

	if (!strcmp(id, CLAP_EXT_PARAMS)) return &params_ext;
    if (!strcmp(id, CLAP_EXT_NOTE_PORTS))  return &note_ports_ext;
    if (!strcmp(id, CLAP_EXT_AUDIO_PORTS)) return &audio_ports_ext;
	if (!strcmp(id, CLAP_EXT_GUI)) return &gui_ext;
	if (!strcmp(id, CLAP_EXT_POSIX_FD_SUPPORT)) return &posix_fd_support_ext;
	if (!strcmp(id, CLAP_EXT_TIMER_SUPPORT)) return &timer_support_ext;
	if (!strcmp(id, CLAP_EXT_STATE)) return &state_ext;
    return NULL;
}

#endif 