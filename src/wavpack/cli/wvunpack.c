////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//                Copyright (c) 1998 - 2020 David Bryant.                 //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// wvunpack.c

// This is the main module for the WavPack command-line decompressor.

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <conio.h>
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
#include <iconv.h>
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
#include <ctype.h>

#include "wavpack.h"
#include "utils.h"
#include "md5.h"

#ifdef _WIN32
#include "win32_unicode_support.h"
#define fputs fputs_utf8
#define fprintf fprintf_utf8
#define remove(f) unlink_utf8(f)
#define rename(o,n) rename_utf8(o,n)
#define fopen(f,m) fopen_utf8(f,m)
#define strdup(x) _strdup(x)
#define snprintf _snprintf
#endif

///////////////////////////// local variable storage //////////////////////////

static const char *sign_on = "\n"
" WVUNPACK  Hybrid Lossless Audio Decompressor  %s Version %s\n"
" Copyright (c) 1998 - 2020 David Bryant.  All Rights Reserved.\n\n";

static const char *version_warning = "\n"
" WARNING: WVUNPACK using libwavpack version %s, expected %s (see README)\n\n";

static const char *usage =
#if defined (_WIN32)
" Usage:   WVUNPACK [-options] infile[.wv]|- [outfile[.ext]|outpath|-]\n"
"           (default is restore original file, infile may contain wildcards: ?,*)\n\n"
#else
" Usage:   WVUNPACK [-options] infile[.wv]|- [...] [-o outfile[.ext]|outpath|-]\n"
"           (default is restore original file, multiple input files allowed)\n\n"
#endif
" Formats: Microsoft RIFF:   'wav', force with -w or --wav, makes RF64 if > 4 GB\n"
"          Sony Wave64:      'w64', force with --w64\n"
"          Apple Core Audio: 'caf', force with --caf-be or --caf-le\n"
"          Raw PCM or DSD:   'raw', force with -r or --raw, little-endian\n"
"          Philips DSDIFF:   'dff', force with --dsdiff or --dff\n"
"          Sony DSF:         'dsf', force with --dsf\n\n"
" Options: -m  = calculate and display MD5 signature; verify if lossless\n"
#ifdef _WIN32
"          --pause = pause before exiting (if console window disappears)\n"
#else
"          -o FILENAME | PATH = specify output filename or path\n"
#endif
"          -q  = quiet (keep console output to a minimum)\n"
"          -s  = display summary information only to stdout (no audio decode)\n"
"          -ss = display super summary (including tags) to stdout (no decode)\n"
"          -v  = verify source data only (no output file created)\n"
"          --help = complete help\n\n"
" Web:     Visit www.wavpack.com for latest version and info\n";

static const char *help =
#if defined (_WIN32)
" Usage:   WVUNPACK [-options] infile[.wv]|- [outfile[.ext]|outpath|-]\n\n"
"          Wildcard characters (?,*) may be included in the input filename.\n"
"          Output format and extension come from the source and by default\n"
"          the entire file is restored (including headers and trailers).\n"
"          However, this can be overridden to one of the supported formats\n"
"          listed below (which discard the original headers).\n\n"
#else
" Usage:   WVUNPACK [-options] infile[.wv]|- [...] [-o outfile[.ext]|outpath|-]\n\n"
"          Multiple input files may be specified. Output format and extension\n"
"          come from the source and by default the entire file is restored\n"
"          (including the original headers and trailers). However, this can\n"
"          be overridden to one of the supported formats listed below (which\n"
"          also causes the original headers to be discarded).\n\n"
#endif
" Formats: Microsoft RIFF:   'wav', force with -w or --wav, makes RF64 if > 4 GB\n"
"          Sony Wave64:      'w64', force with --w64\n"
"          Apple Core Audio: 'caf', force with --caf-be or --caf-le\n"
"          Raw PCM or DSD:   'raw', force with -r or --raw, little-endian\n"
"          Philips DSDIFF:   'dff', force with --dsdiff or --dff\n"
"          Sony DSF:         'dsf', force with --dsf\n\n"
" Options:\n"
"    -b                    blindly decode all stream blocks & ignore length info\n"
"    -c                    extract cuesheet only to stdout (no audio decode)\n"
"                           (note: equivalent to -x \"cuesheet\")\n"
"    -cc                   extract cuesheet file (.cue) in addition to audio file\n"
"                           (note: equivalent to -xx \"cuesheet=%a.cue\")\n"
"    --caf-be              force output to big-endian Core Audio (extension .caf)\n"
"    --caf-le              force output to little-endian Core Audio (extension .caf)\n"
"    -d                    delete source file if successful (use with caution!)\n"
"    --dff or --dsdiff     force output to Philips DSDIFF (DSD audio only,\n"
"                           extension .dff)\n"
"    --dsf                 force output to Sony DSF (DSD audio only, extension .dsf)\n"
"    -f[n]                 file info to stdout in machine-parsable format\n"
"                           (optional \"n\" = 1-10 for specific item, otherwise all)\n"
"    --help                this extended help display\n"
"    -i                    ignore .wvc file (forces hybrid lossy decompression)\n"
#if defined (_WIN32) || defined (__OS2__)
"    -l                    run at low priority (for smoother multitasking)\n"
#endif
"    -m                    calculate and display MD5 signature; verify if lossless\n"
"    -n                    no audio decoding (use with -xx to extract tags only)\n"
#ifdef _WIN32
"    --no-utf8-convert     leave tag items in UTF-8 when extracting to files\n"
"    --pause               pause before exiting (if console window disappears)\n"
#else
"    --no-utf8-convert     leave tag items in UTF-8 on extract or display\n"
"    -o FILENAME | PATH    specify output filename or path\n"
#endif
"    -q                    quiet (keep console output to a minimum)\n"
"    -r or --raw           force raw audio decode (results in .raw extension)\n"
"    -s                    display summary info only to stdout (no audio decode)\n"
"    -ss                   display super summary (with tags) to stdout (no decode)\n"
"    --skip=[-][sample|hh:mm:ss.ss]\n"
"                          start decoding at specified sample/time\n"
"                           (specifying a '-' makes sample/time relative to end)\n"
"    -t                    copy input file's time stamp to output file(s)\n"
"    --until=[+|-][sample|hh:mm:ss.ss]\n"
"                          stop decoding at specified sample/time\n"
"                           (adding '+' makes sample/time relative to '--skip'\n"
"                            point; adding '-' makes sample/time relative to end)\n"
"    -v                    verify source data only (no output file created)\n"
"    --version             write the version to stdout\n"
"    -w or --wav           force output to Microsoft RIFF/RF64 (extension .wav)\n"
"    --w64                 force output to Sony Wave64 format (extension .w64)\n"
"    -x \"Field\"            extract specified tag field to stdout (no audio decode)\n"
"    -xx \"Field[=file]\"    extract specified tag field to file, optional filename\n"
"                           specification can include following replacement codes:\n"
"                            %a = audio output filename\n"
"                            %t = tag field name (comes from data for binary tags)\n"
"                            %e = extension from binary tag source file, else 'txt'\n"
"    -y                    yes to overwrite warning (use with caution!)\n"
#if defined (_WIN32)
"    -z                    don't set console title to indicate progress\n\n"
#else
"    -z1                   set console title to indicate progress\n\n"
#endif
" Web:     Visit www.wavpack.com for latest version and info\n";

int WriteCaffHeader (FILE *outfile, WavpackContext *wpc, int64_t total_samples, int qmode);
int WriteWave64Header (FILE *outfile, WavpackContext *wpc, int64_t total_samples, int qmode);
int WriteRiffHeader (FILE *outfile, WavpackContext *wpc, int64_t total_samples, int qmode);
int WriteDsdiffHeader (FILE *outfile, WavpackContext *wpc, int64_t total_samples, int qmode);
int WriteDsfHeader (FILE *outfile, WavpackContext *wpc, int64_t total_samples, int qmode);

static struct {
    char *default_extension, *format_name;
    int (* WriteHeader) (FILE *outfile, WavpackContext *wpc, int64_t total_samples, int qmode);
    int chunk_alignment;
} file_formats [] = {
    { "wav", "Microsoft RIFF",   WriteRiffHeader,   2 },
    { "w64", "Sony Wave64",      WriteWave64Header, 8 },
    { "caf", "Apple Core Audio", WriteCaffHeader,   1 },
    { "dff", "Philips DSDIFF",   WriteDsdiffHeader, 2 },
    { "dsf", "Sony DSF",         WriteDsfHeader,    1 }
};

#define NUM_FILE_FORMATS (sizeof (file_formats) / sizeof (file_formats [0]))

// this global is used to indicate the special "debug" mode where extra debug messages
// are displayed and all messages are logged to the file \wavpack.log

int debug_logging_mode;

static int overwrite_all, delete_source, raw_decode, no_utf8_convert, no_audio_decode, file_info,
    summary, ignore_wvc, quiet_mode, calc_md5, copy_time, blind_decode, decode_format, format_specified, caf_be, set_console_title;

static int num_files, file_index, outbuf_k;

static struct sample_time_index {
    int value_is_time, value_is_relative, value_is_valid;
    double value;
} skip, until;

static char *tag_extract_stdout;    // extract single tag to stdout
static char **tag_extractions;      // extract multiple tags to named files
static int num_tag_extractions;

#ifdef _WIN32
static int pause_mode;
#endif

/////////////////////////// local function declarations ///////////////////////

static void add_tag_extraction_to_list (char *spec);
static void parse_sample_time_index (struct sample_time_index *dst, char *src);
static int unpack_file (char *infilename, char *outfilename, int add_extension);
static void display_progress (double file_progress);

#ifdef _WIN32
static void TextToUTF8 (void *string, int len);
#endif

#define WAVPACK_NO_ERROR    0
#define WAVPACK_SOFT_ERROR  1
#define WAVPACK_HARD_ERROR  2

// The "main" function for the command-line WavPack decompressor. Note that on Windows
// this is actually a static function that is called from the "real" main() defined
// immediately afterward that converts the wchar argument list into UTF-8 strings
// and sets the console to UTF-8 for better Unicode support.

