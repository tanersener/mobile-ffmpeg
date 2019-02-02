/* sine.cpp

   Free software by Richard W.E. Furse. Do with as you will. No
   warranty. 

   This LADSPA plugin is written in C++ and provides a class that
   implements a sine oscillator using a wavetable. Band-limiting to
   avoid aliasing is trivial because of the waveform in use. Four
   versions of the oscillator are provided, allowing the amplitude and
   frequency inputs of the oscillator to be audio signals rather than
   controls (for use in AM and FM synthesis).

   This file has poor memory protection. Failures during new() will
   not recover nicely. */

/*****************************************************************************/

#include <cmath>
#include <cstdlib>
#include <cstring>

/*****************************************************************************/

#include "ladspa.h"

/*****************************************************************************/

/* The port numbers for the plugin: */

#define OSC_FREQUENCY 0
#define OSC_AMPLITUDE 1
#define OSC_OUTPUT    2

/*****************************************************************************/

/* Sine table size is given by (1<<SINE_TABLE_BITS). */
#define SINE_TABLE_BITS 14
#define SINE_TABLE_SHIFT (8 * sizeof(unsigned long) - SINE_TABLE_BITS)

static LADSPA_Data * g_pfSineTable = NULL;
static LADSPA_Data g_fPhaseStepBase = 0;

/*****************************************************************************/

static inline char * 
localStrdup(const char * input) {
  char * output = new char[strlen(input) + 1];
  strcpy(output, input);
  return output;
}

/*****************************************************************************/

static void
initialise_sine_table() {
  if (g_pfSineTable == NULL) {
    long lTableSize = (1 << SINE_TABLE_BITS);
    double dShift = (double(M_PI) * 2) / lTableSize;
    g_pfSineTable = new float[lTableSize];
    if (g_pfSineTable != NULL)
      for (long lIndex = 0; lIndex < lTableSize; lIndex++)
	g_pfSineTable[lIndex] = LADSPA_Data(sin(dShift * lIndex));
  }
  if (g_fPhaseStepBase == 0) {
    g_fPhaseStepBase = (LADSPA_Data)pow(2, sizeof(unsigned long) * 8);
  }
}

/*****************************************************************************/

static LADSPA_Handle instantiateSineOscillator(const LADSPA_Descriptor *,
                                               unsigned long SampleRate);
static void connectPortToSineOscillator(LADSPA_Handle Instance,
                                        unsigned long Port,
                                        LADSPA_Data * DataLocation);
static void activateSineOscillator(void * pvHandle);
static void runSineOscillator_FreqAudio_AmpAudio(LADSPA_Handle Instance,
                                                 unsigned long SampleCount);
static void runSineOscillator_FreqAudio_AmpCtrl(LADSPA_Handle Instance,
                                                unsigned long SampleCount);
static void runSineOscillator_FreqCtrl_AmpAudio(LADSPA_Handle Instance,
                                                unsigned long SampleCount);
static void runSineOscillator_FreqCtrl_AmpCtrl(LADSPA_Handle Instance,
                                               unsigned long SampleCount);
static void cleanupSineOscillator(void *pvHandle);

class SineOscillator {
private:

  /* Ports: */
  LADSPA_Data * m_pfFrequency;
  LADSPA_Data * m_pfAmplitude;
  LADSPA_Data * m_pfOutput;

  /* Oscillator state: */
  unsigned long     m_lPhase;
  unsigned long     m_lPhaseStep;
  LADSPA_Data       m_fCachedFrequency;
  const LADSPA_Data m_fLimitFrequency;
  const LADSPA_Data m_fPhaseStepScalar;

  SineOscillator(const long lSampleRate) 
    : m_lPhaseStep(0), 
    m_fCachedFrequency(0),
    m_fLimitFrequency(LADSPA_Data(lSampleRate * 0.5)),
    m_fPhaseStepScalar(LADSPA_Data(g_fPhaseStepBase / lSampleRate)) {
  }

  void setPhaseStepFromFrequency(const LADSPA_Data fFrequency) {
    if (fFrequency != m_fCachedFrequency) {
      if (fFrequency >= 0 && fFrequency < m_fLimitFrequency) 
	m_lPhaseStep = (unsigned long)(m_fPhaseStepScalar * fFrequency);
      else 
	m_lPhaseStep = 0;
      m_fCachedFrequency = fFrequency;
    }
  }

