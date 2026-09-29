#ifdef __STANDALONE__

#ifndef RECORD_H
#define RECORD_H

#include <stdio.h>

/* Wav header structure */
typedef struct 
{
	unsigned char chunk_id[4];
	unsigned int chunk_size;
	unsigned char format[4];
	unsigned char sub1_id[4];
	unsigned int sub1_size;
	unsigned short audio_format;
	unsigned short num_channels;
	unsigned int sample_rate;
	unsigned int byte_rate;
	unsigned short block_align;
	unsigned short bits_per_sample;
	unsigned char sub2_id[4];
	unsigned int sub2_size;
} wav_header_t;

/* Initialize wav header */
int init_wav_header(wav_header_t *header);

/* Initialize a wav file with a filename and wav header */
int init_wav_file(char *fname, FILE **fwav, wav_header_t *header);

/* Close a wav file */
int close_wav_file(FILE *fwav);

#endif /* HEADER GUARD */
#endif /* __STANDALONE__ */