////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//                Copyright (c) 1998 - 2019 David Bryant.                 //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// wvgain.c

// This is the main module for the WavPack command-line ReplayGain Scanner/Tagger.

// This implementation is based on the ReplayGain proposal by David Robinson
// with table values copied from the Foobar2000 source code. Many thanks are
// due David Robinson and the others who contributed to ReplayGain.

// ReplayGain's [somewhat outdated] website: http://replaygain.org/

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#else
#if defined(__OS2__)
#define INCL_DOSPROCESS
#include <os2.h>
#include <io.h>
#endif
#include <sys/stat.h>
#include <sys/param.h>
#include <locale.h>
#if defined (__GNUC__)
#include <unistd.h>
#include <glob.h>
#endif
#endif

#if defined(__GNUC__) && !defined(_WIN32)
#include <sys/time.h>
#else
#include <sys/timeb.h>
#endif

#include <math.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#include "wavpack.h"
#include "utils.h"

#ifdef _WIN32
#include "win32_unicode_support.h"
#define fputs fputs_utf8
#define fprintf fprintf_utf8
#define fopen(f,m) fopen_utf8(f,m)
#endif

///////////////////////////// local variable storage //////////////////////////

static const char *sign_on = "\n"
" WVGAIN  ReplayGain Scanner/Tagger for WavPack  %s Version %s\n"
" Copyright (c) 2005 - 2019 David Bryant.  All Rights Reserved.\n\n";

static const char *version_warning = "\n"
" WARNING: WVGAIN using libwavpack version %s, expected %s (see README)\n\n";

static const char *usage =
" Usage:   WVGAIN [-options] [@]infile[.wv] [...]\n"
#if defined (_WIN32) || defined (__OS2__)
"             (infiles may contain wildcards: ?,*)\n\n"
#else
"             (multiple input files are allowed)\n\n"
#endif
" Options: -a  = album mode (all files scanned are considered an album)\n"
"          -c  = clean ReplayGain values from all files (no analysis)\n"
"          -d  = display calculated values only (no files are modified)\n"
"          -i  = ignore .wvc file (forces hybrid lossy)\n"
#if defined (_WIN32) || defined (__OS2__)
"          -l  = run at low priority (for smoother multitasking)\n"
#endif
"          -n  = new files only (skip files with track info, or album\n"
"                 info if album mode specified)\n"
"          -q  = quiet (keep console output to a minimum)\n"
"          -s  = show stored values only (no analysis)\n"
"          -v  = write the version to stdout\n"
#if defined (_WIN32)
"          -z  = don't set console title to indicate progress\n\n"
#else
"          -z1 = set console title to indicate progress\n\n"
#endif
" Web:     Visit www.wavpack.com for latest version and info\n";

// this global is used to indicate the special "debug" mode where extra debug messages
// are displayed and all messages are logged to the file wavpack.log

int debug_logging_mode;

#define HISTOGRAM_SLOTS 12000
static uint32_t track_histogram [HISTOGRAM_SLOTS], album_histogram [HISTOGRAM_SLOTS];

static char album_mode, clean_mode, display_mode, ignore_wvc, quiet_mode, show_mode, new_mode, set_console_title;
static int num_files, file_index;

/////////////////////////// local function declarations ///////////////////////

static int update_file (char *infilename, float track_gain, float track_peak, float album_gain, float album_peak);
static int analyze_file (char *infilename, uint32_t *histogram, float *peak);
static int show_file_info (char *infilename, FILE *dst);
static float calc_replaygain (uint32_t *histogram);
static void *decimation_init (int num_channels, int ratio);
static int decimation_run (void *context, int32_t *samples, int num_samples);
static void *decimation_destroy (void *context);
static void display_progress (double file_progress);

#ifdef _WIN32
static void TextToUTF8 (void *string, int len);
#endif

#define WAVPACK_NO_ERROR    0
#define WAVPACK_SOFT_ERROR  1
#define WAVPACK_HARD_ERROR  2

// The "main" function for the command-line WavPack ReplayGain Scanner/Processor.
// Note that on Windows this is actually a static function that is called from the
// "real" main() defined immediately afterward that converts the wchar argument list
// into UTF-8 strings and sets the console to UTF-8 for better Unicode support.