  friend LADSPA_Handle instantiateSineOscillator(const LADSPA_Descriptor *,
						 unsigned long SampleRate);
  friend void connectPortToSineOscillator(LADSPA_Handle Instance,
					  unsigned long Port,
					  LADSPA_Data * DataLocation);
  friend void activateSineOscillator(void * pvHandle);
  friend void runSineOscillator_FreqAudio_AmpAudio(LADSPA_Handle Instance,
						   unsigned long SampleCount);
  friend void runSineOscillator_FreqAudio_AmpCtrl(LADSPA_Handle Instance,
						  unsigned long SampleCount);
  friend void runSineOscillator_FreqCtrl_AmpAudio(LADSPA_Handle Instance,
						  unsigned long SampleCount);
  friend void runSineOscillator_FreqCtrl_AmpCtrl(LADSPA_Handle Instance,
						 unsigned long SampleCount);
  friend void cleanupSineOscillator(void *pvHandle);

};

/*****************************************************************************/

static LADSPA_Handle 
instantiateSineOscillator(const LADSPA_Descriptor *,
                          unsigned long SampleRate) {
  return new SineOscillator(SampleRate);
}

/*****************************************************************************/

static void 
connectPortToSineOscillator(LADSPA_Handle Instance,
                            unsigned long Port,
                            LADSPA_Data * DataLocation) {
  switch (Port) {
  case OSC_FREQUENCY:
    ((SineOscillator *)Instance)->m_pfFrequency = DataLocation;
    break;
  case OSC_AMPLITUDE:
    ((SineOscillator *)Instance)->m_pfAmplitude = DataLocation;
    break;
  case OSC_OUTPUT:
    ((SineOscillator *)Instance)->m_pfOutput = DataLocation;
    break;
  }
}

/*****************************************************************************/

static void
activateSineOscillator(void * pvHandle) {
  ((SineOscillator *)pvHandle)->m_lPhase = 0;
}

/*****************************************************************************/

static void 
runSineOscillator_FreqAudio_AmpAudio(LADSPA_Handle Instance,
                                     unsigned long SampleCount) {
  SineOscillator * poSineOscillator = (SineOscillator *)Instance;
  for (unsigned long lIndex = 0; lIndex < SampleCount; lIndex++) {
    /* Extract frequency at this point to guarantee inplace
       support. */
    LADSPA_Data fFrequency
      = (poSineOscillator->m_pfFrequency[lIndex]);
    poSineOscillator->m_pfOutput[lIndex]
      = (g_pfSineTable[poSineOscillator->m_lPhase >> SINE_TABLE_SHIFT]
         * poSineOscillator->m_pfAmplitude[lIndex]);
    poSineOscillator->setPhaseStepFromFrequency(fFrequency);
    poSineOscillator->m_lPhase 
      += poSineOscillator->m_lPhaseStep;
  }
}

/*****************************************************************************/

static void
runSineOscillator_FreqAudio_AmpCtrl(LADSPA_Handle Instance,
                                    unsigned long SampleCount) {
  SineOscillator * poSineOscillator = (SineOscillator *)Instance;
  LADSPA_Data fAmplitude = *(poSineOscillator->m_pfAmplitude);
  for (unsigned long lIndex = 0; lIndex < SampleCount; lIndex++) {
    /* Extract frequency at this point to guarantee inplace
       support. */
    LADSPA_Data fFrequency
      = (poSineOscillator->m_pfFrequency[lIndex]);
    poSineOscillator->m_pfOutput[lIndex]
      = (g_pfSineTable[poSineOscillator->m_lPhase >> SINE_TABLE_SHIFT]
         * fAmplitude);
    poSineOscillator->setPhaseStepFromFrequency(fFrequency);
    poSineOscillator->m_lPhase 
      += poSineOscillator->m_lPhaseStep;
  }
}

/*****************************************************************************/

static void
runSineOscillator_FreqCtrl_AmpAudio(LADSPA_Handle Instance,
                                    unsigned long SampleCount) {
  SineOscillator * poSineOscillator = (SineOscillator *)Instance;
  poSineOscillator->setPhaseStepFromFrequency
    (*(poSineOscillator->m_pfFrequency));
  for (unsigned long lIndex = 0; lIndex < SampleCount; lIndex++) {
    poSineOscillator->m_pfOutput[lIndex]
      = (g_pfSineTable[poSineOscillator->m_lPhase >> SINE_TABLE_SHIFT]
         * poSineOscillator->m_pfAmplitude[lIndex]);
    poSineOscillator->m_lPhase 
      += poSineOscillator->m_lPhaseStep;
  }
}

