/* default.c

   Free software by Richard W.E. Furse. Do with as you will. No
   warranty. */

/*****************************************************************************/

#include <math.h>

/*****************************************************************************/

#include "ladspa.h"
#include "utils.h"

/*****************************************************************************/

int 
getLADSPADefault(const LADSPA_PortRangeHint * psPortRangeHint,
		 const unsigned long          lSampleRate,
		 LADSPA_Data                * pfResult) {

  int iHintDescriptor;

  iHintDescriptor = psPortRangeHint->HintDescriptor & LADSPA_HINT_DEFAULT_MASK;

  switch (iHintDescriptor & LADSPA_HINT_DEFAULT_MASK) {
  case LADSPA_HINT_DEFAULT_NONE:
    return -1;
  case LADSPA_HINT_DEFAULT_MINIMUM:
    *pfResult = psPortRangeHint->LowerBound;
    if (LADSPA_IS_HINT_SAMPLE_RATE(psPortRangeHint->HintDescriptor))
      *pfResult *= lSampleRate;
    return 0;
  case LADSPA_HINT_DEFAULT_LOW:
    if (LADSPA_IS_HINT_LOGARITHMIC(iHintDescriptor)) {
      *pfResult = exp(log(psPortRangeHint->LowerBound) * 0.75
		      + log(psPortRangeHint->UpperBound) * 0.25);
    }
    else {
      *pfResult = (psPortRangeHint->LowerBound * 0.75
		   + psPortRangeHint->UpperBound * 0.25);
    }
    if (LADSPA_IS_HINT_SAMPLE_RATE(psPortRangeHint->HintDescriptor))
      *pfResult *= lSampleRate;
    return 0;
  case LADSPA_HINT_DEFAULT_MIDDLE:
    if (LADSPA_IS_HINT_LOGARITHMIC(iHintDescriptor)) {
      *pfResult = sqrt(psPortRangeHint->LowerBound
		       * psPortRangeHint->UpperBound);
    }
    else {
      *pfResult = 0.5 * (psPortRangeHint->LowerBound
			 + psPortRangeHint->UpperBound);
    }
    if (LADSPA_IS_HINT_SAMPLE_RATE(psPortRangeHint->HintDescriptor))
      *pfResult *= lSampleRate;
    return 0;
  case LADSPA_HINT_DEFAULT_HIGH:
    if (LADSPA_IS_HINT_LOGARITHMIC(iHintDescriptor)) {
      *pfResult = exp(log(psPortRangeHint->LowerBound) * 0.25
		      + log(psPortRangeHint->UpperBound) * 0.75);
    }
    else {
      *pfResult = (psPortRangeHint->LowerBound * 0.25
		   + psPortRangeHint->UpperBound * 0.75);
    }
    if (LADSPA_IS_HINT_SAMPLE_RATE(psPortRangeHint->HintDescriptor))
      *pfResult *= lSampleRate;
    return 0;
  case LADSPA_HINT_DEFAULT_MAXIMUM:
    *pfResult = psPortRangeHint->UpperBound;
    if (LADSPA_IS_HINT_SAMPLE_RATE(psPortRangeHint->HintDescriptor))
      *pfResult *= lSampleRate;
    return 0;
  case LADSPA_HINT_DEFAULT_0:
    *pfResult = 0;
    return 0;
  case LADSPA_HINT_DEFAULT_1:
    *pfResult = 1;
    return 0;
  case LADSPA_HINT_DEFAULT_100:
    *pfResult = 100;
    return 0;
  case LADSPA_HINT_DEFAULT_440:
    *pfResult = 440;
    return 0;
  }

  /* We don't recognise this default flag. It's probably from a more
     recent version of LADSPA. */
  return -1;
}

/*****************************************************************************/

/* EOF */
