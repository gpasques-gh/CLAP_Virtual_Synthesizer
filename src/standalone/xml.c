#ifndef __CLAP__

#ifdef __WINDOWS__

#include "libxml/parser.h"
#include "libxml/tree.h"

/* Dirty and ugly trick to have both raylib
and Windows API working together */
#define INITGUID
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define CloseWindow CloseWindowWin32
#define ShowCursor ShowCursorWin32
#include <windows.h>
#include <shobjidl.h>
#undef CloseWindow
#undef ShowCursor
#undef LoadImage
#undef DrawText
#undef DrawTextEx
#undef PlaySound

#elif defined(__LINUX__)
#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/tree.h>
#endif

#include "lib_raygui/src/raygui.h"
#define __NO_RL__
#include "defs.h"
#undef __NO_RL__
#include "standalone/xml.h"

/*
 * Saving a preset into an XML file :
 * - ADSR envelope parameters
 * - Filter ADSR envelope parameters and ON/OFF
 * - Filter cutoff
 * - Oscillators waveforms
 * - Detune effect
 * - Amplification
 */
int save_preset(
	synth_t synth,
	float attack, float decay,
	float sustain, float release,
	int wave_a, int wave_b, int wave_c,
	char *preset_filename, bool *saving_preset,
	bool distortion, bool overdrive,
	float distortion_amount)
{

	char filename[1024] = "presets/";

	/* Textbox for the preset name */
	int res = GuiTextInputBox((Rectangle){WIDTH / 2 - 100, HEIGHT / 2 - 50, 200, 100}, "Preset name :", "", preset_filename, 20, "Save preset", (int *)saving_preset, false);

	if (res == 1)
	{
		*saving_preset = false;
		strcat(filename, preset_filename);
		strcat(filename, ".xml");

		char text_element[1024];

		/* Getting the XML document pointer */
		xmlDocPtr doc = NULL;

		/* Getting the XML node pointers*/
		xmlNodePtr root_node = NULL;
		xmlNodePtr adsr_node = NULL;
		xmlNodePtr filter_node = NULL;
		xmlNodePtr filter_adsr_node = NULL;
		xmlNodePtr osc_node = NULL;
		xmlNodePtr effects_node = NULL;
		xmlNodePtr lfo_node = NULL;
		xmlNodePtr distortion_node = NULL;

		LIBXML_TEST_VERSION

		/* Initializing the XML document */
		doc = xmlNewDoc(BAD_CAST "1.0");

		/* Getting the root node */
		root_node = xmlNewNode(NULL, BAD_CAST "preset");
		xmlDocSetRootElement(doc, root_node);

		/* ADSR */
		adsr_node = xmlNewChild(root_node, NULL, BAD_CAST "adsr", NULL);
		/* Attack */
		snprintf(text_element, 1024, "%.2f", attack);
		xmlNewChild(adsr_node, NULL, BAD_CAST "attack", BAD_CAST text_element);
		/* Decay */
		snprintf(text_element, 1024, "%.2f", decay);
		xmlNewChild(adsr_node, NULL, BAD_CAST "decay", BAD_CAST text_element);
		/* Sustain */
		snprintf(text_element, 1024, "%.2f", sustain);
		xmlNewChild(adsr_node, NULL, BAD_CAST "sustain", BAD_CAST text_element);
		/* Release */
		snprintf(text_element, 1024, "%.2f", release);
		xmlNewChild(adsr_node, NULL, BAD_CAST "release", BAD_CAST text_element);

		/* Filter */
		filter_node = xmlNewChild(root_node, NULL, BAD_CAST "filter", NULL);
		/* Filter ADSR */
		filter_adsr_node = xmlNewChild(filter_node, NULL, BAD_CAST "filter_adsr", NULL);
		/* Attack */
		snprintf(text_element, 1024, "%.2f", synth.filter.adsr.attack);
		xmlNewChild(filter_adsr_node, NULL, BAD_CAST "attack", BAD_CAST text_element);
		/* Decay */
		snprintf(text_element, 1024, "%.2f", synth.filter.adsr.decay);
		xmlNewChild(filter_adsr_node, NULL, BAD_CAST "decay", BAD_CAST text_element);
		/* Sustain */
		snprintf(text_element, 1024, "%.2f", synth.filter.adsr.sustain);
		xmlNewChild(filter_adsr_node, NULL, BAD_CAST "sustain", BAD_CAST text_element);
		/* Release */
		snprintf(text_element, 1024, "%.2f", synth.filter.adsr.release);
		xmlNewChild(filter_adsr_node, NULL, BAD_CAST "release", BAD_CAST text_element);
		/* Filter cutoff */
		snprintf(text_element, 1024, "%.2f", synth.filter.cutoff);
		xmlNewChild(filter_node, NULL, BAD_CAST "cutoff", BAD_CAST text_element);
		/* Filter envelope ON/OFF */
		snprintf(text_element, 1024, "%d", synth.filter.env);
		xmlNewChild(filter_node, NULL, BAD_CAST "envelope_on", BAD_CAST text_element);

		/* Oscillators waveforms */
		osc_node = xmlNewChild(root_node, NULL, BAD_CAST "oscillators", NULL);
		/* Oscillator A */
		snprintf(text_element, 1024, "%d", wave_a);
		xmlNewChild(osc_node, NULL, BAD_CAST "osc_a", BAD_CAST text_element);
		/* Oscillator B */
		snprintf(text_element, 1024, "%d", wave_b);
		xmlNewChild(osc_node, NULL, BAD_CAST "osc_b", BAD_CAST text_element);
		/* Oscillator C */
		snprintf(text_element, 1024, "%d", wave_c);
		xmlNewChild(osc_node, NULL, BAD_CAST "osc_c", BAD_CAST text_element);

		/* Effects */
		effects_node = xmlNewChild(root_node, NULL, BAD_CAST "effects", NULL);
		/* Detune */
		snprintf(text_element, 1024, "%.2f", synth.detune);
		xmlNewChild(effects_node, NULL, BAD_CAST "detune", BAD_CAST text_element);
		/* Amplification */
		snprintf(text_element, 1024, "%.2f", synth.amp);
		xmlNewChild(effects_node, NULL, BAD_CAST "amp", BAD_CAST text_element);
		/* Arpeggio */
		snprintf(text_element, 1024, "%d", synth.arp);
		xmlNewChild(effects_node, NULL, BAD_CAST "arp", BAD_CAST text_element);
		/* BPM */
		snprintf(text_element, 1024, "%.2f", synth.bpm);
		xmlNewChild(effects_node, NULL, BAD_CAST "bpm", BAD_CAST text_element);

		/* LFO*/
		lfo_node = xmlNewChild(effects_node, NULL, BAD_CAST "lfo", NULL);
		/* LFO waveform */
		snprintf(text_element, 1024, "%d", synth.lfo.osc.wave);
		xmlNewChild(lfo_node, NULL, BAD_CAST "lfo_wave", BAD_CAST text_element);
		/* LFO frequency */
		snprintf(text_element, 1024, "%.2f", synth.lfo.osc.freq);
		xmlNewChild(lfo_node, NULL, BAD_CAST "lfo_freq", BAD_CAST text_element);
		/* LFO parameter */
		snprintf(text_element, 1024, "%d", synth.lfo.mod_param);
		xmlNewChild(lfo_node, NULL, BAD_CAST "lfo_param", BAD_CAST text_element);

		/* Distortion */
		distortion_node = xmlNewChild(effects_node, NULL, BAD_CAST "distortion", NULL);
		/* Distortion ON/OFF */
		snprintf(text_element, 1024, "%d", distortion);
		xmlNewChild(distortion_node, NULL, BAD_CAST "dist_on_off", BAD_CAST text_element);
		/* Overdrive ON/OFF */
		snprintf(text_element, 1024, "%d", overdrive);
		xmlNewChild(distortion_node, NULL, BAD_CAST "od_on_off", BAD_CAST text_element);
		/* Distortion amount */
		snprintf(text_element, 1024, "%.2f", distortion_amount);
		xmlNewChild(distortion_node, NULL, BAD_CAST "amount", BAD_CAST text_element);

		/* Saving the XML document into the file */
		xmlSaveFormatFileEnc(filename, doc, "UTF-8", 1);
		xmlFreeDoc(doc);
		xmlCleanupParser();
	}

	return 0;
}

