/* noise.c

   Free software by Richard W.E. Furse. Do with as you will. No
   warranty.

   This LADSPA plugin provides a simple mono noise source. 

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

#define NOISE_AMPLITUDE 0
#define NOISE_OUTPUT    1

/*****************************************************************************/

/* The structure used to hold port connection information (and gain if
   runAdding() is in use) and state (actually there's no further state
   to store here). */

typedef struct {

  /* Ports:
     ------ */

  LADSPA_Data * m_pfAmplitudeValue;
  LADSPA_Data * m_pfOutputBuffer;

  /* Run Adding Gain:
     ---------------- */

  LADSPA_Data m_fRunAddingGain;

} NoiseSource;

/*****************************************************************************/

/* Construct a new plugin instance. */
static LADSPA_Handle 
instantiateNoiseSource(const LADSPA_Descriptor * Descriptor,
		       unsigned long             SampleRate) {
  return malloc(sizeof(NoiseSource));
}

/*****************************************************************************/

/* Connect a port to a data location. */
static void 
connectPortToNoiseSource(LADSPA_Handle Instance,
			 unsigned long Port,
			 LADSPA_Data * DataLocation) {
  switch (Port) {
  case NOISE_AMPLITUDE:
    ((NoiseSource *)Instance)->m_pfAmplitudeValue = DataLocation;
    break;
  case NOISE_OUTPUT:
    ((NoiseSource *)Instance)->m_pfOutputBuffer = DataLocation;
    break;
  }
}

/*****************************************************************************/

/* Run a delay line instance for a block of SampleCount samples. */
static void 
runNoiseSource(LADSPA_Handle Instance,
	       unsigned long SampleCount) {
  
  NoiseSource * psNoiseSource;
  LADSPA_Data * pfOutput;
  LADSPA_Data fAmplitude;
  unsigned long lSampleIndex;

  psNoiseSource = (NoiseSource *)Instance;

  fAmplitude 
    = *(psNoiseSource->m_pfAmplitudeValue) * (LADSPA_Data)(2.0 / RAND_MAX);

  pfOutput = psNoiseSource->m_pfOutputBuffer;
  for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) 
    *(pfOutput++) = (rand() - (RAND_MAX / 2)) * fAmplitude;
}

/*****************************************************************************/

/* Run a delay line instance for a block of SampleCount samples. *ADD*
   the output to the output buffer. */
static void 
runAddingNoiseSource(LADSPA_Handle Instance,
		     unsigned long SampleCount) {
  
  NoiseSource * psNoiseSource;
  LADSPA_Data * pfOutput;
  LADSPA_Data fAmplitude;
  unsigned long lSampleIndex;

  psNoiseSource = (NoiseSource *)Instance;

  fAmplitude 
    = (*(psNoiseSource->m_pfAmplitudeValue)
       * psNoiseSource->m_fRunAddingGain 
       * (LADSPA_Data)(2.0 / RAND_MAX));

  pfOutput = psNoiseSource->m_pfOutputBuffer;
  for (lSampleIndex = 0; lSampleIndex < SampleCount; lSampleIndex++) 
    *(pfOutput++) += (rand() - (RAND_MAX / 2)) * fAmplitude;
}

/*****************************************************************************/

static void 
setNoiseSourceRunAddingGain(LADSPA_Handle Instance,
			    LADSPA_Data   Gain) {
  ((NoiseSource *)Instance)->m_fRunAddingGain = Gain;
}

/*****************************************************************************/

/* Throw away a simple delay line. */
static void 
cleanupNoiseSource(LADSPA_Handle Instance) {
  free(Instance);
}

/*****************************************************************************/

static LADSPA_Descriptor * g_psDescriptor;

/*****************************************************************************/

