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

#include "strategyselector.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

hardware_flags_t kvz_g_hardware_flags;
hardware_flags_t kvz_g_strategies_in_use;
hardware_flags_t kvz_g_strategies_available;

static void set_hardware_flags(int32_t cpuid);
static void* strategyselector_choose_for(const strategy_list_t * const strategies, const char * const strategy_type);

//Strategies to include (add new file here)

//Returns 1 if successful
int kvz_strategyselector_init(int32_t cpuid, uint8_t bitdepth) {
  const strategy_to_select_t *cur_strategy_to_select = strategies_to_select;
  strategy_list_t strategies;
  
  strategies.allocated = 0;
  strategies.count = 0;
  strategies.strategies = NULL;
  
  set_hardware_flags(cpuid);
  
  //Add new register function here
  if (!kvz_strategy_register_picture(&strategies, bitdepth)) {
    fprintf(stderr, "kvz_strategy_register_picture failed!\n");
    return 0;
  }
  
  if (!kvz_strategy_register_nal(&strategies, bitdepth)) {
    fprintf(stderr, "kvz_strategy_register_nal failed!\n");
    return 0;
  }

  if (!kvz_strategy_register_dct(&strategies, bitdepth)) {
    fprintf(stderr, "kvz_strategy_register_dct failed!\n");
    return 0;
  }

  if (!kvz_strategy_register_ipol(&strategies, bitdepth)) {
    fprintf(stderr, "kvz_strategy_register_ipol failed!\n");
    return 0;
  }

  if (!kvz_strategy_register_quant(&strategies, bitdepth)) {
    fprintf(stderr, "kvz_strategy_register_quant failed!\n");
    return 0;
  }

  if (!kvz_strategy_register_intra(&strategies, bitdepth)) {
    fprintf(stderr, "kvz_strategy_register_intra failed!\n");
    return 0;
  }

  if (!kvz_strategy_register_sao(&strategies, bitdepth)) {
    fprintf(stderr, "kvz_strategy_register_sao failed!\n");
    return 0;
  }
  
  if (!kvz_strategy_register_encode(&strategies, bitdepth)) {
    fprintf(stderr, "kvz_strategy_register_encode failed!\n");
    return 0;
  }
  
  while(cur_strategy_to_select->fptr) {
    *(cur_strategy_to_select->fptr) = strategyselector_choose_for(&strategies, cur_strategy_to_select->strategy_type);
    
    if (!(*(cur_strategy_to_select->fptr))) {
      fprintf(stderr, "Could not find a strategy for %s!\n", cur_strategy_to_select->strategy_type);
      return 0;
    }
    ++cur_strategy_to_select;
  }

  //We can free the structure now, as all strategies are statically set to pointers
  if (strategies.allocated) {
	  //Also check what optimizations are available and what are in use
	  //SIMD optimizations available
	  bool strategies_available = false;
	  fprintf(stderr, "Available: ");
	  if (kvz_g_strategies_available.intel_flags.avx != 0){
		  fprintf(stderr, "avx(%d) ", kvz_g_strategies_available.intel_flags.avx);
		  strategies_available = true;
	  }
	  if (kvz_g_strategies_available.intel_flags.avx2 != 0){
		  fprintf(stderr, "avx2(%d) ", kvz_g_strategies_available.intel_flags.avx2);
		  strategies_available = true;
	  }
	  if (kvz_g_strategies_available.intel_flags.mmx != 0) {
		  fprintf(stderr, "mmx(%d) ", kvz_g_strategies_available.intel_flags.mmx);
		  strategies_available = true;
	  }
	  if (kvz_g_strategies_available.intel_flags.sse != 0) {
		  fprintf(stderr, "sse(%d) ", kvz_g_strategies_available.intel_flags.sse);
		  strategies_available = true;
	  }
	  if (kvz_g_strategies_available.intel_flags.sse2 != 0) {
		  fprintf(stderr, "sse2(%d) ", kvz_g_strategies_available.intel_flags.sse2);
		  strategies_available = true;
	  }
	  if (kvz_g_strategies_available.intel_flags.sse3 != 0) {
		  fprintf(stderr, "sse3(%d) ", kvz_g_strategies_available.intel_flags.sse3);
		  strategies_available = true;
	  }
	  if (kvz_g_strategies_available.intel_flags.sse41 != 0) {
		  fprintf(stderr, "sse41(%d) ", kvz_g_strategies_available.intel_flags.sse41);
		  strategies_available = true;
	  }
	  if (kvz_g_strategies_available.intel_flags.sse42 != 0) {
		  fprintf(stderr, "sse42(%d) ", kvz_g_strategies_available.intel_flags.sse42);
		  strategies_available = true;
	  }
	  if (kvz_g_strategies_available.intel_flags.ssse3 != 0) {
		  fprintf(stderr, "ssse3(%d) ", kvz_g_strategies_available.intel_flags.ssse3);
		  strategies_available = true;
	  }
	  if (kvz_g_strategies_available.arm_flags.neon != 0) {
		  fprintf(stderr, "neon(%d) ", kvz_g_strategies_available.arm_flags.neon);
		  strategies_available = true;
	  }
	  if (kvz_g_strategies_available.powerpc_flags.altivec != 0) {
		  fprintf(stderr, "altivec(%d) ", kvz_g_strategies_available.powerpc_flags.altivec);
		  strategies_available = true;
	  }
	  //If there is no strategies available
	  if (!strategies_available){
		  fprintf(stderr, "no SIMD optimizations");
	  }
	  fprintf(stderr, "\n");

	  //SIMD optimizations in use
	  bool strategies_in_use = false;
	  fprintf(stderr, "In use: ");
	  if (kvz_g_strategies_in_use.intel_flags.avx != 0){
		  fprintf(stderr, "avx(%d) ", kvz_g_strategies_in_use.intel_flags.avx);
		  strategies_in_use = true;
	  }
	  if (kvz_g_strategies_in_use.intel_flags.avx2 != 0){ 
		  fprintf(stderr, "avx2(%d) ", kvz_g_strategies_in_use.intel_flags.avx2);
		  strategies_in_use = true;
	  }
	  if (kvz_g_strategies_in_use.intel_flags.mmx != 0) {
		  fprintf(stderr, "mmx(%d) ", kvz_g_strategies_in_use.intel_flags.mmx);
		  strategies_in_use = true;
	  }
	  if (kvz_g_strategies_in_use.intel_flags.sse != 0) {
		  fprintf(stderr, "sse(%d) ", kvz_g_strategies_in_use.intel_flags.sse);
		  strategies_in_use = true;
	  }
	  if (kvz_g_strategies_in_use.intel_flags.sse2 != 0) {
		  fprintf(stderr, "sse2(%d) ", kvz_g_strategies_in_use.intel_flags.sse2);
		  strategies_in_use = true;
	  }
	  if (kvz_g_strategies_in_use.intel_flags.sse3 != 0) {
		  fprintf(stderr, "sse3(%d) ", kvz_g_strategies_in_use.intel_flags.sse3);
		  strategies_in_use = true;
	  }
	  if (kvz_g_strategies_in_use.intel_flags.sse41 != 0) {
		  fprintf(stderr, "sse41(%d) ", kvz_g_strategies_in_use.intel_flags.sse41);
		  strategies_in_use = true;
	  }
	  if (kvz_g_strategies_in_use.intel_flags.sse42 != 0) {
		  fprintf(stderr, "sse42(%d) ", kvz_g_strategies_in_use.intel_flags.sse42);
		  strategies_in_use = true;
	  }
	  if (kvz_g_strategies_in_use.intel_flags.ssse3 != 0) {
		  fprintf(stderr, "ssse3(%d) ", kvz_g_strategies_in_use.intel_flags.ssse3);
		  strategies_in_use = true;
	  }
	  if (kvz_g_strategies_in_use.arm_flags.neon != 0) {
		  fprintf(stderr, "neon(%d) ", kvz_g_strategies_in_use.arm_flags.neon);
		  strategies_in_use = true;
	  }
	  if (kvz_g_strategies_in_use.powerpc_flags.altivec != 0) {
		  fprintf(stderr, "altivec(%d) ", kvz_g_strategies_in_use.powerpc_flags.altivec);
		  strategies_in_use = true;
	  }
	  //If there is no strategies in use
	  if (!strategies_in_use){
		  fprintf(stderr, "no SIMD optimizations");
	  }
	  fprintf(stderr, "\n");

	  //Free memory
	  free(strategies.strategies);
  }

  return 1;
}