#ifdef _WIN32
static int wvgain_main(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
#ifdef __EMX__ /* OS/2 */
    _wildcard (&argc, &argv);
#endif
    int  error_count = 0;
    char **matches = NULL;
    int result = WAVPACK_NO_ERROR;

#if defined(_WIN32)
    char selfname [MAX_PATH];

    if (GetModuleFileName (NULL, selfname, sizeof (selfname)) && filespec_name (selfname) &&
        _strupr (filespec_name (selfname)) && strstr (filespec_name (selfname), "DEBUG"))
            debug_logging_mode = TRUE;

    strcpy (selfname, *argv);
#else
    if (filespec_name (*argv) &&
        (strstr (filespec_name (*argv), "ebug") || strstr (filespec_name (*argv), "DEBUG")))
            debug_logging_mode = TRUE;
#endif

    if (debug_logging_mode) {
        char **argv_t = argv;
        int argc_t = argc;

        while (--argc_t)
            error_line ("arg %d: %s", argc - argc_t, *++argv_t);
    }

#if defined (_WIN32)
   set_console_title = 1;      // on Windows, we default to messing with the console title
#endif                          // on Linux, this is considered uncool to do by default

    // loop through command-line arguments

    while (--argc) {
#if defined (_WIN32)
        if ((**++argv == '-' || **argv == '/') && (*argv)[1])
#else
        if ((**++argv == '-') && (*argv)[1])
#endif
            while (*++*argv)
                switch (**argv) {

                    case 'V': case 'v':
                        printf ("wvgain %s\n", PACKAGE_VERSION);
                        printf ("libwavpack %s\n", WavpackGetLibraryVersionString ());
                        return 0;

                    case 'A': case 'a':
                        album_mode = 1;
                        break;

                    case 'C': case 'c':
                        clean_mode = 1;
                        break;

                    case 'D': case 'd':
                        display_mode = 1;
                        break;

#if defined (_WIN32)
                    case 'L': case 'l':
                        SetPriorityClass (GetCurrentProcess(), IDLE_PRIORITY_CLASS);
                        break;
#elif defined (__OS2__)
                    case 'L': case 'l':
                        DosSetPriority (0, PRTYC_IDLETIME, 0, 0);
                        break;
#endif
                    case 'N': case 'n':
                        new_mode = 1;
                        break;

                    case 'Q': case 'q':
                        quiet_mode = 1;
                        break;

                    case 'Z': case 'z':
                        set_console_title = (char) strtol (++*argv, argv, 10);
                        --*argv;
                        break;

                    case 'I': case 'i':
                        ignore_wvc = 1;
                        break;

                    case 'S': case 's':
                        show_mode = 1;
                        break;

                    default:
                        error_line ("illegal option: %c !", **argv);
                        ++error_count;
                }
        else {
            matches = realloc (matches, (num_files + 1) * sizeof (*matches));
            matches [num_files] = malloc (strlen (*argv) + 10);
            strcpy (matches [num_files], *argv);

            if (*(matches [num_files]) != '-' && *(matches [num_files]) != '@' &&
                !filespec_ext (matches [num_files]))
                    strcat (matches [num_files], ".wv");

            num_files++;
        }
    }

    // check for various command-line argument problems

    if (clean_mode && (album_mode || display_mode || show_mode)) {
        error_line ("clean mode can't be used with album, show, or display mode!");
        ++error_count;
    }
    else if (show_mode && (album_mode || display_mode)) {
        error_line ("show mode can't be used with album or display mode!");
        ++error_count;
    }

    if (strcmp (WavpackGetLibraryVersionString (), PACKAGE_VERSION)) {
        fprintf (stderr, version_warning, WavpackGetLibraryVersionString (), PACKAGE_VERSION);
        fflush (stderr);
    }
    else if (!quiet_mode && !error_count) {
        fprintf (stderr, sign_on, VERSION_OS, WavpackGetLibraryVersionString ());
        fflush (stderr);
    }

    if (!num_files) {
        printf ("%s", usage);
        return 1;
    }

    if (error_count)
        return 1;

    setup_break ();

    for (file_index = 0; file_index < num_files; ++file_index) {
        char *infilename = matches [file_index];

        // If the single infile specification begins with a '@', then it
        // actually points to a file that contains the names of the files
        // to be converted. This was included for use by Wim Speekenbrink's
        // frontends, but could be used for other purposes.

        if (*infilename == '@') {
            FILE *list = fopen (infilename+1, "rb");
            char *listbuff = NULL, *cp;
            int listbytes = 0, di, c;

            for (di = file_index; di < num_files - 1; di++)
                matches [di] = matches [di + 1];

            file_index--;
            num_files--;

            if (list == NULL) {
                error_line ("file %s not found!", infilename+1);
                free (infilename);
                return 1;
            }

            while (1) {
                int bytes_read;

                listbuff = realloc (listbuff, listbytes + 1024);
                memset (listbuff + listbytes, 0, 1024);
                listbytes += bytes_read = (int) fread (listbuff + listbytes, 1, 1024, list);

                if (bytes_read < 1024)
                    break;
            }

#if defined (_WIN32)
            listbuff = realloc (listbuff, listbytes *= 2);
            TextToUTF8 (listbuff, listbytes);
#endif
            cp = listbuff;

            while ((c = *cp++)) {

                while (c == '\n' || c == '\r')
                    c = *cp++;

                if (c) {
                    char *fname = malloc (PATH_MAX);
                    int ci = 0;

                    do
                        fname [ci++] = c;
                    while ((c = *cp++) != '\n' && c != '\r' && c && ci < PATH_MAX);

                    fname [ci++] = '\0';
                    matches = realloc (matches, ++num_files * sizeof (*matches));

                    for (di = num_files - 1; di > file_index + 1; di--)
                        matches [di] = matches [di - 1];

                    matches [++file_index] = fname;
                }

                if (!c)
                    break;
            }

            fclose (list);
            free (listbuff);
            free (infilename);
        }
#if defined (_WIN32)
        else if (filespec_wild (infilename)) {
            wchar_t *winfilename = utf8_to_utf16(infilename);
            struct _wfinddata_t _wfinddata_t;
            intptr_t file;
            int di;

            for (di = file_index; di < num_files - 1; di++)
                matches [di] = matches [di + 1];

            file_index--;
            num_files--;

            if ((file = _wfindfirst (winfilename, &_wfinddata_t)) != (intptr_t) -1) {
                do {
                    char *name_utf8;

                    if (!(_wfinddata_t.attrib & _A_SUBDIR) && (name_utf8 = utf16_to_utf8(_wfinddata_t.name))) {
                        matches = realloc (matches, ++num_files * sizeof (*matches));

                        for (di = num_files - 1; di > file_index + 1; di--)
                            matches [di] = matches [di - 1];

                        matches [++file_index] = malloc (strlen (infilename) + strlen (name_utf8) + 10);
                        strcpy (matches [file_index], infilename);
                        *filespec_name (matches [file_index]) = '\0';
                        strcat (matches [file_index], name_utf8);
                        free (name_utf8);
                    }
                } while (_wfindnext (file, &_wfinddata_t) == 0);

                _findclose (file);
            }

            free (winfilename);
            free (infilename);
        }
#endif
    }

    // if we found any files to process, this is where we start

    if (num_files) {
        float *track_gains, *track_peaks, album_gain;
        float track_peak, album_peak = 0.0;
        int i;

        track_gains = malloc (sizeof (*track_gains) * num_files);
        track_peaks = malloc (sizeof (*track_peaks) * num_files);

        // Loop through and analyze files in list. If we are in album mode we just keep
        // track of everything and modify the tags in another pass. If we're not in
        // album mode then we can update the files here.

        for (file_index = 0; !clean_mode && !show_mode && file_index < num_files; ++file_index) {
            if (check_break ())
                break;

            if (num_files > 1 && !quiet_mode) {
                fprintf (stderr, "\n%s:\n", matches [file_index]);
                fflush (stderr);
            }

            if (new_mode) {
                WavpackContext *wpc;
                char error [80];
#ifdef _WIN32
                wpc = WavpackOpenFileInput (matches [file_index], error, OPEN_TAGS | OPEN_FILE_UTF8 | OPEN_DSD_AS_PCM, 0);
#else
                wpc = WavpackOpenFileInput (matches [file_index], error, OPEN_TAGS | OPEN_DSD_AS_PCM, 0);
#endif
                if (wpc) {
                    int alreadyHasTag = WavpackGetTagItem (wpc, album_mode ? "replaygain_album_gain" : "replaygain_track_gain", NULL, 0);
                    WavpackCloseFile (wpc);

                    if (alreadyHasTag) {
                        if (album_mode) {
                            error_line ("ReplayGain album information already present...aborting");
                            result = WAVPACK_HARD_ERROR;
                            break;
                        }
                        else {
                            error_line ("ReplayGain track information already present...skipping");
                            continue;
                        }
                    }
                }
            }

            result = analyze_file (matches [file_index], track_histogram, &track_peak);

            if (result != WAVPACK_NO_ERROR) {
                ++error_count;

                if (album_mode || result == WAVPACK_HARD_ERROR) {
                    result = WAVPACK_HARD_ERROR;
                    break;
                }
                else
                    continue;
            }

            track_gains [file_index] = calc_replaygain (track_histogram);
            track_peaks [file_index] = track_peak;

            if (!quiet_mode) {
                error_line ("replaygain_track_gain = %+.2f dB", track_gains [file_index]);
                error_line ("replaygain_track_peak = %.6f", track_peaks [file_index]);
            }

            if (album_mode) {
                for (i = 0; i < HISTOGRAM_SLOTS; ++i)
                    album_histogram [i] += track_histogram [i];

                if (track_peak > album_peak)
                    album_peak = track_peak;
            }
            else if (!display_mode) {
                result = update_file (matches [file_index], track_gains [file_index], track_peaks [file_index], 0, 0);

                if (result != WAVPACK_NO_ERROR) {
                    ++error_count;

                    if (result == WAVPACK_HARD_ERROR)
                        break;
                }
            }
        }

        if (result != WAVPACK_HARD_ERROR) {
            album_gain = calc_replaygain (album_histogram);

            if (album_mode && !quiet_mode && num_files > 1) {
                error_line ("\nalbum results:");
                error_line ("replaygain_album_gain = %+.2f dB", album_gain);
                error_line ("replaygain_album_peak = %.6f", album_peak);
            }
        }

        // If we are in album mode or clear mode, this is where we loop through and modify
        // the tags (or just show existing stored values).

        if (result != WAVPACK_HARD_ERROR)
            for (file_index = 0; (clean_mode || album_mode || show_mode) && !display_mode && file_index < num_files; ++file_index) {
                if (check_break ())
                    break;

                if (num_files > 1 && !quiet_mode) {
                    fprintf (stderr, "\n%s:\n", matches [file_index]);
                    fflush (stderr);
                }

                if (show_mode)
                    result = show_file_info (matches [file_index], stdout);
                else
                    result = update_file (matches [file_index], track_gains [file_index], track_peaks [file_index], album_gain, album_peak);

                free (matches [file_index]);

                if (result != WAVPACK_NO_ERROR) {
                    ++error_count;

                    if (result == WAVPACK_HARD_ERROR)
                        break;
                }
            }

        if (num_files > 1) {
            if (error_count) {
                fprintf (stderr, "\n **** warning: errors occurred in %d of %d files! ****\n", error_count, num_files);
                fflush (stderr);
            }
            else if (!quiet_mode) {
                fprintf (stderr, "\n **** %d files successfully processed ****\n", num_files);
                fflush (stderr);
            }
        }

        free (track_peaks);
        free (track_gains);
        free (matches);
    }
    else {
        ++error_count;
        error_line ("nothing to do!");
    }

    if (set_console_title)
        DoSetConsoleTitle ("WvGain Completed");

    return error_count ? 1 : 0;
}

