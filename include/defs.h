#ifndef DEFS_H
#define DEFS_H

#ifdef __STANDALONE__
#if !defined(__NO_RL__)
#include "lib_raylib/src/raylib.h"
#endif /* NO RAYLIB */
#endif /* __CLAP__ */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Notes semitones used for keyboard input */
#define nC 0
#define nC_SHARP 1
#define nD 2
#define nD_SHARP 3
#define nE 4
#define nF 5
#define nF_SHARP 6
#define nG 7
#define nG_SHARP 8
#define nA 9
#define nA_SHARP 10
#define nB 11

/* MIDI packets informations */
#define MIDI_MAX_VALUE 127.0
#define PRESSED 0xF0
#define NOTE_ON 0x90
#define NOTE_OFF 0x80
#define KNOB_TURNED 0xB0

/* CC Values for the Arturia Keylab Essential 61 knobs */
/* ADSR parameters knobs */
#define ARTURIA_ATT_KNOB 1
#define ARTURIA_DEC_KNOB 2
#define ARTURIA_SUS_KNOB 3
#define ARTURIA_REL_KNOB 18

/* Oscillators waveforms knobs */
#define ARTURIA_OSC_A_KNOB 5
#define ARTURIA_OSC_B_KNOB 6
#define ARTURIA_OSC_C_KNOB 7

/* Synthesizer parameters knobs */
#define ARTURIA_CUTOFF_KNOB 8
#define ARTURIA_DETUNE_KNOB 4
#define ARTURIA_AMPLITUDE_KNOB 85

/* Note and synth related */
#define VOICES 16
#define DEFAULT_OCTAVE 4
#define A_4 440
#define RATE 44100
#define DEFAULT_AMPLITUDE 0.5
#define A4_POSITION 57

/* Oscillators waveforms */
#define SINE_WAVE 0
#define SQUARE_WAVE 1
#define TRIANGLE_WAVE 2
#define SAWTOOTH_WAVE 3

/* ALSA buffering and latency */
#define FRAMES 1024
#define LATENCY 40000
#define MAX_SAMPLES 512000
#define MONO 1
#define STEREO 2
#define BITS 16
#if defined(__WINDOWS__) && !defined(__CLAP__)
#define NUM_BUFFERS 4
#endif

#ifdef __STANDALONE__

/* Keyboard layouts */
#define QWERTY 0
#define AZERTY 1

/* Keyboard note keys */
#ifndef __NO_RL__
#define kC KEY_A
#define kC_SHARP KEY_W
#define kD KEY_S
#define kD_SHARP KEY_E
#define kE KEY_D
#define kF KEY_F
#define kF_SHARP KEY_T
#define kG KEY_G
#define kG_SHARP KEY_Y
#define kA KEY_H
#define kA_SHARP KEY_U
#define kB KEY_J

/* Keyboard oscillator keys */
#define KEY_OSC_A KEY_Z
#define KEY_OSC_B KEY_X
#define KEY_OSC_C KEY_C

/* Keyboard ADSR envelope keys */
#define KEY_ADSR KEY_LEFT_SHIFT
#define KEY_ATT KEY_Z
#define KEY_DEC KEY_X
#define KEY_SUS KEY_C
#define KEY_REL KEY_V
#endif

/* Raygui interface */
#define WIDTH 1769
#define HEIGHT 800
#define TITLE "ALSA & raygui Synthesizer"
#endif /* __STANDALONE__ */

/* MIDI piano visualizer */
#define WHITE_KEYS 52
#define BLACK_KEYS 36
#define WHITE_KEYS_WIDTH WIDTH / WHITE_KEYS
#define WHITE_KEYS_HEIGHT HEIGHT / 4
#define BLACK_KEYS_WIDTH WHITE_KEYS_WIDTH / 2
#define BLACK_KEYS_HEIGHT (WHITE_KEYS_HEIGHT * 2) / 3

/* LFO modulated parameters */
#define LFO_OFF 0
#define LFO_CUTOFF 1
#define LFO_DETUNE 2
#define LFO_AMP 3

#endif
