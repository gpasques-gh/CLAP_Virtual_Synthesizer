#ifndef __CLAP__

#if !defined(__WINDOWS__) && !defined(__LINUX__)
#error "Unsupported OS."
#endif

#ifndef __NO_GUI__
#ifdef __WINDOWS__
#include "standalone/win_defs.h"
#endif
#define RAYGUI_IMPLEMENTATION
#include "lib_raygui/src/raygui.h"
#endif

#ifdef __WINDOWS__
#include <windows.h>

#ifdef NOGDI
typedef struct tagBITMAPINFOHEADER
{
	DWORD biSize;
	LONG biWidth;
	LONG biHeight;
	WORD biPlanes;
	WORD biBitCount;
	DWORD biCompression;
	DWORD biSizeImage;
	LONG biXPelsPerMeter;
	LONG biYPelsPerMeter;
	DWORD biClrUsed;
	DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER, *LPBITMAPINFOHEADER;
#endif

#include <mmsystem.h>
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>

#elif defined(__LINUX__)

#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>
#include <alsa/asoundlib.h>

#endif

#include <float.h>
#include <stdint.h>

#include "defs.h"

#include "core/synth.h"
#include "core/effects.h"

#include "standalone/audio_thread.h"
#include "standalone/midi.h"

#ifndef __NO_GUI__
#include "standalone/xml.h"
#include "standalone/interface.h"
#include "standalone/keyboard.h"
#include "standalone/record.h"
#endif

/* Prints the usage of the CLI arguments into the error output */
static void __usage()
{
	fprintf(stderr, "synth -midi <midi hardware id> : midi keyboard input, able to change parameters of the sounds (ADSR, cutoff, detune and oscillators waveforms)\n");
	fprintf(stderr, "use amidi -l to list your connected midi devices and find your midi device hardware id, often something like : hw:0,0,0 or hw:1,0,0\n");
	fprintf(stderr, "to see this helper again, use synth -h or synth -help\n");
}

/* MAIN LOOP */
int main(int argc, char **argv)
{
	/* Command line arguments parsing */
	char midi_device[256];
	int midi_input = 0;
	if (argc > 1)
	{
		if (strcmp(argv[1], "-midi") == 0 && argc >= 3)
		{
			midi_input = 1;
			strncpy(midi_device, argv[2], sizeof(midi_device) - 1);
			midi_device[sizeof(midi_device) - 1] = '\0';
		}
		else if (strcmp(argv[1], "-midi") == 0 && argc < 3)
		{
			fprintf(stderr, "missing midi hardware device id.\n");
			return 1;
		}
		else
		{
			__usage();
			return 1;
		}
	}

	int octave = DEFAULT_OCTAVE;

	/* SYNTHESIZER COMPONENTS */

	/* Waveforms*/
	int wave_a = SINE_WAVE, wave_b = SINE_WAVE, wave_c = SINE_WAVE;
	int osc_lfo = SINE_WAVE;

	/* ADSR envelope parameters */
	float attack = 0.2;
	float decay = 0.3;
	float sustain = 0.7;
	float release = 0.2;

	/* Filter ADSR envelope parameters */
	float filter_attack = 0.0;
	float filter_decay = 0.3;
	float filter_sustain = 0.0;
	float filter_release = 0.2;

	/* Filter ADSR envelope */
	adsr_t filter_adsr =
		{
			.attack = filter_attack,
			.decay = filter_decay,
			.sustain = filter_sustain,
			.release = filter_release,
			.state = ENV_IDLE,
			.type = ENV_TYPE_FILTER};

	/* Low-pass filter */
	lp_filter_t filter =
		{
			.cutoff = 0.5,
			.prev_input = 0.0,
			.prev_output = 0.0,
			.adsr = filter_adsr,
			.env = false};

	/* Low Frequency Oscillator oscillator */
	osc_t lfo_osc =
		{
			.freq = 0.5,
			.phase = 0.0,
			.wave = osc_lfo};

	/* Low Frequency Oscillator */
	lfo_t lfo =
		{
			.osc = lfo_osc,
			.mod_param = LFO_OFF};

	/* Polyphonic Synthesizer */
	synth_t synth =
		{
			.voices = malloc(sizeof(voice_t) * VOICES),
			.amp = DEFAULT_AMPLITUDE,
			.detune = 0.0,
			.filter = filter,
			.lfo = lfo,
			.arp = false,
			.active_arp = 0,
			.active_arp_float = 1.0,
			.bpm = 150.0};

	if (synth.voices == NULL)
	{
		fprintf(stderr, "memory allocation failed.\n");
		return 1;
	}

	/* Create the synthesizer voices */
	for (int i = 0; i < VOICES; i++)
	{
		/* Synthesizer ADSR envelope */
		synth.voices[i].adsr.attack = attack;
		synth.voices[i].adsr.decay = decay;
		synth.voices[i].adsr.sustain = sustain;
		synth.voices[i].adsr.release = release;
		synth.voices[i].adsr.state = ENV_IDLE;
		synth.voices[i].adsr.type = ENV_TYPE_FILTER;
		synth.voices[i].adsr.output = 0.0;

		synth.voices[i].note = -1;
		synth.voices[i].velocity_amp = 0.0;
		synth.voices[i].pressed = 0;

		/* Allocating the oscillators */
		synth.voices[i].oscillators = malloc(sizeof(osc_t) * 3);
		if (synth.voices[i].oscillators == NULL)
		{
			fprintf(stderr, "memory allocation failed.\n");
			goto cleanup_synth;
		}

		for (int j = 0; j < 3; j++)
		{
			synth.voices[i].oscillators[j].freq = 0.0;
			synth.voices[i].oscillators[j].phase = 0.0;
		}

		synth.voices[i].oscillators[0].wave = wave_a;
		synth.voices[i].oscillators[1].wave = wave_b;
		synth.voices[i].oscillators[2].wave = wave_c;
	}

	/* Accessing the synthesizer data through a pointer for thread concurrency */
	synth_t *synth_ptr = &synth;

	/* Sound and waveform display buffer */
	short buffer[FRAMES];
	memset(buffer, 0, sizeof(short) * FRAMES);
#ifdef __WINDOWS__
	/* Print the devices and get the current one */
	UINT sound_devices = waveOutGetNumDevs();
	for (UINT i = 0; i < sound_devices; i++)
	{
		WAVEOUTCAPS caps;
		if (waveOutGetDevCaps(i, &caps, sizeof(WAVEOUTCAPS)) == MMSYSERR_NOERROR)
			fprintf(stderr, "device %u: %s\n", i, caps.szPname);
	}

	/* Initialize the audio format */
	WAVEFORMATEXTENSIBLE wfx = {0};
	wfx.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	wfx.Format.nChannels = MONO;
	wfx.Format.nSamplesPerSec = RATE;
	wfx.Format.wBitsPerSample = 32;
	wfx.Format.nBlockAlign = wfx.Format.nChannels * (wfx.Format.wBitsPerSample / 8);
	wfx.Format.nAvgBytesPerSec = wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign;
	wfx.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);

	wfx.Samples.wValidBitsPerSample = 32;
	wfx.dwChannelMask = SPEAKER_FRONT_CENTER;
	wfx.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

	/* Initialize the audio thread context */
	audio_thread_ctx_t ctx = {0};
	ctx.synth = synth;
	ctx.buffer_free[0] =
		ctx.buffer_free[1] =
			ctx.buffer_free[2] =
				ctx.buffer_free[3] = 1;
	InitializeCriticalSection(&ctx.lock);

	/* Open the sound card */
	HWAVEOUT wave_out;
	MMRESULT sound_res = waveOutOpen(
		&wave_out,
		WAVE_MAPPER,
		(WAVEFORMATEX *)&wfx,
		(DWORD_PTR)waveOutProc,
		(DWORD_PTR)&ctx,
		CALLBACK_FUNCTION);

	if (sound_res != MMSYSERR_NOERROR)
	{
		fprintf(stderr, "couldn't open sound card %lu", GetLastError());
		return 1;
	}

	ctx.wave_out = wave_out;

	/* Handle the MIDI interface initialization */
	HMIDIIN midi_in;
	UINT midi_valid = 1;
	LONG midi_id;
	LONG midi_devices;

	/* If the user specified MIDI input */
	if (midi_input)
	{
		/* Get the number of MIDI devices */
		midi_devices = midiInGetNumDevs();
		if (midi_devices == 0)
		{
			fprintf(stderr, "no midi device found.\n");
			midi_valid = 0;
		}

		/* Check if the given port
		is a valid integer */
		char *endptr;
		midi_id =
			strtol(midi_device, &endptr, 10);
		if (endptr == midi_device)
			midi_valid = 0;
		else if (midi_id < 0 || midi_id >= midi_devices)
			midi_valid = 0;
	}

	/* If the informations from the user are correct */
	if (midi_valid && midi_input)
	{
		/* Check the available MIDI devices */
		for (LONG i = 0; i < midi_devices; i++)
		{
			MIDIINCAPS caps;
			midiInGetDevCaps(i, &caps, sizeof(MIDIINCAPS));
			printf("midi_device: [%lu] %s\n", i, caps.szPname);
		}

		/* Initialize the MIDI queue */
		midi_queue_init(&ctx.midi_queue);

		/* Open the MIDI interface */
		MMRESULT midi_res = midiInOpen(
			&midi_in, midi_id,
			(DWORD_PTR)MidiInProc,
			(DWORD_PTR)&ctx.midi_queue,
			CALLBACK_FUNCTION);

		if (midi_res != MMSYSERR_NOERROR)
		{
			fprintf(stderr, "error while opening midi device.\n");
			return 1;
		}

		/* Start the MIDI communication */
		midiInStart(midi_in);
		fprintf(stderr, "midi started on port %ld\n", midi_id);
	}
	else
	{
		midi_valid = 0;
	}

	ctx.midi_valid = midi_valid;

	/* Create and run the audio thread */
	HANDLE audio_thread = CreateThread(
		NULL, 0,
		audio_thread_proc,
		&ctx, 0, NULL);

	if (audio_thread == NULL)
	{
		fprintf(stderr, "error while initializing audio thread: %lu\n", GetLastError());
		exit(1);
	}

	SetThreadPriority(audio_thread, THREAD_PRIORITY_HIGHEST);
	synth_ptr = &ctx.synth;

#elif defined(__LINUX__)
	/* Initialize the audio thread context */
	audio_thread_ctx_t ctx = {0};
	ctx.synth = synth;

	/* Open the sound card */
	snd_pcm_t *handle = NULL;
	if (snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0)
	{
		fprintf(stderr, "error while opening sound card.\n");
		goto cleanup_synth;
	}

	ctx.audio_out = handle;

	/* Set the parameters of the sound card */
	int params_err = snd_pcm_set_params(
		ctx.audio_out,
		SND_PCM_FORMAT_S16_LE,
		SND_PCM_ACCESS_RW_INTERLEAVED,
		MONO, RATE, 1, LATENCY);
	if (params_err < 0)
	{
		fprintf(stderr, "error while setting sound card parameters: %s\n", snd_strerror(params_err));
		goto cleanup_alsa;
	}

	/* Open the MIDI interface communication */
	snd_rawmidi_t *midi_in;
	if (midi_input)
	{
		if (snd_rawmidi_open(&midi_in, NULL, midi_device, SND_RAWMIDI_NONBLOCK) < 0)
		{
			fprintf(stderr, "error while opening midi device %s\n", midi_device);
			goto cleanup_alsa;
		}
	}

	ctx.midi_valid = midi_input;
	ctx.midi_in = midi_in;
	atomic_init(&ctx.should_stop, false);
	synth_ptr = &ctx.synth;

	/* Create and run the audio thread */
	pthread_t audio_thread_id;
	pthread_create(
		&audio_thread_id,
		NULL,
		&audio_thread_proc,
		&ctx);
#endif

#ifndef __NO_GUI__
	/* WAVE recording variables */
	char audio_filename[1024] = "\0";
	bool recording = false;

	/* Oscillators dropdown menus booleans */
	bool ddm_a = false, ddm_b = false, ddm_c = false;
	bool saving_preset = false, saving_audio_file = false, loading_preset = false;
	bool lfo_wave_ddm = false, lfo_params_ddm = false;
	bool distortion_on = false, overdrive = false;
	float distortion_amount = 0.0;

	char preset_filename[1024] = "\0";

	/* Initialize the raygui windows */
	SetTraceLogLevel(LOG_WARNING);
	InitWindow(WIDTH, HEIGHT, "ALSA & raygui synthesizer");
	SetWindowState(FLAG_VSYNC_HINT);
	Font annotation = LoadFont("Regular.ttf");
	GuiSetFont(annotation);
	GuiSetStyle(DEFAULT, TEXT_SIZE, GuiGetFont().baseSize * 0.5);

	/* Main loop */
	while (!WindowShouldClose())
	{
		/* Handle keyboard input from the user */
		if (!saving_preset && !saving_audio_file)
		{
			handle_input(synth_ptr, &octave,
						 &attack, &decay, &sustain, &release,
						 &wave_a, &wave_b, &wave_c);
			handle_release(synth_ptr, octave);
		}

		/* Update the context of the audio thread */
		/* This part of the context is the same on both Linux and Win32 */
		ctx.distortion_on = distortion_on;
		ctx.distortion_amount = distortion_amount;
		ctx.overdrive = overdrive;

		/* Get the sound data to display in the waveform visualizer */
#ifdef __WINDOWS__
		EnterCriticalSection(&ctx.lock);
		memcpy(buffer, ctx.display_buffer, sizeof(buffer));
		LeaveCriticalSection(&ctx.lock);
#elif defined(__LINUX__)
		pthread_mutex_lock(&ctx.lock);
		memcpy(buffer, ctx.buffer, sizeof(buffer));
		pthread_mutex_unlock(&ctx.lock);
#endif
		/* Start recording if the WAVE file is not initialized*/
		if (ctx.recording_file == NULL && recording == true)
		{
			char audio_full_filename[1024] = "audio/";
			strcat(audio_full_filename, audio_filename);
			strcat(audio_full_filename, ".wav");
			init_wav_header(&ctx.wave_header);
			init_wav_file(audio_full_filename, &ctx.recording_file, &ctx.wave_header);
			audio_filename[0] = '\0';
			ctx.recording_on = 1;
		}
		else if (recording == false && ctx.recording_on)
		{
			ctx.recording_on = 0;
		}

		/* Graphical User Interface rendering */
		BeginDrawing();
		ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

		/* Title */
		GuiLabel((Rectangle){WIDTH / 2 - 115, 5, 230, 20}, "ALSA & raygui Synthesizer");
		/* Waveform visualizer */
		render_waveform(buffer);
		/* ADSR envelope GUI */
		render_adsr(&attack, &decay, &sustain, &release);
		/* Filter ADSR envelope GUI */
		render_filter_adsr(&filter_attack, &filter_decay, &filter_sustain, &filter_release);
		/* Oscillators waveforms selection GUI */
		render_osc_waveforms(
			&wave_a, &wave_b, &wave_c,
			&ddm_a, &ddm_b, &ddm_c);
		/* Miscelannous synthesizer parameters GUI */
		render_synth_params(synth_ptr);
		/* Miscelannous options GUI (presets loading/saving & recording) */
		render_options(
			synth_ptr,
			audio_filename,
			&saving_preset, &loading_preset,
			&saving_audio_file, &recording);
		/* Effects parameters GUI (LFO & distortion) */
		render_effects(
			synth_ptr,
			&lfo_wave_ddm, &lfo_params_ddm,
			&distortion_on, &overdrive,
			&distortion_amount);

		/* Loading preset, shows file dialog */
		if (loading_preset)
		{
			load_preset(
				synth_ptr,
				&attack, &decay, &sustain, &release,
				&wave_a, &wave_b, &wave_c,
				&distortion_on, &overdrive,
				&distortion_amount,
				&loading_preset);
		}

		/* Saving preset, popup window for file name */
		if (saving_preset)
		{
			save_preset(
				*synth_ptr,
				attack, decay, sustain, release,
				wave_a, wave_b, wave_c,
				preset_filename, &saving_preset,
				distortion_on, overdrive,
				distortion_amount);
		}

		update_synth_oscillators(synth_ptr, wave_a, wave_b, wave_c);
		update_synth_envelope(synth_ptr, attack, decay, sustain, release);
		update_filter_params(synth_ptr, synth_ptr->filter.cutoff, filter_attack, filter_decay, filter_sustain, filter_release, synth_ptr->filter.env);

		/* Keyboard visualizer rendering */

		/* White keys */
		render_white_keys();
		for (int v = 0; v < VOICES; v++)
		{
			if (synth_ptr->voices[v].pressed &&
				!is_black_key(synth_ptr->voices[v].note))
			{
				render_key(synth_ptr->voices[v].note, false);
			}
		}
		if (synth_ptr->arp &&
			!is_black_key(synth_ptr->voices[synth_ptr->active_arp].note) &&
			synth_ptr->voices[synth_ptr->active_arp].pressed)
		{
			render_key(synth_ptr->voices[synth_ptr->active_arp].note, true);
		}

		/* Black keys */
		render_black_keys();
		for (int v = 0; v < VOICES; v++)
		{
			if (synth_ptr->voices[v].pressed &&
				is_black_key(synth_ptr->voices[v].note))
			{
				render_key(synth_ptr->voices[v].note, false);
			}
		}
		if (synth_ptr->arp &&
			is_black_key(synth_ptr->voices[synth_ptr->active_arp].note) &&
			synth_ptr->voices[synth_ptr->active_arp].pressed)
		{
			render_key(synth_ptr->voices[synth_ptr->active_arp].note, true);
		}

		EndDrawing();
	}

	CloseWindow();

#else
	/* If no GUI, simple while loop to let the audio thread running */
	while (1)
	{
	}
#endif /* __NO_GUI__*/
	/* If we quit the application during recording, change WAV header and close WAV file */
	if (ctx.recording_file != NULL)
	{
		ctx.wave_header.sub2_size = FRAMES * ctx.fwrite_count * (unsigned int)ctx.wave_header.num_channels * (unsigned int)ctx.wave_header.bits_per_sample / 8;
		ctx.wave_header.chunk_size = (unsigned int)ctx.wave_header.sub2_size + 36;
		fseek(ctx.recording_file, 0, SEEK_SET);
		fwrite(&ctx.wave_header, 1, sizeof(ctx.wave_header), ctx.recording_file);
		close_wav_file(ctx.recording_file);
	}
#ifdef __WINDOWS__
	/* Close the MIDI interface */
	if (midi_valid)
	{
		midiInStop(midi_in);
		midiInClose(midi_in);
	}

	/* Close the Windows audio thread */
	InterlockedExchange(&ctx.should_stop, 1);
	WaitForSingleObject(audio_thread, INFINITE);
	CloseHandle(audio_thread);
	DeleteCriticalSection(&ctx.lock);

	/* Close the sound interface */
	waveOutClose(wave_out);
#elif defined(__LINUX__)
	/* Close the Linux audio thread */
	atomic_store(&ctx.should_stop, true);
	pthread_join(audio_thread_id, NULL);

	/* Close the MIDI interface */
	if (ctx.midi_in)
		snd_rawmidi_close(ctx.midi_in);

cleanup_alsa:
	/* Close the sound interface */
	if (ctx.audio_out)
	{
		snd_pcm_drain(ctx.audio_out);
		snd_pcm_close(ctx.audio_out);
	}
#endif
cleanup_synth:
	/* Free the synthesizer memory */
	for (int i = 0; i < VOICES; i++)
	{
		free(synth.voices[i].oscillators);
	}
	free(synth.voices);

	return 0;
}

#endif