#ifdef _WIN32

// On Windows, this "real" main() acts as a shell to our static wvgain_main().
// Its purpose is to convert the wchar command-line arguments into UTF-8 encoded
// strings and set the console output to UTF-8.

int main(int argc, char **argv)
{
    int ret = -1, argc_utf8 = -1;
    char **argv_utf8 = NULL;
    char **argv_copy = NULL;

    init_commandline_arguments_utf8(&argc_utf8, &argv_utf8);

    // we have to make a copy of the argv pointer array because the command parser
    // sometimes modifies them, which is problematic when it comes time to free them

    if (argc_utf8 && argv_utf8) {
        argv_copy = malloc (sizeof (char*) * argc_utf8);
        memcpy (argv_copy, argv_utf8, sizeof (char*) * argc_utf8);
    }

    ret = wvgain_main(argc_utf8, argv_copy);

    if (argv_copy)
        free (argv_copy);

    free_commandline_arguments_utf8(&argc_utf8, &argv_utf8);

    return ret;
}

#endif

// Unpack the specified WavPack input file and analyze it for ReplayGain
// information.

static void calc_stereo_peak (float *samples, uint32_t samcnt, float *peak_p);
static double calc_stereo_rms (float *samples, uint32_t samcnt);
static int filter_init (uint32_t sample_rate);
static void filter_stereo_samples (float *samples, uint32_t samcnt);
static void float_samples (float *dst, int32_t *src, uint32_t samcnt, float scale);

static int analyze_file (char *infilename, uint32_t *histogram, float *peak)
{
    int result = WAVPACK_NO_ERROR, open_flags = 0, num_channels, wvc_mode;
    uint32_t sample_rate, window_samples;
    int64_t total_unpacked_samples = 0;
    void *decimation_context = NULL;
    double progress = -1.0;
    int32_t *temp_buffer;
    WavpackContext *wpc;
    char error [80];

    memset (histogram, 0, sizeof (*histogram) * HISTOGRAM_SLOTS);
    *peak = 0.0;

    // use library to open WavPack file

#ifdef _WIN32
    open_flags |= OPEN_FILE_UTF8;
#endif

    if (!ignore_wvc)
        open_flags |= OPEN_WVC;

    open_flags |= OPEN_TAGS | OPEN_NORMALIZE | OPEN_DSD_AS_PCM;

    wpc = WavpackOpenFileInput (infilename, error, open_flags, 0);

    if (!wpc) {
        error_line (error);
        return WAVPACK_SOFT_ERROR;
    }

    wvc_mode = WavpackGetMode (wpc) & MODE_WVC;
    num_channels = WavpackGetNumChannels (wpc);

    if (num_channels > 2) {
        error_line ("can't handle multichannel files yet!");
        return WAVPACK_SOFT_ERROR;
    }

    if (!quiet_mode) {
        fprintf (stderr, "analyzing %s%s,", *infilename == '-' ? "stdin" :
            FN_FIT (infilename), wvc_mode ? " (+.wvc)" : "");
        fflush (stderr);
    }

    sample_rate = WavpackGetSampleRate (wpc);

    if (sample_rate >= 256000) {
        decimation_context = decimation_init (num_channels, 4);
        sample_rate /= 4;
    }

    window_samples = sample_rate / 20;

    if (decimation_context)
        temp_buffer = malloc (window_samples * 8 * 4);
    else
        temp_buffer = malloc (window_samples * 8);

    if (!filter_init (sample_rate))
        result = WAVPACK_SOFT_ERROR;

    while (result == WAVPACK_NO_ERROR) {
        uint32_t samples_to_unpack, samples_unpacked;
        int32_t level;

        samples_to_unpack = window_samples;

        if (decimation_context) {
            samples_unpacked = WavpackUnpackSamples (wpc, temp_buffer, samples_to_unpack * 4);
            total_unpacked_samples += samples_unpacked;
            samples_unpacked = decimation_run (decimation_context, temp_buffer, samples_unpacked);
        }
        else {
            samples_unpacked = WavpackUnpackSamples (wpc, temp_buffer, samples_to_unpack);
            total_unpacked_samples += samples_unpacked;
        }

        if (samples_unpacked) {
            if (!(WavpackGetMode (wpc) & MODE_FLOAT))
                switch (WavpackGetBytesPerSample (wpc)) {
                    case 1:
                        float_samples ((float *) temp_buffer, temp_buffer, samples_unpacked * num_channels, 1.0 / 128.0);
                        break;

                    case 2:
                        float_samples ((float *) temp_buffer, temp_buffer, samples_unpacked * num_channels, 1.0 / 32768.0);
                        break;

                    case 3:
                        float_samples ((float *) temp_buffer, temp_buffer, samples_unpacked * num_channels, 1.0 / 8388608.0);
                        break;

                    case 4:
                        float_samples ((float *) temp_buffer, temp_buffer, samples_unpacked * num_channels, 1.0 / 2147483648.0);
                        break;
                }

            if (num_channels == 1) {
                int32_t *dst = temp_buffer + samples_unpacked * 2;
                int32_t *src = temp_buffer + samples_unpacked;
                uint32_t cnt = samples_unpacked;

                while (cnt--) {
                    *--dst = *--src;
                    *--dst = *src;
                }
            }

            calc_stereo_peak ((float *) temp_buffer, samples_unpacked, peak);
            filter_stereo_samples ((float *) temp_buffer, samples_unpacked);
            level = (int32_t) floor (100 * calc_stereo_rms ((float *) temp_buffer, samples_unpacked));

            if (level < 0)
                histogram [0]++;
            else if (level >= HISTOGRAM_SLOTS)
                histogram [HISTOGRAM_SLOTS - 1]++;
            else
                histogram [level]++;
        }
        else
            break;

        if (check_break ()) {
#if defined(_WIN32)
            fprintf (stderr, "^C\n");
#else
            fprintf (stderr, "\n");
#endif
            fflush (stderr);
            result = WAVPACK_HARD_ERROR;
            break;
        }

        if (WavpackGetProgress (wpc) != -1.0 &&
            progress != floor (WavpackGetProgress (wpc) * 100.0 + 0.5)) {
                int nobs = progress == -1.0;

                progress = WavpackGetProgress (wpc);
                display_progress (progress);
                progress = floor (progress * 100.0 + 0.5);

                if (!quiet_mode) {
                    fprintf (stderr, "%s%3d%% done...",
                        nobs ? " " : "\b\b\b\b\b\b\b\b\b\b\b\b", (int) progress);
                    fflush (stderr);
                }
        }
    }

    free (temp_buffer);

    if (decimation_context)
        decimation_destroy (decimation_context);

    if (result == WAVPACK_NO_ERROR && WavpackGetNumSamples64 (wpc) != -1 &&
        total_unpacked_samples != WavpackGetNumSamples64 (wpc)) {
            error_line ("incorrect number of samples!");
            result = WAVPACK_SOFT_ERROR;
    }

    if (result == WAVPACK_NO_ERROR && WavpackGetNumErrors (wpc)) {
        error_line ("crc errors detected in %d block(s)!", WavpackGetNumErrors (wpc));
        result = WAVPACK_SOFT_ERROR;
    }

    WavpackCloseFile (wpc);
    return result;
}

