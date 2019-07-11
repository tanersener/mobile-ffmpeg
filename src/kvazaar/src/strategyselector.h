#ifndef STRATEGYSELECTOR_H_
#define STRATEGYSELECTOR_H_
/*****************************************************************************
 * This file is part of Kvazaar HEVC encoder.
 *
 * Copyright (C) 2013-2015 Tampere University of Technology and others (see
 * COPYING file).
 *
 * Kvazaar is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * Kvazaar is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Kvazaar.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/

/**
 * \ingroup Optimization
 * \file
 * Dynamic dispatch based on cpuid.
 */

#include "global.h" // IWYU pragma: keep

#if defined(KVZ_DEBUG) && !defined(DEBUG_STRATEGYSELECTOR)
# define DEBUG_STRATEGYSELECTOR
#endif

typedef struct {
  const char *type; //Type of the function, usually its name
  const char *strategy_name; //Name of the strategy (e.g. sse2)
  unsigned int priority; //Priority. 0 = lowest (default strategy)
  void *fptr; //Pointer to the function
} strategy_t;

typedef struct {
  unsigned int count;
  unsigned int allocated;//How much memory is allocated
  strategy_t* strategies;
} strategy_list_t;

#define STRATEGY_LIST_ALLOC_SIZE 16

typedef struct {
  const char *strategy_type;
  void **fptr;
} strategy_to_select_t;

typedef struct {
  struct {
    int mmx;
    int sse;
    int sse2;
    int sse3;
    int ssse3;
    int sse41;
    int sse42;
    int avx;
    int avx2;

    bool hyper_threading;
  } intel_flags;
  
  struct {
    int altivec;
  } powerpc_flags;
  
  struct {
    int neon;
  } arm_flags;

  int logical_cpu_count;
  int physical_cpu_count;
} hardware_flags_t;

extern hardware_flags_t kvz_g_hardware_flags;
extern hardware_flags_t kvz_g_strategies_in_use;
extern hardware_flags_t kvz_g_strategies_available;

int kvz_strategyselector_init(int32_t cpuid, uint8_t bitdepth);
int kvz_strategyselector_register(void *opaque, const char *type, const char *strategy_name, int priority, void *fptr);


//Strategy to include
#include "strategies/strategies-nal.h"
#include "strategies/strategies-picture.h"
#include "strategies/strategies-dct.h"
#include "strategies/strategies-ipol.h"
#include "strategies/strategies-quant.h"
#include "strategies/strategies-intra.h"
#include "strategies/strategies-sao.h"
#include "strategies/strategies-encode.h"

static const strategy_to_select_t strategies_to_select[] = {
  STRATEGIES_NAL_EXPORTS
  STRATEGIES_PICTURE_EXPORTS
  STRATEGIES_DCT_EXPORTS
  STRATEGIES_IPOL_EXPORTS
  STRATEGIES_QUANT_EXPORTS
  STRATEGIES_INTRA_EXPORTS
  STRATEGIES_SAO_EXPORTS
  STRATEGIES_ENCODE_EXPORTS
  { NULL, NULL },
};

#endif //STRATEGYSELECTOR_H_
