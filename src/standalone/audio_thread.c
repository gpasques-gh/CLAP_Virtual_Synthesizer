#ifdef __STANDALONE__

#include <stdlib.h>

#include "core/synth.h"
#include "core/effects.h"

#include "standalone/audio_thread.h"
#include "standalone/midi.h"

/* Handle the recording of the WAVE file */
static void __recording_handling(audio_thread_ctx_t *ctx, short buffer[FRAMES])
{
	/* Handling WAVE file recording */
	if (ctx->recording_on && ctx->recording_file)
	{
		fwrite(buffer, sizeof(short), FRAMES, ctx->recording_file);
		ctx->fwrite_count++;
	}
	/* Closing the WAVE file */
	else if (!ctx->recording_on && ctx->recording_file)
	{
		/* Calculating the size of the audio data */
		ctx->wave_header.sub2_size = 
			FRAMES * 
			ctx->fwrite_count * 
			(unsigned int)ctx->wave_header.num_channels *
			(unsigned int)ctx->wave_header.bits_per_sample / 8;
		/* Calculating the offset of the header data */
		ctx->wave_header.chunk_size = 
			(unsigned int)ctx->wave_header.sub2_size + 36;
		/* Writing the new header to the WAVE file and closing it */
		fseek(ctx->recording_file, 0, SEEK_SET);
		fwrite(&ctx->wave_header, 1, sizeof(ctx->wave_header), ctx->recording_file);
		close_wav_file(ctx->recording_file);
		ctx->recording_file = NULL;
		ctx->fwrite_count = 0;
	}
}

/* Process the synthesizer sound data into a buffer */
static void __process_synthesizer(audio_thread_ctx_t *ctx, short buffer[FRAMES])
{
	/* Count active voices */
	for (int v = 0; v < VOICES; v++)
		if (ctx->synth.voices[v].adsr.state != ENV_IDLE)
			ctx->active_voices++;

	/* Process the synthesizer data */
	for (int i = 0; i < FRAMES; i++)
	{
		process_lfo(&ctx->synth);
		double sample = process_voices(&ctx->synth);
		sample = process_gain(&ctx->synth, sample, ctx->active_voices);
		sample = process_filter(&ctx->synth, sample);
		buffer[i] = (short)(sample * 32767.0);
		if (ctx->distortion_on)
		{
			buffer[i] = distortion(buffer[i],
				ctx->distortion_amount, ctx->overdrive);
		}
		process_arpeggiator(&ctx->synth, ctx->active_voices);
	}

	ctx->active_voices = 0;
}

#ifdef __WINDOWS__

/* Callback function for the sound card */
void CALLBACK waveOutProc(
	HWAVEOUT wave_out,
	UINT msg,
	DWORD_PTR instance,
	DWORD_PTR param1,
	DWORD_PTR param2)
{
	(void)wave_out; (void)param2;
	if (msg == WOM_DONE)
	{
		audio_thread_ctx_t *ctx = (audio_thread_ctx_t *)instance;
		WAVEHDR *hdr = (WAVEHDR *)param1;
		int idx = (int)((float *)hdr->lpData - &ctx->buffers[0][0]) / FRAMES;
		InterlockedExchange(&ctx->buffer_free[idx], 1);
	}
}

/* Audio thread function */
DWORD WINAPI audio_thread_proc(LPVOID param)
{
	audio_thread_ctx_t *ctx = (audio_thread_ctx_t *)param;
	timeBeginPeriod(1);

	/* Getting the state of the program from the main thread */
	while (!InterlockedCompareExchange(&ctx->should_stop, 0, 0))
	{
		/* Poll MIDI queue */
		if (ctx->midi_valid)
			poll_midi_queue(&ctx->midi_queue, &ctx->synth);

		/* Process the synthesizer sound data */
		/* Create a local buffer before float conversion */
		short local_buf[FRAMES];
		__process_synthesizer(ctx, local_buf);

		/* Critical Section : copy the local buffer to the context display buffer */
		/* The display buffer is used in the main thread for the waveform visualizer */
		EnterCriticalSection(&ctx->lock);
		memcpy(ctx->display_buffer, local_buf, sizeof(local_buf));
		LeaveCriticalSection(&ctx->lock);

		/* Check if the current buffer is free */
		int buf_idx = ctx->current_buffer;
		while (!InterlockedCompareExchange(&ctx->buffer_free[buf_idx], 0, 1))
			Sleep(1);

		/* Reset the current header */
		if (ctx->headers[buf_idx].dwFlags & WHDR_PREPARED)
			waveOutUnprepareHeader(
				ctx->wave_out,
				&ctx->headers[buf_idx],
				sizeof(WAVEHDR));

		/* Convert the integer data to float data */
		for (int i = 0; i < FRAMES; i++)
			ctx->buffers[buf_idx][i] = (float)local_buf[i] / 32768.0f;

		/* Update the headers */
		ctx->headers[buf_idx].lpData = (LPSTR)ctx->buffers[buf_idx];
		ctx->headers[buf_idx].dwBufferLength = FRAMES * sizeof(float);
		ctx->headers[buf_idx].dwFlags = 0;

		/* Write the float data to the sound card */
		waveOutPrepareHeader(ctx->wave_out, &ctx->headers[buf_idx], sizeof(WAVEHDR));
		waveOutWrite(ctx->wave_out, &ctx->headers[buf_idx], sizeof(WAVEHDR));

		/* Increment the current buffer */
		ctx->current_buffer = (buf_idx + 1) % NUM_BUFFERS;

		__recording_handling(ctx, local_buf);
	}

	timeEndPeriod(1);

	return 0;
}

#elif defined(__LINUX__)

#include <stdatomic.h>

/* Audio thread function for Linux */
void *audio_thread_proc(void *param)
{
	/* Thread is asynchronous to avoir ALSA I/O errors */
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
	audio_thread_ctx_t *ctx = (audio_thread_ctx_t *)param;
	 
	while (!atomic_load(&ctx->should_stop))
	{
		/* Get MIDI events */
		if (ctx->midi_valid)
			get_midi(ctx->midi_in, &ctx->synth);

		/* Process the synthesizer sound data */
		short local_buf[FRAMES];
		__process_synthesizer(ctx, local_buf);
		
		/* Enter critical section and copy 
		the local buffer to the context buffer */
		/* This syncs the sound data with the main for
		waveform visualization in the GUI */
		pthread_mutex_lock(&ctx->lock);
		memcpy(ctx->buffer, local_buf, sizeof(local_buf));
		pthread_mutex_unlock(&ctx->lock);

		/* Write the sound buffer to the sound card */
		int err = snd_pcm_writei(ctx->audio_out, ctx->buffer, FRAMES);
		if (err == -EPIPE)
		{
			fprintf(stderr, "ALSA underrun.\n");
			snd_pcm_prepare(ctx->audio_out);
		}
		else if (err < 0)
		{
			fprintf(stderr, "ALSA write error: %s\n", snd_strerror(err));
			snd_pcm_prepare(ctx->audio_out);
		}

		/* Start the recording process */
		__recording_handling(ctx, ctx->buffer);
	}

	return 0;
}

#endif 
#endif 