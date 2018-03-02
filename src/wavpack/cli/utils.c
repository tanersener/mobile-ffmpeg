////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2006 Conifer Software.               //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// utils.c

// This module provides general purpose utilities for the WavPack command-line
// utilities and the self-extraction module.

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#include <conio.h>
#include <shlobj.h>
#elif defined(__GNUC__) || defined(__sun)
#include <glob.h>
#include <unistd.h>
#endif

#include <sys/stat.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "wavpack.h"
#include "utils.h"

#ifdef _WIN32
#include "win32_unicode_support.h"
#define fprintf fprintf_utf8
#define fputs fputs_utf8
#define remove(f) unlink_utf8(f)
#endif

#ifdef _WIN32

int copy_timestamp (const char *src_filename, const char *dst_filename)
{
    wchar_t *src_filename_utf16 = utf8_to_utf16(src_filename);
    wchar_t *dst_filename_utf16 = utf8_to_utf16(dst_filename);
    FILETIME last_modified;
    HANDLE src, dst;
    int res = TRUE;

    if (*src_filename == '-' || *dst_filename == '-')
        return res;

    if (!src_filename_utf16 || !dst_filename_utf16)
        return FALSE;

    src = CreateFileW (src_filename_utf16, GENERIC_READ, FILE_SHARE_READ, NULL,
         OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    dst = CreateFileW (dst_filename_utf16, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
         OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (src == INVALID_HANDLE_VALUE || dst == INVALID_HANDLE_VALUE ||
        !GetFileTime (src, NULL, NULL, &last_modified) ||
        !SetFileTime (dst, NULL, NULL, &last_modified))
            res = FALSE;

    if (src != INVALID_HANDLE_VALUE)
        CloseHandle (src);

    if (dst != INVALID_HANDLE_VALUE)
        CloseHandle (dst);

    free (src_filename_utf16);
    free (dst_filename_utf16);

    return res;
}

#else

#include <sys/time.h>
#include <sys/types.h>

int copy_timestamp(const char *src_filename, const char *dst_filename)
{
    struct stat fileinfo;
    struct timeval times[2];

    if (strcmp(src_filename, "-") == 0 || strcmp(dst_filename, "-") == 0)
        return TRUE;

    if (stat(src_filename, &fileinfo))
        return FALSE; /* stat failed */

    times[0].tv_sec  = fileinfo.st_atime;
    times[0].tv_usec = 0;

    times[1].tv_sec  = fileinfo.st_mtime;
    times[1].tv_usec = 0;

    if (utimes(dst_filename, times))
        return FALSE; /* utimes failed */

    return TRUE;
}

#endif

//////////////////////////////////////////////////////////////////////////////
// This function parses a filename (with or without full path) and returns  //
// a pointer to the extension (including the "."). If no extension is found //
// then NULL is returned. Extensions with more than 4 letters don't count.  //
//////////////////////////////////////////////////////////////////////////////

#if defined(_WIN32)

char *filespec_ext (char *filespec)
{
    char *cp = filespec + strlen (filespec);

    while (--cp >= filespec) {

        if (*cp == '\\' || *cp == ':')
            return NULL;

        if (*cp == '.') {
            if (strlen (cp+1) && strlen (cp+1) <= 4)
                return cp;
            else
                return NULL;
        }
    }

    return NULL;
}

#else

char *filespec_ext (char *filespec)
{
    char *cp = filespec + strlen (filespec);

    while (--cp >= filespec) {

        if (*cp == '/')
            return NULL;

        if (*cp == '.') {
            if (strlen (cp+1) && strlen (cp+1) <= 4)
                return cp;
            else
                return NULL;
        }
    }

    return NULL;
}

#endif

//////////////////////////////////////////////////////////////////////////////
// This function determines if the specified filespec is a valid pathname.  //
// If not, NULL is returned. If it is in the format of a pathname, then the //
// original pointer is returned. If the format is ambiguous, then a lookup  //
// is performed to determine if it is in fact a valid path, and if so a "\" //
// is appended so that the pathname can be used and the original pointer is //
// returned.                                                                //
//////////////////////////////////////////////////////////////////////////////

#if (defined(__GNUC__) || defined(__sun)) && !defined(_WIN32)

char *filespec_path (char *filespec)
{
    char *cp = filespec + strlen (filespec);
    glob_t globs;
    struct stat fstats;

    if (cp == filespec || filespec_wild (filespec))
        return NULL;

    if (*--cp == '/')
        return filespec;

    if (*cp == '.' && cp == filespec)
        return strcat (filespec, "/");

    if (glob (filespec, GLOB_MARK|GLOB_NOSORT, NULL, &globs) == 0 &&
        globs.gl_pathc > 0)
    {
        /* test if the file is a directory */
        if (stat(globs.gl_pathv[0], &fstats) == 0 && (fstats.st_mode & S_IFDIR)) {
                filespec[0] = '\0';
                strcat (filespec, globs.gl_pathv[0]);
                globfree(&globs);
                return filespec;
        }
    }
    globfree(&globs);

    return NULL;
}

#else

char *filespec_path (char *filespec)
{
    char *cp = filespec + strlen (filespec);
    struct _wfinddata_t wfinddata;
    wchar_t *filespec_utf16;
    intptr_t file;

    if (cp == filespec || filespec_wild (filespec))
        return NULL;

    --cp;

    if (*cp == '\\' || *cp == ':')
        return filespec;

    if (*cp == '.' && cp == filespec)
        return strcat (filespec, "\\");

    filespec_utf16 = utf8_to_utf16(filespec);

    if (!filespec_utf16)
        return NULL;

    if ((file = _wfindfirst (filespec_utf16, &wfinddata)) != (intptr_t) -1 &&
        (wfinddata.attrib & _A_SUBDIR)) {
            _findclose (file);
            free (filespec_utf16);
            return strcat (filespec, "\\");
    }

    if (file != -1L)
        _findclose(file);

    free (filespec_utf16);
    return NULL;
}

#endif

//////////////////////////////////////////////////////////////////////////////
// This function returns non-NULL if the specified filename spec has any    //
// wildcard characters.                                                     //
//////////////////////////////////////////////////////////////////////////////

char *filespec_wild (char *filespec)
{
    return strpbrk (filespec, "*?");
}

//////////////////////////////////////////////////////////////////////////////
// This function parses a filename (with or without full path) and returns  //
// a pointer to the actual filename, or NULL if no filename can be found.   //
//////////////////////////////////////////////////////////////////////////////

#if defined(_WIN32)

char *filespec_name (char *filespec)
{
    char *cp = filespec + strlen (filespec);

    while (--cp >= filespec) {

        if (*cp == '\\' || *cp == ':')
            break;
    }

    if (strlen (cp + 1))
        return cp + 1;
    else
        return NULL;
}

#else

char *filespec_name (char *filespec)
{
    char *cp = filespec + strlen (filespec);

    while (--cp >= filespec)
        if (*cp == '/')
            break;

    if (strlen (cp + 1))
        return cp + 1;
    else
        return NULL;
}

#endif

//////////////////////////////////////////////////////////////////////////////
// This function allows the user to type 'y', 'n', or 'a' (with Enter) in   //
// response to a system query. The return value is the key typed as         //
// lowercase (regardless of the typed case).                                //
//////////////////////////////////////////////////////////////////////////////

static int waiting_input;

char yna (void)
{
    char choice = 0;
    int key;

    waiting_input = 1;

    while (1) {
#if defined(_WIN32)
        key = _getch ();
#else
        key = fgetc(stdin);
#endif
        if (key == 3) {
            fprintf (stderr, "^C\n");
            exit (1);
        }
        else if (key == EOF) {
            fprintf (stderr, "\r\n");
            exit (1);
        }
        else if (key == '\r' || key == '\n') {
            if (choice) {
                fprintf (stderr, "\r\n");
                fflush (stderr);
                break;
            }
            else {
                fprintf (stderr, "%c", 7);
                fflush (stderr);
            }
        }
        else if (key == 'Y' || key == 'y') {
#ifdef _WIN32
            fprintf (stderr, "%c\b", key);
            fflush (stderr);
#endif
            choice = 'y';
        }
        else if (key == 'N' || key == 'n') {
#ifdef _WIN32
            fprintf (stderr, "%c\b", key);
            fflush (stderr);
#endif
            choice = 'n';
        }
        else if (key == 'A' || key == 'a') {
#ifdef _WIN32
            fprintf (stderr, "%c\b", key);
            fflush (stderr);
#endif
            choice = 'a';
        }
        else {
            fprintf (stderr, "%c", 7);
            fflush (stderr);
        }
    }

    waiting_input = 0;

    return choice;
}

//////////////////////////////////////////////////////////////////////////////
// Display the specified message on the console through stderr. Note that   //
// the cursor may start anywhere in the line and all text already on the    //
// line is erased. A terminating newline is not needed and function works   //
// with printf strings and args.                                            //
//////////////////////////////////////////////////////////////////////////////

extern int debug_logging_mode;

#ifdef _WIN32

int get_app_path (char *app_path)
{
    static char file_path [MAX_PATH], tried, result;

    HINSTANCE hinstLib;
    FARPROC ProcAdd;

    if (tried) {
        if (result)
            strcpy (app_path, file_path);

        return result;
    }

    tried = TRUE;
    hinstLib = LoadLibrary ("shell32.dll");

    if (hinstLib) {
        ProcAdd = GetProcAddress (hinstLib, "SHGetFolderPathA");

        if (ProcAdd && SUCCEEDED ((ProcAdd) (NULL, CSIDL_APPDATA | 0x8000, NULL, 0, file_path)))
            result = TRUE;

        if (!result) {
            ProcAdd = GetProcAddress (hinstLib, "SHGetSpecialFolderPathA");

            if (ProcAdd && SUCCEEDED ((ProcAdd) (NULL, file_path, CSIDL_APPDATA, TRUE)))
                result = TRUE;
        }

        FreeLibrary (hinstLib);
    }

    if (!result) {
        hinstLib = LoadLibrary ("shfolder.dll");

        if (hinstLib) {
            ProcAdd = GetProcAddress (hinstLib, "SHGetFolderPathA");

            if (ProcAdd && SUCCEEDED ((ProcAdd) (NULL, CSIDL_APPDATA | 0x8000, NULL, 0, file_path)))
                result = TRUE;

            FreeLibrary (hinstLib);
        }
    }

    if (result)
        strcpy (app_path, file_path);

    return result;
}

void error_line (char *error, ...)
{
    char error_msg [512];
    va_list argptr;

    error_msg [0] = '\r';
    va_start (argptr, error);
    vsprintf (error_msg + 1, error, argptr);
    va_end (argptr);
    fputs (error_msg, stderr);
    finish_line ();

    if (debug_logging_mode) {
        char file_path [MAX_PATH];
        FILE *error_log = NULL;

        if (get_app_path (file_path)) {
            strcat (file_path, "\\WavPack\\wavpack.log");
            error_log = fopen (file_path, "a+");

            if (!error_log) {
                get_app_path (file_path);
                strcat (file_path, "\\WavPack");

                if (CreateDirectory (file_path, NULL)) {
                    strcat (file_path, "\\wavpack.log");
                    error_log = fopen (file_path, "a+");
                }
            }
        }

        if (!error_log)
            error_log = fopen ("c:\\wavpack.log", "a+");

        if (error_log) {
            fputs (error_msg + 1, error_log);
            fputc ('\n', error_log);
            fclose (error_log);
        }
    }
}

#else

void error_line (char *error, ...)
{
    char error_msg [512];
    va_list argptr;

    error_msg [0] = '\r';
    va_start (argptr, error);
    vsprintf (error_msg + 1, error, argptr);
    va_end (argptr);
    fputs (error_msg, stderr);
    finish_line ();
}

#endif

//////////////////////////////////////////////////////////////////////////////
// Function to intercept ^C or ^Break typed at the console.                 //
//////////////////////////////////////////////////////////////////////////////
#if defined(_WIN32)
static int break_flag;

BOOL WINAPI ctrl_handler (DWORD ctrl)
{
    if (ctrl == CTRL_C_EVENT) {
        break_flag = TRUE;
        return TRUE;
    }

    if (ctrl == CTRL_BREAK_EVENT) {

        if (waiting_input) {
            return FALSE;
        }
        else {
            break_flag = TRUE;
            return TRUE;
        }
    }

    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
// Function to initialize console for intercepting ^C and ^Break.           //
//////////////////////////////////////////////////////////////////////////////

void setup_break (void)
{
    HANDLE hConIn = GetStdHandle (STD_INPUT_HANDLE);

    SetConsoleMode (hConIn, ENABLE_PROCESSED_INPUT);
    FlushConsoleInputBuffer (hConIn);
    SetConsoleCtrlHandler (ctrl_handler, TRUE);
    break_flag = 0;
}

//////////////////////////////////////////////////////////////////////////////
// Function to determine whether ^C or ^Break has been issued by user.      //
//////////////////////////////////////////////////////////////////////////////

int check_break (void)
{
    return break_flag;
}

//////////////////////////////////////////////////////////////////////////////
// Function to clear the stderr console to the end of the current line (and //
// go to the beginning next line).                                          //
//////////////////////////////////////////////////////////////////////////////

void finish_line (void)
{
    HANDLE hConIn = GetStdHandle (STD_ERROR_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO coninfo;

    if (hConIn && GetConsoleScreenBufferInfo (hConIn, &coninfo) &&
        (coninfo.dwCursorPosition.X || coninfo.dwCursorPosition.Y)) {
            unsigned char spaces = coninfo.dwSize.X - coninfo.dwCursorPosition.X;

            while (spaces--)
                fputc (' ', stderr);
    }
    else
        fprintf (stderr, "                                \n");

    fflush (stderr);
}
#else
//////////////////////////////////////////////////////////////////////////////
// Function to clear the stderr console to the end of the current line (and //
// go to the beginning next line).                                          //
//////////////////////////////////////////////////////////////////////////////

void finish_line (void)
{
    fprintf (stderr, "                                \n");
    fflush (stderr);
}

//////////////////////////////////////////////////////////////////////////////
// Function to initialize console for intercepting ^C and ^Break.           //
//////////////////////////////////////////////////////////////////////////////

#include <signal.h>

static int break_flag;

static void int_handler(int s)
{
    break_flag = 1;
}

void setup_break (void)
{
    struct sigaction sigIntHandler;

    break_flag = 0;
    sigIntHandler.sa_handler = int_handler;
    sigemptyset (&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction (SIGINT, &sigIntHandler, NULL);
}

//////////////////////////////////////////////////////////////////////////////
// Function to determine whether ^C or ^Break has been issued by user.      //
//////////////////////////////////////////////////////////////////////////////

int check_break (void)
{
    return break_flag;
}

#endif

//////////////////////////// File I/O Wrapper ////////////////////////////////

int DoReadFile (FILE *hFile, void *lpBuffer, uint32_t nNumberOfBytesToRead, uint32_t *lpNumberOfBytesRead)
{
    uint32_t bcount;

    *lpNumberOfBytesRead = 0;

    while (nNumberOfBytesToRead) {
        bcount = (uint32_t) fread ((unsigned char *) lpBuffer + *lpNumberOfBytesRead, 1, nNumberOfBytesToRead, hFile);

        if (bcount) {
            *lpNumberOfBytesRead += bcount;
            nNumberOfBytesToRead -= bcount;
        }
        else
            break;
    }

    return !ferror (hFile);
}

int DoWriteFile (FILE *hFile, void *lpBuffer, uint32_t nNumberOfBytesToWrite, uint32_t *lpNumberOfBytesWritten)
{
    uint32_t bcount;

    *lpNumberOfBytesWritten = 0;

    while (nNumberOfBytesToWrite) {
        bcount = (uint32_t) fwrite ((unsigned char *) lpBuffer + *lpNumberOfBytesWritten, 1, nNumberOfBytesToWrite, hFile);

        if (bcount) {
            *lpNumberOfBytesWritten += bcount;
            nNumberOfBytesToWrite -= bcount;
        }
        else
            break;
    }

    return !ferror (hFile);
}

#ifdef _WIN32

int64_t DoGetFileSize (FILE *hFile)
{
    LARGE_INTEGER Size;
    HANDLE        fHandle;

    if (hFile == NULL)
        return 0;

    fHandle = (HANDLE)_get_osfhandle(_fileno(hFile));
    if (fHandle == INVALID_HANDLE_VALUE)
        return 0;

    Size.u.LowPart = GetFileSize(fHandle, &Size.u.HighPart);

    if (Size.u.LowPart == INVALID_FILE_SIZE && GetLastError() != NO_ERROR)
        return 0;

    return (int64_t)Size.QuadPart;
}

int64_t DoGetFilePosition (FILE *hFile)
{
    return _ftelli64 (hFile);
}

int DoSetFilePositionAbsolute (FILE *hFile, int64_t pos)
{
    return _fseeki64 (hFile, pos, SEEK_SET);
}

int DoSetFilePositionRelative (FILE *hFile, int64_t pos, int mode)
{
    return _fseeki64 (hFile, pos, mode);
}

#else

int64_t DoGetFileSize (FILE *hFile)
{
    struct stat statbuf;

    if (!hFile || fstat (fileno (hFile), &statbuf) || !S_ISREG(statbuf.st_mode))
        return 0;

    return (int64_t) statbuf.st_size;
}

int64_t DoGetFilePosition (FILE *hFile)
{
    return ftell (hFile);
}

int DoSetFilePositionAbsolute (FILE *hFile, int64_t pos)
{
    return fseek (hFile, pos, SEEK_SET);
}

int DoSetFilePositionRelative (FILE *hFile, int64_t pos, int mode)
{
    return fseek (hFile, pos, mode);
}

#endif

// if ungetc() is not available, a seek of -1 is fine also because we do not
// change the byte read.

int DoUngetc (int c, FILE *hFile)
{
    return ungetc (c, hFile);
}

int DoCloseHandle (FILE *hFile)
{
    return hFile ? !fclose (hFile) : 0;
}

int DoTruncateFile (FILE *hFile)
{
    if (hFile) {
        fflush (hFile);
#if defined(_WIN32)
        return !_chsize (_fileno (hFile), 0);
#else
        return !ftruncate(fileno (hFile), 0);
#endif
    }

    return 0;
}

int DoDeleteFile (char *filename)
{
    return filename ? !remove (filename) : 0;
}

/////////////////////////////////////////////////////////////////////////////////
// Function to set the name of the console window. This is very handy for      //
// displaying progress of batch operations with the console window minimized.  //
/////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32

void DoSetConsoleTitle (char *text)
{
    SetConsoleTitle (text);
}

#else

void DoSetConsoleTitle (char *text)
{
    fprintf (stderr, "\033]0;%s\007", text);
    fflush (stderr);
}

#endif