/*
 * Load a preset from an XML file to the application :
 * - ADSR envelope parameters
 * - Filter ADSR envelope parameters and ON/OFF
 * - Filter cutoff
 * - Oscillators waveforms
 * - Detune effect
 * - Amplification
 */
int load_preset(
	synth_t *synth,
	float *attack, float *decay,
	float *sustain, float *release,
	int *wave_a, int *wave_b, int *wave_c,
	bool *distortion, bool *overdrive,
	float *distortion_amount,
	bool *loading_preset)
{
	char filename[1024];

#ifdef __WINDOWS__
	/* Opening Windows file dialog */
	HRESULT hr = CoInitializeEx(
		NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (!SUCCEEDED(hr))
	{
		fprintf(stderr, "error while opening windows file dialog.\n");
		*loading_preset = false;
		return 1;
	}

	/* Opening the file dialog */
	IFileOpenDialog *file_dialog;
	hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
						  &IID_IFileOpenDialog, (void **)(&file_dialog));
	if (!SUCCEEDED(hr))
	{
		CoUninitialize();
		fprintf(stderr, "error while opening windows file dialog.\n");
		*loading_preset = false;
		return 1;
	}

	/* Showing the file dialog to the screen */
	hr = file_dialog->lpVtbl->Show(file_dialog, NULL);
	if (!SUCCEEDED(hr))
	{
		file_dialog->lpVtbl->Release(file_dialog);
		CoUninitialize();
		fprintf(stderr, "error while opening windows file dialog.\n");
		*loading_preset = false;
		return 1;
	}

	/* Getting the item selected by
	the user from the file dialog */
	IShellItem *item;
	hr = file_dialog->lpVtbl->GetResult(file_dialog, &item);
	if (!SUCCEEDED(hr))
	{
		file_dialog->lpVtbl->Release(file_dialog);
		CoUninitialize();
		fprintf(stderr, "error while opening windows file dialog.\n");
		*loading_preset = false;
		return 1;
	}

	/* Getting the file path from the selected item */
	PWSTR file_path;
	hr = item->lpVtbl->GetDisplayName(
		item, SIGDN_FILESYSPATH, &file_path);
	if (!SUCCEEDED(hr))
	{
		item->lpVtbl->Release(item);
		file_dialog->lpVtbl->Release(file_dialog);
		CoUninitialize();
		fprintf(stderr, "error while opening windows file dialog.\n");
		*loading_preset = false;
		return 1;
	}

	/* Converting the PWSTR file path
	to the char file path*/
	WideCharToMultiByte(
		CP_UTF8, 0,
		file_path, -1,
		filename, sizeof(filename),
		NULL, NULL);

	/* Free the Windows file dialog window */
	CoTaskMemFree(file_path);
	item->lpVtbl->Release(item);
	file_dialog->lpVtbl->Release(file_dialog);
	CoUninitialize();
#elif defined(__LINUX__)
	/* Run Zenity for a simple file dialog solution */
	const char *home = getenv("HOME");
	char zenity_command[1024] = "zenity --file-selection --filename '";
	strcat(zenity_command, home);
	strcat(zenity_command, "/ALSA_raygui_Synthesizer/presets/' --file-filter '*.xml'");

	/* Get the file path from Zenity */
	FILE *f = popen(zenity_command, "r");
	fgets(filename, 1024, f);
	filename[strcspn(filename, "\n")] = '\0';
	pclose(f);
#endif

	/* Getting the XML document pointer */
	xmlDoc *doc = NULL;

	/* Getting the XML node pointers*/
	xmlNode *root = NULL;
	xmlNode *node = NULL;

	/* Reading the XML file into the XML document pointer */
	doc = xmlReadFile(filename, NULL, 0);

	*loading_preset = false;

	if (doc == NULL)
	{
		fprintf(stderr, "failed to parse xml file.\n");
		return 1;
	}

	/* Getting the root node of the XML document */
	root = xmlDocGetRootElement(doc);

	/* Looping on the main nodes */
	for (node = root->children; node; node = node->next)
	{
		/* ADSR envelope */
		if (node->type == XML_ELEMENT_NODE &&
			xmlStrcmp(node->name, BAD_CAST "adsr") == 0)
		{
			parse_adsr(
				node, synth, attack,
				decay, sustain, release, false);
		}
		/* Filter */
		else if (node->type == XML_ELEMENT_NODE &&
				 xmlStrcmp(node->name, BAD_CAST "filter") == 0)
		{
			parse_filter(node, synth);
		}
		/* Oscillators waveforms */
		else if (node->type == XML_ELEMENT_NODE &&
				 xmlStrcmp(node->name, BAD_CAST "oscillators") == 0)
		{
			parse_oscillators(node, wave_a, wave_b, wave_c);
		}
		/* Effects */
		else if (node->type == XML_ELEMENT_NODE &&
				 xmlStrcmp(node->name, BAD_CAST "effects") == 0)
		{
			parse_effects(node, synth,
						  distortion, overdrive, distortion_amount);
		}
	}

	return 0;
}