// Update the tag of the specified file to reflect the results of the ReplayGain analysis
// (or just to remove existing ReplayGain information).

static int update_file (char *infilename, float track_gain, float track_peak, float album_gain, float album_peak)
{
    int write_tag = FALSE;
    char error [80], value [20];
    WavpackContext *wpc;

    // use library to open WavPack file

#ifdef _WIN32
    wpc = WavpackOpenFileInput (infilename, error, OPEN_EDIT_TAGS | OPEN_FILE_UTF8 | OPEN_DSD_AS_PCM, 0);
#else
    wpc = WavpackOpenFileInput (infilename, error, OPEN_EDIT_TAGS | OPEN_DSD_AS_PCM, 0);
#endif

    if (!wpc) {
        error_line (error);
        return WAVPACK_SOFT_ERROR;
    }

    if (clean_mode) {
        int items_removed = 0;

        if (WavpackDeleteTagItem (wpc, "replaygain_track_gain"))
            ++items_removed;

        if (WavpackDeleteTagItem (wpc, "replaygain_track_peak"))
            ++items_removed;

        if (WavpackDeleteTagItem (wpc, "replaygain_album_gain"))
            ++items_removed;

        if (WavpackDeleteTagItem (wpc, "replaygain_album_peak"))
            ++items_removed;

        if (items_removed) {
            if (!quiet_mode)
                error_line ("%d ReplayGain values cleaned", items_removed);

            write_tag = TRUE;
        }
        else
            error_line ("no ReplayGain values found");
    }
    else {
        if ((WavpackGetMode (wpc) & (MODE_VALID_TAG | MODE_APETAG)) == MODE_VALID_TAG) {
            char title [40], artist [40], album [40], year [10], comment [40], track [10];

            WavpackGetTagItem (wpc, "title", title, sizeof (title));
            WavpackGetTagItem (wpc, "artist", artist, sizeof (artist));
            WavpackGetTagItem (wpc, "album", album, sizeof (album));
            WavpackGetTagItem (wpc, "year", year, sizeof (year));
            WavpackGetTagItem (wpc, "comment", comment, sizeof (comment));
            WavpackGetTagItem (wpc, "track", track, sizeof (track));

            if (title [0])
                WavpackAppendTagItem (wpc, "Title", title, (int) strlen (title));

            if (artist [0])
                WavpackAppendTagItem (wpc, "Artist", artist, (int) strlen (artist));

            if (album [0])
                WavpackAppendTagItem (wpc, "Album", album, (int) strlen (album));

            if (year [0])
                WavpackAppendTagItem (wpc, "Year", year, (int) strlen (year));

            if (comment [0])
                WavpackAppendTagItem (wpc, "Comment", comment, (int) strlen (comment));

            if (track [0])
                WavpackAppendTagItem (wpc, "Track", track, (int) strlen (track));

            error_line ("warning: ID3v1 tag converted to APEv2");
        }

        sprintf (value, "%+.2f dB", track_gain);
        WavpackAppendTagItem (wpc, "replaygain_track_gain", value, (int) strlen (value));

        sprintf (value, "%.6f", track_peak);
        WavpackAppendTagItem (wpc, "replaygain_track_peak", value, (int) strlen (value));

        if (album_mode) {
            sprintf (value, "%+.2f dB", album_gain);
            WavpackAppendTagItem (wpc, "replaygain_album_gain", value, (int) strlen (value));
            sprintf (value, "%.6f", album_peak);
            WavpackAppendTagItem (wpc, "replaygain_album_peak", value, (int) strlen (value));
        }

        if (!quiet_mode)
            error_line ("%d ReplayGain values appended", album_mode ? 4 : 2);

        write_tag = TRUE;
    }

    if (write_tag && !WavpackWriteTag (wpc)) {
        error_line ("%s", WavpackGetErrorMessage (wpc));
        return WAVPACK_SOFT_ERROR;
    }

    WavpackCloseFile (wpc);
    return WAVPACK_NO_ERROR;
}

// Just show any ReplayGain tags for the specified file

static int show_file_info (char *infilename, FILE *dst)
{
    char error [80], value [20];
    WavpackContext *wpc;
    int items = 0;

    // use library to open WavPack file

#ifdef _WIN32
    wpc = WavpackOpenFileInput (infilename, error, OPEN_TAGS | OPEN_FILE_UTF8 | OPEN_DSD_AS_PCM, 0);
#else
    wpc = WavpackOpenFileInput (infilename, error, OPEN_TAGS | OPEN_DSD_AS_PCM, 0);
#endif

    if (!wpc) {
        error_line (error);
        return WAVPACK_SOFT_ERROR;
    }

    fprintf (dst, "\nfile: %s\n", infilename);

    if (WavpackGetTagItem (wpc, "replaygain_track_gain", value, sizeof (value))) {
        fprintf (dst, "replaygain_track_gain = %s\n", value);
        ++items;
    }

    if (WavpackGetTagItem (wpc, "replaygain_track_peak", value, sizeof (value))) {
        fprintf (dst, "replaygain_track_peak = %s\n", value);
        ++items;
    }

    if (WavpackGetTagItem (wpc, "replaygain_album_gain", value, sizeof (value))) {
        fprintf (dst, "replaygain_album_gain = %s\n", value);
        ++items;
    }

    if (WavpackGetTagItem (wpc, "replaygain_album_peak", value, sizeof (value))) {
        fprintf (dst, "replaygain_album_peak = %s\n", value);
        ++items;
    }

    if (!items)
        fprintf (dst, "no ReplayGain values found\n");

    WavpackCloseFile (wpc);
    return WAVPACK_NO_ERROR;
}

// Calculate the ReplayGain value from the specified loudness histogram; clip to -24 / +64 dB

static float calc_replaygain (uint32_t *histogram)
{
    uint32_t loud_count = 0, total_windows = 0;
    float unclipped_gain;
    int i;

    for (i = 0; i < HISTOGRAM_SLOTS; i++)
        total_windows += histogram [i];

    while (i--)
        if ((loud_count += histogram [i]) * 20 >= total_windows)
            break;

    unclipped_gain = (float)(64.54 - i / 100.0);

    if (unclipped_gain > 64.0)
        return 64.0;
    else if (unclipped_gain < -24.0)
        return -24.0;
    else
        return unclipped_gain;
}

