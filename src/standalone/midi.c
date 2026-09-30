#ifndef __CLAP__

#include <stdint.h>

#define __NO_RL__
#include "defs.h"
#undef __NO_RL__
#include "core/synth.h"
#include "standalone/midi.h"

/* Apply the MIDI event to the synthesizer */
/* MIDI event either come directly from the main loop in Linux */
/* Or from the MIDIIN thread and MIDI queue polling in Windows */
static void __apply_midi_event(
	synth_t *synth,
	uint8_t status,
	uint8_t data1,
	uint8_t data2)
{
	/* If the MIDI message is a NOTE ON */
	if ((status & PRESSED) == NOTE_ON && data2 > 0)
	{
		/* Activate a voice with the given note and velocity */
		voice_on(synth, data1, data2);
	}
	/* If the MIDI message is a NOTE OFF */
	else if ((status & PRESSED) == NOTE_OFF ||
			 ((status & PRESSED) == NOTE_ON && data2 == 0))
	{
		/* Depress a voice with the given note */
		voice_off(synth, data1);
	}
}

#ifdef __WINDOWS__

#include "standalone/win_defs.h"
#include <windows.h>
#include <mmeapi.h>

/* Initialize a MIDI queue */
void midi_queue_init(midi_queue_t *q)
{
	q->head = 0;
	q->tail = 0;
}

/* Callback function for the HMIDIIN */
void CALLBACK MidiInProc(
	HMIDIIN midi_in,
	UINT msg,
	DWORD_PTR instance,
	DWORD_PTR param1,
	DWORD_PTR param2)
{
	(void)midi_in;
	(void)param2;
	if (msg != MIM_DATA)
		return;

	/* Get the MIDI queue */
	midi_queue_t *q = (midi_queue_t *)instance;

	/* Get the MIDI message bytes */
	BYTE status = param1 & 0xFF;
	BYTE data1 = (param1 >> 8) & 0xFF;
	BYTE data2 = (param1 >> 16) & 0xFF;

	/* Iterate through the MIDI queue */
	LONG head = q->head;
	LONG next = (head + 1) & (MIDI_QUEUE_SIZE - 1);

	/* If the queue is empty */
	if (next == q->tail)
		return;

	/* Initialize the MIDI event */
	q->events[head].status = status;
	q->events[head].data1 = data1;
	q->events[head].data2 = data2;

	/* Synchronize the data with the main thread */
	MemoryBarrier();
	InterlockedExchange(&q->head, next);
}

/* Poll the MIDI queue for new MIDI events
and apply them to the synthesizer */
/* This function is called from the audio thread */
void poll_midi_queue(midi_queue_t *q, synth_t *synth)
{
	LONG tail = q->tail;
	while (tail != q->head)
	{
		/* Apply the new MIDI events */
		midi_event_t ev = q->events[tail];
		__apply_midi_event(synth, ev.status, ev.data1, ev.data2);
		tail = (tail + 1) & (MIDI_QUEUE_SIZE - 1);
	}

	/* Synchronize the data with the main thread */
	InterlockedExchange(&q->tail, tail);
}
#elif defined(__LINUX__)

#include <alsa/asoundlib.h>

/*
 * Get the MIDI input from the ALSA RawMIDI input (snd_rawmidi_t)
 * Activate the synth voices and update their frequencies with the given note
 * Turn off the synth voices when their assigned note are being released
 * Change the ADSR parameters when the assigned knobs are being triggered
 * Change the cutoff, detune and amplification when the assigned knobs are being triggered
 */
int get_midi(snd_rawmidi_t *midi_in, synth_t *synth)
{
	unsigned char midi_buffer[1024];
	ssize_t ret = snd_rawmidi_read(midi_in, midi_buffer, sizeof(midi_buffer));

	if (ret < 0)
	{
		return 1;
	}

	for (int i = 0; i + 2 < ret; i += 3)
	{ /* Getting the MIDI bytes informations */
		unsigned char status = midi_buffer[i];
		unsigned char data1 = midi_buffer[i + 1];
		unsigned char data2 = midi_buffer[i + 2];

		__apply_midi_event(synth, status, data1, data2);
	}

	return 0;
}

#endif /* __LINUX__ */
#endif /* __CLAP__ */