#include <stdbool.h>

#include "defs.h"
#include "core/effects.h"

/* Applies an amount of distortion onto a sound buffer */
short distortion(short sample, float amount, bool overdriving)
{
	if (amount > 1.0)
	{
		amount = 1.0;
	}
	else if (amount < 0.0)
	{
		amount = 0.0;
	}

	short clip;

	if (overdriving)
	{
		clip = (32767 / 2) * (1 - amount);
	}
	else
	{
		clip = 32767 * (1 - amount);
	}

	if (sample > clip)
	{
		sample = clip;
	}
	else if (sample < -clip)
	{
		sample = -clip;
	}

	/* Gain to avoid silencing when amount is high */
	sample *= 1.0 + (1.0 - clip / 32767.0);
	return sample;
}