// Convert the specified samples into floating-point using the specified scale factor.

static void float_samples (float *dst, int32_t *src, uint32_t samcnt, float scale)
{
    while (samcnt--)
        *dst++ = *src++ * scale;
}

// These are the filters used to calculate perceived loudness. The table data was copied
// from the Foobar2000 source code.

#define YULE_ORDER         10
#define BUTTER_ORDER        2

struct rg_freqinfo
{
    uint32_t rate;
    double BYule[YULE_ORDER+1],AYule[YULE_ORDER+1],BButter[BUTTER_ORDER+1],AButter[BUTTER_ORDER+1];
};

static struct rg_freqinfo freqinfos[] =
{
    {
        48000,
        { 0.03857599435200, -0.02160367184185, -0.00123395316851, -0.00009291677959, -0.01655260341619,  0.02161526843274, -0.02074045215285,  0.00594298065125,  0.00306428023191,  0.00012025322027,  0.00288463683916 },
        { 1., -3.84664617118067,  7.81501653005538,-11.34170355132042, 13.05504219327545,-12.28759895145294,  9.48293806319790, -5.87257861775999,  2.75465861874613, -0.86984376593551, 0.13919314567432 },
        { 0.98621192462708, -1.97242384925416, 0.98621192462708 },
        { 1., -1.97223372919527, 0.97261396931306 },
    },

    {
        44100,
        { 0.05418656406430, -0.02911007808948, -0.00848709379851, -0.00851165645469, -0.00834990904936,  0.02245293253339, -0.02596338512915,  0.01624864962975, -0.00240879051584,  0.00674613682247, -0.00187763777362 },
        { 1., -3.47845948550071,  6.36317777566148, -8.54751527471874,  9.47693607801280, -8.81498681370155,  6.85401540936998, -4.39470996079559,  2.19611684890774, -0.75104302451432, 0.13149317958808 },
        { 0.98500175787242, -1.97000351574484, 0.98500175787242 },
        { 1., -1.96977855582618, 0.97022847566350 },
    },

    {
        32000,
        { 0.15457299681924, -0.09331049056315, -0.06247880153653,  0.02163541888798, -0.05588393329856,  0.04781476674921,  0.00222312597743,  0.03174092540049, -0.01390589421898,  0.00651420667831, -0.00881362733839 },
        { 1., -2.37898834973084,  2.84868151156327, -2.64577170229825,  2.23697657451713, -1.67148153367602,  1.00595954808547, -0.45953458054983,  0.16378164858596, -0.05032077717131, 0.02347897407020 },
        { 0.97938932735214, -1.95877865470428, 0.97938932735214 },
        { 1., -1.95835380975398, 0.95920349965459 },
    },

    {
        24000,
        { 0.30296907319327, -0.22613988682123, -0.08587323730772,  0.03282930172664, -0.00915702933434, -0.02364141202522, -0.00584456039913,  0.06276101321749, -0.00000828086748,  0.00205861885564, -0.02950134983287 },
        { 1., -1.61273165137247,  1.07977492259970, -0.25656257754070, -0.16276719120440, -0.22638893773906,  0.39120800788284, -0.22138138954925,  0.04500235387352,  0.02005851806501, 0.00302439095741 },
        { 0.97531843204928, -1.95063686409857, 0.97531843204928 },
        { 1., -1.95002759149878, 0.95124613669835 },
    },

    {
        22050,
        { 0.33642304856132, -0.25572241425570, -0.11828570177555,  0.11921148675203, -0.07834489609479, -0.00469977914380, -0.00589500224440,  0.05724228140351,  0.00832043980773, -0.01635381384540, -0.01760176568150 },
        { 1., -1.49858979367799,  0.87350271418188,  0.12205022308084, -0.80774944671438,  0.47854794562326, -0.12453458140019, -0.04067510197014,  0.08333755284107, -0.04237348025746, 0.02977207319925 },
        { 0.97316523498161, -1.94633046996323, 0.97316523498161 },
        { 1., -1.94561023566527, 0.94705070426118 },
    },

    {
        16000,
        { 0.44915256608450, -0.14351757464547, -0.22784394429749, -0.01419140100551,  0.04078262797139, -0.12398163381748,  0.04097565135648,  0.10478503600251, -0.01863887810927, -0.03193428438915,  0.00541907748707 },
        { 1., -0.62820619233671,  0.29661783706366, -0.37256372942400,  0.00213767857124, -0.42029820170918,  0.22199650564824,  0.00613424350682,  0.06747620744683,  0.05784820375801, 0.03222754072173 },
        { 0.96454515552826, -1.92909031105652, 0.96454515552826 },
        { 1., -1.92783286977036, 0.93034775234268 },
    },

    {
        12000,
        { 0.56619470757641, -0.75464456939302,  0.16242137742230,  0.16744243493672, -0.18901604199609,  0.30931782841830, -0.27562961986224,  0.00647310677246,  0.08647503780351, -0.03788984554840, -0.00588215443421 },
        { 1., -1.04800335126349,  0.29156311971249, -0.26806001042947,  0.00819999645858,  0.45054734505008, -0.33032403314006,  0.06739368333110, -0.04784254229033,  0.01639907836189, 0.01807364323573 },
        { 0.96009142950541, -1.92018285901082, 0.96009142950541 },
        { 1., -1.91858953033784, 0.92177618768381 },
    },

    {
        11025,
        { 0.58100494960553, -0.53174909058578, -0.14289799034253,  0.17520704835522,  0.02377945217615,  0.15558449135573, -0.25344790059353,  0.01628462406333,  0.06920467763959, -0.03721611395801, -0.00749618797172 },
        { 1., -0.51035327095184, -0.31863563325245, -0.20256413484477,  0.14728154134330,  0.38952639978999, -0.23313271880868, -0.05246019024463, -0.02505961724053,  0.02442357316099, 0.01818801111503 },
        { 0.95856916599601, -1.91713833199203, 0.95856916599601 },
        { 1., -1.91542108074780, 0.91885558323625 },
    },

    {
        8000,
        { 0.53648789255105, -0.42163034350696, -0.00275953611929,  0.04267842219415, -0.10214864179676,  0.14590772289388, -0.02459864859345, -0.11202315195388, -0.04060034127000,  0.04788665548180, -0.02217936801134 },
        { 1., -0.25049871956020, -0.43193942311114, -0.03424681017675, -0.04678328784242,  0.26408300200955,  0.15113130533216, -0.17556493366449, -0.18823009262115,  0.05477720428674, 0.04704409688120 },
        { 0.94597685600279, -1.89195371200558, 0.94597685600279 },
        { 1., -1.88903307939452, 0.89487434461664 },
    },

    {
        18900,
        {0.38524531015142,  -0.27682212062067,  -0.09980181488805,   0.09951486755646,  -0.08934020156622,  -0.00322369330199,  -0.00110329090689,   0.03784509844682,   0.01683906213303,  -0.01147039862572,  -0.01941767987192 },
        {1.00000000000000,  -1.29708918404534,   0.90399339674203,  -0.29613799017877,  -0.42326645916207,   0.37934887402200,  -0.37919795944938,   0.23410283284785,  -0.03892971758879,   0.00403009552351,   0.03640166626278 },
        {0.96535326815829,  -1.93070653631658,   0.96535326815829 },
        {1.00000000000000,  -1.92950577983524,   0.93190729279793 },
    },

