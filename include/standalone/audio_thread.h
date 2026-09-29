#ifdef __STANDALONE__

#ifndef __AUDIO_THREAD_H__
#define __AUDIO_THREAD_H__

#include "core/synth.h"
#include "standalone/record.h"

#ifdef __WINDOWS__

#define __NO_RL__
#include "defs.h"
#undef __NO_RL__
#include <windows.h>
#include "standalone/midi.h"

/* Audio thread context structure */
typedef struct audio_thread_ctx_s
{
	synth_t synth;

	/* MIDI */
	int midi_valid;
	midi_queue_t midi_queue;

	/* Audio buffers */
	short display_buffer[FRAMES];
	float buffers[NUM_BUFFERS][FRAMES];
	WAVEHDR headers[NUM_BUFFERS];
	volatile LONG buffer_free[NUM_BUFFERS];
	int current_buffer;
	HWAVEOUT wave_out;

	/* File recording variables */
	int recording_on;
	FILE *recording_file;
	long fwrite_count;
	wav_header_t wave_header;

	/* Audio effects variables */
	int distortion_on;
	float distortion_amount;
	float overdrive;

	/* Synthesizer and program state */
	int active_voices;
	volatile LONG should_stop;

	/* Critical section */
	CRITICAL_SECTION lock;
} audio_thread_ctx_t;

/* Callback function for the sound card */
void CALLBACK waveOutProc(
	HWAVEOUT wave_out,
	UINT msg,
	DWORD_PTR instance,
	DWORD_PTR param1,
	DWORD_PTR param2);

/* Audio thread function */
DWORD WINAPI audio_thread_proc(LPVOID param);

#elif defined(__LINUX__)

#include <pthread.h>
#include <stdatomic.h>
#include <alsa/asoundlib.h>

#include "defs.h"

/* Linux audio thread context */
typedef struct audio_thread_ctx_s
{
	/* Synthesizer */
	synth_t synth;
	int active_voices;

	/* MIDI */
	int midi_valid;
	snd_rawmidi_t *midi_in;

	/* Audio buffer and handle */
	snd_pcm_t *audio_out;
	short buffer[FRAMES];

	/* File recording variables */
	int recording_on;
	FILE *recording_file;
	long fwrite_count;
	wav_header_t wave_header;

	/* Parameters that are synced with main thread */
	int distortion_on;
	int overdrive;
	float distortion_amount;
	atomic_bool should_stop;

	/* Critical section lock */
	pthread_mutex_t lock;
} audio_thread_ctx_t;

/* Linux audio thread main loop */
void *audio_thread_proc(void *param);

#endif /* __WINDOWS__ */
#endif /* __AUDIO_THREAD_H__ */
#endif /* __STANDALONE__ */