int parse_effects(xmlNode *effects_node, synth_t *synth,
				  bool *distortion, bool *overdrive, float *distortion_amount)
{
	xmlNode *child = NULL;
	/* Looping on effects */
	for (child = effects_node->children; child; child = child->next)
	{
		/* Detune effect */
		if (child->type == XML_ELEMENT_NODE &&
			xmlStrcmp(child->name, BAD_CAST "detune") == 0)
		{
			xmlChar *detune = xmlNodeGetContent(child);
			char *end_ptr = NULL;
			float detune_float = strtof((const char *)detune, &end_ptr);
			if (end_ptr == (char *)detune)
			{
				fprintf(stderr, "bad cutoff value.\n");
				return 1;
			}

			if (detune_float > 1.0)
			{
				detune_float = 1.0;
			}
			else if (detune_float < 0.0)
			{
				detune_float = 0.0;
			}
			synth->detune = detune_float;
		}
		/* Amplification */
		else if (child->type == XML_ELEMENT_NODE &&
				 xmlStrcmp(child->name, BAD_CAST "amp") == 0)
		{
			xmlChar *amp = xmlNodeGetContent(child);
			char *end_ptr = NULL;
			float amp_float = strtof((const char *)amp, &end_ptr);
			if (end_ptr == (char *)amp)
			{
				fprintf(stderr, "bad cutoff value.\n");
				return 1;
			}

			if (amp_float > 1.0)
			{
				amp_float = 1.0;
			}
			else if (amp_float < 0.0)
			{
				amp_float = 0.0;
			}
			synth->amp = amp_float;
		}
		else if (child->type == XML_ELEMENT_NODE &&
				 xmlStrcmp(child->name, BAD_CAST "arp") == 0)
		{
			xmlChar *arp = xmlNodeGetContent(child);
			char *end_ptr = NULL;
			int arp_int = strtol((const char *)arp, &end_ptr, 10);
			if (end_ptr == (char *)arp)
			{
				fprintf(stderr, "bad arp value.\n");
				return 1;
			}

			if (arp_int > 1)
			{
				arp_int = 1;
			}
			else if (arp_int < 0)
			{
				arp_int = 0;
			}
			synth->arp = arp_int;
		}
		else if (child->type == XML_ELEMENT_NODE &&
				 xmlStrcmp(child->name, BAD_CAST "bpm") == 0)
		{
			xmlChar *bpm = xmlNodeGetContent(child);
			char *end_ptr = NULL;
			int bpm_float = strtof((const char *)bpm, &end_ptr);
			if (end_ptr == (char *)bpm)
			{
				fprintf(stderr, "bad bpm value.\n");
				return 1;
			}

			if (bpm_float > 250.0)
			{
				bpm_float = 250.0;
			}
			else if (bpm_float < 0.0)
			{
				bpm_float = 0.0;
			}
			synth->bpm = bpm_float;
		}
		/* LFO */
		else if (child->type == XML_ELEMENT_NODE &&
				 xmlStrcmp(child->name, BAD_CAST "lfo") == 0)
		{
			parse_lfo(child, synth);
		}
		/* Distortion */
		else if (child->type == XML_ELEMENT_NODE &&
				 xmlStrcmp(child->name, BAD_CAST "distortion") == 0)
		{
			parse_distortion(child, distortion,
							 overdrive, distortion_amount);
		}
	}
	return 0;
}

