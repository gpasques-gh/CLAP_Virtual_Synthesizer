#ifdef __CLAP__

#include <string.h>
#include <stdio.h>

#include "lib_clap/include/clap/clap.h"
#include "clap/clap_audio_ports.h"

uint32_t audio_ports_count(
	const clap_plugin_t *plugin,
	bool is_input)
{
	(void)plugin;
	(void)is_input;
	return is_input ? 0 : 1;
}

bool audio_ports_get(
	const clap_plugin_t *plugin,
	uint32_t index,
	bool is_input,
	clap_audio_port_info_t *info)
{
	(void)plugin;
	(void)is_input;

	if (index != 0)
		return false;

	info->id = 0;
	snprintf(info->name, sizeof(info->name), "%s", "Stereo");
	info->channel_count = 2;
	info->flags = CLAP_AUDIO_PORT_IS_MAIN;
	info->port_type = CLAP_PORT_STEREO;
	info->in_place_pair = CLAP_INVALID_ID;
	return true;
}

#endif