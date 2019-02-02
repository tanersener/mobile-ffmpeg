/* amp.c

   Free software by Richard W.E. Furse. Do with as you will. No
   warranty.

   This LADSPA plugin provides simple mono and stereo amplifiers.

   This file has poor memory protection. Failures during malloc() will
   not recover nicely. */

/*****************************************************************************/

#include <stdlib.h>
#include <string.h>

/*****************************************************************************/

#include "ladspa.h"
#include "utils.h"

/*****************************************************************************/

/* The port numbers for the plugin: */

#define AMP_CONTROL 0
#define AMP_INPUT1  1
#define AMP_OUTPUT1 2
#define AMP_INPUT2  3
#define AMP_OUTPUT2 4

/*****************************************************************************/

/* The structure used to hold port connection information and state
   (actually gain controls require no further state). */

typedef struct {

  /* Ports:
     ------ */

  LADSPA_Data * m_pfControlValue;
  LADSPA_Data * m_pfInputBuffer1;
  LADSPA_Data * m_pfOutputBuffer1;
  LADSPA_Data * m_pfInputBuffer2;  /* (Not used for mono) */
  LADSPA_Data * m_pfOutputBuffer2; /* (Not used for mono) */

} Amplifier;

/*****************************************************************************/

/* Construct a new plugin instance. */
static LADSPA_Handle 
instantiateAmplifier(const LADSPA_Descriptor * Descriptor,
		     unsigned long             SampleRate) {
  return malloc(sizeof(Amplifier));
}

/*****************************************************************************/

/* Connect a port to a data location. */
static void 
connectPortToAmplifier(LADSPA_Handle Instance,
		       unsigned long Port,
		       LADSPA_Data * DataLocation) {

  Amplifier * psAmplifier;

  psAmplifier = (Amplifier *)Instance;
  switch (Port) {
  case AMP_CONTROL:
    psAmplifier->m_pfControlValue = DataLocation;
    break;
  case AMP_INPUT1:
    psAmplifier->m_pfInputBuffer1 = DataLocation;
    break;
  case AMP_OUTPUT1:
    psAmplifier->m_pfOutputBuffer1 = DataLocation;
    break;
  case AMP_INPUT2:
    /* (This should only happen for stereo.) */
    psAmplifier->m_pfInputBuffer2 = DataLocation;
    break;
  case AMP_OUTPUT2:
    /* (This should only happen for stereo.) */
    psAmplifier->m_pfOutputBuffer2 = DataLocation;
    break;
  }
}

/*****************************************************************************/

static void 
runMonoAmplifier(LADSPA_Handle Instance,
		 unsigned long SampleCount) {
  
  LADSPA_Data * pfInput;
  LADSPA_Data * pfOutput;
  LADSPA_Data fGain;
  Amplifier * psAmplifier;
  unsigned long lSampleIndex;

  psAmplifier = (Amplifier *)Instance;

  pfInput = psAmplifier->m_pfInputBuffer1;
  pfOutput = psAmplifier->m_pfOutputBuffer1;
  fGain = *(psAmplifier->m_pfControlValue);

  for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) 
    *(pfOutput++) = *(pfInput++) * fGain;
}

/*****************************************************************************/

static void 
runStereoAmplifier(LADSPA_Handle Instance,
		   unsigned long SampleCount) {
  
  LADSPA_Data * pfInput;
  LADSPA_Data * pfOutput;
  LADSPA_Data fGain;
  Amplifier * psAmplifier;
  unsigned long lSampleIndex;

  psAmplifier = (Amplifier *)Instance;

  fGain = *(psAmplifier->m_pfControlValue);

  pfInput = psAmplifier->m_pfInputBuffer1;
  pfOutput = psAmplifier->m_pfOutputBuffer1;
  for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) 
    *(pfOutput++) = *(pfInput++) * fGain;

  pfInput = psAmplifier->m_pfInputBuffer2;
  pfOutput = psAmplifier->m_pfOutputBuffer2;
  for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) 
    *(pfOutput++) = *(pfInput++) * fGain;
}

/*****************************************************************************/

