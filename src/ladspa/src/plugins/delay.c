/* delay.c

   Free software by Richard W.E. Furse. Do with as you will. No
   warranty.

   This LADSPA plugin provides a simple delay line implemented in
   C. There is a fixed maximum delay length and no feedback is
   provided.

   This file has poor memory protection. Failures during malloc() will
   not recover nicely. */

/*****************************************************************************/

#include <stdlib.h>
#include <string.h>

/*****************************************************************************/

#include "ladspa.h"
#include "utils.h"

/*****************************************************************************/

/* The maximum delay valid for the delay line (in seconds). If you
   change this, remember that the label is currently "delay_5s". */
#define MAX_DELAY 5

/*****************************************************************************/

/* The port numbers for the plugin: */

#define SDL_DELAY_LENGTH 0
#define SDL_DRY_WET      1
#define SDL_INPUT        2
#define SDL_OUTPUT       3

/*****************************************************************************/

#define LIMIT_BETWEEN_0_AND_1(x)          \
(((x) < 0) ? 0 : (((x) > 1) ? 1 : (x)))
#define LIMIT_BETWEEN_0_AND_MAX_DELAY(x)  \
(((x) < 0) ? 0 : (((x) > MAX_DELAY) ? MAX_DELAY : (x)))

/*****************************************************************************/

/* Instance data for the simple delay line plugin. */
typedef struct {

  LADSPA_Data m_fSampleRate;

  LADSPA_Data * m_pfBuffer;

  /* Buffer size, a power of two. */
  unsigned long m_lBufferSize;

  /* Write pointer in buffer. */
  unsigned long m_lWritePointer;

  /* Ports:
     ------ */

  /* Delay control, in seconds. Accepted between 0 and 1 (only 1 sec
     of buffer is allocated by this crude delay line). */
  LADSPA_Data * m_pfDelay;

  /* Dry/wet control. 0 for entirely dry, 1 for entirely wet. */
  LADSPA_Data * m_pfDryWet;

  /* Input audio port data location. */
  LADSPA_Data * m_pfInput;

  /* Output audio port data location. */
  LADSPA_Data * m_pfOutput;

} SimpleDelayLine;

/*****************************************************************************/

/* Construct a new plugin instance. */
static LADSPA_Handle 
instantiateSimpleDelayLine(const LADSPA_Descriptor * Descriptor,
			   unsigned long             SampleRate) {

  unsigned long lMinimumBufferSize;
  SimpleDelayLine * psDelayLine;

  psDelayLine 
    = (SimpleDelayLine *)malloc(sizeof(SimpleDelayLine));

  if (psDelayLine == NULL) 
    return NULL;
  
  psDelayLine->m_fSampleRate = (LADSPA_Data)SampleRate;

  /* Buffer size is a power of two bigger than max delay time. */
  lMinimumBufferSize = (unsigned long)((LADSPA_Data)SampleRate * MAX_DELAY);
  psDelayLine->m_lBufferSize = 1;
  while (psDelayLine->m_lBufferSize < lMinimumBufferSize)
    psDelayLine->m_lBufferSize <<= 1;
  
  psDelayLine->m_pfBuffer 
    = (LADSPA_Data *)calloc(psDelayLine->m_lBufferSize, sizeof(LADSPA_Data));
  if (psDelayLine->m_pfBuffer == NULL) {
    free(psDelayLine);
    return NULL;
  }

  psDelayLine->m_lWritePointer = 0;
  
  return psDelayLine;
}

/*****************************************************************************/

/* Initialise and activate a plugin instance. */
static void
activateSimpleDelayLine(LADSPA_Handle Instance) {

  SimpleDelayLine * psSimpleDelayLine;
  psSimpleDelayLine = (SimpleDelayLine *)Instance;

  /* Need to reset the delay history in this function rather than
     instantiate() in case deactivate() followed by activate() have
     been called to reinitialise a delay line. */
  memset(psSimpleDelayLine->m_pfBuffer, 
	 0, 
	 sizeof(LADSPA_Data) * psSimpleDelayLine->m_lBufferSize);
}

/*****************************************************************************/

/* Connect a port to a data location. */
static void 
connectPortToSimpleDelayLine(LADSPA_Handle Instance,
			     unsigned long Port,
			     LADSPA_Data * DataLocation) {

  SimpleDelayLine * psSimpleDelayLine;

  psSimpleDelayLine = (SimpleDelayLine *)Instance;
  switch (Port) {
  case SDL_DELAY_LENGTH:
    psSimpleDelayLine->m_pfDelay = DataLocation;
    break;
  case SDL_DRY_WET:
    psSimpleDelayLine->m_pfDryWet = DataLocation;
    break;
  case SDL_INPUT:
    psSimpleDelayLine->m_pfInput = DataLocation;
    break;
  case SDL_OUTPUT:
    psSimpleDelayLine->m_pfOutput = DataLocation;
    break;
  }
}

/*****************************************************************************/