/*****************************************************************************/

static void
runSineOscillator_FreqCtrl_AmpCtrl(LADSPA_Handle Instance,
                                   unsigned long SampleCount) {
  SineOscillator * poSineOscillator = (SineOscillator *)Instance;
  LADSPA_Data fAmplitude = *(poSineOscillator->m_pfAmplitude);
  poSineOscillator->setPhaseStepFromFrequency
    (*(poSineOscillator->m_pfFrequency));
  for (unsigned long lIndex = 0; lIndex < SampleCount; lIndex++) {
    poSineOscillator->m_pfOutput[lIndex]
      = (g_pfSineTable[poSineOscillator->m_lPhase >> SINE_TABLE_SHIFT]
         * fAmplitude);
    poSineOscillator->m_lPhase 
      += poSineOscillator->m_lPhaseStep;
  }
}

/*****************************************************************************/

static void
cleanupSineOscillator(void *pvHandle) {
  delete (SineOscillator *)pvHandle;
}

/*****************************************************************************/

typedef char * char_ptr;

static LADSPA_Descriptor * g_psDescriptors[4] = { NULL, NULL, NULL, NULL };

/*****************************************************************************/

/* Global object used handle startup initialisation and shut down
   tidying. Performs the function of the _init() and _fini() calls in
   the C modules. */
class StartupShutdownHandler {
public:
  
