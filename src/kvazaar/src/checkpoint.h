#ifndef CHECKPOINT_H_
#define CHECKPOINT_H_
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
 * \file
 * Printing of debug information.
 */

#ifdef CHECKPOINTS
#ifdef NDEBUG
#error "CHECKPOINTS require assertions to be enabled!"
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "global.h" // IWYU pragma: keep


extern FILE* g_ckpt_file;
extern int g_ckpt_enabled; //Do we check?
extern int g_ckpt_record; //Do we record?

#define CHECKPOINTS_INIT() do { \
  if (getenv("CHECKPOINTS")) {\
    if (strcmp(getenv("CHECKPOINTS"),"record") == 0) { \
      g_ckpt_file = fopen("__debug_ckpt.log", "w"); assert(g_ckpt_file); \
      g_ckpt_record = 1; \
    } else if (strcmp(getenv("CHECKPOINTS"),"check") == 0) { \
      g_ckpt_file = fopen("__debug_ckpt.log", "r"); assert(g_ckpt_file); \
      g_ckpt_record = 0; \
      g_ckpt_enabled = 0; \
    } else { \
      g_ckpt_file = NULL; \
    } \
  } else {\
    g_ckpt_file = NULL; \
  } \
} while (0)
    
#define CHECKPOINTS_FINALIZE() do {if (g_ckpt_file) fclose(g_ckpt_file); g_ckpt_file = NULL;} while (0)

#define CHECKPOINT_MARK(str, ...) do { \
  if (g_ckpt_file) { \
    if (g_ckpt_record) { \
      fprintf(g_ckpt_file, "MARK: " str "\n", __VA_ARGS__); \
    } else { \
      char buffer_ckpt[4096]; \
      long pos; \
      snprintf(buffer_ckpt, 4095, "MARK: " str "\n", __VA_ARGS__); \
      pos = ftell(g_ckpt_file); \
      g_ckpt_enabled = 0; \
      while (!feof(g_ckpt_file)) { \
        char buffer_file[4096]; \
        assert(fgets(buffer_file, 4095, g_ckpt_file) != NULL); \
        if (strncmp(buffer_file, buffer_ckpt, 4096)==0) { \
          g_ckpt_enabled = 1; \
          break; \
        } \
      } \
      if (!g_ckpt_enabled) fseek(g_ckpt_file, pos, SEEK_SET); \
    } \
  } \
} while (0)
#define CHECKPOINT(str, ...) do { \
  if (g_ckpt_file) { \
    if (g_ckpt_record) { \
      fprintf(g_ckpt_file, str "\n", __VA_ARGS__); \
    } else if (g_ckpt_enabled) { \
      char buffer_file[4096], buffer_ckpt[4096]; \
      assert(fgets(buffer_file, 4095, g_ckpt_file) != NULL); \
      snprintf(buffer_ckpt, 4095, str "\n", __VA_ARGS__); \
      if (strncmp(buffer_file, buffer_ckpt, 4096)!=0) { \
        fprintf(stderr, "Checkpoint failed (at %ld):\nFile: %sExec: %s", ftell(g_ckpt_file), buffer_file, buffer_ckpt); \
        assert(0); \
      } \
    } \
  } \
} while (0)
#endif

#if !defined(CHECKPOINTS)
#define CHECKPOINTS_INIT() 
#define CHECKPOINTS_FINALIZE() 
#define CHECKPOINT_MARK(str, ...) 
#define CHECKPOINT(str, ...) 
#endif



#endif //CHECKPOINT_H_