/* Run a delay line instance for a block of SampleCount samples. */
static void 
runSimpleDelayLine(LADSPA_Handle Instance,
		   unsigned long SampleCount) {
  
  LADSPA_Data * pfBuffer;
  LADSPA_Data * pfInput;
  LADSPA_Data * pfOutput;
  LADSPA_Data fDry;
  LADSPA_Data fInputSample;
  LADSPA_Data fWet;
  SimpleDelayLine * psSimpleDelayLine;
  unsigned long lBufferReadOffset;
  unsigned long lBufferSizeMinusOne;
  unsigned long lBufferWriteOffset;
  unsigned long lDelay;
  unsigned long lSampleIndex;

  psSimpleDelayLine = (SimpleDelayLine *)Instance;

  lBufferSizeMinusOne = psSimpleDelayLine->m_lBufferSize - 1;
  lDelay = (unsigned long)
    (LIMIT_BETWEEN_0_AND_MAX_DELAY(*(psSimpleDelayLine->m_pfDelay)) 
     * psSimpleDelayLine->m_fSampleRate);

  pfInput = psSimpleDelayLine->m_pfInput;
  pfOutput = psSimpleDelayLine->m_pfOutput;
  pfBuffer = psSimpleDelayLine->m_pfBuffer;
  lBufferWriteOffset = psSimpleDelayLine->m_lWritePointer;
  lBufferReadOffset
    = lBufferWriteOffset + psSimpleDelayLine->m_lBufferSize - lDelay;
  fWet = LIMIT_BETWEEN_0_AND_1(*(psSimpleDelayLine->m_pfDryWet));
  fDry = 1 - fWet;

  for (lSampleIndex = 0;
       lSampleIndex < SampleCount;
       lSampleIndex++) {
    fInputSample = *(pfInput++);
    *(pfOutput++) = (fDry * fInputSample
		     + fWet * pfBuffer[((lSampleIndex + lBufferReadOffset)
					& lBufferSizeMinusOne)]);
    pfBuffer[((lSampleIndex + lBufferWriteOffset)
	      & lBufferSizeMinusOne)] = fInputSample;
  }

  psSimpleDelayLine->m_lWritePointer
    = ((psSimpleDelayLine->m_lWritePointer + SampleCount)
       & lBufferSizeMinusOne);
}

/*****************************************************************************/

/* Throw away a simple delay line. */
static void 
cleanupSimpleDelayLine(LADSPA_Handle Instance) {

  SimpleDelayLine * psSimpleDelayLine;

  psSimpleDelayLine = (SimpleDelayLine *)Instance;

  free(psSimpleDelayLine->m_pfBuffer);
  free(psSimpleDelayLine);
}

/*****************************************************************************/

static LADSPA_Descriptor * g_psDescriptor = NULL;

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
      = 1043;
    g_psDescriptor->Label
      = strdup("delay_5s");
    g_psDescriptor->Properties
      = LADSPA_PROPERTY_HARD_RT_CAPABLE;
    g_psDescriptor->Name 
      = strdup("Simple Delay Line");
    g_psDescriptor->Maker
      = strdup("Richard Furse (LADSPA example plugins)");
    g_psDescriptor->Copyright
      = strdup("None");
    g_psDescriptor->PortCount 
      = 4;
    piPortDescriptors
      = (LADSPA_PortDescriptor *)calloc(4, sizeof(LADSPA_PortDescriptor));
    g_psDescriptor->PortDescriptors 
      = (const LADSPA_PortDescriptor *)piPortDescriptors;
    piPortDescriptors[SDL_DELAY_LENGTH]
      = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
    piPortDescriptors[SDL_DRY_WET]
      = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
    piPortDescriptors[SDL_INPUT]
      = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
    piPortDescriptors[SDL_OUTPUT]
      = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
    pcPortNames
      = (char **)calloc(4, sizeof(char *));
    g_psDescriptor->PortNames
      = (const char **)pcPortNames;
    pcPortNames[SDL_DELAY_LENGTH]
      = strdup("Delay (Seconds)");
    pcPortNames[SDL_DRY_WET] 
      = strdup("Dry/Wet Balance");
    pcPortNames[SDL_INPUT] 
      = strdup("Input");
    pcPortNames[SDL_OUTPUT]
      = strdup("Output");
    psPortRangeHints = ((LADSPA_PortRangeHint *)
			calloc(4, sizeof(LADSPA_PortRangeHint)));
    g_psDescriptor->PortRangeHints
      = (const LADSPA_PortRangeHint *)psPortRangeHints;
    psPortRangeHints[SDL_DELAY_LENGTH].HintDescriptor
      = (LADSPA_HINT_BOUNDED_BELOW 
	 | LADSPA_HINT_BOUNDED_ABOVE
	 | LADSPA_HINT_DEFAULT_1);
    psPortRangeHints[SDL_DELAY_LENGTH].LowerBound 
      = 0;
    psPortRangeHints[SDL_DELAY_LENGTH].UpperBound
      = (LADSPA_Data)MAX_DELAY;
    psPortRangeHints[SDL_DRY_WET].HintDescriptor
      = (LADSPA_HINT_BOUNDED_BELOW 
	 | LADSPA_HINT_BOUNDED_ABOVE
	 | LADSPA_HINT_DEFAULT_MIDDLE);
    psPortRangeHints[SDL_DRY_WET].LowerBound 
      = 0;
    psPortRangeHints[SDL_DRY_WET].UpperBound
      = 1;
    psPortRangeHints[SDL_INPUT].HintDescriptor
      = 0;
    psPortRangeHints[SDL_OUTPUT].HintDescriptor
      = 0;
    g_psDescriptor->instantiate
      = instantiateSimpleDelayLine;
    g_psDescriptor->connect_port 
      = connectPortToSimpleDelayLine;
    g_psDescriptor->activate
      = activateSimpleDelayLine;
    g_psDescriptor->run 
      = runSimpleDelayLine;
    g_psDescriptor->run_adding
      = NULL;
    g_psDescriptor->set_run_adding_gain
      = NULL;
    g_psDescriptor->deactivate
      = NULL;
    g_psDescriptor->cleanup
      = cleanupSimpleDelayLine;
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

/* Return a descriptor of the requested plugin type. Only one plugin
   type is available in this library. */
const LADSPA_Descriptor * 
ladspa_descriptor(unsigned long Index) {
  if (Index == 0)
    return g_psDescriptor;
  else
    return NULL;
}

/*****************************************************************************/

/* EOF */