int parse_filter(xmlNode *filter_node,
				 synth_t *synth)
{
	xmlNode *child = NULL;

	/* Looping on the filter node children */
	for (child = filter_node->children; child; child = child->next)
	{ /* Filter ADSR envelope */
		if (child->type == XML_ELEMENT_NODE &&
			xmlStrcmp(child->name, BAD_CAST "filter_adsr") == 0)
		{
			parse_adsr(
				child, synth, NULL,
				NULL, NULL, NULL, true);
		}
		/* Filter cutoff */
		else if (child->type == XML_ELEMENT_NODE &&
				 xmlStrcmp(child->name, BAD_CAST "cutoff") == 0)
		{
			xmlChar *cutoff = xmlNodeGetContent(child);
			char *end_ptr = NULL;
			float cutoff_float = strtof((const char *)cutoff, &end_ptr);
			if (end_ptr == (char *)cutoff)
			{
				fprintf(stderr, "bad cutoff value.\n");
				return 1;
			}

			if (cutoff_float > 1.0)
			{
				cutoff_float = 1.0;
			}
			else if (cutoff_float < 0.0)
			{
				cutoff_float = 0.0;
			}
			synth->filter.cutoff = cutoff_float;
		}
		/* Filter ADSR envelope ON/OFF */
		else if (child->type == XML_ELEMENT_NODE &&
				 xmlStrcmp(child->name, BAD_CAST "envelope_on") == 0)
		{
			xmlChar *envelope_on = xmlNodeGetContent(child);
			char *end_ptr = NULL;
			int env_on_int = strtol((const char *)envelope_on, &end_ptr, 10);
			if (end_ptr == (char *)envelope_on)
			{
				fprintf(stderr, "bad envelope value.\n");
				return 1;
			}

			if (env_on_int > 1)
			{
				env_on_int = 1;
			}
			else if (env_on_int < 0)
			{
				env_on_int = 0;
			}
			synth->filter.env = env_on_int;
		}
	}
	return 0;
}