/* Throw away an amplifier. */
static void 
cleanupAmplifier(LADSPA_Handle Instance) {
  free(Instance);
}

/*****************************************************************************/

LADSPA_Descriptor * g_psMonoDescriptor = NULL;
LADSPA_Descriptor * g_psStereoDescriptor = NULL;

/*****************************************************************************/

/* Called automatically when the plugin library is first loaded. */
ON_LOAD_ROUTINE {

  char ** pcPortNames;
  LADSPA_PortDescriptor * piPortDescriptors;
  LADSPA_PortRangeHint * psPortRangeHints;

  g_psMonoDescriptor
    = (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));
  g_psStereoDescriptor 
    = (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

  if (g_psMonoDescriptor) {
  
    g_psMonoDescriptor->UniqueID
      = 1048;
    g_psMonoDescriptor->Label
      = strdup("amp_mono");
    g_psMonoDescriptor->Properties
      = LADSPA_PROPERTY_HARD_RT_CAPABLE;
    g_psMonoDescriptor->Name 
      = strdup("Mono Amplifier");
    g_psMonoDescriptor->Maker
      = strdup("Richard Furse (LADSPA example plugins)");
    g_psMonoDescriptor->Copyright
      = strdup("None");
    g_psMonoDescriptor->PortCount
      = 3;
    piPortDescriptors
      = (LADSPA_PortDescriptor *)calloc(3, sizeof(LADSPA_PortDescriptor));
    g_psMonoDescriptor->PortDescriptors
      = (const LADSPA_PortDescriptor *)piPortDescriptors;
    piPortDescriptors[AMP_CONTROL]
      = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
    piPortDescriptors[AMP_INPUT1]
      = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
    piPortDescriptors[AMP_OUTPUT1]
      = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
    pcPortNames
      = (char **)calloc(3, sizeof(char *));
    g_psMonoDescriptor->PortNames 
      = (const char **)pcPortNames;
    pcPortNames[AMP_CONTROL]
      = strdup("Gain");
    pcPortNames[AMP_INPUT1]
      = strdup("Input");
    pcPortNames[AMP_OUTPUT1]
      = strdup("Output");
    psPortRangeHints = ((LADSPA_PortRangeHint *)
			calloc(3, sizeof(LADSPA_PortRangeHint)));
    g_psMonoDescriptor->PortRangeHints
      = (const LADSPA_PortRangeHint *)psPortRangeHints;
    psPortRangeHints[AMP_CONTROL].HintDescriptor
      = (LADSPA_HINT_BOUNDED_BELOW 
	 | LADSPA_HINT_LOGARITHMIC
	 | LADSPA_HINT_DEFAULT_1);
    psPortRangeHints[AMP_CONTROL].LowerBound 
      = 0;
    psPortRangeHints[AMP_INPUT1].HintDescriptor
      = 0;
    psPortRangeHints[AMP_OUTPUT1].HintDescriptor
      = 0;
    g_psMonoDescriptor->instantiate 
      = instantiateAmplifier;
    g_psMonoDescriptor->connect_port 
      = connectPortToAmplifier;
    g_psMonoDescriptor->activate
      = NULL;
    g_psMonoDescriptor->run
      = runMonoAmplifier;
    g_psMonoDescriptor->run_adding
      = NULL;
    g_psMonoDescriptor->set_run_adding_gain
      = NULL;
    g_psMonoDescriptor->deactivate
      = NULL;
    g_psMonoDescriptor->cleanup
      = cleanupAmplifier;
  }
  
  if (g_psStereoDescriptor) {
    
    g_psStereoDescriptor->UniqueID
      = 1049;
    g_psStereoDescriptor->Label
      = strdup("amp_stereo");
    g_psStereoDescriptor->Properties
      = LADSPA_PROPERTY_HARD_RT_CAPABLE;
    g_psStereoDescriptor->Name 
      = strdup("Stereo Amplifier");
    g_psStereoDescriptor->Maker
      = strdup("Richard Furse (LADSPA example plugins)");
    g_psStereoDescriptor->Copyright
      = strdup("None");
    g_psStereoDescriptor->PortCount
      = 5;
    piPortDescriptors
      = (LADSPA_PortDescriptor *)calloc(5, sizeof(LADSPA_PortDescriptor));
    g_psStereoDescriptor->PortDescriptors
      = (const LADSPA_PortDescriptor *)piPortDescriptors;
    piPortDescriptors[AMP_CONTROL]
      = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
    piPortDescriptors[AMP_INPUT1]
      = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
    piPortDescriptors[AMP_OUTPUT1]
      = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
    piPortDescriptors[AMP_INPUT2]
      = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
    piPortDescriptors[AMP_OUTPUT2]
      = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
    pcPortNames
      = (char **)calloc(5, sizeof(char *));
    g_psStereoDescriptor->PortNames 
      = (const char **)pcPortNames;
    pcPortNames[AMP_CONTROL]
      = strdup("Gain");
    pcPortNames[AMP_INPUT1]
      = strdup("Input (Left)");
    pcPortNames[AMP_OUTPUT1]
      = strdup("Output (Left)");
    pcPortNames[AMP_INPUT2]
      = strdup("Input (Right)");
    pcPortNames[AMP_OUTPUT2]
      = strdup("Output (Right)");
    psPortRangeHints = ((LADSPA_PortRangeHint *)
			calloc(5, sizeof(LADSPA_PortRangeHint)));
    g_psStereoDescriptor->PortRangeHints
      = (const LADSPA_PortRangeHint *)psPortRangeHints;
    psPortRangeHints[AMP_CONTROL].HintDescriptor
      = (LADSPA_HINT_BOUNDED_BELOW 
	 | LADSPA_HINT_LOGARITHMIC
	 | LADSPA_HINT_DEFAULT_1);
    psPortRangeHints[AMP_CONTROL].LowerBound 
      = 0;
    psPortRangeHints[AMP_INPUT1].HintDescriptor
      = 0;
    psPortRangeHints[AMP_OUTPUT1].HintDescriptor
      = 0;
    psPortRangeHints[AMP_INPUT2].HintDescriptor
      = 0;
    psPortRangeHints[AMP_OUTPUT2].HintDescriptor
      = 0;
    g_psStereoDescriptor->instantiate 
      = instantiateAmplifier;
    g_psStereoDescriptor->connect_port 
      = connectPortToAmplifier;
    g_psStereoDescriptor->activate
      = NULL;
    g_psStereoDescriptor->run
      = runStereoAmplifier;
    g_psStereoDescriptor->run_adding
      = NULL;
    g_psStereoDescriptor->set_run_adding_gain
      = NULL;
    g_psStereoDescriptor->deactivate
      = NULL;
    g_psStereoDescriptor->cleanup
      = cleanupAmplifier;
  }
}

