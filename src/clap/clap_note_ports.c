#ifdef __CLAP__

#include <stdio.h>

#include "lib_clap/include/clap/clap.h"
#include "clap/clap_note_ports.h"

uint32_t note_ports_count(
	const clap_plugin_t *plugin,
	bool is_input)
{
	(void)plugin;
	return is_input ? 1 : 0;
}

bool note_ports_get(
	const clap_plugin_t *plugin,
	uint32_t index,
	bool is_input,
	clap_note_port_info_t *info)
{
	(void)plugin;
	if (!is_input || index != 0)
		return false;
	info->id = 0;
	info->supported_dialects = CLAP_NOTE_DIALECT_CLAP | CLAP_NOTE_DIALECT_MIDI;
	info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
	snprintf(info->name, sizeof(info->name), "%s", "Notes In");
	return true;
}

#endif