//Returns 1 if successful, 0 otherwise
int kvz_strategyselector_register(void * const opaque, const char * const type, const char * const strategy_name, int priority, void * const fptr) {
  strategy_list_t * const strategies = opaque;
  
  if (strategies->allocated == strategies->count) {
    strategy_t* new_strategies = realloc(strategies->strategies, sizeof(strategy_t) * (strategies->allocated + STRATEGY_LIST_ALLOC_SIZE));
    if (!new_strategies) {
      fprintf(stderr, "Could not increase strategies list size!\n");
      return 0;
    }
    strategies->strategies = new_strategies;
    strategies->allocated += STRATEGY_LIST_ALLOC_SIZE;
  }
  
  {
    strategy_t *new_strategy = &strategies->strategies[strategies->count++];
    new_strategy->type = type;
    new_strategy->strategy_name = strategy_name;
    new_strategy->priority = priority;
    new_strategy->fptr = fptr;
  }

  //Check what strategies are available when they are registered
  if (strcmp(strategy_name, "avx") == 0) kvz_g_strategies_available.intel_flags.avx++;
  if (strcmp(strategy_name, "x86_asm_avx") == 0) kvz_g_strategies_available.intel_flags.avx++;
  if (strcmp(strategy_name, "avx2") == 0) kvz_g_strategies_available.intel_flags.avx2++;
  if (strcmp(strategy_name, "mmx") == 0) kvz_g_strategies_available.intel_flags.mmx++;
  if (strcmp(strategy_name, "sse") == 0) kvz_g_strategies_available.intel_flags.sse++;
  if (strcmp(strategy_name, "sse2") == 0) kvz_g_strategies_available.intel_flags.sse2++;
  if (strcmp(strategy_name, "sse3") == 0) kvz_g_strategies_available.intel_flags.sse3++;
  if (strcmp(strategy_name, "sse41") == 0) kvz_g_strategies_available.intel_flags.sse41++;
  if (strcmp(strategy_name, "sse42") == 0) kvz_g_strategies_available.intel_flags.sse42++;
  if (strcmp(strategy_name, "ssse3") == 0) kvz_g_strategies_available.intel_flags.ssse3++;
  if (strcmp(strategy_name, "altivec") == 0) kvz_g_strategies_available.powerpc_flags.altivec++;
  if (strcmp(strategy_name, "neon") == 0) kvz_g_strategies_available.arm_flags.neon++;

#ifdef DEBUG_STRATEGYSELECTOR
  fprintf(stderr, "Registered strategy %s:%s with priority %d (%p)\n", type, strategy_name, priority, fptr);
#endif //DEBUG_STRATEGYSELECTOR
  
  return 1;
}

