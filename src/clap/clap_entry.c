#ifdef __CLAP__

#include <string.h>

#include "lib_clap/include/clap/clap.h"
#include "clap/clap_entry.h"
#include "clap/clap_factory.h"

/* Plugin entry initialize */
bool entry_init(const char *plugin_path)
{
	(void)plugin_path;
	return true;
}

/* Plugin entry deinitialize */
void entry_deinit(void) {}

/* Get the plugin factory */
const void *entry_get_factory(const char *factory_id)
{
	extern const clap_plugin_factory_t plugin_factory;
	if (!strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID))
		return &plugin_factory;
	return NULL;
}

/* CLAP entry point */
CLAP_EXPORT const clap_plugin_entry_t clap_entry =
	{
		.clap_version = CLAP_VERSION_INIT,
		.init = entry_init,
		.deinit = entry_deinit,
		.get_factory = entry_get_factory};

#endif /* __CLAP__ */