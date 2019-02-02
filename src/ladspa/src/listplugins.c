/* listplugins.c

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

static void 
describePluginLibrary(const char * pcFullFilename, 
		      void * pvPluginHandle,
		      LADSPA_Descriptor_Function fDescriptorFunction) {

  const LADSPA_Descriptor * psDescriptor;
  long lIndex;

  printf("%s:\n", pcFullFilename);
  for (lIndex = 0;
       (psDescriptor = fDescriptorFunction(lIndex)) != NULL;
       lIndex++) 
    printf("\t%s (%lu/%s)\n", 
	   psDescriptor->Name,
	   psDescriptor->UniqueID,
	   psDescriptor->Label);

  dlclose(pvPluginHandle);
}

/*****************************************************************************/

/* Returns 0 if all goes well, otherwise returns 1. */
static int
listPlugins() {
  LADSPAPluginSearch(describePluginLibrary);
  return(0);
}

/*****************************************************************************/

int 
main(const int iArgc, const char ** ppcArgv) {
  return listPlugins();
}

/*****************************************************************************/

/* EOF */