    {
        37800,
        {0.08717879977844,  -0.01000374016172,  -0.06265852122368,  -0.01119328800950,  -0.00114279372960,   0.02081333954769,  -0.01603261863207,   0.01936763028546,   0.00760044736442,  -0.00303979112271,  -0.00075088605788 },
        {1.00000000000000,  -2.62816311472146,   3.53734535817992,  -3.81003448678921,   3.91291636730132,  -3.53518605896288,   2.71356866157873,  -1.86723311846592,   1.12075382367659,  -0.48574086886890,   0.11330544663849 },
        {0.98252400815195,  -1.96504801630391,   0.98252400815195 },
        {1.00000000000000,  -1.96474258269041,   0.96535344991740 },
    },

    {
        56000,
        {0.03144914734085,  -0.06151729206963,   0.08066788708145,  -0.09737939921516,   0.08943210803999,  -0.06989984672010,   0.04926972841044,  -0.03161257848451,   0.01456837493506,  -0.00316015108496,   0.00132807215875 },
        {1.00000000000000,  -4.87377313090032,  12.03922160140209, -20.10151118381395,  25.10388534415171, -24.29065560815903,  18.27158469090663, -10.45249552560593,   4.30319491872003,  -1.13716992070185,   0.14510733527035 },
        {0.98816995007392,  -1.97633990014784,   0.98816995007392 },
        {1.00000000000000,  -1.97619994516973,   0.97647985512594 },
    },

    {
        64000,
        {0.02613056568174,  -0.08128786488109,   0.14937282347325,  -0.21695711675126,   0.25010286673402,  -0.23162283619278,   0.17424041833052,  -0.10299599216680,   0.04258696481981,  -0.00977952936493,   0.00105325558889 },
        {1.00000000000000,  -5.73625477092119,  16.15249794355035, -29.68654912464508,  39.55706155674083, -39.82524556246253,  30.50605345013009, -17.43051772821245,   7.05154573908017,  -1.80783839720514,   0.22127840210813 },
        {0.98964101933472,  -1.97928203866944,   0.98964101933472 },
        {1.00000000000000,  -1.97917472731009,   0.97938935002880 },
    },

    {
        88200,
        {0.02667482047416,  -0.11377479336097,   0.23063167910965,  -0.30726477945593,   0.33188520686529,  -0.33862680249063,   0.31807161531340,  -0.23730796929880,   0.12273894790371,  -0.03840017967282,   0.00549673387936 },
        {1.00000000000000,  -6.31836451657302,  18.31351310801799, -31.88210014815921,  36.53792146976740, -28.23393036467559,  14.24725258227189,  -4.04670980012854,   0.18865757280515,   0.25420333563908,  -0.06012333531065 },
        {0.99247255046129,  -1.98494510092259,   0.99247255046129 },
        {1.00000000000000,  -1.98488843762335,   0.98500176422183 },
    },

    {
        96000,
        {0.00588138296683,  -0.01613559730421,   0.02184798954216,  -0.01742490405317,   0.00464635643780,   0.01117772513205,  -0.02123865824368,   0.01959354413350,  -0.01079720643523,   0.00352183686289,  -0.00063124341421 },
        {1.00000000000000,  -5.97808823642008,  16.21362507964068, -25.72923730652599,  25.40470663139513, -14.66166287771134,   2.81597484359752,   2.51447125969733,  -2.23575306985286,   0.75788151036791,  -0.10078025199029 },
        {0.99308203517541,  -1.98616407035082,   0.99308203517541 },
        {1.00000000000000,  -1.98611621154089,   0.98621192916075 },
    },

    {
        112000,
        {0.00528778718259,  -0.01893240907245,   0.03185982561867,  -0.02926260297838,   0.00715743034072,   0.01985743355827,  -0.03222614850941,   0.02565681978192,  -0.01210662313473,   0.00325436284541,  -0.00044173593001 },
        {1.00000000000000,  -6.24932108456288,  17.42344320538476, -27.86819709054896,  26.79087344681326, -13.43711081485123,  -0.66023612948173,   6.03658091814935,  -4.24926577030310,   1.40829268709186,  -0.19480852628112 },
        {0.99406737810867,  -1.98813475621734,   0.99406737810867 },
        {1.00000000000000,  -1.98809955990514,   0.98816995252954 },
    },

    {
        128000,
        {0.00553120584305,  -0.02112620545016,   0.03549076243117,  -0.03362498312306,   0.01425867248183,   0.01344686928787,  -0.03392770787836,   0.03464136459530,  -0.02039116051549,   0.00667420794705,  -0.00093763762995 },
        {1.00000000000000,  -6.14581710839925,  16.04785903675838, -22.19089131407749,  15.24756471580286,  -0.52001440400238,  -8.00488641699940,   6.60916094768855,  -2.37856022810923,   0.33106947986101,   0.00459820832036 },
        {0.99480702681278,  -1.98961405362557,   0.99480702681278 },
        {1.00000000000000,  -1.98958708647324,   0.98964102077790 },
    },

    {
        144000,
        {0.00639682359450,  -0.02556437970955,   0.04230854400938,  -0.03722462201267,   0.01718514827295,   0.00610592243009,  -0.03065965747365,   0.04345745003539,  -0.03298592681309,   0.01320937236809,  -0.00220304127757 },
        {1.00000000000000,  -6.14814623523425,  15.80002457141566, -20.78487587686937,  11.98848552310315,   3.36462015062606, -10.22419868359470,   6.65599702146473,  -1.67141861110485,  -0.05417956536718,   0.07374767867406 },
        {0.99538268958706,  -1.99076537917413,   0.99538268958706 },
        {1.00000000000000,  -1.99074405950505,   0.99078669884321 },
    },

    {
        176400,
        {0.00268568524529,  -0.00852379426080,   0.00852704191347,   0.00146116310295,  -0.00950855828762,   0.00625449515499,   0.00116183868722,  -0.00362461417136,   0.00203961000134,  -0.00050664587933,   0.00004327455427 },
        {1.00000000000000,  -5.57512782763045,  12.44291056065794, -12.87462799681221,   3.08554846961576,   6.62493459880692,  -7.07662766313248,   2.51175542736441,   0.06731510802735,  -0.24567753819213,   0.03961404162376 },
        {0.99622916581118,  -1.99245833162236,   0.99622916581118 },
        {1.00000000000000,  -1.99244411238133,   0.99247255086339 },
    },

    {
        192000,
        {0.01184742123123,  -0.04631092400086,   0.06584226961238,  -0.02165588522478,  -0.05656260778952,   0.08607493592760,  -0.03375544339786,  -0.04216579932754,   0.06416711490648,  -0.03444708260844,   0.00697275872241 },
        {1.00000000000000,  -5.24727318348167,  10.60821585192244,  -8.74127665810413,  -1.33906071371683,   8.07972882096606,  -5.46179918950847,   0.54318070652536,   0.87450969224280,  -0.34656083539754,   0.03034796843589 },
        {0.99653501465135,  -1.99307002930271,   0.99653501465135 },
        {1.00000000000000,  -1.99305802314321,   0.99308203546221 },
    }
};