/* Called automatically when the plugin library is first loaded. */
ON_LOAD_ROUTINE {

  char ** pcPortNames;
  LADSPA_PortDescriptor * piPortDescriptors;
  LADSPA_PortRangeHint * psPortRangeHints;

  g_psDescriptor
    = (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

  if (g_psDescriptor) {

    g_psDescriptor->UniqueID
      = 1050;
    g_psDescriptor->Label
      = strdup("noise_white");
    g_psDescriptor->Properties
      = LADSPA_PROPERTY_HARD_RT_CAPABLE;
    g_psDescriptor->Name 
      = strdup("White Noise Source");
    g_psDescriptor->Maker
      = strdup("Richard Furse (LADSPA example plugins)");
    g_psDescriptor->Copyright
      = strdup("None");
    g_psDescriptor->PortCount
      = 2;
    piPortDescriptors
      = (LADSPA_PortDescriptor *)calloc(2, sizeof(LADSPA_PortDescriptor));
    g_psDescriptor->PortDescriptors
      = (const LADSPA_PortDescriptor *)piPortDescriptors;
    piPortDescriptors[NOISE_AMPLITUDE]
      = (LADSPA_PORT_INPUT 
	 | LADSPA_PORT_CONTROL);
    piPortDescriptors[NOISE_OUTPUT]
      = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
    pcPortNames
      = (char **)calloc(2, sizeof(char *));
    g_psDescriptor->PortNames 
      = (const char **)pcPortNames;
    pcPortNames[NOISE_AMPLITUDE]
      = strdup("Amplitude");
    pcPortNames[NOISE_OUTPUT]
      = strdup("Output");
    psPortRangeHints = ((LADSPA_PortRangeHint *)
			calloc(2, sizeof(LADSPA_PortRangeHint)));
    g_psDescriptor->PortRangeHints
      = (const LADSPA_PortRangeHint *)psPortRangeHints;
    psPortRangeHints[NOISE_AMPLITUDE].HintDescriptor
      = (LADSPA_HINT_BOUNDED_BELOW 
	 | LADSPA_HINT_LOGARITHMIC
	 | LADSPA_HINT_DEFAULT_1);
    psPortRangeHints[NOISE_AMPLITUDE].LowerBound 
      = 0;
    psPortRangeHints[NOISE_OUTPUT].HintDescriptor
      = 0;
    g_psDescriptor->instantiate 
      = instantiateNoiseSource;
    g_psDescriptor->connect_port 
      = connectPortToNoiseSource;
    g_psDescriptor->activate
      = NULL;
    g_psDescriptor->run
      = runNoiseSource;
    g_psDescriptor->run_adding
      = runAddingNoiseSource;
    g_psDescriptor->set_run_adding_gain
      = setNoiseSourceRunAddingGain;
    g_psDescriptor->deactivate
      = NULL;
    g_psDescriptor->cleanup
      = cleanupNoiseSource;
  }
}

/*****************************************************************************/

/* Called automatically when the library is unloaded. */
ON_UNLOAD_ROUTINE {
  long lIndex;
  if (g_psDescriptor) {
    free((char *)g_psDescriptor->Label);
    free((char *)g_psDescriptor->Name);
    free((char *)g_psDescriptor->Maker);
    free((char *)g_psDescriptor->Copyright);
    free((LADSPA_PortDescriptor *)g_psDescriptor->PortDescriptors);
    for (lIndex = 0; lIndex < g_psDescriptor->PortCount; lIndex++)
      free((char *)(g_psDescriptor->PortNames[lIndex]));
    free((char **)g_psDescriptor->PortNames);
    free((LADSPA_PortRangeHint *)g_psDescriptor->PortRangeHints);
    free(g_psDescriptor);
  }
}
  
/*****************************************************************************/

/* Return a descriptor of the requested plugin type. */
const LADSPA_Descriptor * 
ladspa_descriptor(unsigned long Index) {
  if (Index == 0)
    return g_psDescriptor;
  else
    return NULL;
}

/*****************************************************************************/

/* EOF */