static void* strategyselector_choose_for(const strategy_list_t * const strategies, const char * const strategy_type) {
  unsigned int max_priority = 0;
  int max_priority_i = -1;
  char buffer[256];
  char *override = NULL;
  int i = 0;
  
  // Because VS doesn't support snprintf, let's assert that there is
  // enough room in the buffer. Max length for strategy type is
  // buffersize (256) - prefix including terminating zero.
  assert(strlen(strategy_type) < 256 - sizeof("KVAZAAR_OVERRIDE_") );
  sprintf(buffer, "KVAZAAR_OVERRIDE_%s", strategy_type);

  override = getenv(buffer);
  
  for (i=0; i < strategies->count; ++i) {
    if (strcmp(strategies->strategies[i].type, strategy_type) == 0) {
      if (override && strcmp(strategies->strategies[i].strategy_name, override) == 0) {
        fprintf(stderr, "%s environment variable present, choosing %s:%s\n", buffer, strategy_type, strategies->strategies[i].strategy_name);
        return strategies->strategies[i].fptr;
      }
      if (strategies->strategies[i].priority >= max_priority) {
        max_priority_i = i;
        max_priority = strategies->strategies[i].priority;
      }
    }
  }
  
  if (override) {
    fprintf(stderr, "%s environment variable present, but no strategy %s was found!\n", buffer, override);
    return NULL;
  }

#ifdef DEBUG_STRATEGYSELECTOR
  fprintf(stderr, "Choosing strategy for %s:\n", strategy_type);
  for (i=0; i < strategies->count; ++i) {
    if (strcmp(strategies->strategies[i].type, strategy_type) == 0) {
      if (i != max_priority_i) {
        fprintf(stderr, "- %s (%d, %p)\n", strategies->strategies[i].strategy_name, strategies->strategies[i].priority, strategies->strategies[i].fptr);
      } else {
        fprintf(stderr, "> %s (%d, %p)\n", strategies->strategies[i].strategy_name, strategies->strategies[i].priority, strategies->strategies[i].fptr);
      }
    }
  }
#endif //DEBUG_STRATEGYSELECTOR
  
  
  if (max_priority_i == -1) {
    return NULL;
  }

  //Check what strategy we are going to use
  if (strcmp(strategies->strategies[max_priority_i].strategy_name, "avx") == 0) kvz_g_strategies_in_use.intel_flags.avx++;
  if (strcmp(strategies->strategies[max_priority_i].strategy_name, "x86_asm_avx") == 0) kvz_g_strategies_in_use.intel_flags.avx++;
  if (strcmp(strategies->strategies[max_priority_i].strategy_name, "avx2") == 0) kvz_g_strategies_in_use.intel_flags.avx2++;
  if (strcmp(strategies->strategies[max_priority_i].strategy_name, "mmx") == 0) kvz_g_strategies_in_use.intel_flags.mmx++;
  if (strcmp(strategies->strategies[max_priority_i].strategy_name, "sse") == 0) kvz_g_strategies_in_use.intel_flags.sse++;
  if (strcmp(strategies->strategies[max_priority_i].strategy_name, "sse2") == 0) kvz_g_strategies_in_use.intel_flags.sse2++;
  if (strcmp(strategies->strategies[max_priority_i].strategy_name, "sse3") == 0) kvz_g_strategies_in_use.intel_flags.sse3++;
  if (strcmp(strategies->strategies[max_priority_i].strategy_name, "sse41") == 0) kvz_g_strategies_in_use.intel_flags.sse41++;
  if (strcmp(strategies->strategies[max_priority_i].strategy_name, "sse42") == 0) kvz_g_strategies_in_use.intel_flags.sse42++;
  if (strcmp(strategies->strategies[max_priority_i].strategy_name, "ssse3") == 0) kvz_g_strategies_in_use.intel_flags.ssse3++;
  if (strcmp(strategies->strategies[max_priority_i].strategy_name, "altivec") == 0) kvz_g_strategies_in_use.powerpc_flags.altivec++;
  if (strcmp(strategies->strategies[max_priority_i].strategy_name, "neon") == 0) kvz_g_strategies_in_use.arm_flags.neon++;
  
  return strategies->strategies[max_priority_i].fptr;
}