int parse_oscillators(xmlNode *lfo_node,
					  int *wave_a, int *wave_b, int *wave_c)
{
	xmlNode *child = NULL;
	/* Looping on the oscillators nodes*/
	for (child = lfo_node->children; child; child = child->next)
	{ /* Oscillator A */
		if (child->type == XML_ELEMENT_NODE &&
			xmlStrcmp(child->name, BAD_CAST "osc_a") == 0)
		{
			xmlChar *osc_a = xmlNodeGetContent(child);
			char *end_ptr = NULL;
			short osc_a_wave = strtol((const char *)osc_a, &end_ptr, 10);
			if (end_ptr == (char *)osc_a)
			{
				fprintf(stderr, "bad osc a value.\n");
				return 1;
			}

			if (osc_a_wave > 4)
			{
				osc_a_wave = 4;
			}
			else if (osc_a_wave < 0)
			{
				osc_a_wave = 0;
			}
			*wave_a = osc_a_wave;
		}
		/* Oscillator B */
		else if (child->type == XML_ELEMENT_NODE &&
				 xmlStrcmp(child->name, BAD_CAST "osc_b") == 0)
		{
			xmlChar *osc_b = xmlNodeGetContent(child);
			char *end_ptr = NULL;
			short osc_b_wave = strtol((const char *)osc_b, &end_ptr, 10);
			if (end_ptr == (char *)osc_b)
			{
				fprintf(stderr, "bad osc b value.\n");
				return 1;
			}

			if (osc_b_wave > SAWTOOTH_WAVE)
			{
				osc_b_wave = SAWTOOTH_WAVE;
			}
			else if (osc_b_wave < SINE_WAVE)
			{
				osc_b_wave = SINE_WAVE;
			}
			*wave_b = osc_b_wave;
		}
		/* Oscillator C */
		else if (child->type == XML_ELEMENT_NODE &&
				 xmlStrcmp(child->name, BAD_CAST "osc_c") == 0)
		{
			xmlChar *osc_c = xmlNodeGetContent(child);
			char *end_ptr = NULL;
			short osc_c_wave = strtol((const char *)osc_c, &end_ptr, 10);
			if (end_ptr == (char *)osc_c)
			{
				fprintf(stderr, "bad osc b value.\n");
				return 1;
			}

			if (osc_c_wave > SAWTOOTH_WAVE)
			{
				osc_c_wave = SAWTOOTH_WAVE;
			}
			else if (osc_c_wave < SINE_WAVE)
			{
				osc_c_wave = SINE_WAVE;
			}
			*wave_c = osc_c_wave;
		}
	}
	return 0;
}