static double *yule_coeff_a, *yule_coeff_b, *butter_coeff_a, *butter_coeff_b;
static float yule_hist_a [256], yule_hist_b [256], butter_hist_a [256], butter_hist_b [256];
static int yule_hist_i, butter_hist_i;

// Initialize filters; return FALSE is specified sampling rate is not supported

static int filter_init (uint32_t sample_rate)
{
    int i, n = sizeof (freqinfos) / sizeof (freqinfos [0]);

    for (i = 0; i < n; ++i)
        if (freqinfos [i].rate == sample_rate)
            break;

    if (i == n) {
        error_line ("sample rate of %d is not supported!", sample_rate);
        return FALSE;
    }

    yule_coeff_a = freqinfos [i].AYule;
    yule_coeff_b = freqinfos [i].BYule;
    butter_coeff_a = freqinfos [i].AButter;
    butter_coeff_b = freqinfos [i].BButter;

    memset (yule_hist_a, 0, sizeof (yule_hist_a));
    memset (yule_hist_b, 0, sizeof (yule_hist_b));
    yule_hist_i = 20;

    memset (butter_hist_a, 0, sizeof (butter_hist_a));
    memset (butter_hist_b, 0, sizeof (butter_hist_b));
    butter_hist_i = 4;
    return TRUE;
}

// Optimized mplementation of 2nd-order IIR stereo filter

static void butter_filter_stereo_samples (float *samples, uint32_t samcnt)
{
    double left, right;
    int i, j;

    i = butter_hist_i;

    // If filter history is very small magnitude, clear it completely to prevent denormals
    // from rattling around in there forever (slowing us down).

    for (j = -4; j < 0; ++j)
        if (fabs (butter_hist_a [i + j]) > 1e-10 || fabs (butter_hist_b [i + j]) > 1e-10)
            break;

    if (!j) {
        memset (butter_hist_a, 0, sizeof (butter_hist_a));
        memset (butter_hist_b, 0, sizeof (butter_hist_b));
    }

    while (samcnt--) {
        left   = (butter_hist_b [i] = samples [0]) * butter_coeff_b [0];
        right  = (butter_hist_b [i + 1] = samples [1]) * butter_coeff_b [0];
        left  += butter_hist_b [i - 2] * butter_coeff_b [1] - butter_hist_a [i - 2] * butter_coeff_a [1];
        right += butter_hist_b [i - 1] * butter_coeff_b [1] - butter_hist_a [i - 1] * butter_coeff_a [1];
        left  += butter_hist_b [i - 4] * butter_coeff_b [2] - butter_hist_a [i - 4] * butter_coeff_a [2];
        right += butter_hist_b [i - 3] * butter_coeff_b [2] - butter_hist_a [i - 3] * butter_coeff_a [2];
        samples [0] = butter_hist_a [i] = (float) left;
        samples [1] = butter_hist_a [i + 1] = (float) right;
        samples += 2;

        if ((i += 2) == 256) {
            memcpy (butter_hist_a, butter_hist_a + 252, sizeof (butter_hist_a [0]) * 4);
            memcpy (butter_hist_b, butter_hist_b + 252, sizeof (butter_hist_b [0]) * 4);
            i = 4;
        }
    }

    butter_hist_i = i;
}

// Optimized mplementation of 10th-order IIR stereo filter

static void yule_filter_stereo_samples (float *samples, uint32_t samcnt)
{
    double left, right;
    int i, j;

    i = yule_hist_i;

    // If filter history is very small magnitude, clear it completely to prevent denormals
    // from rattling around in there forever (slowing us down).

    for (j = -20; j < 0; ++j)
        if (fabs (yule_hist_a [i + j]) > 1e-10 || fabs (yule_hist_b [i + j]) > 1e-10)
            break;

    if (!j) {
        memset (yule_hist_a, 0, sizeof (yule_hist_a));
        memset (yule_hist_b, 0, sizeof (yule_hist_b));
    }

    while (samcnt--) {
        left   = (yule_hist_b [i] = samples [0]) * yule_coeff_b [0];
        right  = (yule_hist_b [i + 1] = samples [1]) * yule_coeff_b [0];
        left  += yule_hist_b [i - 2] * yule_coeff_b [1] - yule_hist_a [i - 2] * yule_coeff_a [1];
        right += yule_hist_b [i - 1] * yule_coeff_b [1] - yule_hist_a [i - 1] * yule_coeff_a [1];
        left  += yule_hist_b [i - 4] * yule_coeff_b [2] - yule_hist_a [i - 4] * yule_coeff_a [2];
        right += yule_hist_b [i - 3] * yule_coeff_b [2] - yule_hist_a [i - 3] * yule_coeff_a [2];
        left  += yule_hist_b [i - 6] * yule_coeff_b [3] - yule_hist_a [i - 6] * yule_coeff_a [3];
        right += yule_hist_b [i - 5] * yule_coeff_b [3] - yule_hist_a [i - 5] * yule_coeff_a [3];
        left  += yule_hist_b [i - 8] * yule_coeff_b [4] - yule_hist_a [i - 8] * yule_coeff_a [4];
        right += yule_hist_b [i - 7] * yule_coeff_b [4] - yule_hist_a [i - 7] * yule_coeff_a [4];
        left  += yule_hist_b [i - 10] * yule_coeff_b [5] - yule_hist_a [i - 10] * yule_coeff_a [5];
        right += yule_hist_b [i - 9] * yule_coeff_b [5] - yule_hist_a [i - 9] * yule_coeff_a [5];
        left  += yule_hist_b [i - 12] * yule_coeff_b [6] - yule_hist_a [i - 12] * yule_coeff_a [6];
        right += yule_hist_b [i - 11] * yule_coeff_b [6] - yule_hist_a [i - 11] * yule_coeff_a [6];
        left  += yule_hist_b [i - 14] * yule_coeff_b [7] - yule_hist_a [i - 14] * yule_coeff_a [7];
        right += yule_hist_b [i - 13] * yule_coeff_b [7] - yule_hist_a [i - 13] * yule_coeff_a [7];
        left  += yule_hist_b [i - 16] * yule_coeff_b [8] - yule_hist_a [i - 16] * yule_coeff_a [8];
        right += yule_hist_b [i - 15] * yule_coeff_b [8] - yule_hist_a [i - 15] * yule_coeff_a [8];
        left  += yule_hist_b [i - 18] * yule_coeff_b [9] - yule_hist_a [i - 18] * yule_coeff_a [9];
        right += yule_hist_b [i - 17] * yule_coeff_b [9] - yule_hist_a [i - 17] * yule_coeff_a [9];
        left  += yule_hist_b [i - 20] * yule_coeff_b [10] - yule_hist_a [i - 20] * yule_coeff_a [10];
        right += yule_hist_b [i - 19] * yule_coeff_b [10] - yule_hist_a [i - 19] * yule_coeff_a [10];
        samples [0] = yule_hist_a [i] = (float) left;
        samples [1] = yule_hist_a [i + 1] = (float) right;
        samples += 2;

        if ((i += 2) == 256) {
            memcpy (yule_hist_a, yule_hist_a + 236, sizeof (yule_hist_a [0]) * 20);
            memcpy (yule_hist_b, yule_hist_b + 236, sizeof (yule_hist_b [0]) * 20);
            i = 20;
        }
    }

    yule_hist_i = i;
}

// Apply both filters sequentially

