/* analyseplugin.c

   Free software by Richard W.E. Furse. Do with as you will. No
   warranty. */

/*****************************************************************************/

#include <dlfcn.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

/*****************************************************************************/

#include "ladspa.h"

#include "utils.h"

/*****************************************************************************/

/* Returns 0 if all goes well, otherwise returns 1. Label may be null
   indicating `all plugins.' */
static int
analysePlugin(const char * pcPluginFilename,
	      const char * pcPluginLabel,
	      const int bVerbose) {

  LADSPA_Descriptor_Function pfDescriptorFunction;
  const LADSPA_Descriptor * psDescriptor;
  unsigned long lPluginIndex;
  unsigned long lPortIndex;
  unsigned long lSpaceIndex;
  unsigned long lSpacePadding1;
  unsigned long lSpacePadding2;
  unsigned long lLength;
  void * pvPluginHandle;
  LADSPA_PortRangeHintDescriptor iHintDescriptor;
  LADSPA_Data fBound;
  LADSPA_Data fDefault;

  pvPluginHandle = loadLADSPAPluginLibrary(pcPluginFilename);

  dlerror();
  pfDescriptorFunction 
    = (LADSPA_Descriptor_Function)dlsym(pvPluginHandle, "ladspa_descriptor");
  if (!pfDescriptorFunction) {
    const char * pcError = dlerror();
    if (pcError) 
      fprintf(stderr,
	      "Unable to find ladspa_descriptor() function in plugin file "
	      "\"%s\": %s.\n"
	      "Are you sure this is a LADSPA plugin file?\n", 
	      pcPluginFilename,
	      pcError);
    return 1;
  }

  lSpacePadding1 = 0;
  lSpacePadding2 = 0;
  if (!bVerbose) {
    for (lPluginIndex = 0;; lPluginIndex++) {
      psDescriptor = pfDescriptorFunction(lPluginIndex);
      if (!psDescriptor)
	break;
      if (pcPluginLabel != NULL)
	if (strcmp(pcPluginLabel, psDescriptor->Label) != 0)
	  continue;
      
      lLength = strlen(psDescriptor->Label);
      if (lSpacePadding1 < lLength)
	lSpacePadding1 = lLength;

      lLength = (long)(log10(psDescriptor->UniqueID)) + 1;
      if (lSpacePadding2 < lLength)
	lSpacePadding2 = lLength;
    }
    lSpacePadding1 += 2;
    lSpacePadding2 += 2;
  }

  for (lPluginIndex = 0;; lPluginIndex++) {

    psDescriptor = pfDescriptorFunction(lPluginIndex);
    if (!psDescriptor)
      break;
    if (pcPluginLabel != NULL)
      if (strcmp(pcPluginLabel, psDescriptor->Label) != 0)
	continue;

    if (!bVerbose) {
      printf("%s", psDescriptor->Label);
      for (lSpaceIndex = strlen(psDescriptor->Label);
	   lSpaceIndex < lSpacePadding1;
	   lSpaceIndex++)
	putchar(' ');
      printf("%lu", psDescriptor->UniqueID);
      for (lSpaceIndex = (long)(log10(psDescriptor->UniqueID)) + 1;
	   lSpaceIndex < lSpacePadding2;
	   lSpaceIndex++)
	putchar(' ');
      puts(psDescriptor->Name);
    }
    else {

      putchar('\n');
      
      printf("Plugin Name: \"%s\"\n", psDescriptor->Name);
      printf("Plugin Label: \"%s\"\n", psDescriptor->Label);
      printf("Plugin Unique ID: %lu\n", psDescriptor->UniqueID);
      printf("Maker: \"%s\"\n", psDescriptor->Maker);
      printf("Copyright: \"%s\"\n", psDescriptor->Copyright);
      
      printf("Must Run Real-Time: ");
      if (LADSPA_IS_REALTIME(psDescriptor->Properties))
	printf("Yes\n");
      else
	printf("No\n");
      
      printf("Has activate() Function: ");
      if (psDescriptor->activate != NULL)
	printf("Yes\n");
      else
	printf("No\n");    
      printf("Has deactivate() Function: ");
      if (psDescriptor->deactivate != NULL)
	printf("Yes\n");
      else
	printf("No\n");
      printf("Has run_adding() Function: ");
      if (psDescriptor->run_adding != NULL)
	printf("Yes\n");
      else
	printf("No\n");
      
      if (psDescriptor->instantiate == NULL)
	printf("ERROR: PLUGIN HAS NO INSTANTIATE FUNCTION.\n");
      if (psDescriptor->connect_port == NULL)
	printf("ERROR: PLUGIN HAS NO CONNECT_PORT FUNCTION.\n");
      if (psDescriptor->run == NULL)
	printf("ERROR: PLUGIN HAS NO RUN FUNCTION.\n");
      if (psDescriptor->run_adding != NULL
	  && psDescriptor->set_run_adding_gain == NULL)
	printf("ERROR: PLUGIN HAS RUN_ADDING FUNCTION BUT "
	       "NOT SET_RUN_ADDING_GAIN.\n");
      if (psDescriptor->run_adding == NULL
	  && psDescriptor->set_run_adding_gain != NULL)
	printf("ERROR: PLUGIN HAS SET_RUN_ADDING_GAIN FUNCTION BUT "
	       "NOT RUN_ADDING.\n");
      if (psDescriptor->cleanup == NULL)
	printf("ERROR: PLUGIN HAS NO CLEANUP FUNCTION.\n");
      
      printf("Environment: ");
      if (LADSPA_IS_HARD_RT_CAPABLE(psDescriptor->Properties))
	printf("Normal or Hard Real-Time\n");
      else
	printf("Normal\n");
      
      if (LADSPA_IS_INPLACE_BROKEN(psDescriptor->Properties))
	printf("This plugin cannot use in-place processing. "
	       "It will not work with all hosts.\n");
      
      printf("Ports:");

      if (psDescriptor->PortCount == 0)
	printf("\tERROR: PLUGIN HAS NO PORTS.\n");
      
      for (lPortIndex = 0; 
	   lPortIndex < psDescriptor->PortCount; 
	   lPortIndex++) {
	
	printf("\t\"%s\" ", psDescriptor->PortNames[lPortIndex]);
	
	if (LADSPA_IS_PORT_INPUT
	    (psDescriptor->PortDescriptors[lPortIndex])
	    && LADSPA_IS_PORT_OUTPUT
	    (psDescriptor->PortDescriptors[lPortIndex]))
	  printf("ERROR: INPUT AND OUTPUT");
	else if (LADSPA_IS_PORT_INPUT
		 (psDescriptor->PortDescriptors[lPortIndex]))
	  printf("input");
	else if (LADSPA_IS_PORT_OUTPUT
		 (psDescriptor->PortDescriptors[lPortIndex]))
	  printf("output");
	else 
	  printf("ERROR: NEITHER INPUT NOR OUTPUT");
	
	if (LADSPA_IS_PORT_CONTROL
	    (psDescriptor->PortDescriptors[lPortIndex])
	    && LADSPA_IS_PORT_AUDIO
	    (psDescriptor->PortDescriptors[lPortIndex]))
	  printf(", ERROR: CONTROL AND AUDIO");
	else if (LADSPA_IS_PORT_CONTROL
		 (psDescriptor->PortDescriptors[lPortIndex]))
	  printf(", control");
	else if (LADSPA_IS_PORT_AUDIO
		 (psDescriptor->PortDescriptors[lPortIndex]))
	  printf(", audio");
	else 
	  printf(", ERROR: NEITHER CONTROL NOR AUDIO");
	
	iHintDescriptor 
	  = psDescriptor->PortRangeHints[lPortIndex].HintDescriptor;
	
	if (LADSPA_IS_HINT_BOUNDED_BELOW(iHintDescriptor)
	    || LADSPA_IS_HINT_BOUNDED_ABOVE(iHintDescriptor)) {
	  printf(", ");
	  if (LADSPA_IS_HINT_BOUNDED_BELOW(iHintDescriptor)) {
	    fBound = psDescriptor->PortRangeHints[lPortIndex].LowerBound;
	    if (LADSPA_IS_HINT_SAMPLE_RATE(iHintDescriptor) && fBound != 0) 
	      printf("%g*srate", fBound);
	    else
	      printf("%g", fBound);
	  }
	  else
	    printf("...");
	  printf(" to ");
	  if (LADSPA_IS_HINT_BOUNDED_ABOVE(iHintDescriptor)) {
	    fBound = psDescriptor->PortRangeHints[lPortIndex].UpperBound;
	    if (LADSPA_IS_HINT_SAMPLE_RATE(iHintDescriptor) && fBound != 0)
	      printf("%g*srate", fBound);
	    else
	      printf("%g", fBound);
	  }
	  else
	    printf("...");
	}
	
	if (LADSPA_IS_HINT_TOGGLED(iHintDescriptor)) {
	  if ((iHintDescriptor 
	       | LADSPA_HINT_DEFAULT_0
	       | LADSPA_HINT_DEFAULT_1)
	       != (LADSPA_HINT_TOGGLED 
		   | LADSPA_HINT_DEFAULT_0
		   | LADSPA_HINT_DEFAULT_1))
	    printf(", ERROR: TOGGLED INCOMPATIBLE WITH OTHER HINT");
	  else
	    printf(", toggled");
	}
      
	switch (iHintDescriptor & LADSPA_HINT_DEFAULT_MASK) {
	case LADSPA_HINT_DEFAULT_NONE:
	  break;
	case LADSPA_HINT_DEFAULT_MINIMUM:
	  fDefault = psDescriptor->PortRangeHints[lPortIndex].LowerBound;
	  if (LADSPA_IS_HINT_SAMPLE_RATE(iHintDescriptor) && fDefault != 0) 
	    printf(", default %g*srate", fDefault);
	  else 
	    printf(", default %g", fDefault);
	  break;
	case LADSPA_HINT_DEFAULT_LOW:
	  if (LADSPA_IS_HINT_LOGARITHMIC(iHintDescriptor)) {
	    fDefault 
	      = exp(log(psDescriptor->PortRangeHints[lPortIndex].LowerBound) 
		    * 0.75
		    + log(psDescriptor->PortRangeHints[lPortIndex].UpperBound) 
		    * 0.25);
	  }
	  else {
	    fDefault 
	      = (psDescriptor->PortRangeHints[lPortIndex].LowerBound
		 * 0.75
		 + psDescriptor->PortRangeHints[lPortIndex].UpperBound
		 * 0.25);
	  }
	  if (LADSPA_IS_HINT_SAMPLE_RATE(iHintDescriptor) && fDefault != 0) 
	    printf(", default %g*srate", fDefault);
	  else 
	    printf(", default %g", fDefault);
	  break;
	case LADSPA_HINT_DEFAULT_MIDDLE:
	  if (LADSPA_IS_HINT_LOGARITHMIC(iHintDescriptor)) {
	    fDefault 
	      = sqrt(psDescriptor->PortRangeHints[lPortIndex].LowerBound
		     * psDescriptor->PortRangeHints[lPortIndex].UpperBound);
	  }
	  else {
	    fDefault 
	      = 0.5 * (psDescriptor->PortRangeHints[lPortIndex].LowerBound
		       + psDescriptor->PortRangeHints[lPortIndex].UpperBound);
	  }
	  if (LADSPA_IS_HINT_SAMPLE_RATE(iHintDescriptor) && fDefault != 0) 
	    printf(", default %g*srate", fDefault);
	  else 
	    printf(", default %g", fDefault);
	  break;
	case LADSPA_HINT_DEFAULT_HIGH:
	  if (LADSPA_IS_HINT_LOGARITHMIC(iHintDescriptor)) {
	    fDefault 
	      = exp(log(psDescriptor->PortRangeHints[lPortIndex].LowerBound) 
		    * 0.25
		    + log(psDescriptor->PortRangeHints[lPortIndex].UpperBound) 
		    * 0.75);
	  }
	  else {
	    fDefault 
	      = (psDescriptor->PortRangeHints[lPortIndex].LowerBound
		 * 0.25
		 + psDescriptor->PortRangeHints[lPortIndex].UpperBound
		 * 0.75);
	  }
	  if (LADSPA_IS_HINT_SAMPLE_RATE(iHintDescriptor) && fDefault != 0) 
	    printf(", default %g*srate", fDefault);
	  else 
	    printf(", default %g", fDefault);
	  break;
	case LADSPA_HINT_DEFAULT_MAXIMUM:
	  fDefault = psDescriptor->PortRangeHints[lPortIndex].UpperBound;
	  if (LADSPA_IS_HINT_SAMPLE_RATE(iHintDescriptor) && fDefault != 0) 
	    printf(", default %g*srate", fDefault);
	  else 
	    printf(", default %g", fDefault);
	  break;
	case LADSPA_HINT_DEFAULT_0:
	  printf(", default 0");
	  break;
	case LADSPA_HINT_DEFAULT_1:
	  printf(", default 1");
	  break;
	case LADSPA_HINT_DEFAULT_100:
	  printf(", default 100");
	  break;
	case LADSPA_HINT_DEFAULT_440:
	  printf(", default 440");
	  break;
	default:
	  printf(", UNKNOWN DEFAULT CODE");
	  /* (Not necessarily an error - may be a newer version.) */
	  break;
	}
	
	if (LADSPA_IS_HINT_LOGARITHMIC(iHintDescriptor))
	  printf(", logarithmic");
	
	if (LADSPA_IS_HINT_INTEGER(iHintDescriptor))
	  printf(", integer");
	
	putchar('\n');
      }
    }
  }

  if (bVerbose)
    putchar('\n');

  unloadLADSPAPluginLibrary(pvPluginHandle);
  
  return(0);
}

