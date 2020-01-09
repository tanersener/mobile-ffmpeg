////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2006 Conifer Software.               //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// utils.h

#ifndef UTILS_H
#define UTILS_H

#ifndef PATH_MAX
#ifdef MAX_PATH
#define PATH_MAX MAX_PATH
#elif defined (MAXPATHLEN)
#define PATH_MAX MAXPATHLEN
#else
#define PATH_MAX 1024
#endif
#endif

#if defined(_WIN32)
#undef VERSION_OS
#ifdef _WIN64
#define VERSION_OS "Win64"
#else
#define VERSION_OS "Win32"
#endif
#define PACKAGE_VERSION "5.2.0"
#endif

#define FALSE 0
#define TRUE 1

#define CLEAR(destin) memset (&destin, 0, sizeof (destin));

int copy_timestamp (const char *src_filename, const char *dst_filename);
char *filespec_ext (char *filespec), *filespec_path (char *filespec);
char *filespec_name (char *filespec), *filespec_wild (char *filespec);
void error_line (char *error, ...);
void setup_break (void), finish_line (void);
int check_break (void);
char yna (void);

int DoReadFile (FILE *hFile, void *lpBuffer, uint32_t nNumberOfBytesToRead, uint32_t *lpNumberOfBytesRead);
int DoWriteFile (FILE *hFile, void *lpBuffer, uint32_t nNumberOfBytesToWrite, uint32_t *lpNumberOfBytesWritten);
int64_t DoGetFileSize (FILE *hFile);
int64_t DoGetFilePosition (FILE *hFile);
int DoSetFilePositionAbsolute (FILE *hFile, int64_t pos);
int DoSetFilePositionRelative (FILE *hFile, int64_t pos, int mode);
int DoUngetc (int c, FILE *hFile);
int DoCloseHandle (FILE *hFile);
int DoTruncateFile (FILE *hFile);
int DoDeleteFile (char *filename);
void DoSetConsoleTitle (char *text);

#define FN_FIT(fn) ((strlen (fn) > 30) ? filespec_name (fn) : fn)

#endif