#ifdef _WIN32
static int wvunpack_main(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
#ifdef __EMX__ /* OS/2 */
    _wildcard (&argc, &argv);
#endif
    int verify_only = 0, error_count = 0, add_extension = 0, output_spec = 0, c_count = 0, x_count = 0;
    char outpath, **matches = NULL, *outfilename = NULL;
    int result;

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
        if (**++argv == '-' && (*argv)[1] == '-' && (*argv)[2]) {
            char *long_option = *argv + 2, *long_param = long_option;

            while (*long_param)
                if (*long_param++ == '=')
                    break;

            if (!strcmp (long_option, "help")) {                        // --help
                printf ("%s", help);
                return 0;
            }
            else if (!strcmp (long_option, "version")) {                // --version
                printf ("wvunpack %s\n", PACKAGE_VERSION);
                printf ("libwavpack %s\n", WavpackGetLibraryVersionString ());
                return 0;
            }
#ifdef _WIN32
            else if (!strcmp (long_option, "pause"))                    // --pause
                pause_mode = 1;
#endif
            else if (!strcmp (long_option, "no-utf8-convert"))          // --no-utf8-convert
                no_utf8_convert = 1;
            else if (!strncmp (long_option, "skip", 4)) {               // --skip
                parse_sample_time_index (&skip, long_param);

                if (!skip.value_is_valid) {
                    error_line ("invalid --skip parameter!");
                    ++error_count;
                }
            }
            else if (!strncmp (long_option, "until", 5)) {              // --until
                parse_sample_time_index (&until, long_param);

                if (!until.value_is_valid) {
                    error_line ("invalid --until parameter!");
                    ++error_count;
                }
            }
            else if (!strcmp (long_option, "caf-be")) {                 // --caf-be
                decode_format = WP_FORMAT_CAF;
                caf_be = format_specified = 1;
            }
            else if (!strcmp (long_option, "caf-le")) {                 // --caf-le
                decode_format = WP_FORMAT_CAF;
                format_specified = 1;
            }
            else if (!strcmp (long_option, "dsf")) {                    // --dsf
                decode_format = WP_FORMAT_DSF;
                format_specified = 1;
            }
            else if (!strcmp (long_option, "dsdiff") || !strcmp (long_option, "dff")) {
                decode_format = WP_FORMAT_DFF;                          // --dsdiff or --dff
                format_specified = 1;
            }
            else if (!strcmp (long_option, "w64")) {                    // --w64
                decode_format = WP_FORMAT_W64;
                format_specified = 1;
            }
            else if (!strcmp (long_option, "wav")) {                    // --wav
                decode_format = WP_FORMAT_WAV;
                format_specified = 1;
            }
            else if (!strcmp (long_option, "raw"))                      // --raw
                raw_decode = 1;
            else {
                error_line ("unknown option: %s !", long_option);
                ++error_count;
            }
        }
#if defined (_WIN32)
        else if ((**argv == '-' || **argv == '/') && (*argv)[1])
#else
        else if ((**argv == '-') && (*argv)[1])
#endif
            while (*++*argv)
                switch (**argv) {
                    case 'Y': case 'y':
                        overwrite_all = 1;
                        break;

                    case 'C': case 'c':
                        if (++c_count == 2) {
                            add_tag_extraction_to_list ("cuesheet=%a.cue");
                            c_count = 0;
                        }

                        break;

                    case 'D': case 'd':
                        delete_source = 1;
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
#if defined (_WIN32)
                    case 'O': case 'o':  // ignore -o in Windows to be Linux compatible
                        break;
#else
                    case 'O': case 'o':
                        output_spec = 1;
                        break;
#endif
                    case 'T': case 't':
                        copy_time = 1;
                        break;

                    case 'V': case 'v':
                        verify_only = 1;
                        break;

                    case 'F': case 'f':
                        file_info = (char) strtol (++*argv, argv, 10);

                        if (file_info < 0 || file_info > 10) {
                            error_line ("-f option must be 1-10, or omit (or 0) for all!");
                            ++error_count;
                        }
                        else {
                            quiet_mode = no_audio_decode = 1;
                            file_info++;
                        }

                        --*argv;
                        break;

                    case 'S': case 's':
                        no_audio_decode = 1;
                        ++summary;
                        break;

                    case 'K': case 'k':
                        outbuf_k = strtol (++*argv, argv, 10);

                        if (outbuf_k < 1 || outbuf_k > 16384)       // range-check for reasonable values
                            outbuf_k = 0;

                        --*argv;
                        break;

                    case 'M': case 'm':
                        calc_md5 = 1;
                        break;

                    case 'B': case 'b':
                        blind_decode = 1;
                        break;

                    case 'N': case 'n':
                        no_audio_decode = 1;
                        break;

                    case 'R': case 'r':
                        raw_decode = 1;
                        break;

                    case 'W': case 'w':
                        decode_format = WP_FORMAT_WAV;
                        format_specified = 1;
                        break;

                    case 'Q': case 'q':
                        quiet_mode = 1;
                        break;

                    case 'Z': case 'z':
                        set_console_title = (char) strtol (++*argv, argv, 10);
                        --*argv;
                        break;

                    case 'X': case 'x':
                        if (++x_count == 3) {
                            error_line ("illegal option: %s !", *argv);
                            ++error_count;
                            x_count = 0;
                        }

                        break;

                    case 'I': case 'i':
                        ignore_wvc = 1;
                        break;

                    default:
                        error_line ("illegal option: %c !", **argv);
                        ++error_count;
                }
        else {
            if (x_count) {
                if (x_count == 1) {
                    if (tag_extract_stdout) {
                        error_line ("can't extract more than 1 tag item to stdout at a time!");
                        ++error_count;
                    }
                    else {
                        tag_extract_stdout = *argv;
                        no_audio_decode = 1;
                    }
                }
                else if (x_count == 2)
                    add_tag_extraction_to_list (*argv);

                x_count = 0;
            }
#if defined (_WIN32)
            else if (!num_files) {
                matches = realloc (matches, (num_files + 1) * sizeof (*matches));
                matches [num_files] = malloc (strlen (*argv) + 10);
                strcpy (matches [num_files], *argv);

                if (*(matches [num_files]) != '-' && *(matches [num_files]) != '@' &&
                    !filespec_ext (matches [num_files]))
                        strcat (matches [num_files], ".wv");

                num_files++;
            }
            else if (!outfilename) {
                outfilename = malloc (strlen (*argv) + PATH_MAX);
                strcpy (outfilename, *argv);
            }
            else {
                error_line ("extra unknown argument: %s !", *argv);
                ++error_count;
            }
#else
            else if (output_spec) {
                outfilename = malloc (strlen (*argv) + PATH_MAX);
                strcpy (outfilename, *argv);
                output_spec = 0;
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
#endif
        }
    }

   // check for various command-line argument problems

    if (output_spec) {
        error_line ("no output filename or path specified with -o option!");
        ++error_count;
    }

    if (delete_source && (verify_only || skip.value_is_valid || until.value_is_valid)) {
        error_line ("can't delete in verify mode or when --skip or --until are used!");
        delete_source = 0;
    }

    if (raw_decode && format_specified) {
        error_line ("-r (raw decode) and -w (wav header) modes are incompatible!");
        ++error_count;
    }

    if (verify_only && outfilename) {
        error_line ("outfile specification and verify mode are incompatible!");
        ++error_count;
    }

    if (c_count == 1) {
        if (tag_extract_stdout) {
            error_line ("can't extract more than 1 tag item to stdout at a time!");
            error_count++;
        }
        else {
            tag_extract_stdout = "cuesheet";
            no_audio_decode = 1;
        }
    }

    if ((summary || tag_extract_stdout) && (num_tag_extractions || outfilename || verify_only || delete_source || format_specified || raw_decode)) {
        error_line ("can't display summary information or extract a tag to stdout and do anything else!");
        ++error_count;
    }

    if ((tag_extract_stdout || num_tag_extractions) && outfilename && *outfilename == '-') {
        error_line ("can't extract tags when unpacking audio to stdout!");
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

    // If the outfile specification begins with a '@', then it actually points
    // to a file that contains the output specification. This was included for
    // use by Wim Speekenbrink's frontends because certain filenames could not
    // be passed on the command-line, but could be used for other purposes.

    if (outfilename && outfilename [0] == '@') {
        char listbuff [PATH_MAX * 2], *lp = listbuff;
        FILE *list = fopen (outfilename+1, "rb");
        int c;

        if (list == NULL) {
            error_line ("file %s not found!", outfilename+1);
            free(outfilename);
            return 1;
        }

        memset (listbuff, 0, sizeof (listbuff));
        c = (int) fread (listbuff, 1, sizeof (listbuff) - 1, list);   // assign c only to suppress warning

#if defined (_WIN32)
        TextToUTF8 (listbuff, PATH_MAX * 2);
#endif

        while ((c = *lp++) == '\n' || c == '\r');

        if (c) {
            int ci = 0;

            do
                outfilename [ci++] = c;
            while ((c = *lp++) != '\n' && c != '\r' && c && ci < PATH_MAX);

            outfilename [ci] = '\0';
        }
        else {
            error_line ("output spec file is empty!");
            free(outfilename);
            fclose (list);
            return 1;
        }

        fclose (list);
    }

    // if we found any files to process, this is where we start

    if (num_files) {
        if (outfilename && *outfilename != '-') {
            outpath = (filespec_path (outfilename) != NULL);

            if (num_files > 1 && !outpath) {
                error_line ("%s is not a valid output path", outfilename);
                free (outfilename);
                return 1;
            }
        }
        else
            outpath = 0;

        add_extension = !outfilename || outpath || !filespec_ext (outfilename);

        // loop through and process files in list

        for (file_index = 0; file_index < num_files; ++file_index) {
            if (check_break ())
                break;

            // generate output filename

            if (outpath) {
                strcat (outfilename, filespec_name (matches [file_index]));

                if (filespec_ext (outfilename))
                    *filespec_ext (outfilename) = '\0';
            }
            else if (!outfilename) {
                outfilename = malloc (strlen (matches [file_index]) + 10);
                strcpy (outfilename, matches [file_index]);

                if (filespec_ext (outfilename))
                    *filespec_ext (outfilename) = '\0';
            }

            if (num_files > 1 && !quiet_mode) {
                fprintf (stderr, "\n%s:\n", matches [file_index]);
                fflush (stderr);
            }

            result = unpack_file (matches [file_index], verify_only ? NULL : outfilename, add_extension);

            if (result != WAVPACK_NO_ERROR)
                ++error_count;

            if (result == WAVPACK_HARD_ERROR)
                break;

            // clean up in preparation for potentially another file

            if (outpath)
                *filespec_name (outfilename) = '\0';
            else if (*outfilename != '-') {
                free (outfilename);
                outfilename = NULL;
            }

            free (matches [file_index]);
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

        free (matches);
    }
    else {
        error_line ("nothing to do!");
        ++error_count;
    }

    if (outfilename)
        free (outfilename);

    if (set_console_title)
        DoSetConsoleTitle ("WvUnpack Completed");

    return error_count ? 1 : 0;
}

#ifdef _WIN32

// On Windows, this "real" main() acts as a shell to our static wvunpack_main().
// Its purpose is to convert the wchar command-line arguments into UTF-8 encoded
// strings.

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

    ret = wvunpack_main(argc_utf8, argv_copy);

    if (argv_copy)
        free (argv_copy);

    free_commandline_arguments_utf8(&argc_utf8, &argv_utf8);

    if (pause_mode) {
        fprintf (stderr, "\nPress any key to continue . . . ");
        fflush (stderr);
        while (!_kbhit ());
        _getch ();
        fprintf (stderr, "\n");
    }

    return ret;
}

#endif

// Parse the parameter of the --skip and --until commands, which are of the form:
//   [+|-] [samples | hh:mm:ss.ss]
// The value is returned in a double (in the "dst" struct) as either samples or
// seconds (if a time is specified). If sample, the value must be an integer.

static void parse_sample_time_index (struct sample_time_index *dst, char *src)
{
    int colons = 0;
    double temp;

    memset (dst, 0, sizeof (*dst));

    if (*src == '+' || *src == '-')
        dst->value_is_relative = (*src++ == '+') ? 1 : -1;

    while (*src)
        if (*src == ':') {
            if (++colons == 3)
                return;

            src++;
            dst->value_is_time = 1;
            dst->value *= 60.0;
            continue;
        }
        else if (*src == '.' || isdigit (*src)) {
            temp = strtod (src, &src);

            if (temp < 0.0 || (dst->value_is_time && temp >= 60.0) ||
                (!dst->value_is_time && temp != floor (temp)))
                    return;

            dst->value += temp;
        }
        else
            return;

    dst->value_is_valid = 1;
}


// Open specified file for writing, with overwrite check. If the specified file already exists (and the user has
// agreed to overwrite) then open a temp file instead and store a pointer to that filename at "tempfilename" (otherwise
// the pointer is set to NULL). The caller will be required to perform the rename (and free the pointer) once the file
// is completely written and closed. Note that for a file to be considered "overwritable", it must both be openable for
// reading and have at least 1 readable byte - this prevents us getting stuck on "nul" (Windows).

static FILE *open_output_file (char *filename, char **tempfilename)
{
    FILE *retval, *testfile;
    char dummy;

    *tempfilename = NULL;

    if (*filename == '-') {
#if defined(_WIN32)
        _setmode (_fileno (stdout), O_BINARY);
#endif
#if defined(__OS2__)
        setmode (fileno (stdout), O_BINARY);
#endif
        return stdout;
    }

    testfile = fopen (filename, "rb");

    if (testfile) {
        size_t res = fread (&dummy, 1, 1, testfile);

        fclose (testfile);

        if (res == 1) {
            int count = 0;

            if (!overwrite_all) {
                fprintf (stderr, "overwrite %s (yes/no/all)? ", FN_FIT (filename));
                fflush (stderr);

                if (set_console_title)
                    DoSetConsoleTitle ("overwrite?");

                switch (yna ()) {
                    case 'n':
                        return NULL;

                    case 'a':
                        overwrite_all = 1;
                }
            }

            *tempfilename = malloc (strlen (filename) + 16);

            while (1) {
                strcpy (*tempfilename, filename);

                if (filespec_ext (*tempfilename)) {
                    if (count++)
                        sprintf (filespec_ext (*tempfilename), ".tmp%d", count-1);
                    else
                        strcpy (filespec_ext (*tempfilename), ".tmp");

                    strcat (*tempfilename, filespec_ext (filename));
                }
                else {
                    if (count++)
                        sprintf (*tempfilename + strlen (*tempfilename), ".tmp%d", count-1);
                    else
                        strcat (*tempfilename, ".tmp");
                }

                testfile = fopen (*tempfilename, "rb");

                if (!testfile)
                    break;

                res = fread (&dummy, 1, 1, testfile);
                fclose (testfile);

                if (res != 1)
                    break;
            }
        }
    }

    retval = fopen (*tempfilename ? *tempfilename : filename, "w+b");

    if (retval == NULL)
        error_line ("can't create file %s!", *tempfilename ? *tempfilename : filename);

    return retval;
}

// Unpack the specified WavPack input file into the specified output file name.
// This function uses the library routines provided in wputils.c to do all
// unpacking. This function takes care of reformatting the data (which is
// returned in native-endian longs) to the standard little-endian format. This
// function also handles optionally calculating and displaying the MD5 sum of
// the resulting audio data and verifying the sum if a sum was stored in the
// source and lossless compression is used.

static int unpack_audio (WavpackContext *wpc, FILE *outfile, int qmode, unsigned char *md5_digest, int64_t *sample_count);
static int unpack_dsd_audio (WavpackContext *wpc, FILE *outfile, int qmode, unsigned char *md5_digest, int64_t *sample_count);
static int do_tag_extractions (WavpackContext *wpc, char *outfilename);
static void *store_samples (void *dst, int32_t *src, int qmode, int bps, int count);
static void dump_summary (WavpackContext *wpc, char *name, FILE *dst);
static int dump_tag_item_to_file (WavpackContext *wpc, const char *tag_item, FILE *dst, char *fn);
static void dump_file_info (WavpackContext *wpc, char *name, FILE *dst, int parameter);
static void unreorder_channels (int32_t *data, unsigned char *order, int num_chans, int num_samples);

static int unpack_file (char *infilename, char *outfilename, int add_extension)
{
    int64_t skip_sample_index = 0, until_samples_total = 0, total_unpacked_samples = 0;
    int result = WAVPACK_NO_ERROR, md5_diff = FALSE, created_riff_header = FALSE;
    int input_qmode, output_qmode = 0, input_format, output_format = 0;
    int open_flags = 0, padding_bytes = 0, num_channels, wvc_mode;
    unsigned char md5_unpacked [16];
    char *outfilename_temp = NULL;
    char *extension = NULL;
    WavpackContext *wpc;
    uint32_t bcount;
    char error [80];
    FILE *outfile;
    double dtime;

#if defined(_WIN32)
    struct __timeb64 time1, time2;
#else
    struct timeval time1, time2;
    struct timezone timez;
#endif

    // use library to open WavPack file

#ifdef _WIN32
    open_flags |= OPEN_FILE_UTF8;
#endif

    if ((outfilename && !raw_decode && !blind_decode && !format_specified &&
        !skip.value_is_valid && !until.value_is_valid) || summary > 1)
            open_flags |= OPEN_WRAPPER;

    if (blind_decode)
        open_flags |= OPEN_STREAMING | OPEN_NO_CHECKSUM;

    if (!ignore_wvc)
        open_flags |= OPEN_WVC;

    if (summary > 1 || num_tag_extractions || tag_extract_stdout)
        open_flags |= OPEN_TAGS;

    if (format_specified && decode_format != WP_FORMAT_DFF && decode_format != WP_FORMAT_DSF)
        open_flags |= OPEN_DSD_AS_PCM | OPEN_ALT_TYPES;
    else
        open_flags |= OPEN_DSD_NATIVE | OPEN_ALT_TYPES;

    wpc = WavpackOpenFileInput (infilename, error, open_flags, 0);

    if (!wpc) {
        error_line (error);
        return WAVPACK_SOFT_ERROR;
    }

    if (add_extension) {
        if (raw_decode)
            extension = "raw";
        else if (format_specified)
            extension = file_formats [decode_format].default_extension;
        else
            extension = WavpackGetFileExtension (wpc);
    }

    wvc_mode = WavpackGetMode (wpc) & MODE_WVC;
    num_channels = WavpackGetNumChannels (wpc);
    input_qmode = WavpackGetQualifyMode (wpc);
    input_format = WavpackGetFileFormat (wpc);

    // Based on what output format the user specified on the command-line (if any) and what we can
    // tell about the file, decide on how we are going to format the output. Note that the last
    // couple of cases refer to the situation where the file is a format we don't understand,
    // which can't really happen now, but could happen some time in the future when either a new
    // format is added or when someone creates a "custom" format and that file meets a standard
    // decoder.

    if (raw_decode) {                                   // case 1: user specified raw decode
        output_qmode = (input_qmode & QMODE_DSD_AUDIO) ? QMODE_DSD_MSB_FIRST : 0;
    }
    else if (format_specified) {                        // case 2: user specified an output format
        switch (decode_format) {
            case WP_FORMAT_CAF:
                output_qmode = QMODE_SIGNED_BYTES | (caf_be ? QMODE_BIG_ENDIAN : 0) | (input_qmode & QMODE_REORDERED_CHANS);
                output_format = decode_format;
                break;

            case WP_FORMAT_WAV:
            case WP_FORMAT_W64:
                output_format = decode_format;
                output_qmode = 0;
                break;

            case WP_FORMAT_DFF:
            case WP_FORMAT_DSF:
                if (!(input_qmode & QMODE_DSD_AUDIO)) {
                    error_line ("can't export PCM source to DSD file!");
                    WavpackCloseFile (wpc);
                    return WAVPACK_SOFT_ERROR;
                }

                if (decode_format == WP_FORMAT_DSF)
                    output_qmode = QMODE_DSD_LSB_FIRST | QMODE_DSD_IN_BLOCKS;
                else
                    output_qmode = QMODE_DSD_MSB_FIRST;

                output_format = decode_format;
                break;
        }
    }
    else if (input_format < NUM_FILE_FORMATS) {         // case 3: user specified nothing, and this is a format we know about
        output_format = input_format;                   //   (obviously this is the most common situation)
        output_qmode = input_qmode;
    }
    else if (!WavpackGetWrapperBytes (wpc) ||           // case 4: unknown format and no wrapper present or extracting section
        skip.value_is_valid || until.value_is_valid) {  //   so we must override to a known format & extension

        if (input_qmode & QMODE_DSD_AUDIO) {
            output_format = WP_FORMAT_DFF;
            output_qmode = QMODE_DSD_MSB_FIRST;
        }
        else {
            output_format = WP_FORMAT_WAV;
            output_qmode = 0;
        }

        extension = file_formats [output_format].default_extension;
    }
    else                                                // case 5: unknown format, but wrapper is present and we're doing
        output_qmode = input_qmode;                     //   the whole file, so we don't have to understand the format

    if (skip.value_is_valid) {
        if (skip.value_is_time)
            skip_sample_index = (int64_t) (skip.value * WavpackGetSampleRate (wpc));
        else
            skip_sample_index = (int64_t) skip.value;

        if (skip.value_is_relative == -1) {
            if (WavpackGetNumSamples64 (wpc) == -1) {
                error_line ("can't use negative relative --skip command with files of unknown length!");
                WavpackCloseFile (wpc);
                return WAVPACK_SOFT_ERROR;
            }

            if (skip_sample_index < WavpackGetNumSamples64 (wpc))
                skip_sample_index = WavpackGetNumSamples64 (wpc) - skip_sample_index;
            else
                skip_sample_index = 0;
        }

        if (skip_sample_index && !WavpackSeekSample64 (wpc, skip_sample_index)) {
            error_line ("can't seek to specified --skip point!");
            WavpackCloseFile (wpc);
            return WAVPACK_SOFT_ERROR;
        }

        if (WavpackGetNumSamples64 (wpc) != -1)
            until_samples_total = WavpackGetNumSamples64 (wpc) - skip_sample_index;
    }

    if (until.value_is_valid) {
        double until_sample_index = until.value_is_time ? until.value * WavpackGetSampleRate (wpc) : until.value;

        if (until.value_is_relative == -1) {
            if (WavpackGetNumSamples64 (wpc) == -1) {
                error_line ("can't use negative relative --until command with files of unknown length!");
                WavpackCloseFile (wpc);
                return WAVPACK_SOFT_ERROR;
            }

            if (until_sample_index + skip_sample_index < WavpackGetNumSamples64 (wpc))
                until_samples_total = (int64_t) (WavpackGetNumSamples64 (wpc) - until_sample_index - skip_sample_index);
            else
                until_samples_total = 0;
        }
        else {
            if (until.value_is_relative == 1)
                until_samples_total = (int64_t) until_sample_index;
            else if (until_sample_index > skip_sample_index)
                until_samples_total = (int64_t) (until_sample_index - skip_sample_index);
            else
                until_samples_total = 0;

            if (WavpackGetNumSamples64 (wpc) != -1 &&
                skip_sample_index + until_samples_total > WavpackGetNumSamples64 (wpc))
                    until_samples_total = WavpackGetNumSamples64 (wpc) - skip_sample_index;
        }

        if (!until_samples_total) {
            error_line ("--until command results in no samples to decode!");
            WavpackCloseFile (wpc);
            return WAVPACK_SOFT_ERROR;
        }
    }

    if (file_info)
        dump_file_info (wpc, infilename, stdout, file_info - 1);
    else if (summary)
        dump_summary (wpc, infilename, stdout);
    else if (tag_extract_stdout) {
        if (!dump_tag_item_to_file (wpc, tag_extract_stdout, stdout, NULL)) {
            error_line ("tag \"%s\" not found!", tag_extract_stdout);
            WavpackCloseFile (wpc);
            return WAVPACK_SOFT_ERROR;
        }
    }
    else if (num_tag_extractions && outfilename && *outfilename != '-' && filespec_name (outfilename)) {
        result = do_tag_extractions (wpc, outfilename);

        if (result != WAVPACK_NO_ERROR) {
            WavpackCloseFile (wpc);
            return result;
        }
    }

    if (no_audio_decode) {
        WavpackCloseFile (wpc);
        return WAVPACK_NO_ERROR;
    }

    if (outfilename) {
        if (*outfilename != '-' && add_extension) {
            strcat (outfilename, ".");
            strcat (outfilename, extension);
        }

        if ((outfile = open_output_file (outfilename, &outfilename_temp)) == NULL) {
            WavpackCloseFile (wpc);
            return WAVPACK_SOFT_ERROR;
        }
        else if (*outfilename == '-') {
            if (!quiet_mode) {
                fprintf (stderr, "unpacking %s%s to stdout,", *infilename == '-' ?
                    "stdin" : FN_FIT (infilename), wvc_mode ? " (+.wvc)" : "");
                fflush (stderr);
            }
        }
        else if (!quiet_mode) {
            fprintf (stderr, "restoring %s,", FN_FIT (outfilename));
            fflush (stderr);
        }
    }
    else {      // in verify only mode we don't worry about headers
        outfile = NULL;

        if (!quiet_mode) {
            fprintf (stderr, "verifying %s%s,", *infilename == '-' ? "stdin" :
                FN_FIT (infilename), wvc_mode ? " (+.wvc)" : "");
            fflush (stderr);
        }
    }

#if defined(_WIN32)
    _ftime64 (&time1);
#else
    gettimeofday(&time1,&timez);
#endif

    if (outfile && !raw_decode) {
        if (until_samples_total) {
            if (!file_formats [output_format].WriteHeader (outfile, wpc, until_samples_total, output_qmode)) {
                DoTruncateFile (outfile);
                result = WAVPACK_HARD_ERROR;
            }
            else
                created_riff_header = TRUE;
        }
        else if (WavpackGetWrapperBytes (wpc)) {
            if (!DoWriteFile (outfile, WavpackGetWrapperData (wpc), WavpackGetWrapperBytes (wpc), &bcount) ||
                bcount != WavpackGetWrapperBytes (wpc)) {
                    error_line ("can't write .WAV data, disk probably full!");
                    DoTruncateFile (outfile);
                    result = WAVPACK_HARD_ERROR;
            }

            WavpackFreeWrapper (wpc);
        }
        else if (!file_formats [output_format].WriteHeader (outfile, wpc, WavpackGetNumSamples64 (wpc), output_qmode)) {
            DoTruncateFile (outfile);
            result = WAVPACK_HARD_ERROR;
        }
        else
            created_riff_header = TRUE;
    }

    total_unpacked_samples = until_samples_total;

    if (output_qmode & QMODE_DSD_AUDIO)
        result = unpack_dsd_audio (wpc, outfile, output_qmode, calc_md5 ? md5_unpacked : NULL, &total_unpacked_samples);
    else
        result = unpack_audio (wpc, outfile, output_qmode, calc_md5 ? md5_unpacked : NULL, &total_unpacked_samples);

    // if the file format has chunk alignment requirements, and our data chunk does not align, write padding bytes here

    if (result == WAVPACK_NO_ERROR && outfile && !raw_decode && file_formats [output_format].chunk_alignment != 1) {
        int64_t data_chunk_bytes = total_unpacked_samples * num_channels * WavpackGetBytesPerSample (wpc);
        int alignment = file_formats [output_format].chunk_alignment;
        int bytes_over = (int)(data_chunk_bytes % alignment);
        int pcount = bytes_over ? alignment - bytes_over : 0;

        padding_bytes = pcount;

        while (pcount--)
            if (!DoWriteFile (outfile, "", 1, &bcount) || bcount != 1) {
                error_line ("can't write .WAV data, disk probably full!");
                DoTruncateFile (outfile);
                result = WAVPACK_HARD_ERROR;
            }
    }

    if (!check_break () && calc_md5) {
        char md5_string1 [] = "00000000000000000000000000000000";
        char md5_string2 [] = "00000000000000000000000000000000";
        unsigned char md5_original [16];
        int i;

        if (WavpackGetMD5Sum (wpc, md5_original)) {

            for (i = 0; i < 16; ++i)
                sprintf (md5_string1 + (i * 2), "%02x", md5_original [i]);

            error_line ("original md5:  %s", md5_string1);

            if (memcmp (md5_unpacked, md5_original, 16))
                md5_diff = TRUE;
        }

        for (i = 0; i < 16; ++i)
            sprintf (md5_string2 + (i * 2), "%02x", md5_unpacked [i]);

        error_line ("unpacked md5:  %s", md5_string2);
    }

    // this is where we append any trailing wrapper, assuming that we did not create the header
    // and that there is actually one stored that came from the original file

    if (outfile && result == WAVPACK_NO_ERROR && !created_riff_header && WavpackGetWrapperBytes (wpc)) {
        unsigned char *wrapper_data = WavpackGetWrapperData (wpc);
        int wrapper_bytes = WavpackGetWrapperBytes (wpc);

        // This is an odd case. Older versions of WavPack would store any data chunk padding bytes as
        // wrapper, but now we're generating them above based on the chunk size. To correctly handle
        // the former case, we'll eat an appropriate number of NULL wrapper bytes here.

        while (padding_bytes-- && wrapper_bytes && !*wrapper_data) {
            wrapper_bytes--;
            wrapper_data++;
        }

        if (!DoWriteFile (outfile, wrapper_data, wrapper_bytes, &bcount) || bcount != wrapper_bytes) {
            error_line ("can't write .WAV data, disk probably full!");
            DoTruncateFile (outfile);
            result = WAVPACK_HARD_ERROR;
        }

        WavpackFreeWrapper (wpc);
    }

    if (result == WAVPACK_NO_ERROR && outfile && created_riff_header &&
        (WavpackGetNumSamples64 (wpc) == -1 ||
         (until_samples_total ? until_samples_total : WavpackGetNumSamples64 (wpc)) != total_unpacked_samples)) {
            if (*outfilename == '-' || DoSetFilePositionAbsolute (outfile, 0))
                error_line ("can't update file header with actual size");
            else if (!file_formats [output_format].WriteHeader (outfile, wpc, total_unpacked_samples, output_qmode)) {
                DoTruncateFile (outfile);
                result = WAVPACK_HARD_ERROR;
            }
    }

    // if we are not just in verify only mode, flush the output stream and if it's a real file (not stdout)
    // close it and make sure it's not zero length (which means we got an error somewhere)

    if (outfile) {
        fflush (outfile);

        if (*outfilename != '-') {
            int64_t outfile_length = DoGetFileSize (outfile);

            if (!DoCloseHandle (outfile)) {
                error_line ("can't close file %s!", FN_FIT (outfilename));
                result = WAVPACK_SOFT_ERROR;
            }

            if (!outfile_length)
                DoDeleteFile (outfilename_temp ? outfilename_temp : outfilename);
        }
    }

    // if we were writing to a temp file because the target file already existed,
    // do the rename / overwrite now (and if that fails, flag the error)

#if defined(_WIN32)
    if (result == WAVPACK_NO_ERROR && outfilename && outfilename_temp) {
        if (remove (outfilename)) {
            error_line ("can not remove file %s, result saved in %s!", outfilename, outfilename_temp);
            result = WAVPACK_SOFT_ERROR;
        }
        else if (rename (outfilename_temp, outfilename)) {
            error_line ("can not rename temp file %s to %s!", outfilename_temp, outfilename);
            result = WAVPACK_SOFT_ERROR;
        }
    }
#else
    if (result == WAVPACK_NO_ERROR && outfilename && outfilename_temp && rename (outfilename_temp, outfilename)) {
        error_line ("can not rename temp file %s to %s!", outfilename_temp, outfilename);
        result = WAVPACK_SOFT_ERROR;
    }
#endif

    if (outfilename && outfilename_temp) free (outfilename_temp);

    if (result == WAVPACK_NO_ERROR && copy_time && outfilename &&
        !copy_timestamp (infilename, outfilename))
            error_line ("failure copying time stamp!");

    if (result == WAVPACK_NO_ERROR) {
        if (!until_samples_total && WavpackGetNumSamples64 (wpc) != -1) {
            if (total_unpacked_samples < WavpackGetNumSamples64 (wpc)) {
                error_line ("file is missing %llu samples!",
                    WavpackGetNumSamples64 (wpc) - total_unpacked_samples);
                result = WAVPACK_SOFT_ERROR;
            }
            else if (total_unpacked_samples > WavpackGetNumSamples64 (wpc)) {
                error_line ("file has %llu extra samples!",
                    total_unpacked_samples - WavpackGetNumSamples64 (wpc));
                result = WAVPACK_SOFT_ERROR;
            }
        }

        if (WavpackGetNumErrors (wpc)) {
            error_line ("missing data or crc errors detected in %d block(s)!", WavpackGetNumErrors (wpc));
            result = WAVPACK_SOFT_ERROR;
        }
    }

    if (result == WAVPACK_NO_ERROR && md5_diff && (WavpackGetMode (wpc) & MODE_LOSSLESS) && !until_samples_total && input_qmode == output_qmode) {
        error_line ("MD5 signatures should match, but do not!");
        result = WAVPACK_SOFT_ERROR;
    }

    // Compute and display the time consumed along with some other details of
    // the unpacking operation (assuming there was no error).

#if defined(_WIN32)
    _ftime64 (&time2);
    dtime = time2.time + time2.millitm / 1000.0;
    dtime -= time1.time + time1.millitm / 1000.0;
#else
    gettimeofday(&time2,&timez);
    dtime = time2.tv_sec + time2.tv_usec / 1000000.0;
    dtime -= time1.tv_sec + time1.tv_usec / 1000000.0;
#endif

    if (result == WAVPACK_NO_ERROR && !quiet_mode) {
        char *file, *fext, *oper, *cmode, cratio [16] = "";

        if (outfilename && *outfilename != '-') {
            file = FN_FIT (outfilename);
            fext = "";
            oper = "restored";
        }
        else {
            file = (*infilename == '-') ? "stdin" : FN_FIT (infilename);
            fext = wvc_mode ? " (+.wvc)" : "";
            oper = outfilename ? "unpacked" : "verified";
        }

        if (WavpackGetMode (wpc) & MODE_LOSSLESS) {
            cmode = "lossless";

            if (WavpackGetRatio (wpc) != 0.0)
                sprintf (cratio, ", %.2f%%", 100.0 - WavpackGetRatio (wpc) * 100.0);
        }
        else {
            cmode = "lossy";

            if (WavpackGetAverageBitrate (wpc, TRUE) != 0.0)
                sprintf (cratio, ", %d kbps", (int) (WavpackGetAverageBitrate (wpc, TRUE) / 1000.0));
        }

        error_line ("%s %s%s in %.2f secs (%s%s)", oper, file, fext, dtime, cmode, cratio);
    }

    WavpackCloseFile (wpc);

    if (result == WAVPACK_NO_ERROR && delete_source) {
        int res = DoDeleteFile (infilename);

        if (!quiet_mode || !res)
            error_line ("%s source file %s", res ?
                "deleted" : "can't delete", infilename);

        if (wvc_mode) {
            char in2filename [PATH_MAX];

            strcpy (in2filename, infilename);
            strcat (in2filename, "c");
            res = DoDeleteFile (in2filename);

            if (!quiet_mode || !res)
                error_line ("%s source file %s", res ?
                    "deleted" : "can't delete", in2filename);
        }
    }

    return result;
}

#define TEMP_BUFFER_SAMPLES 4096L   // composite samples in temporary buffer used during unpacking

static int unpack_audio (WavpackContext *wpc, FILE *outfile, int qmode, unsigned char *md5_digest, int64_t *sample_count)
{
    unsigned char *output_buffer = NULL, *output_pointer = NULL, *new_channel_order = NULL;
    int bps = WavpackGetBytesPerSample (wpc), num_channels = WavpackGetNumChannels (wpc);
    int64_t until_samples_total = *sample_count, total_unpacked_samples = 0;
    int bytes_per_sample = bps * num_channels, result = WAVPACK_NO_ERROR;
    uint32_t output_buffer_size = 0, bcount;
    double progress = -1.0;
    int32_t *temp_buffer;
    MD5_CTX md5_context;

    if (md5_digest)
        MD5_Init (&md5_context);

    if (outfile) {
        if (outbuf_k)
            output_buffer_size = outbuf_k * 1024;
        else
            output_buffer_size = 1024 * 256;

        output_pointer = output_buffer = malloc (output_buffer_size);

        if (!output_buffer) {
            error_line ("can't allocate buffer for decoding!");
            WavpackCloseFile (wpc);
            return WAVPACK_HARD_ERROR;
        }
    }

    if (qmode & QMODE_REORDERED_CHANS) {
        int layout = WavpackGetChannelLayout (wpc, NULL), i;

        if ((layout & 0xff) <= num_channels) {
            new_channel_order = malloc (num_channels);

            for (i = 0; i < num_channels; ++i)
                new_channel_order [i] = i;

            WavpackGetChannelLayout (wpc, new_channel_order);
        }
    }

    temp_buffer = malloc (TEMP_BUFFER_SAMPLES * num_channels * sizeof (temp_buffer [0]));

    while (result == WAVPACK_NO_ERROR) {
        uint32_t samples_to_unpack, samples_unpacked;

        if (output_buffer) {
            samples_to_unpack = (output_buffer_size - (uint32_t)(output_pointer - output_buffer)) / bytes_per_sample;

            if (samples_to_unpack > TEMP_BUFFER_SAMPLES)
                samples_to_unpack = TEMP_BUFFER_SAMPLES;
        }
        else
            samples_to_unpack = TEMP_BUFFER_SAMPLES;

        if (until_samples_total && samples_to_unpack > until_samples_total - total_unpacked_samples)
            samples_to_unpack = (uint32_t) (until_samples_total - total_unpacked_samples);

        samples_unpacked = WavpackUnpackSamples (wpc, temp_buffer, samples_to_unpack);
        total_unpacked_samples += samples_unpacked;

        if (new_channel_order)
            unreorder_channels (temp_buffer, new_channel_order, num_channels, samples_unpacked);

        if (output_buffer) {
            if (samples_unpacked)
                output_pointer = store_samples (output_pointer, temp_buffer, qmode, bps, samples_unpacked * num_channels);

            if (!samples_unpacked || (output_buffer_size - (output_pointer - output_buffer)) < (uint32_t) bytes_per_sample) {
                if (!DoWriteFile (outfile, output_buffer, (uint32_t)(output_pointer - output_buffer), &bcount) ||
                    bcount != output_pointer - output_buffer) {
                        error_line ("can't write .WAV data, disk probably full!");
                        DoTruncateFile (outfile);
                        result = WAVPACK_HARD_ERROR;
                        break;
                }

                output_pointer = output_buffer;
            }
        }

        if (md5_digest && samples_unpacked) {
            store_samples (temp_buffer, temp_buffer, qmode, bps, samples_unpacked * num_channels);
            MD5_Update (&md5_context, (unsigned char *) temp_buffer, bps * samples_unpacked * num_channels);
        }

        if (!samples_unpacked)
            break;

        if (check_break ()) {
#if defined(_WIN32)
            fprintf (stderr, "^C\n");
#else
            fprintf (stderr, "\n");
#endif
            fflush (stderr);
            DoTruncateFile (outfile);
            result = WAVPACK_SOFT_ERROR;
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

    if (new_channel_order)
        free (new_channel_order);

    if (md5_digest)
        MD5_Final (md5_digest, &md5_context);

    free (temp_buffer);

    if (output_buffer)
        free (output_buffer);

    *sample_count = total_unpacked_samples;
    return result;
}

static const unsigned char bit_reverse_table [] = {
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};

#define DSD_BLOCKSIZE 4096

static int unpack_dsd_audio (WavpackContext *wpc, FILE *outfile, int qmode, unsigned char *md5_digest, int64_t *sample_count)
{
    unsigned char *output_buffer = NULL, *new_channel_order = NULL;
    int num_channels = WavpackGetNumChannels (wpc), result = WAVPACK_NO_ERROR;
    int64_t until_samples_total = *sample_count, total_unpacked_samples = 0;
    uint32_t output_buffer_size = 0, bcount;
    double progress = -1.0;
    int32_t *temp_buffer;
    MD5_CTX md5_context;

    if (md5_digest)
        MD5_Init (&md5_context);

    output_buffer_size = DSD_BLOCKSIZE * num_channels;
    output_buffer = malloc (output_buffer_size);

    if (!output_buffer) {
        error_line ("can't allocate buffer for decoding!");
        WavpackCloseFile (wpc);
        return WAVPACK_HARD_ERROR;
    }

    if (qmode & QMODE_REORDERED_CHANS) {
        int layout = WavpackGetChannelLayout (wpc, NULL), i;

        if ((layout & 0xff) <= num_channels) {
            new_channel_order = malloc (num_channels);

            for (i = 0; i < num_channels; ++i)
                new_channel_order [i] = i;

            WavpackGetChannelLayout (wpc, new_channel_order);
        }
    }

    temp_buffer = malloc (DSD_BLOCKSIZE * num_channels * sizeof (temp_buffer [0]));

    while (result == WAVPACK_NO_ERROR) {
        uint32_t samples_to_unpack = DSD_BLOCKSIZE, samples_unpacked;

        if (until_samples_total && samples_to_unpack > until_samples_total - total_unpacked_samples)
            samples_to_unpack = (uint32_t) (until_samples_total - total_unpacked_samples);

        samples_unpacked = WavpackUnpackSamples (wpc, temp_buffer, samples_to_unpack);
        total_unpacked_samples += samples_unpacked;

        if (new_channel_order)
            unreorder_channels (temp_buffer, new_channel_order, num_channels, samples_unpacked);

        if (samples_unpacked) {
            unsigned char *dptr = output_buffer;
            int32_t *sptr = temp_buffer;

            if (qmode & QMODE_DSD_IN_BLOCKS) {
                int cc = num_channels;

                while (cc--) {
                    uint32_t si;

                    for (si = 0; si < DSD_BLOCKSIZE; si++, sptr += num_channels)
                        if (si < samples_unpacked)
                            *dptr++ = (qmode & QMODE_DSD_LSB_FIRST) ? bit_reverse_table [*sptr & 0xff] : *sptr;
                        else
                            *dptr++ = 0;

                    sptr -= (DSD_BLOCKSIZE * num_channels) - 1;
                }

                samples_unpacked = DSD_BLOCKSIZE;   // make sure we MD5 and write the whole block even if partial (last)
            }
            else {
                int scount = samples_unpacked * num_channels;

                while (scount--)
                    *dptr++ = *sptr++;
            }

            if (md5_digest)
                MD5_Update (&md5_context, output_buffer, samples_unpacked * num_channels);

            if (outfile && (!DoWriteFile (outfile, output_buffer, samples_unpacked * num_channels, &bcount) ||
                bcount != samples_unpacked * num_channels)) {
                    error_line ("can't write .WAV data, disk probably full!");
                    DoTruncateFile (outfile);
                    result = WAVPACK_HARD_ERROR;
                    break;
                }
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
            DoTruncateFile (outfile);
            result = WAVPACK_SOFT_ERROR;
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

    if (new_channel_order)
        free (new_channel_order);

    if (md5_digest)
        MD5_Final (md5_digest, &md5_context);

    free (temp_buffer);

    if (output_buffer)
        free (output_buffer);

    *sample_count = total_unpacked_samples;
    return result;
}

static void add_tag_extraction_to_list (char *spec)
{
    tag_extractions = realloc (tag_extractions, (num_tag_extractions + 1) * sizeof (*tag_extractions));
    tag_extractions [num_tag_extractions] = malloc (strlen (spec) + 10);
    strcpy (tag_extractions [num_tag_extractions], spec);
    num_tag_extractions++;
}

static int do_tag_extractions (WavpackContext *wpc, char *outfilename)
{
    int result = WAVPACK_NO_ERROR, i;
    FILE *outfile;

    for (i = 0; result == WAVPACK_NO_ERROR && i < num_tag_extractions; ++i) {
        char *extraction_spec = strdup (tag_extractions [i]);
        char *output_spec = strchr (extraction_spec, '=');
        char tag_filename [256];

        if (output_spec && output_spec > extraction_spec && strlen (output_spec) > 1)
            *output_spec++ = 0;

        if (dump_tag_item_to_file (wpc, extraction_spec, NULL, tag_filename)) {
            int max_length = (int) strlen (outfilename) + (int) strlen (tag_filename) + 10;
            char *full_filename;

            if (output_spec)
                max_length += (int) strlen (output_spec) + 256;

            full_filename = malloc (max_length * 2 + 1);
            strcpy (full_filename, outfilename);

            if (output_spec) {
                char *dst = filespec_name (full_filename);

                while (*output_spec && dst - full_filename < max_length) {
                    if (*output_spec == '%') {
                        switch (*++output_spec) {
                            case 'a':                           // audio filename
                                strcpy (dst, filespec_name (outfilename));

                                if (filespec_ext (dst))         // get rid of any extension
                                    dst = filespec_ext (dst);
                                else
                                    dst += strlen (dst);

                                output_spec++;
                                break;

                            case 't':                           // tag field name
                                strcpy (dst, tag_filename);

                                if (filespec_ext (dst))         // get rid of any extension
                                    dst = filespec_ext (dst);
                                else
                                    dst += strlen (dst);

                                output_spec++;
                                break;

                            case 'e':                           // default extension
                                if (filespec_ext (tag_filename)) {
                                    strcpy (dst, filespec_ext (tag_filename) + 1);
                                    dst += strlen (dst);
                                }

                                output_spec++;
                                break;

                            default:
                                *dst++ = '%';
                        }
                    }
                    else
                        *dst++ = *output_spec++;
                }

                *dst = 0;
            }
            else
                strcpy (filespec_name (full_filename), tag_filename);

            if (!overwrite_all && (outfile = fopen (full_filename, "r")) != NULL) {
                DoCloseHandle (outfile);
                fprintf (stderr, "overwrite %s (yes/no/all)? ", FN_FIT (full_filename));
                fflush (stderr);

                if (set_console_title)
                    DoSetConsoleTitle ("overwrite?");

                switch (yna ()) {

                    case 'n':
                        *full_filename = 0;
                        break;

                    case 'a':
                        overwrite_all = 1;
                }
            }

            // open output file for writing

            if (*full_filename) {
                if ((outfile = fopen (full_filename, "w")) == NULL) {
                    error_line ("can't create file %s!", FN_FIT (full_filename));
                    result = WAVPACK_SOFT_ERROR;
                }
                else {
                    dump_tag_item_to_file (wpc, extraction_spec, outfile, NULL);

                    if (!DoCloseHandle (outfile)) {
                        error_line ("can't close file %s!", FN_FIT (full_filename));
                        result = WAVPACK_SOFT_ERROR;
                    }
                    else if (!quiet_mode)
                        error_line ("extracted tag \"%s\" to file %s", extraction_spec, FN_FIT (full_filename));
                }
            }

            free (full_filename);
        }

        free (extraction_spec);
    }

    return result;
}

// Code to store samples. Source is an array of int32_t data (which is what WavPack uses
// internally), but the destination can have from 1 to 4 bytes per sample. Also, the destination
// data is assumed to be little-endian and signed, except for byte data which is unsigned (these
// are WAV file defaults). The endian and signedness can be overridden with the qmode flags
// to support other formats.

static void *store_little_endian_unsigned_samples (void *dst, int32_t *src, int bps, int count);
static void *store_little_endian_signed_samples (void *dst, int32_t *src, int bps, int count);
static void *store_big_endian_unsigned_samples (void *dst, int32_t *src, int bps, int count);
static void *store_big_endian_signed_samples (void *dst, int32_t *src, int bps, int count);

static void *store_samples (void *dst, int32_t *src, int qmode, int bps, int count)
{
    if (qmode & QMODE_BIG_ENDIAN) {
        if ((qmode & QMODE_UNSIGNED_WORDS) || (bps == 1 && !(qmode & QMODE_SIGNED_BYTES)))
            return store_big_endian_unsigned_samples (dst, src, bps, count);
        else
            return store_big_endian_signed_samples (dst, src, bps, count);
    }
    else if ((qmode & QMODE_UNSIGNED_WORDS) || (bps == 1 && !(qmode & QMODE_SIGNED_BYTES)))
        return store_little_endian_unsigned_samples (dst, src, bps, count);
    else
        return store_little_endian_signed_samples (dst, src, bps, count);
}

static void *store_little_endian_unsigned_samples (void *dst, int32_t *src, int bps, int count)
{
    unsigned char *dptr = dst;
    int32_t temp;

    switch (bps) {

        case 1:
            while (count--)
                *dptr++ = *src++ + 0x80;

            break;

        case 2:
            while (count--) {
                *dptr++ = (unsigned char) (temp = *src++ + 0x8000);
                *dptr++ = (unsigned char) (temp >> 8);
            }

            break;

        case 3:
            while (count--) {
                *dptr++ = (unsigned char) (temp = *src++ + 0x800000);
                *dptr++ = (unsigned char) (temp >> 8);
                *dptr++ = (unsigned char) (temp >> 16);
            }

            break;

        case 4:
            while (count--) {
                *dptr++ = (unsigned char) (temp = *src++ + 0x80000000);
                *dptr++ = (unsigned char) (temp >> 8);
                *dptr++ = (unsigned char) (temp >> 16);
                *dptr++ = (unsigned char) (temp >> 24);
            }

            break;
    }

    return dptr;
}

static void *store_little_endian_signed_samples (void *dst, int32_t *src, int bps, int count)
{
    unsigned char *dptr = dst;
    int32_t temp;

    switch (bps) {

        case 1:
            while (count--)
                *dptr++ = *src++;

            break;

        case 2:
            while (count--) {
                *dptr++ = (unsigned char) (temp = *src++);
                *dptr++ = (unsigned char) (temp >> 8);
            }

            break;

        case 3:
            while (count--) {
                *dptr++ = (unsigned char) (temp = *src++);
                *dptr++ = (unsigned char) (temp >> 8);
                *dptr++ = (unsigned char) (temp >> 16);
            }

            break;

        case 4:
            while (count--) {
                *dptr++ = (unsigned char) (temp = *src++);
                *dptr++ = (unsigned char) (temp >> 8);
                *dptr++ = (unsigned char) (temp >> 16);
                *dptr++ = (unsigned char) (temp >> 24);
            }

            break;
    }

    return dptr;
}

static void *store_big_endian_unsigned_samples (void *dst, int32_t *src, int bps, int count)
{
    unsigned char *dptr = dst;
    int32_t temp;

    switch (bps) {

        case 1:
            while (count--)
                *dptr++ = *src++ + 0x80;

            break;

        case 2:
            while (count--) {
                *dptr++ = (unsigned char) ((temp = *src++ + 0x8000) >> 8);
                *dptr++ = (unsigned char) temp;
            }

            break;

        case 3:
            while (count--) {
                *dptr++ = (unsigned char) ((temp = *src++ + 0x800000) >> 16);
                *dptr++ = (unsigned char) (temp >> 8);
                *dptr++ = (unsigned char) temp;
            }

            break;

        case 4:
            while (count--) {
                *dptr++ = (unsigned char) ((temp = *src++ + 0x80000000) >> 24);
                *dptr++ = (unsigned char) (temp >> 16);
                *dptr++ = (unsigned char) (temp >> 8);
                *dptr++ = (unsigned char) temp;
            }

            break;
    }

    return dptr;
}

static void *store_big_endian_signed_samples (void *dst, int32_t *src, int bps, int count)
{
    unsigned char *dptr = dst;
    int32_t temp;

    switch (bps) {

        case 1:
            while (count--)
                *dptr++ = *src++;

            break;

        case 2:
            while (count--) {
                *dptr++ = (unsigned char) ((temp = *src++) >> 8);
                *dptr++ = (unsigned char) temp;
            }

            break;

        case 3:
            while (count--) {
                *dptr++ = (unsigned char) ((temp = *src++) >> 16);
                *dptr++ = (unsigned char) (temp >> 8);
                *dptr++ = (unsigned char) temp;
            }

            break;

        case 4:
            while (count--) {
                *dptr++ = (unsigned char) ((temp = *src++) >> 24);
                *dptr++ = (unsigned char) (temp >> 16);
                *dptr++ = (unsigned char) (temp >> 8);
                *dptr++ = (unsigned char) temp;
            }

            break;
    }

    return dptr;
}

static void unreorder_channels (int32_t *data, unsigned char *order, int num_chans, int num_samples)
{
    int32_t reorder_buffer [16], *temp = reorder_buffer;

    if (num_chans > 16)
        temp = malloc (num_chans * sizeof (*data));

    while (num_samples--) {
        int chan;

        for (chan = 0; chan < num_chans; ++chan)
            temp [chan] = data [order[chan]];

        memcpy (data, temp, num_chans * sizeof (*data));
        data += num_chans;
    }

    if (num_chans > 16)
        free (temp);
}

static void dump_UTF8_string (char *string, FILE *dst);
static void UTF8ToAnsi (char *string, int len);
static const char *speakers [] = {
    "FL", "FR", "FC", "LFE", "BL", "BR", "FLC", "FRC", "BC",
    "SL", "SR", "TC", "TFL", "TFC", "TFR", "TBL", "TBC", "TBR"
};

static void dump_summary (WavpackContext *wpc, char *name, FILE *dst)
{
    uint32_t channel_mask = (uint32_t) WavpackGetChannelMask (wpc);
    int num_channels = WavpackGetNumChannels (wpc);
    unsigned char md5_sum [16];
    char modes [80];

    fprintf (dst, "\n");

    if (name && *name != '-') {
        fprintf (dst, "file name:         %s%s\n", name, (WavpackGetMode (wpc) & MODE_WVC) ? " (+wvc)" : "");
        fprintf (dst, "file size:         %lld bytes\n", (long long) WavpackGetFileSize64 (wpc));
    }

    if (WavpackGetQualifyMode (wpc) & QMODE_DSD_AUDIO)
        fprintf (dst, "source:            1-bit DSD at %u Hz\n", WavpackGetNativeSampleRate (wpc));
    else
        fprintf (dst, "source:            %d-bit %s at %u Hz\n", WavpackGetBitsPerSample (wpc),
            (WavpackGetMode (wpc) & MODE_FLOAT) ? "floats" : "ints",
            WavpackGetSampleRate (wpc));

    if (!channel_mask)
        strcpy (modes, "unassigned speakers");
    else if (num_channels == 1 && channel_mask == 0x4)
        strcpy (modes, "mono");
    else if (num_channels == 2 && channel_mask == 0x3)
        strcpy (modes, "stereo");
    else if (num_channels == 4 && channel_mask == 0x33)
        strcpy (modes, "quad");
    else if (num_channels == 6 && channel_mask == 0x3f)
        strcpy (modes, "5.1 surround");
    else if (num_channels == 6 && channel_mask == 0x60f)
        strcpy (modes, "5.1 surround side");
    else if (num_channels == 8 && channel_mask == 0x63f)
        strcpy (modes, "7.1 surround");
    else if (num_channels == 8 && channel_mask == 0x6000003f)
        strcpy (modes, "5.1 + stereo");
    else {
        int cc = num_channels, si = 0;
        uint32_t cm = channel_mask;

        modes [0] = 0;

        while (cc && cm) {
            if (cm & 1) {
                strcat (modes, si < 18 ? speakers [si] : "--");
                if (--cc)
                    strcat (modes, ",");
            }
            cm >>= 1;
            si++;
        }

        if (cc)
            strcat (modes, "...");
    }

    fprintf (dst, "channels:          %d (%s)\n", num_channels, modes);

    if (WavpackGetNumSamples64 (wpc) != -1) {
        double seconds = (double) WavpackGetNumSamples64 (wpc) / WavpackGetSampleRate (wpc);
        int minutes = (int) floor (seconds / 60.0);
        int hours = (int) floor (seconds / 3600.0);

        seconds -= minutes * 60.0;
        minutes -= (int)(hours * 60.0);

        fprintf (dst, "duration:          %d:%02d:%05.2f\n", hours, minutes, seconds);
    }

    modes [0] = 0;

    if (WavpackGetMode (wpc) & MODE_HYBRID)
        strcat (modes, "hybrid ");

    strcat (modes, (WavpackGetMode (wpc) & MODE_LOSSLESS) ? "lossless" : "lossy");

    if (WavpackGetMode (wpc) & MODE_FAST)
        strcat (modes, ", fast");
    else if (WavpackGetMode (wpc) & MODE_VERY_HIGH)
        strcat (modes, ", very high");
    else if (WavpackGetMode (wpc) & MODE_HIGH)
        strcat (modes, ", high");

    if (WavpackGetMode (wpc) & MODE_EXTRA) {
        strcat (modes, ", extra");

        if (WavpackGetMode (wpc) & MODE_XMODE) {
            char xmode[3] = "-0";

            xmode [1] = ((WavpackGetMode (wpc) & MODE_XMODE) >> 12) + '0';
            strcat (modes, xmode);
        }
    }

    if (WavpackGetMode (wpc) & MODE_SFX)
        strcat (modes, ", sfx");

    if (WavpackGetMode (wpc) & MODE_DNS)
        strcat (modes, ", dns");

    fprintf (dst, "modalities:        %s\n", modes);

    if (WavpackGetRatio (wpc) != 0.0) {
        fprintf (dst, "compression:       %.2f%%\n", 100.0 - (100 * WavpackGetRatio (wpc)));
        fprintf (dst, "ave bitrate:       %d kbps\n", (int) ((WavpackGetAverageBitrate (wpc, TRUE) + 500.0) / 1000.0));

        if (WavpackGetMode (wpc) & MODE_WVC)
            fprintf (dst, "ave lossy bitrate: %d kbps\n", (int) ((WavpackGetAverageBitrate (wpc, FALSE) + 500.0) / 1000.0));
    }

    if (WavpackGetVersion (wpc))
        fprintf (dst, "encoder version:   %d\n", WavpackGetVersion (wpc));

    if (WavpackGetMD5Sum (wpc, md5_sum)) {
        char md5_string [] = "00000000000000000000000000000000";
        int i;

        for (i = 0; i < 16; ++i)
            sprintf (md5_string + (i * 2), "%02x", md5_sum [i]);

        fprintf (dst, "original md5:      %s\n", md5_string);
    }

    if (summary > 1) {
        uint32_t header_bytes = WavpackGetWrapperBytes (wpc), trailer_bytes, i;
        unsigned char *header_data = WavpackGetWrapperData (wpc);
        char header_name [5];

        strcpy (header_name, "????");

        for (i = 0; i < 4 && i < header_bytes; ++i)
            if (header_data [i] >= 0x20 && header_data [i] <= 0x7f)
                header_name [i] = header_data [i];

        WavpackFreeWrapper (wpc);
        WavpackSeekTrailingWrapper (wpc);
        trailer_bytes = WavpackGetWrapperBytes (wpc);

        if (WavpackGetFileFormat (wpc) < NUM_FILE_FORMATS)
            fprintf (dst, "source format:     %s with '%s' extension\n",
                file_formats [WavpackGetFileFormat (wpc)].format_name, WavpackGetFileExtension (wpc));
        else
            fprintf (dst, "source format:     '%s' file\n", WavpackGetFileExtension (wpc));

        if (header_bytes && trailer_bytes) {
            unsigned char *trailer_data = WavpackGetWrapperData (wpc);
            char trailer_name [5];

            strcpy (trailer_name, "????");

            for (i = 0; i < 4 && i < trailer_bytes; ++i)
                if (trailer_data [i] >= 0x20 && trailer_data [i] <= 0x7f)
                    trailer_name [i] = trailer_data [i];

            fprintf (dst, "file wrapper:      %u + %u bytes (%s, %s)\n",
                header_bytes, trailer_bytes, header_name, trailer_name);
        }
        else if (header_bytes)
            fprintf (dst, "file wrapper:      %u byte %s header\n",
                header_bytes, header_name);
        else if (trailer_bytes)
            fprintf (dst, "file wrapper:      %u byte trailer only\n",
                trailer_bytes);
        else
            fprintf (dst, "file wrapper:      none stored\n");
    }

    if (WavpackGetMode (wpc) & MODE_VALID_TAG) {
        int ape_tag = WavpackGetMode (wpc) & MODE_APETAG;
        int num_binary_items = WavpackGetNumBinaryTagItems (wpc);
        int num_items = WavpackGetNumTagItems (wpc), i;
        char *spaces = "                  ";

        fprintf (dst, "\n%s tag items:   %d\n", ape_tag ? "APEv2" : "ID3v1", num_items + num_binary_items);

        for (i = 0; i < num_items; ++i) {
            int item_len, value_len, j;
            char *item, *value;

            item_len = WavpackGetTagItemIndexed (wpc, i, NULL, 0);
            item = malloc (item_len + 1);
            WavpackGetTagItemIndexed (wpc, i, item, item_len + 1);
            value_len = WavpackGetTagItem (wpc, item, NULL, 0);
            value = malloc (value_len * 2 + 1);
            WavpackGetTagItem (wpc, item, value, value_len + 1);

            fprintf (dst, "%s:%s", item, strlen (item) < strlen (spaces) ? spaces + strlen (item) : " ");

            if (ape_tag) {
                for (j = 0; j < value_len; ++j)
                    if (!value [j])
                        value [j] = '\\';

                if (strchr (value, '\n'))
                    fprintf (dst, "%d-byte multi-line text string\n", value_len);
                else {
                    dump_UTF8_string (value, dst);
                    fprintf (dst, "\n");
                }
            }
            else
                fprintf (dst, "%s\n", value);

            free (value);
            free (item);
        }

        for (i = 0; i < num_binary_items; ++i) {
            int item_len, value_len;
            char *item, fname [256];

            item_len = WavpackGetBinaryTagItemIndexed (wpc, i, NULL, 0);
            item = malloc (item_len + 1);
            WavpackGetBinaryTagItemIndexed (wpc, i, item, item_len + 1);
            value_len = dump_tag_item_to_file (wpc, item, NULL, fname);
            fprintf (dst, "%s:%s", item, strlen (item) < strlen (spaces) ? spaces + strlen (item) : " ");

            if (filespec_ext (fname))
                fprintf (dst, "%d-byte binary item (%s)\n", value_len, filespec_ext (fname)+1);
            else
                fprintf (dst, "%d-byte binary item\n", value_len);

#if 0   // debug binary tag reading
            {
                char md5_string [] = "00000000000000000000000000000000";
                unsigned char md5_result [16];
                MD5_CTX md5_context;
                char *value;
                int i, j;

                MD5_Init (&md5_context);
                value_len = WavpackGetBinaryTagItem (wpc, item, NULL, 0);
                value = malloc (value_len);
                value_len = WavpackGetBinaryTagItem (wpc, item, value, value_len);

                for (i = 0; i < value_len; ++i)
                    if (!value [i]) {
                        MD5_Update (&md5_context, (unsigned char *) value + i + 1, value_len - i - 1);
                        MD5_Final (md5_result, &md5_context);
                        for (j = 0; j < 16; ++j)
                            sprintf (md5_string + (j * 2), "%02x", md5_result [j]);
                        fprintf (dst, "    %d byte string >>%s<<\n", i, value);
                        fprintf (dst, "    %d bytes binary data >>%s<<\n", value_len - i - 1, md5_string);
                        break;
                    }

                if (i == value_len)
                    fprintf (dst, "    no NULL found in binary value (or value not readable)\n");

                free (value);
            }
#endif
            free (item);
        }
    }
}

// Dump a summary of the file information in a machine-parsable format to the specified file (usually stdout).
// The items are separated by semi-colons and the line is newline terminated, like in this example:
//
// 44100;16;int;2;0x3;9878400;023066a6345773674c0755ee6be54d87;4;0x18a2;Track01.wv
//
// The fields are, in order:
//
// 1. sampling rate
// 2. bit-depth (1-32)
// 3. format ("int" or "float")
// 4. number of channels
// 5. channel mask (in hex because it's a mask, always prefixed with "0x")
// 6. number of samples (missing if unknown)
// 7. md5sum (technically is hex, but not prefixed with "0x", might be missing)
// 8. encoder version (basically this will always be 4 or 5, but there are some old files out there)
// 9. encoding mode (in hex because it's a bitfield, always prefixed with "0x")
// 10. filename (if available)

static void dump_file_item (WavpackContext *wpc, char *str, int item_id);

static void dump_file_info (WavpackContext *wpc, char *name, FILE *dst, int parameter)
{
    char str [80];
    int item_id;

    str [0] = 0;

    if (parameter == 0) {
        for (item_id = 1; item_id <= 9; ++item_id) {
            dump_file_item (wpc, str, item_id);
            strcat (str, ";");
        }

        if (name && *name != '-')
            fprintf (dst, "%s%s\n", str, name);
        else
            fprintf (dst, "%s\n", str);
    }
    else if (parameter < 10) {
        dump_file_item (wpc, str, parameter);
        fprintf (dst, "%s\n", str);
    }
    else if (parameter == 10 && name && *name != '-')
        fprintf (dst, "%s\n", name);
    else
        fprintf (dst, "\n");
}

static void dump_file_item (WavpackContext *wpc, char *str, int item_id)
{
    unsigned char md5_sum [16];

    switch (item_id) {
        case 1:
            sprintf (str + strlen (str), "%d", WavpackGetNativeSampleRate (wpc));
            break;

        case 2:
            sprintf (str + strlen (str), "%d", (WavpackGetQualifyMode (wpc) & QMODE_DSD_AUDIO) ? 1 : WavpackGetBitsPerSample (wpc));
            break;

        case 3:
            sprintf (str + strlen (str), "%s", (WavpackGetMode (wpc) & MODE_FLOAT) ? "float" : "int");
            break;

        case 4:
            sprintf (str + strlen (str), "%d", WavpackGetNumChannels (wpc));
            break;

        case 5:
            sprintf (str + strlen (str), "0x%x", WavpackGetChannelMask (wpc));
            break;

        case 6:
            if (WavpackGetNumSamples64 (wpc) != -1)
                sprintf (str + strlen (str), "%lld",
                    (long long int) WavpackGetNumSamples64 (wpc) * (WavpackGetQualifyMode (wpc) & QMODE_DSD_AUDIO ? 8 : 1));

            break;

        case 7:
            if (WavpackGetMD5Sum (wpc, md5_sum)) {
                char md5_string [] = "00000000000000000000000000000000";
                int i;

                for (i = 0; i < 16; ++i)
                    sprintf (md5_string + (i * 2), "%02x", md5_sum [i]);

                sprintf (str + strlen (str), "%s", md5_string);
            }

            break;

        case 8:
            sprintf (str + strlen (str), "%d", WavpackGetVersion (wpc));
            break;

        case 9:
            sprintf (str + strlen (str), "0x%x", WavpackGetMode (wpc));
            break;

        default:
            break;
    }
}

// Dump the specified tag field to the specified stream. Both text and binary tags may be written,
// and in Windows the appropriate file mode will be set. If the tag is not found then 0 is returned,
// otherwise the length of the data is returned, and this is true even when the file pointer is NULL
// so this can be used to determine if the tag exists before further processing.
//
// The "fname" parameter can optionally be set to a character array that will accept the suggested
// filename. This is formed by the tag item name with the extension ".txt" for text fields; for
// binary fields this is supplied by convention as a NULL terminated string at the beginning of the
// data, so this is returned. The string should have 256 bytes available (for 255 chars + NULL).

static int dump_tag_item_to_file (WavpackContext *wpc, const char *tag_item, FILE *dst, char *fname)
{
    const char *sanitized_tag_item = filespec_name ((char *) tag_item) ? filespec_name ((char *) tag_item) : tag_item;

    if (WavpackGetMode (wpc) & MODE_VALID_TAG) {
        if (WavpackGetTagItem (wpc, tag_item, NULL, 0)) {
            int value_len = WavpackGetTagItem (wpc, tag_item, NULL, 0);
            char *value;

            if (fname) {
                snprintf (fname, 256, "%s.txt", sanitized_tag_item);
                fname [255] = 0;
            }

            if (!value_len || !dst)
                return value_len;

#if defined(_WIN32)
            _setmode (_fileno (dst), O_TEXT);
#endif
#if defined(__OS2__)
            setmode (fileno (dst), O_TEXT);
#endif
            value = malloc (value_len * 2 + 1);
            WavpackGetTagItem (wpc, tag_item, value, value_len + 1);
            dump_UTF8_string (value, dst);
            free (value);
            return value_len;
        }
        else if (WavpackGetBinaryTagItem (wpc, tag_item, NULL, 0)) {
            int value_len = WavpackGetBinaryTagItem (wpc, tag_item, NULL, 0), res = 0, i;
            uint32_t bcount = 0;
            char *value;

            value = malloc (value_len);
            WavpackGetBinaryTagItem (wpc, tag_item, value, value_len);

            for (i = 0; i < value_len; ++i)
                if (!value [i]) {

                    if (dst) {
#if defined(_WIN32)
                        _setmode (_fileno (dst), O_BINARY);
#endif
#if defined(__OS2__)
                        setmode (fileno (dst), O_BINARY);
#endif
                        res = DoWriteFile (dst, (unsigned char *) value + i + 1, value_len - i - 1, &bcount);
                    }

                    if (fname) {
                        char *sanitized_tag_value = filespec_name (value) ? filespec_name (value) : value;

                        if (strlen (sanitized_tag_value) < 256)
                            strcpy (fname, sanitized_tag_value);
                        else {
                            snprintf (fname, 256, "%s.bin", sanitized_tag_item);
                            fname [255] = 0;
                        }
                    }

                    break;
                }

            free (value);

            if (i == value_len)
                return 0;

            if (dst && (!res || bcount != value_len - i - 1))
                return 0;

            return value_len - i - 1;
        }
        else
            return 0;
    }
    else
        return 0;
}

// Dump the specified null-terminated, possibly multi-line, UTF-8 string to
// the specified stream. To make sure that this works correctly on both
// Windows and Linux, all CR characters ('\r') are removed from the stream
// and it is assumed that the output FILE will be in "text" mode (on Windows).
// Lines are processed and transmitted one at a time.

static void dump_UTF8_string (char *string, FILE *dst)
{
    while (*string) {
        char *p = string, *temp;
        int len = 0;

        while (*p) {
            if (*p != '\r')
                ++len;

            if (*p++ == '\n')
                break;
        }

        if (!len)
            return;

        p = temp = malloc (len * 2 + 1);

        while (*string) {
            if (*string != '\r')
                *p++ = *string;

            if (*string++ == '\n')
                break;
        }

        *p = 0;

#ifdef _WIN32
        if (!no_utf8_convert && dst != stdout && dst != stderr)
#else
        if (!no_utf8_convert)
#endif
            UTF8ToAnsi (temp, len * 2);

        fputs (temp, dst);
        free (temp);
    }
}

#if defined (_WIN32)

// Convert Unicode UTF-8 string to wide format. UTF-8 string must be NULL
// terminated. Resulting wide string must be able to fit in provided space
// and will also be NULL terminated. The number of characters converted will
// be returned (not counting terminator).

static int UTF8ToWideChar (const unsigned char *pUTF8, wchar_t *pWide)
{
    int trail_bytes = 0;
    int chrcnt = 0;

    while (*pUTF8) {
        if (*pUTF8 & 0x80) {
            if (*pUTF8 & 0x40) {
                if (trail_bytes) {
                    trail_bytes = 0;
                    chrcnt++;
                }
                else {
                    char temp = *pUTF8;

                    while (temp & 0x80) {
                        trail_bytes++;
                        temp <<= 1;
                    }

                    pWide [chrcnt] = temp >> trail_bytes--;
                }
            }
            else if (trail_bytes) {
                pWide [chrcnt] = (pWide [chrcnt] << 6) | (*pUTF8 & 0x3f);

                if (!--trail_bytes)
                    chrcnt++;
            }
        }
        else
            pWide [chrcnt++] = *pUTF8;

        pUTF8++;
    }

    pWide [chrcnt] = 0;
    return chrcnt;
}

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

// Convert a Unicode UTF-8 format string into its Ansi equivalent. The
// conversion is done in-place so the maximum length of the string buffer must
// be specified because the string may become longer or shorter. If the
// resulting string will not fit in the specified buffer size then it is
// truncated.

static void UTF8ToAnsi (char *string, int len)
{
    int max_chars = (int) strlen (string);
#if defined (_WIN32)
    wchar_t *temp = malloc ((max_chars + 1) * 2);
    int act_chars = UTF8ToWideChar (string, temp);

    while (act_chars) {
        memset (string, 0, len);

        if (WideCharToMultiByte (CP_ACP, 0, temp, act_chars, string, len - 1, NULL, NULL))
            break;
        else
            act_chars--;
    }

    if (!act_chars)
        *string = 0;
#else
    char *temp = malloc (len);
    char *outp = temp;
    char *inp = string;
    size_t insize = max_chars;
    size_t outsize = len - 1;
    int err = 0;
    char *old_locale;
    iconv_t converter;

    memset(temp, 0, len);
    old_locale = setlocale (LC_CTYPE, "");
    converter = iconv_open ("", "UTF-8");

    if (converter != (iconv_t) -1) {
        err = iconv (converter, &inp, &insize, &outp, &outsize);
        iconv_close (converter);
    }
    else
        err = -1;

    setlocale (LC_CTYPE, old_locale);

    if (err == -1) {
        free(temp);
        return;
    }

    memmove (string, temp, len);
#endif
    free (temp);
}

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
        sprintf (title, "%d%% (WvUnpack)", (int) ((file_progress * 100.0) + 0.5));
        DoSetConsoleTitle (title);
    }
}