  StartupShutdownHandler() {

    char ** pcPortNames;
    LADSPA_PortDescriptor * piPortDescriptors;
    LADSPA_PortRangeHint * psPortRangeHints;
    
    initialise_sine_table();
    
    for (long lPluginIndex = 0; lPluginIndex < 4; lPluginIndex++) {
      
      g_psDescriptors[lPluginIndex] = new LADSPA_Descriptor;
      if (g_psDescriptors[lPluginIndex] == NULL)
	break;
      
      g_psDescriptors[lPluginIndex]->UniqueID
	= 1044 + lPluginIndex; /* 1044-1047. */
      g_psDescriptors[lPluginIndex]->Properties
	= LADSPA_PROPERTY_HARD_RT_CAPABLE;
      g_psDescriptors[lPluginIndex]->Maker
	= localStrdup("Richard Furse (LADSPA example plugins)");
      g_psDescriptors[lPluginIndex]->Copyright
	= localStrdup("None");
      g_psDescriptors[lPluginIndex]->PortCount 
	= 3;
      piPortDescriptors
	= new LADSPA_PortDescriptor[3];
      g_psDescriptors[lPluginIndex]->PortDescriptors 
	= (const LADSPA_PortDescriptor *)piPortDescriptors;
      piPortDescriptors[OSC_OUTPUT]
	= LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
      pcPortNames
	= new char_ptr[3];
      g_psDescriptors[lPluginIndex]->PortNames
	= (const char **)pcPortNames;
      pcPortNames[OSC_FREQUENCY]
	= localStrdup("Frequency (Hz)");
      pcPortNames[OSC_AMPLITUDE] 
	= localStrdup("Amplitude");
      pcPortNames[OSC_OUTPUT]
	= localStrdup("Output");
      psPortRangeHints 
	= new LADSPA_PortRangeHint[3];
      g_psDescriptors[lPluginIndex]->PortRangeHints
	= (const LADSPA_PortRangeHint *)psPortRangeHints;
      psPortRangeHints[OSC_FREQUENCY].HintDescriptor
	= (LADSPA_HINT_BOUNDED_BELOW
	   | LADSPA_HINT_BOUNDED_ABOVE
	   | LADSPA_HINT_SAMPLE_RATE
	   | LADSPA_HINT_LOGARITHMIC
	   | LADSPA_HINT_DEFAULT_440);
      psPortRangeHints[OSC_FREQUENCY].LowerBound 
	= 0;
      psPortRangeHints[OSC_FREQUENCY].UpperBound
	= 0.5;
      psPortRangeHints[OSC_AMPLITUDE].HintDescriptor
	= (LADSPA_HINT_BOUNDED_BELOW 
	   | LADSPA_HINT_LOGARITHMIC
	   | LADSPA_HINT_DEFAULT_1);
      psPortRangeHints[OSC_AMPLITUDE].LowerBound 
	= 0;
      psPortRangeHints[OSC_OUTPUT].HintDescriptor
	= 0;
      g_psDescriptors[lPluginIndex]->instantiate
	= instantiateSineOscillator;
      g_psDescriptors[lPluginIndex]->connect_port 
	= connectPortToSineOscillator;
      g_psDescriptors[lPluginIndex]->activate
	= activateSineOscillator;
      g_psDescriptors[lPluginIndex]->run_adding
	= NULL;
      g_psDescriptors[lPluginIndex]->set_run_adding_gain
	= NULL;
      g_psDescriptors[lPluginIndex]->deactivate
	= NULL;
      g_psDescriptors[lPluginIndex]->cleanup
	= cleanupSineOscillator;
      
      switch (lPluginIndex) {
      case 0:
	g_psDescriptors[lPluginIndex]->Label
	  = localStrdup("sine_faaa");
	g_psDescriptors[lPluginIndex]->Name 
	  = localStrdup("Sine Oscillator (Freq:audio, Amp:audio)");
	piPortDescriptors[OSC_FREQUENCY]
	  = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	piPortDescriptors[OSC_AMPLITUDE]
	  = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	g_psDescriptors[lPluginIndex]->run 
	  = runSineOscillator_FreqAudio_AmpAudio;
	break;
      case 1:
	g_psDescriptors[lPluginIndex]->Label
	  = localStrdup("sine_faac");
	g_psDescriptors[lPluginIndex]->Name 
	  = localStrdup("Sine Oscillator (Freq:audio, Amp:control)");
	piPortDescriptors[OSC_FREQUENCY]
	  = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	piPortDescriptors[OSC_AMPLITUDE]
	  = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	g_psDescriptors[lPluginIndex]->run 
	  = runSineOscillator_FreqAudio_AmpCtrl;
	break;
      case 2:
	g_psDescriptors[lPluginIndex]->Label
	  = localStrdup("sine_fcaa");
	g_psDescriptors[lPluginIndex]->Name 
	  = localStrdup("Sine Oscillator (Freq:control, Amp:audio)");
	piPortDescriptors[OSC_FREQUENCY]
	  = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[OSC_AMPLITUDE]
	  = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
	g_psDescriptors[lPluginIndex]->run 
	  = runSineOscillator_FreqCtrl_AmpAudio;
	break;
      case 3:
	g_psDescriptors[lPluginIndex]->Label
	  = localStrdup("sine_fcac");
	g_psDescriptors[lPluginIndex]->Name 
	  = localStrdup("Sine Oscillator (Freq:control, Amp:control)");
	piPortDescriptors[OSC_FREQUENCY]
	  = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	piPortDescriptors[OSC_AMPLITUDE]
	  = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	g_psDescriptors[lPluginIndex]->run 
	  = runSineOscillator_FreqCtrl_AmpCtrl;
	break;
      }
    }
  }
  
  void deleteDescriptor(LADSPA_Descriptor * psDescriptor) {
    unsigned long lIndex;
    if (psDescriptor) {
      delete [] psDescriptor->Label;
      delete [] psDescriptor->Name;
      delete [] psDescriptor->Maker;
      delete [] psDescriptor->Copyright;
      delete [] psDescriptor->PortDescriptors;
      for (lIndex = 0; lIndex < psDescriptor->PortCount; lIndex++)
	delete [] psDescriptor->PortNames[lIndex];
      delete [] psDescriptor->PortNames;
      delete [] psDescriptor->PortRangeHints;
      delete psDescriptor;
    }
  }
  
  ~StartupShutdownHandler() {
    deleteDescriptor(g_psDescriptors[0]);
    deleteDescriptor(g_psDescriptors[1]);
    deleteDescriptor(g_psDescriptors[2]);
    deleteDescriptor(g_psDescriptors[3]);
    delete [] g_pfSineTable;
  }

};

static StartupShutdownHandler g_oShutdownStartupHandler;

/*****************************************************************************/
  
const LADSPA_Descriptor * 
ladspa_descriptor(unsigned long Index) {
  if (Index < 4)
    return g_psDescriptors[Index];
  else
    return NULL;
}

/*****************************************************************************/

/* EOF */
