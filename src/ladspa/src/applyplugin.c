/* applyplugin.c

   Free software by Richard W.E. Furse. Do with as you will. No
   warranty. */

/*****************************************************************************/

#include <dlfcn.h>
#include <endian.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*****************************************************************************/

#include "ladspa.h"

#include "utils.h"

/*****************************************************************************/

#define BUFFER_SIZE 2048

/*****************************************************************************/

/* Macros provide hook for word-order swapping because of
   big/little-endian issues. */

#ifndef BYTE_ORDER
#error "Could not determine byte order."
#endif

#if (BYTE_ORDER == LITTLE_ENDIAN)

/* (i386 is little-endian.) */

#define WORD_ORDER_SWAP_16BIT(ptr)
#define WORD_ORDER_SWAP_32BIT(ptr)

#else

/* The following isn't ANSI C, but it'll work fine on GCC. To make it
   ANSI compiliant, make cTmp global, remove the braces (etc). Then
   check carefully to see where WORD_ORDER_SWAP_* is
   used. Alternatively, rewrite as procedures. */

#define WORD_ORDER_SWAP_16BIT(pvPtr) {					\
  char cTmp, * pcPtr = (char *)pvPtr;					\
  cTmp = pcPtr[0];							\
  pcPtr[0] = pcPtr[1];							\
  pcPtr[1] = cTmp;							\
}
#define WORD_ORDER_SWAP_32BIT(pvPtr) {					\
  char cTmp, * pcPtr = (char *)pvPtr;					\
  cTmp = pcPtr[0];							\
  pcPtr[0] = pcPtr[3];							\
  pcPtr[3] = cTmp;							\
  cTmp = pcPtr[1];							\
  pcPtr[1] = pcPtr[2];							\
  pcPtr[2] = cTmp;							\
}

#endif

/*****************************************************************************/

/* Horrid hard-coded Wave Interface:
   --------------------------------- */
/* Please please feel free to replace this with something nice. 

   Only handles 16bit PCM files in a single data block. Returns
   channel count and sample rate by reference. Only one file can be
   open for read at a time. */

FILE * g_poInputFile;
FILE * g_poOutputFile;
LADSPA_Data g_fPeakWritten = 0;

unsigned long g_lInputFileChannelCount;
unsigned long g_lOutputFileChannelCount;

short * g_psInputFileBuffer;
short * g_psOutputFileBuffer;

static void 
openWaveFile(const char * pcFilename,
	     unsigned long * plChannelCount,
	     unsigned long * plSampleRate,
	     unsigned long * plLength) {

  char pcHeader[44];
  size_t lReadLength;

  g_poInputFile = fopen(pcFilename, "rb");
  if (!g_poInputFile) {
    fprintf(stderr,
	    "Failed to open input file \"%s\": %s\n",
	    pcFilename,
	    strerror(errno));
    exit(1);
  }

  lReadLength = fread(pcHeader, sizeof(char), 44, g_poInputFile);
  if (lReadLength < 44) {
    fprintf(stderr,
	    "Failed to read header from input file \"%s\": %s\n",
	    pcFilename,
	    strerror(errno));
    exit(1);
  }
  if (strncmp(pcHeader + 0, "RIFF", 4) != 0 ||
      strncmp(pcHeader + 8, "WAVE", 4) != 0 ||
      strncmp(pcHeader + 12, "fmt ", 4) != 0 ||
      pcHeader[20] != 1 || pcHeader[21] != 0 || /* PCM */
      pcHeader[34] != 16 || pcHeader[35] != 0 || /* 16 bit */
      strncmp(pcHeader + 36, "data", 4) != 0) /* Data block elsewhere */ {
    fprintf(stderr,
	    "\"applyplugin\" has very limited support for sound files types. "
	    "The file \"%s\" is not a simple 16bit PCM Wave File.\n",
	    pcFilename);
    exit(1);
  }

  /* Not portable casting (short assumed 16bit, unsigned int assumed
     32bit), alignment assumed not a problem. This is all bad. */
  WORD_ORDER_SWAP_16BIT(pcHeader + 22);
  g_lInputFileChannelCount 
    = *plChannelCount 
    = *((unsigned short *)(pcHeader + 22));
  WORD_ORDER_SWAP_32BIT(pcHeader + 24);
  *plSampleRate
    = *((unsigned int *)(pcHeader + 24));
  WORD_ORDER_SWAP_32BIT(pcHeader + 40);
  *plLength
    = (*((unsigned int *)(pcHeader + 40))
       / (*plChannelCount * sizeof(short)));

  g_psInputFileBuffer
    = (short *)calloc(*plChannelCount * BUFFER_SIZE, sizeof(short));
}

