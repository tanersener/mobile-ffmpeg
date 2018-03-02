/* Copyright (c) 2004-2012 LoRd_MuldeR <mulder2@gmx.de>
   File: unicode_support.h

   This file was originally part of a patch included with LameXP,
   released under the same license as the original audio tools.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef UNICODE_SUPPORT_H_INCLUDED
#define UNICODE_SUPPORT_H_INCLUDED

#include <stdio.h>
#include <sys/stat.h>

#define WIN_UNICODE 1

char *utf16_to_utf8(const wchar_t *input);
wchar_t *utf8_to_utf16(const char *input);
void init_commandline_arguments_utf8(int *argc, char ***argv);
void free_commandline_arguments_utf8(int *argc, char ***argv);
int fprintf_utf8 (FILE *stream, const char *format, ...);
int fputs_utf8 (const char *string, FILE *stream);
FILE *fopen_utf8(const char *filename_utf8, const char *mode_utf8);
int stat_utf8(const char *path_utf8, struct _stat *buf);
int rename_utf8(const char *oldname_utf8, const char *newname_utf8);
int unlink_utf8(const char *path_utf8);

#endif