int parse_lfo(xmlNode *lfo_node, synth_t *synth)
{
	xmlNode *lfo_child = NULL;

	for (lfo_child = lfo_node->children; lfo_child; lfo_child = lfo_child->next)
	{
		/* LFO waveform*/
		if (lfo_child->type == XML_ELEMENT_NODE &&
			xmlStrcmp(lfo_child->name, BAD_CAST "lfo_wave") == 0)
		{
			xmlChar *lfo_wave = xmlNodeGetContent(lfo_child);
			char *end_ptr = NULL;
			int lfo_wave_int = strtol((const char *)lfo_wave, &end_ptr, 10);
			if (end_ptr == (char *)lfo_wave)
			{
				fprintf(stderr, "bad lfo waveform value.\n");
				return 1;
			}

			if (lfo_wave_int > SAWTOOTH_WAVE)
			{
				lfo_wave_int = SAWTOOTH_WAVE;
			}
			else if (lfo_wave_int < SINE_WAVE)
			{
				lfo_wave_int = SINE_WAVE;
			}
			synth->lfo.osc.wave = lfo_wave_int;
		}
		else if (lfo_child->type == XML_ELEMENT_NODE &&
				 xmlStrcmp(lfo_child->name, BAD_CAST "lfo_freq") == 0)
		{
			xmlChar *lfo_freq = xmlNodeGetContent(lfo_child);
			char *end_ptr = NULL;
			float lfo_freq_float = strtof((const char *)lfo_freq, &end_ptr);
			if (end_ptr == (char *)lfo_freq)
			{
				fprintf(stderr, "bad lfo frequency value.\n");
				return 1;
			}

			if (lfo_freq_float > 1.0)
			{
				lfo_freq_float = 1.0;
			}
			else if (lfo_freq_float < 0.0)
			{
				lfo_freq_float = 0.0;
			}
			synth->lfo.osc.freq = lfo_freq_float;
		}
		else if (lfo_child->type == XML_ELEMENT_NODE &&
				 xmlStrcmp(lfo_child->name, BAD_CAST "lfo_param") == 0)
		{
			xmlChar *lfo_param = xmlNodeGetContent(lfo_child);
			char *end_ptr = NULL;
			int lfo_param_int = strtol((const char *)lfo_param, &end_ptr, 10);
			if (end_ptr == (char *)lfo_param)
			{
				fprintf(stderr, "bad lfo param value.\n");
				return 1;
			}

			if (lfo_param_int > LFO_AMP)
			{
				lfo_param_int = LFO_AMP;
			}
			else if (lfo_param_int < LFO_OFF)
			{
				lfo_param_int = LFO_OFF;
			}
			synth->lfo.mod_param = lfo_param_int;
		}
	}
	return 0;
}

int parse_distortion(xmlNode *distortion_node,
					 bool *distortion, bool *overdrive, float *distortion_amount)
{
	xmlNode *dist_child = NULL;

	for (dist_child = distortion_node->children; dist_child; dist_child = dist_child->next)
	{
		/* Distortion ON/OFF */
		if (dist_child->type == XML_ELEMENT_NODE &&
			xmlStrcmp(dist_child->name, BAD_CAST "dist_on_off") == 0)
		{
			xmlChar *dist_on_off = xmlNodeGetContent(dist_child);
			char *end_ptr = NULL;
			int dist_bool = strtol((const char *)dist_on_off, &end_ptr, 10);
			if (end_ptr == (char *)dist_on_off)
			{
				fprintf(stderr, "bad distortion on/off value.\n");
				return 1;
			}

			if (dist_bool > 1)
			{
				dist_bool = 1;
			}
			else if (dist_bool < 0)
			{
				dist_bool = 0;
			}
			*distortion = dist_bool;
		}
		/* Overdrive ON/OFF */
		else if (dist_child->type == XML_ELEMENT_NODE &&
				 xmlStrcmp(dist_child->name, BAD_CAST "od_on_off") == 0)
		{
			xmlChar *od_on_off = xmlNodeGetContent(dist_child);
			char *end_ptr = NULL;
			int od_bool = strtol((const char *)od_on_off, &end_ptr, 10);
			if (end_ptr == (char *)od_on_off)
			{
				fprintf(stderr, "bad overdrive on/off value.\n");
				return 1;
			}

			if (od_bool > 1)
			{
				od_bool = 1;
			}
			else if (od_bool < 0)
			{
				od_bool = 0;
			}
			*overdrive = od_bool;
		}
		/* Distortion amount */
		else if (dist_child->type == XML_ELEMENT_NODE &&
				 xmlStrcmp(dist_child->name, BAD_CAST "amount") == 0)
		{
			xmlChar *dist_amount = xmlNodeGetContent(dist_child);
			char *end_ptr = NULL;
			float dist_amount_float = strtof((const char *)dist_amount, &end_ptr);
			if (end_ptr == (char *)dist_amount)
			{
				fprintf(stderr, "bad distortion amount value.\n");
				return 1;
			}

			if (dist_amount_float > 1.0)
			{
				dist_amount_float = 1.0;
			}
			else if (dist_amount_float < 0.0)
			{
				dist_amount_float = 0.0;
			}
			*distortion_amount = dist_amount_float;
		}
	}
	return 0;
}