static void 
createWaveFile(const char * pcFilename,
	       unsigned long lChannelCount,
	       unsigned long lSampleRate,
	       unsigned long lLength) {


  char pcHeader[44];
  size_t lWriteLength;

  static const char pcHeaderBase[44] = {
    'R', 'I', 'F', 'F',
    0, 0, 0, 0,
    'W', 'A', 'V', 'E',
    'f', 'm', 't', ' ',
    16, 0, 0, 0,
    1, 0,
    1, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0,
    16, 0,
    'd', 'a', 't', 'a',
    0, 0, 0, 0
  };

  g_poOutputFile = fopen(pcFilename, "wb");
  if (!g_poOutputFile) {
    fprintf(stderr,
	    "Failed to open output file \"%s\": %s\n",
	    pcFilename,
	    strerror(errno));
    exit(1);
  }

  /* Not portable casting (short assumed 16bit, unsigned int assumed
     32bit), alignment assumed not a problem. This is all bad. */
  memcpy(pcHeader, pcHeaderBase, 44);

  *(unsigned int *)(pcHeader + 4) 
    = (unsigned int)(lLength * lChannelCount * sizeof(short) + 36);
  WORD_ORDER_SWAP_32BIT(pcHeader + 4);
  /* (Or should it be +32?) */
  *(unsigned short *)(pcHeader + 22) 
    = (unsigned short)lChannelCount;
  WORD_ORDER_SWAP_16BIT(pcHeader + 22);
  *(unsigned int *)(pcHeader + 24) 
    = (unsigned int)lSampleRate;
  WORD_ORDER_SWAP_32BIT(pcHeader + 24);
  *(unsigned int *)(pcHeader + 28) 
    = (unsigned int)(lSampleRate * lChannelCount * sizeof(short));
  WORD_ORDER_SWAP_32BIT(pcHeader + 28);
  *(unsigned short *)(pcHeader + 32) 
    = (unsigned short)(lChannelCount * sizeof(short));
  WORD_ORDER_SWAP_16BIT(pcHeader + 32);
  *(unsigned int *)(pcHeader + 40) 
    = (unsigned int)(lLength * lChannelCount * sizeof(short));
  WORD_ORDER_SWAP_32BIT(pcHeader + 40);

  g_lOutputFileChannelCount = lChannelCount;

  lWriteLength = fwrite(pcHeader, sizeof(char), 44, g_poOutputFile);
  if (lWriteLength < 44) {
    fprintf(stderr,
	    "Failed to write header to output file \"%s\": %s\n",
	    pcFilename,
	    strerror(errno));
    exit(1);
  }

  g_psOutputFileBuffer 
    = (short *)calloc(lChannelCount * BUFFER_SIZE, sizeof(short));
}

static void
readIntoBuffers(LADSPA_Data ** ppfBuffers, 
		const unsigned long lFrameSize) {

  short * psReadPointer;
  size_t lReadLength;
  unsigned long lChannelIndex;
  unsigned long lFrameIndex;

  lReadLength = fread(g_psInputFileBuffer,
		      sizeof(short) * g_lInputFileChannelCount,
		      lFrameSize,
		      g_poInputFile);
  if (lReadLength < lFrameSize) {
    fprintf(stderr,
	    "Failed to read audio from input file. Is the file damaged?\n");
    exit(1);
  }

  for (lChannelIndex = 0;
       lChannelIndex < g_lInputFileChannelCount; 
       lChannelIndex++) {
    psReadPointer = g_psInputFileBuffer + lChannelIndex;
    for (lFrameIndex = 0; 
	 lFrameIndex < lFrameSize; 
	 lFrameIndex++, psReadPointer += g_lInputFileChannelCount) {
      WORD_ORDER_SWAP_16BIT(psReadPointer);
      ppfBuffers[lChannelIndex][lFrameIndex] 
	= ((LADSPA_Data)*psReadPointer) * (1.0f / 32767.5f);
    }
  }
}