static void filter_stereo_samples (float *samples, uint32_t samcnt)
{
    yule_filter_stereo_samples (samples, samcnt);
    butter_filter_stereo_samples (samples, samcnt);
}

// Update largest absolute sample value

static void calc_stereo_peak (float *samples, uint32_t samcnt, float *peak_p)
{
    float peak = 0.0;

    while (samcnt--) {
        if (samples [0] > peak)
            peak = samples [0];
        else if (-samples [0] > peak)
            peak = -samples [0];

        if (samples [1] > peak)
            peak = samples [1];
        else if (-samples [1] > peak)
            peak = -samples [1];

        samples += 2;
    }

    if (peak > *peak_p)
        *peak_p = peak;
}

// Calculate stereo rms level. Minimum value is about -100 dB for digital silence. The 90 dB
// offset is to compensate for the normalized float range and 3 dB is for stereo samples.

static double calc_stereo_rms (float *samples, uint32_t samcnt)
{
    uint32_t cnt = samcnt;
    double sum = 1e-16;

    while (cnt--) {
        sum += samples [0] * samples [0] + samples [1] * samples [1];
        samples += 2;
    }

    return 10 * log10 (sum / samcnt) + 90.0 - 3.0;
}

/////////////////////////////////////////////////////////////////////////////////
// Decimation code for properly handling DSD or PCM sample rates >= 256,000 Hz //
/////////////////////////////////////////////////////////////////////////////////

// sinc low-pass filter, cutoff = fs/12, 80 terms

static int32_t filter [] = {
    50, 464, 968, 711, -1203, -5028, -9818, -13376,
    -12870, -6021, 7526, 25238, 41688, 49778, 43050, 18447,
    -21428, -67553, -105876, -120890, -100640, -41752, 47201, 145510,
    224022, 252377, 208224, 86014, -97312, -301919, -470919, -541796,
    -461126, -199113, 239795, 813326, 1446343, 2043793, 2509064, 2763659,
    2763659, 2509064, 2043793, 1446343, 813326, 239795, -199113, -461126,
    -541796, -470919, -301919, -97312, 86014, 208224, 252377, 224022,
    145510, 47201, -41752, -100640, -120890, -105876, -67553, -21428,
    18447, 43050, 49778, 41688, 25238, 7526, -6021, -12870,
    -13376, -9818, -5028, -1203, 711, 968, 464, 50
};

#define NUM_TERMS ((int)(sizeof (filter) / sizeof (filter [0])))

typedef struct chan_state {
    int delay [NUM_TERMS], index, num_channels, ratio;
} ChanState;

static void *decimation_init (int num_channels, int ratio)
{
    ChanState *sp = malloc (sizeof (ChanState) * num_channels);
    int i;

    if (sp) {
        memset (sp, 0, sizeof (ChanState) * num_channels);

        for (i = 0; i < num_channels; ++i) {
            sp [i].num_channels = num_channels;
            sp [i].index = NUM_TERMS - ratio;
            sp [i].ratio = ratio;
        }
    }

    return sp;
}

static int decimation_run (void *context, int32_t *samples, int num_samples)
{
    int32_t *in_samples = samples, *out_samples = samples;
    ChanState *sp = (ChanState *) context;
    int num_channels, ratio, chan;

    if (!sp)
        return 0;

    num_channels = sp->num_channels;
    ratio = sp->ratio;
    chan = 0;

    while (num_samples) {
        sp = ((ChanState *) context) + chan;

        sp->delay [sp->index++] = *in_samples++;

        if (sp->index == NUM_TERMS) {
            int64_t sum = 0;
            int i;

            for (i = 0; i < NUM_TERMS; ++i)
                sum += (int64_t) filter [i] * sp->delay [i];

            *out_samples++ = (int32_t)(sum >> 24);
            memmove (sp->delay, sp->delay + ratio, sizeof (sp->delay [0]) * (NUM_TERMS - ratio));
            sp->index = NUM_TERMS - ratio;
        }

        if (++chan == num_channels) {
            num_samples--;
            chan = 0;
        }
    }

    return (int)(out_samples - samples) / num_channels;
}

static void *decimation_destroy (void *context)
{
    if (context)
        free (context);

    return NULL;
}

#ifdef _WIN32

// Convert the Unicode wide-format string into a UTF-8 string using no more
// than the specified buffer length. The wide-format string must be NULL
// terminated and the resulting string will be NULL terminated. The actual
// number of characters converted (not counting terminator) is returned, which
// may be less than the number of characters in the wide string if the buffer
// length is exceeded.

static int WideCharToUTF8 (const wchar_t *Wide, unsigned char *pUTF8, int len)
{
    const wchar_t *pWide = Wide;
    int outndx = 0;

    while (*pWide) {
        if (*pWide < 0x80 && outndx + 1 < len)
            pUTF8 [outndx++] = (unsigned char) *pWide++;
        else if (*pWide < 0x800 && outndx + 2 < len) {
            pUTF8 [outndx++] = (unsigned char) (0xc0 | ((*pWide >> 6) & 0x1f));
            pUTF8 [outndx++] = (unsigned char) (0x80 | (*pWide++ & 0x3f));
        }
        else if (outndx + 3 < len) {
            pUTF8 [outndx++] = (unsigned char) (0xe0 | ((*pWide >> 12) & 0xf));
            pUTF8 [outndx++] = (unsigned char) (0x80 | ((*pWide >> 6) & 0x3f));
            pUTF8 [outndx++] = (unsigned char) (0x80 | (*pWide++ & 0x3f));
        }
        else
            break;
    }

    pUTF8 [outndx] = 0;
    return (int)(pWide - Wide);
}

// Convert a text string into its Unicode UTF-8 format equivalent. The
// conversion is done in-place so the maximum length of the string buffer must
// be specified because the string may become longer or shorter. If the
// resulting string will not fit in the specified buffer size then it is
// truncated.

static void TextToUTF8 (void *string, int len)
{
    unsigned char *inp = string;

    // simple case: test for UTF8 BOM and if so, simply delete the BOM

    if (len > 3 && inp [0] == 0xEF && inp [1] == 0xBB && inp [2] == 0xBF) {
        memmove (inp, inp + 3, len - 3);
        inp [len - 3] = 0;
    }
    else if (* (wchar_t *) string == 0xFEFF) {
        wchar_t *temp = _wcsdup (string);

        WideCharToUTF8 (temp + 1, (unsigned char *) string, len);
        free (temp);
    }
    else {
        int max_chars = (int) strlen (string);
        wchar_t *temp = (wchar_t *) malloc ((max_chars + 1) * 2);

        MultiByteToWideChar (CP_ACP, 0, string, -1, temp, max_chars + 1);
        WideCharToUTF8 (temp, (unsigned char *) string, len);
        free (temp);
    }
}

#endif

//////////////////////////////////////////////////////////////////////////////
// This function displays the progress status on the title bar of the DOS   //
// window that WavPack is running in. The "file_progress" argument is for   //
// the current file only and ranges from 0 - 1; this function takes into    //
// account the total number of files to generate a batch progress number.   //
//////////////////////////////////////////////////////////////////////////////

void display_progress (double file_progress)
{
    char title [40];

    if (set_console_title) {
        file_progress = (file_index + file_progress) / num_files;
        sprintf (title, "%d%% (WvGain)", (int) ((file_progress * 100.0) + 0.5));
        DoSetConsoleTitle (title);
    }
}