/* Parse an ADSR XML Node whether it's basic ADSR of filter ADSR */
int parse_adsr(
	xmlNode *adsr_root_node,
	synth_t *synth,
	float *attack, float *decay,
	float *sustain, float *release,
	bool filter)
{
	xmlNode *child = NULL;
	/* Looping throught the child of the ADSR root node*/
	for (child = adsr_root_node->children; child; child = child->next)
	{
		/* Attack Node */
		if (child->type == XML_ELEMENT_NODE &&
			xmlStrcmp(child->name, BAD_CAST "attack") == 0)
		{
			xmlChar *attack_str = xmlNodeGetContent(child);
			char *end_ptr = NULL;
			float attack_float = strtof((const char *)attack_str, &end_ptr);
			if (end_ptr == (char *)attack_str)
			{
				fprintf(stderr, "bad attack value.\n");
				return 1;
			}
			if (attack_float > 2.0)
			{
				attack_float = 2.0;
			}
			else if (attack_float < 0.0)
			{
				attack_float = 0.0;
			}

			if (filter)
			{
				synth->filter.adsr.attack = attack_float;
			}
			else
			{
				*attack = attack_float;
			}
		}
		/* Decay Node */
		else if (child->type == XML_ELEMENT_NODE &&
				 xmlStrcmp(child->name, BAD_CAST "decay") == 0)
		{
			xmlChar *decay_str = xmlNodeGetContent(child);
			char *end_ptr = NULL;
			float decay_float = strtof((const char *)decay_str, &end_ptr);
			if (end_ptr == (char *)decay_str)
			{
				fprintf(stderr, "bad decay value.\n");
				return 1;
			}
			if (decay_float > 2.0)
			{
				decay_float = 2.0;
			}
			else if (decay_float < 0.0)
			{
				decay_float = 0.0;
			}

			if (filter)
			{
				synth->filter.adsr.decay = decay_float;
			}
			else
			{
				*decay = decay_float;
			}
		}
		/* Sustain Node */
		else if (child->type == XML_ELEMENT_NODE &&
				 xmlStrcmp(child->name, BAD_CAST "sustain") == 0)
		{
			xmlChar *sustain_str = xmlNodeGetContent(child);
			char *end_ptr = NULL;
			float sustain_float = strtof((const char *)sustain_str, &end_ptr);
			if (end_ptr == (char *)sustain_str)
			{
				fprintf(stderr, "bad sustain value.\n");
				return 1;
			}
			if (sustain_float > 1.0)
			{
				sustain_float = 1.0;
			}
			else if (sustain_float < 0.0)
			{
				sustain_float = 0.0;
			}

			if (filter)
			{
				synth->filter.adsr.sustain = sustain_float;
			}
			else
			{
				*sustain = sustain_float;
			}
		}
		/* Release Node */
		else if (child->type == XML_ELEMENT_NODE &&
				 xmlStrcmp(child->name, BAD_CAST "release") == 0)
		{
			xmlChar *release_str = xmlNodeGetContent(child);
			char *end_ptr = NULL;
			float release_float = strtof((const char *)release_str, &end_ptr);
			if (end_ptr == (char *)release_str)
			{
				fprintf(stderr, "bad release value.\n");
				return 1;
			}
			if (release_float > 1.0)
			{
				release_float = 1.0;
			}
			else if (release_float < 0.0)
			{
				release_float = 0.0;
			}

			if (filter)
			{
				synth->filter.adsr.release = release_float;
			}
			else
			{
				*release = release_float;
			}
		}
	}
	return 0;
}

#endif