static void
writeFromBuffers(LADSPA_Data ** ppfBuffers, 
		 const unsigned long lFrameSize) {

  LADSPA_Data fValue, fAbsValue;
  short * psWritePointer;
  size_t lWriteLength;
  unsigned long lChannelIndex;
  unsigned long lFrameIndex;

  for (lChannelIndex = 0;
       lChannelIndex < g_lOutputFileChannelCount; 
       lChannelIndex++) {
    psWritePointer = g_psOutputFileBuffer + lChannelIndex;
    for (lFrameIndex = 0; 
	 lFrameIndex < lFrameSize; 
	 lFrameIndex++, psWritePointer += g_lOutputFileChannelCount) {
      fValue = ppfBuffers[lChannelIndex][lFrameIndex] * 32767.5f;
      fAbsValue = fabs(fValue);
      if (fAbsValue > g_fPeakWritten)
	g_fPeakWritten = fAbsValue;
      /* Use hard clipping as it sounds better than wraparound. */
      if (fValue > 32767)
	*psWritePointer = 32767;
      else if (fValue <= -32768)
	*psWritePointer = -32768;
      else
	*psWritePointer = (short)fValue;
      WORD_ORDER_SWAP_16BIT(psWritePointer);
    }
  }

  lWriteLength = fwrite(g_psOutputFileBuffer,
			sizeof(short) * g_lOutputFileChannelCount,
			lFrameSize,
			g_poOutputFile);
  if (lWriteLength < lFrameSize) {
    fprintf(stderr,
	    "Failed to read audio to output file. Is the disk full?\n");
    exit(1);
  }

}

static void
closeFiles(void) {
  fclose(g_poInputFile);
  fclose(g_poOutputFile);
  printf("Peak output: %g\n", g_fPeakWritten);
}

/*****************************************************************************/

static unsigned long
getPortCountByType(const LADSPA_Descriptor     * psDescriptor,
		   const LADSPA_PortDescriptor   iType) {

  unsigned long lCount;
  unsigned long lIndex;

  lCount = 0;
  for (lIndex = 0; lIndex < psDescriptor->PortCount; lIndex++)
    if ((psDescriptor->PortDescriptors[lIndex] & iType) == iType)
      lCount++;

  return lCount;
}

/*****************************************************************************/

static void
listControlsForPlugin(const LADSPA_Descriptor * psDescriptor) {

  int bFound;
  unsigned long lIndex;
  LADSPA_PortRangeHintDescriptor iHintDescriptor;
  LADSPA_Data fBound;
	
  fprintf(stderr,
	  "Plugin \"%s\" has the following control inputs:\n",
	  psDescriptor->Name);

  bFound = 0;
  for (lIndex = 0; lIndex < psDescriptor->PortCount; lIndex++)
    if (LADSPA_IS_PORT_INPUT(psDescriptor->PortDescriptors[lIndex])
	&& LADSPA_IS_PORT_CONTROL(psDescriptor->PortDescriptors[lIndex])) {
      fprintf(stderr,
	      "\t%s",
	      psDescriptor->PortNames[lIndex]);
      bFound = 1;
      iHintDescriptor = psDescriptor->PortRangeHints[lIndex].HintDescriptor;
      if (LADSPA_IS_HINT_BOUNDED_BELOW(iHintDescriptor)
	  || LADSPA_IS_HINT_BOUNDED_ABOVE(iHintDescriptor)) {
	fprintf(stderr, " (");
	if (LADSPA_IS_HINT_BOUNDED_BELOW(iHintDescriptor)) {
	  fBound = psDescriptor->PortRangeHints[lIndex].LowerBound;
	  if (LADSPA_IS_HINT_SAMPLE_RATE(iHintDescriptor)) {
	    if (fBound == 0)
	      fprintf(stderr, "0");
	    else
	      fprintf(stderr, "%g * sample rate", fBound);
	  }
	  else
	    fprintf(stderr, "%g", fBound);
	}
	else
	  fprintf(stderr, "...");
	fprintf(stderr, " to ");
	if (LADSPA_IS_HINT_BOUNDED_ABOVE(iHintDescriptor)) {
	  fBound = psDescriptor->PortRangeHints[lIndex].UpperBound;
	  if (LADSPA_IS_HINT_SAMPLE_RATE(iHintDescriptor)) {
	    if (fBound == 0)
	      fprintf(stderr, "0");
	    else
	      fprintf(stderr, "%g * sample rate", fBound);
	  }
	  else
	    fprintf(stderr, "%g", fBound);
	}
	else
	  fprintf(stderr, "...");
	fprintf(stderr, ")\n");
      }
      else
	fprintf(stderr, "\n");
    }
      
  if (!bFound)
    fprintf(stderr, "\tnone\n");
}

