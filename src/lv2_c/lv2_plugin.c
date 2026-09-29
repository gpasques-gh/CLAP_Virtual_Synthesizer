#ifdef __LV2__

#include "lv2_h/lv2_plugin.h"

static LV2_Handle instantiate(
	const LV2_Descriptor *descriptor,
	double rate,
	const char *bundle_path,
	const LV2_Feature *const *features)
{
	(void)descriptor; (void)bundle_path;

	/* Allocate and initialize the instance structure */
	synth_plugin_t *self = (synth_plugin_t *)calloc(1, sizeof(synth_plugin_t));
	if (!self) return NULL;

	/* Get the host features */
	const char *missing = lv2_features_query(
		features, 
		LV2_LOG__log, &self->logger.log, false,
		LV2_URID__map, &self->map, true);
	lv2_log_logger_set_map(&self->logger, self->map);

	/* If a feature is missing from the host */
	if (missing)
	{
		lv2_log_error(&self->logger, "Missing feature <%s>\n", missing);
		free(self); return NULL;
	}

	/* Decide which variant to use depending on the plugin URI */
	//if (!strcmp(descriptor->URI, ))
}

#endif 