#if COMPILE_INTEL

typedef struct {
  unsigned int eax;
  unsigned int ebx;
  unsigned int ecx;
  unsigned int edx;
} cpuid_t;

// CPUID adapters for different compilers.
#  if defined(__GNUC__)
#include <cpuid.h>

static INLINE int get_cpuid(unsigned level, unsigned sublevel, cpuid_t *cpu_info) {
  if (__get_cpuid_max(level & 0x80000000, NULL) < level) return 0;
  __cpuid_count(level, sublevel, cpu_info->eax, cpu_info->ebx, cpu_info->ecx, cpu_info->edx);
  return 1;
}
#  elif defined(_MSC_VER)
#include <intrin.h>

static INLINE int get_cpuid(unsigned level, unsigned sublevel, cpuid_t *cpu_info) {
  int vendor_info[4] = { 0, 0, 0, 0 };
  __cpuidex(vendor_info, 0, 0);

  // Check highest supported function.
  if (level > vendor_info[0]) return 0;
  
  int ms_cpu_info[4] = { cpu_info->eax, cpu_info->ebx, cpu_info->ecx, cpu_info->edx };
  __cpuidex(ms_cpu_info, level, sublevel);
  cpu_info->eax = ms_cpu_info[0];
  cpu_info->ebx = ms_cpu_info[1];
  cpu_info->ecx = ms_cpu_info[2];
  cpu_info->edx = ms_cpu_info[3];

  return 1;
}
#  else
static INLINE int get_cpuid(unsigned level, unsigned sublevel, cpuid_t *cpu_info)
{
  return 0;
}
#  endif
#endif // COMPILE_INTEL

#if COMPILE_POWERPC
#  if defined(__linux__) || (defined(__FreeBSD__) && __FreeBSD__ >= 12)
#ifdef __linux__
#include <asm/cputable.h>
#else
#include <machine/cpu.h>
#endif
#include <sys/auxv.h>