/*****************************************************************************/

/* Note that this procedure leaks memory like mad. */
static void
applyPlugin(const char               * pcInputFilename,
	    const char               * pcOutputFilename,
	    const LADSPA_Data          fExtraSeconds,
	    const unsigned long        lPluginCount,
	    const LADSPA_Descriptor ** ppsPluginDescriptors,
	    LADSPA_Data             ** ppfPluginControlValues) {

  LADSPA_PortDescriptor iPortDescriptor;
  LADSPA_Handle * ppsPlugins;
  LADSPA_Data ** ppfBuffers;
  long lFrameSize;
  unsigned long lAudioInputCount;
  unsigned long lAudioOutputCount;
  unsigned long lPreviousAudioOutputCount;
  unsigned long lBufferCount;
  unsigned long lBufferIndex;
  unsigned long lControlIndex;
  unsigned long lInputFileChannelCount;
  unsigned long lInputFileLength;
  unsigned long lOutputFileChannelCount;
  unsigned long lOutputFileLength;
  unsigned long lPluginIndex;
  unsigned long lPortIndex;
  unsigned long lSampleRate;
  unsigned long lTimeAt;
  LADSPA_Data fDummyControlOutput;

  /* Open input file and output file: 
     -------------------------------- */

  lOutputFileChannelCount
    = getPortCountByType(ppsPluginDescriptors[lPluginCount - 1],
			 LADSPA_PORT_AUDIO | LADSPA_PORT_OUTPUT);
  if (lOutputFileChannelCount == 0) {
    fprintf(stderr,
	    "The last plugin in the chain has no audio outputs.\n");
    exit(1);
  }

  openWaveFile(pcInputFilename, 
	       &lInputFileChannelCount, 
	       &lSampleRate,
	       &lInputFileLength);
  if (lInputFileChannelCount
      != getPortCountByType(ppsPluginDescriptors[0],
			    LADSPA_PORT_AUDIO | LADSPA_PORT_INPUT)) {
    fprintf(stderr,
	    "Mismatch between channel count in input file and audio inputs "
	    "on first plugin in chain.\n");
    exit(1);
  }

  lOutputFileLength 
    = lInputFileLength + (unsigned long)(fExtraSeconds * lSampleRate);

  createWaveFile(pcOutputFilename,
		 lOutputFileChannelCount,
		 lSampleRate,
		 lOutputFileLength);

  /* Count buffers and sanity-check the flow graph:
     ---------------------------------------------- */

  lBufferCount = 0;
  lPreviousAudioOutputCount = 0;
  for (lPluginIndex = 0; lPluginIndex < lPluginCount; lPluginIndex++) {

    lAudioInputCount
      = getPortCountByType(ppsPluginDescriptors[lPluginIndex],
			   LADSPA_PORT_AUDIO | LADSPA_PORT_INPUT);
    lAudioOutputCount
      = getPortCountByType(ppsPluginDescriptors[lPluginIndex],
			   LADSPA_PORT_AUDIO | LADSPA_PORT_OUTPUT);

    if (lBufferCount < lAudioInputCount)
      lBufferCount = lAudioInputCount;

    if (lPluginIndex > 0) 
      if (lAudioInputCount != lPreviousAudioOutputCount) {
	fprintf(stderr,
		"There is a mismatch between the number of output channels "
		"on plugin \"%s\" (%ld) and the number of input channels on "
		"plugin \"%s\" (%ld).\n",
		ppsPluginDescriptors[lPluginIndex - 1]->Name,
		lPreviousAudioOutputCount,
		ppsPluginDescriptors[lPluginIndex]->Name,
		lAudioInputCount);
	exit(1);
      }

    lPreviousAudioOutputCount = lAudioOutputCount;

    if (lBufferCount < lAudioOutputCount)
      lBufferCount = lAudioOutputCount;
  }

  /* Create the buffers, create instances, wire them up:
     --------------------------------------------------- */

  ppsPlugins = (LADSPA_Handle *)calloc(lPluginCount, sizeof(LADSPA_Handle));
  ppfBuffers = (LADSPA_Data **)calloc(lBufferCount, sizeof(LADSPA_Data *));
  for (lBufferIndex = 0; lBufferIndex < lBufferCount; lBufferIndex++)
    ppfBuffers[lBufferIndex] 
      = (LADSPA_Data *)calloc(BUFFER_SIZE, sizeof(LADSPA_Data));

  for (lPluginIndex = 0; lPluginIndex < lPluginCount; lPluginIndex++) {

    ppsPlugins[lPluginIndex]
      = ppsPluginDescriptors[lPluginIndex]
      ->instantiate(ppsPluginDescriptors[lPluginIndex],
		    lSampleRate);
    if (!ppsPlugins[lPluginIndex]) {
      fprintf(stderr,
	      "Failed to instantiate plugin of type \"%s\".\n",
	      ppsPluginDescriptors[lPluginIndex]->Name);
      exit(1);
    }

    /* Controls:
       --------- */

    lControlIndex = 0;
    for (lPortIndex = 0;
	 lPortIndex < ppsPluginDescriptors[lPluginIndex]->PortCount; 
	 lPortIndex++) {

      iPortDescriptor 
	= ppsPluginDescriptors[lPluginIndex]->PortDescriptors[lPortIndex];
      
      if (LADSPA_IS_PORT_CONTROL(iPortDescriptor)) {
	if (LADSPA_IS_PORT_INPUT(iPortDescriptor))
	  ppsPluginDescriptors[lPluginIndex]->connect_port
	    (ppsPlugins[lPluginIndex],
	     lPortIndex,
	     ppfPluginControlValues[lPluginIndex] + (lControlIndex++));
	if (LADSPA_IS_PORT_OUTPUT(iPortDescriptor))
	  ppsPluginDescriptors[lPluginIndex]->connect_port
	    (ppsPlugins[lPluginIndex],
	     lPortIndex,
	     &fDummyControlOutput);
      }
    }

    /* Input Buffers:
       -------------- */

    lBufferIndex = 0;
    for (lPortIndex = 0;
	 lPortIndex < ppsPluginDescriptors[lPluginIndex]->PortCount; 
	 lPortIndex++) {
      iPortDescriptor 
	= ppsPluginDescriptors[lPluginIndex]->PortDescriptors[lPortIndex];
      if (LADSPA_IS_PORT_INPUT(iPortDescriptor) 
	  && LADSPA_IS_PORT_AUDIO(iPortDescriptor))
	ppsPluginDescriptors[lPluginIndex]->connect_port
	  (ppsPlugins[lPluginIndex],
	   lPortIndex,
	   ppfBuffers[lBufferIndex++]);
    }
    

    /* Output Buffers:
       --------------- */

    lBufferIndex = 0;
    for (lPortIndex = 0;
	 lPortIndex < ppsPluginDescriptors[lPluginIndex]->PortCount; 
	 lPortIndex++) {
      iPortDescriptor 
	= ppsPluginDescriptors[lPluginIndex]->PortDescriptors[lPortIndex];
      if (LADSPA_IS_PORT_OUTPUT(iPortDescriptor) 
	  && LADSPA_IS_PORT_AUDIO(iPortDescriptor))
	ppsPluginDescriptors[lPluginIndex]->connect_port
	  (ppsPlugins[lPluginIndex],
	   lPortIndex,
	   ppfBuffers[lBufferIndex++]);
    }
  }

  /* Activate:
     --------- */

  for (lPluginIndex = 0; lPluginIndex < lPluginCount; lPluginIndex++) 
    if (ppsPluginDescriptors[lPluginIndex]->activate != NULL)
      ppsPluginDescriptors[lPluginIndex]->activate(ppsPlugins[lPluginIndex]);

  /* Run:
     ---- */

  lTimeAt = 0;
  while (lTimeAt < lOutputFileLength) {

    lFrameSize = lInputFileLength - lTimeAt;
    if (lFrameSize > BUFFER_SIZE)
      lFrameSize = BUFFER_SIZE;
    else {
      /* We've reached or are reaching the end of the file. We're not
         going to fill the buffer from file. Could just memset the end
         part, but there's only one frame where this is worth the
         effort. */
      for (lBufferIndex = 0; lBufferIndex < lBufferCount; lBufferIndex++)
	memset(ppfBuffers[lBufferIndex], 0, sizeof(LADSPA_Data) * BUFFER_SIZE);
    }

    if (lFrameSize > 0) {
      /* Read from disk. */
      readIntoBuffers(ppfBuffers, lFrameSize);
    }

    /* Run the plugins: */
    lFrameSize = lOutputFileLength - lTimeAt;
    if (lFrameSize > BUFFER_SIZE)
      lFrameSize = BUFFER_SIZE;

    for (lPluginIndex = 0; lPluginIndex < lPluginCount; lPluginIndex++) 
      ppsPluginDescriptors[lPluginIndex]
	->run(ppsPlugins[lPluginIndex],
	      lFrameSize);
    
    /* Write the output to disk. */
    writeFromBuffers(ppfBuffers, lFrameSize);

    lTimeAt += lFrameSize;
  }

  /* Deactivate:
     ----------- */

  for (lPluginIndex = 0; lPluginIndex < lPluginCount; lPluginIndex++) 
    if (ppsPluginDescriptors[lPluginIndex]->deactivate != NULL)
      ppsPluginDescriptors[lPluginIndex]->deactivate(ppsPlugins[lPluginIndex]);

  /* Cleanup:
     -------- */

  for (lPluginIndex = 0; lPluginIndex < lPluginCount; lPluginIndex++) 
    ppsPluginDescriptors[lPluginIndex]->cleanup(ppsPlugins[lPluginIndex]);

  /* Close the input and output files:
     --------------------------------- */

  closeFiles();

}