/*****************************************************************************/

int 
main(const int iArgc, const char ** ppcArgv) {
  
  const char * pcPluginName = NULL;
  const char * pcPluginLabel = NULL;
  int bVerbose = 1;

  /* Check for a flag, but only at the start. Cannot get use getopt()
     as it gets thoroughly confused when faced with negative numbers
     on the command line. */
  switch (iArgc) {
  case 2:
    if (strcmp(ppcArgv[1], "-h") != 0) {
      pcPluginName = ppcArgv[1];
      pcPluginLabel = NULL;
    }
    break;
  case 3:
    if (strcmp(ppcArgv[1], "-l") == 0) {
      pcPluginName = ppcArgv[2];
      pcPluginLabel = NULL;
      bVerbose = 0;
    }
    else {
      pcPluginName = ppcArgv[1];
      pcPluginLabel = ppcArgv[2];
    }
    break;
  case 4:
    if (strcmp(ppcArgv[1], "-l") == 0) {
      pcPluginName = ppcArgv[2];
      pcPluginLabel = ppcArgv[3];
      bVerbose = 0;
    }
    break;
  }

  if (!pcPluginName) {
    fprintf(stderr, 
	    "Usage:\tanalyseplugin [flags] <LADSPA plugin file name> "
	    "[<plugin label>].\n"
	    "Flags:"
	    "-l  Produce a summary list rather than a verbose report.\n"
            "Note that the LADSPA_PATH environment variable is used "
            "to help find plugins.\n");
    return(1);
  }

  return analysePlugin(pcPluginName, pcPluginLabel, bVerbose);
}

/*****************************************************************************/

/* EOF */