static int altivec_available(void)
{
    unsigned long hwcap = 0;
#ifdef __linux__
    hwcap = getauxval(AT_HWCAP);
#else
    elf_aux_info(AT_HWCAP, &hwcap, sizeof(hwcap));
#endif
    return !!(hwcap & PPC_FEATURE_HAS_ALTIVEC);
}
#  elif defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <machine/cpu.h>

static int altivec_available(void)
{
  u_long cpu_features = 0;
  size_t len = sizeof(cpu_features);

  sysctlbyname("hw.cpu_features", &cpu_features, &len, NULL, 0);
  return !!(cpu_features & PPC_FEATURE_HAS_ALTIVEC);
}
#  elif defined(__APPLE__) || defined(__NetBSD__) || defined(__OpenBSD__)
#include <sys/param.h>
#include <sys/sysctl.h>
#ifndef __APPLE__
#include <machine/cpu.h>
#endif

static int altivec_available(void)
{
  int cpu_altivec = 0;
  size_t len = sizeof(cpu_altivec);
#ifdef HW_VECTORUNIT
  int mib[] = { CTL_HW, HW_VECTORUNIT };
#else
  int mib[] = { CTL_MACHDEP, CPU_ALTIVEC };
#endif

  sysctl(mib, sizeof(mib)/sizeof(mib[0]), &cpu_altivec, &len, NULL, 0);
  return cpu_altivec;
}
#  else
static int altivec_available(void)
{
#if COMPILE_POWERPC_ALTIVEC
  return 1;
#else
  return 0;
#endif
}
#  endif
#endif //COMPILE_POWERPC