/*****************************************************************************/

/* Note that this function leaks memory and that dynamic libraries
   that are dlopen()ed are never dlclose()d. */
int 
main(const int iArgc, char * const ppcArgv[]) {

  char * pcEndPointer;
  const char * pcControlValue;
  const char * pcInputFilename;
  const char * pcOutputFilename;
  const LADSPA_Descriptor ** ppsPluginDescriptors;
  LADSPA_Data ** ppfPluginControlValues;
  LADSPA_Data fExtraSeconds;
  int bBadParameters;
  int bBadControls;
  LADSPA_Properties iProperties;
  unsigned long lArgumentIndex;
  unsigned long lControlValueCount;
  unsigned long lControlValueIndex;
  unsigned long lPluginCount;
  unsigned long lPluginCountUpperLimit;
  unsigned long lPluginIndex;
  void ** ppvPluginLibraries;

  bBadParameters = 0;
  fExtraSeconds = 0;

  /* Check for a -s flag, but only at the start. Cannot get use
     getopt() as it gets thoroughly confused when faced with negative
     numbers on the command line. */
  lArgumentIndex = 1;
  if (iArgc >= 3) {
    if (strcmp(ppcArgv[1], "-s") == 0) {
      fExtraSeconds = (LADSPA_Data)strtod(ppcArgv[2], &pcEndPointer);
      bBadControls = (ppcArgv[2] + strlen(ppcArgv[2]) 
		      != pcEndPointer);
      lArgumentIndex = 3;
    }
    else if (strncmp(ppcArgv[1], "-s", 2) == 0) {
      fExtraSeconds = (LADSPA_Data)strtod(ppcArgv[1] + 2, &pcEndPointer);
      bBadControls = (ppcArgv[1] + strlen(ppcArgv[1]) 
		      != pcEndPointer);
      lArgumentIndex = 2;
    }
  }
  
  /* We need to analyse the rest of the parameters. The first two
     should be input and output files involved. */
  if (lArgumentIndex + 4 > (unsigned long)iArgc) {
    /* There aren't enough parameters to include an input file, an
       output file and one plugin. */
    bBadParameters = 1;
  }
  else {

    pcInputFilename = ppcArgv[lArgumentIndex];
    pcOutputFilename = ppcArgv[lArgumentIndex + 1];

    /* Now we need to look through any plugins and plugin parameters
       present. At this stage we're loading plugins and parameters,
       but not attempting to wire them up.

       First we construct some arrays to contain the data we're
       hopefully about to extract. Note that these arrays are usually
       larger than is needed, however they will be large enough.

       WARNING: Note that there is no attempt to tidy up the memory at
       the end of this function and libraries are not unloaded under
       error conditions. This is only a toy program. */

    lPluginCountUpperLimit = (iArgc - lArgumentIndex - 1) / 2;

    ppvPluginLibraries = ((void **)
			  calloc(lPluginCountUpperLimit,
				 sizeof(void *)));
    ppsPluginDescriptors = ((const LADSPA_Descriptor **)
			    calloc(lPluginCountUpperLimit,
				   sizeof(LADSPA_Descriptor *)));
    ppfPluginControlValues = ((LADSPA_Data **)
			      calloc(lPluginCountUpperLimit,
				     sizeof(LADSPA_Data *)));
    lPluginIndex = 0;
    lArgumentIndex += 2;
    bBadControls = 0;
    while (lArgumentIndex < (unsigned long)iArgc && !bBadControls) {

      if (lArgumentIndex + 1 == (unsigned long)iArgc) {
	bBadParameters = 1;
	break;
      }

      /* Parameter should be a plugin file name followed by a
         label. Load the plugin. This call will exit() if the load
         fails. */
      ppvPluginLibraries[lPluginIndex]
	= loadLADSPAPluginLibrary(ppcArgv[lArgumentIndex]);
      ppsPluginDescriptors[lPluginIndex] 
	= findLADSPAPluginDescriptor(ppvPluginLibraries[lPluginIndex],
				     ppcArgv[lArgumentIndex],
				     ppcArgv[lArgumentIndex + 1]);

      /* Check the plugin is in-place compatible. */
      iProperties = ppsPluginDescriptors[lPluginIndex]->Properties;
      if (LADSPA_IS_INPLACE_BROKEN(iProperties)) {
	fprintf(stderr,
		"Plugin \"%s\" is not capable of in-place processing and "
		"therefore cannot be used by this program.\n",
		ppsPluginDescriptors[lPluginIndex]->Name);
	/* This is somewhat lazy - this isn't a difficult problem to
           get around. */
	return(1);
      }

      lControlValueCount
	= getPortCountByType(ppsPluginDescriptors[lPluginIndex],
			     LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL);
      
      bBadControls = (lControlValueCount + lArgumentIndex + 2 
		      > (unsigned long)iArgc);
      if (lControlValueCount > 0 && !bBadControls) {
	ppfPluginControlValues[lPluginIndex]
	  = (LADSPA_Data *)calloc(lControlValueCount, sizeof(LADSPA_Data));
	for (lControlValueIndex = 0; 
	     lControlValueIndex < lControlValueCount && !bBadControls;
	     lControlValueIndex++) {
	  pcControlValue = ppcArgv[lArgumentIndex + 2 + lControlValueIndex];

	  ppfPluginControlValues[lPluginIndex][lControlValueIndex]
	    = (LADSPA_Data)strtod(pcControlValue, &pcEndPointer);

	  bBadControls = (pcControlValue + strlen(pcControlValue) 
			  != pcEndPointer);
	}
      }

      if (bBadControls)
	listControlsForPlugin(ppsPluginDescriptors[lPluginIndex]);

      lArgumentIndex += (2 + lControlValueCount);
      lPluginIndex++;
    }

    lPluginCount = lPluginIndex;

    if (!bBadControls) {

      /* We have all the data we need. Go go go. If this function
	 fails it will exit(). */
      applyPlugin(pcInputFilename,
		  pcOutputFilename,
		  fExtraSeconds,
		  lPluginCount,
		  ppsPluginDescriptors,
		  ppfPluginControlValues);

      for (lPluginIndex = 0; lPluginIndex < lPluginCount; lPluginIndex++)
	unloadLADSPAPluginLibrary(ppvPluginLibraries[lPluginIndex]);
    }
  }

  if (bBadParameters) {
    fprintf(stderr,
	    "Usage:\tapplyplugin [flags] <input Wave file> "
	    "<output Wave file>\n"
	    "\t<LADSPA plugin file name> <plugin label> "
	    "<Control1> <Control2>...\n"
	    "\t[<LADSPA plugin file name> <plugin label> "
	    "<Control1> <Control2>...]...\n"
	    "Flags:"
	    "\t-s<seconds>  Add seconds of silence after end of input file.\n"
	    "\n"
	    "To find out what control values are needed by a plugin, "
	    "use the\n"
	    "\"analyseplugin\" program and check for control input ports.\n"
            "Note that the LADSPA_PATH environment variable is used "
            "to help find plugins.\n");
    return(1);
  }

  return(0);
}

/*****************************************************************************/

/* EOF */