/*****************************************************************************/

static void
deleteDescriptor(LADSPA_Descriptor * psDescriptor) {
  unsigned long lIndex;
  if (psDescriptor) {
    free((char *)psDescriptor->Label);
    free((char *)psDescriptor->Name);
    free((char *)psDescriptor->Maker);
    free((char *)psDescriptor->Copyright);
    free((LADSPA_PortDescriptor *)psDescriptor->PortDescriptors);
    for (lIndex = 0; lIndex < psDescriptor->PortCount; lIndex++)
      free((char *)(psDescriptor->PortNames[lIndex]));
    free((char **)psDescriptor->PortNames);
    free((LADSPA_PortRangeHint *)psDescriptor->PortRangeHints);
    free(psDescriptor);
  }
}

/*****************************************************************************/

/* Called automatically when the library is unloaded. */
ON_UNLOAD_ROUTINE {
  deleteDescriptor(g_psMonoDescriptor);
  deleteDescriptor(g_psStereoDescriptor);
}

/*****************************************************************************/

/* Return a descriptor of the requested plugin type. There are two
   plugin types available in this library (mono and stereo). */
const LADSPA_Descriptor * 
ladspa_descriptor(unsigned long Index) {
  /* Return the requested descriptor or null if the index is out of
     range. */
  switch (Index) {
  case 0:
    return g_psMonoDescriptor;
  case 1:
    return g_psStereoDescriptor;
  default:
    return NULL;
  }
}

/*****************************************************************************/

/* EOF */