static void set_hardware_flags(int32_t cpuid) {
  FILL(kvz_g_hardware_flags, 0);

#if COMPILE_INTEL
  if (cpuid) {
    cpuid_t cpuid1 = { 0, 0, 0, 0 };
    /* CPU feature bits */
    enum {
      CPUID1_EDX_MMX = 1 << 23,
      CPUID1_EDX_SSE = 1 << 25,
      CPUID1_EDX_SSE2 = 1 << 26,
      CPUID1_EDX_HYPER_THREADING = 1 << 28,
    };
    enum {
      CPUID1_ECX_SSE3 = 1 << 0,
      CPUID1_ECX_SSSE3 = 1 << 9,
      CPUID1_ECX_SSE41 = 1 << 19,
      CPUID1_ECX_SSE42 = 1 << 20,
      CPUID1_ECX_XSAVE = 1 << 26,
      CPUID1_ECX_OSXSAVE = 1 << 27,
      CPUID1_ECX_AVX = 1 << 28,
    };
    enum {
      CPUID7_EBX_AVX2 = 1 << 5,
    };
    enum {
      XGETBV_XCR0_XMM = 1 << 1,
      XGETBV_XCR0_YMM = 1 << 2,
    };

    // Dig CPU features with cpuid
    get_cpuid(1, 0, &cpuid1);

#ifdef _WIN32
    SYSTEM_INFO systeminfo;
    GetSystemInfo(&systeminfo);

    kvz_g_hardware_flags.logical_cpu_count = systeminfo.dwNumberOfProcessors;
#else
    kvz_g_hardware_flags.logical_cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
#endif

    kvz_g_hardware_flags.physical_cpu_count = kvz_g_hardware_flags.logical_cpu_count;
    kvz_g_hardware_flags.intel_flags.hyper_threading = cpuid1.edx & CPUID1_EDX_HYPER_THREADING;
    if (kvz_g_hardware_flags.intel_flags.hyper_threading) {
      kvz_g_hardware_flags.physical_cpu_count /= 2;
    }
    
    // EDX
    if (cpuid1.edx & CPUID1_EDX_MMX)   kvz_g_hardware_flags.intel_flags.mmx = 1;
    if (cpuid1.edx & CPUID1_EDX_SSE)   kvz_g_hardware_flags.intel_flags.sse = 1;
    if (cpuid1.edx & CPUID1_EDX_SSE2)  kvz_g_hardware_flags.intel_flags.sse2 = 1;
    // ECX
    if (cpuid1.ecx & CPUID1_ECX_SSE3)  kvz_g_hardware_flags.intel_flags.sse3 = 1;;
    if (cpuid1.ecx & CPUID1_ECX_SSSE3) kvz_g_hardware_flags.intel_flags.ssse3 = 1;
    if (cpuid1.ecx & CPUID1_ECX_SSE41) kvz_g_hardware_flags.intel_flags.sse41 = 1;
    if (cpuid1.ecx & CPUID1_ECX_SSE42) kvz_g_hardware_flags.intel_flags.sse42 = 1;
    
    // Check hardware and OS support for xsave and xgetbv.
    if (cpuid1.ecx & (CPUID1_ECX_XSAVE | CPUID1_ECX_OSXSAVE)) {
      uint64_t xcr0 = 0;
      // Use _XCR_XFEATURE_ENABLED_MASK to check if _xgetbv intrinsic is
      // supported by the compiler.
#ifdef _XCR_XFEATURE_ENABLED_MASK
      xcr0 = _xgetbv(_XCR_XFEATURE_ENABLED_MASK);
#elif defined(__GNUC__)
      unsigned eax = 0, edx = 0;
      asm("xgetbv" : "=a"(eax), "=d"(edx) : "c" (0));
      xcr0 = (uint64_t)edx << 32 | eax;
#endif
      bool avx_support = cpuid1.ecx & CPUID1_ECX_AVX || false;
      bool xmm_support = xcr0 & XGETBV_XCR0_XMM || false;
      bool ymm_support = xcr0 & XGETBV_XCR0_YMM || false;

      if (avx_support && xmm_support && ymm_support) {
        kvz_g_hardware_flags.intel_flags.avx = 1;
      }

      if (kvz_g_hardware_flags.intel_flags.avx) {
        cpuid_t cpuid7 = { 0, 0, 0, 0 };
        get_cpuid(7, 0, &cpuid7);
        if (cpuid7.ebx & CPUID7_EBX_AVX2)  kvz_g_hardware_flags.intel_flags.avx2 = 1;
      }
    }
  }

  fprintf(stderr, "Compiled: INTEL, flags:");
#if COMPILE_INTEL_MMX
  fprintf(stderr, " MMX");
#endif
#if COMPILE_INTEL_SSE
  fprintf(stderr, " SSE");
#endif
#if COMPILE_INTEL_SSE2
  fprintf(stderr, " SSE2");
#endif
#if COMPILE_INTEL_SSE3
  fprintf(stderr, " SSE3");
#endif
#if COMPILE_INTEL_SSSE3
  fprintf(stderr, " SSSE3");
#endif
#if COMPILE_INTEL_SSE41
  fprintf(stderr, " SSE41");
#endif
#if COMPILE_INTEL_SSE42
  fprintf(stderr, " SSE42");
#endif
#if COMPILE_INTEL_AVX
  fprintf(stderr, " AVX");
#endif
#if COMPILE_INTEL_AVX2
  fprintf(stderr, " AVX2");
#endif
  fprintf(stderr, "\nDetected: INTEL, flags:");
  if (kvz_g_hardware_flags.intel_flags.mmx) fprintf(stderr, " MMX");
  if (kvz_g_hardware_flags.intel_flags.sse) fprintf(stderr, " SSE");
  if (kvz_g_hardware_flags.intel_flags.sse2) fprintf(stderr, " SSE2");
  if (kvz_g_hardware_flags.intel_flags.sse3) fprintf(stderr, " SSE3");
  if (kvz_g_hardware_flags.intel_flags.ssse3) fprintf(stderr, " SSSE3");
  if (kvz_g_hardware_flags.intel_flags.sse41) fprintf(stderr, " SSE41");
  if (kvz_g_hardware_flags.intel_flags.sse42) fprintf(stderr, " SSE42");
  if (kvz_g_hardware_flags.intel_flags.avx) fprintf(stderr, " AVX");
  if (kvz_g_hardware_flags.intel_flags.avx2) fprintf(stderr, " AVX2");
  fprintf(stderr, "\n");
#endif //COMPILE_INTEL

#if COMPILE_POWERPC
  if (cpuid) {
    kvz_g_hardware_flags.powerpc_flags.altivec = altivec_available();
  }
  
  fprintf(stderr, "Compiled: PowerPC, flags:");
#if COMPILE_POWERPC_ALTIVEC
  fprintf(stderr, " AltiVec");
#endif
  fprintf(stderr, "\nDetected: PowerPC, flags:");
  if (kvz_g_hardware_flags.powerpc_flags.altivec) fprintf(stderr, " AltiVec");
  fprintf(stderr, "\n");
#endif
  
}
