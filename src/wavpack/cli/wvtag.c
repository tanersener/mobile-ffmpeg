////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//                Copyright (c) 1998 - 2019 David Bryant.                 //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// wvtag.c

// This is the main module for the WavPack command-line metadata tagging utility.

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
#include <sys/param.h>
#include <sys/stat.h>
#include <locale.h>
#include <iconv.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>

#include "wavpack.h"
#include "utils.h"
#include "md5.h"

#if (defined(__GNUC__) || defined(__sun)) && !defined(_WIN32)
#include <unistd.h>
#include <glob.h>
#include <sys/time.h>
#else
#include <sys/timeb.h>
#endif

#ifdef _WIN32
#include "win32_unicode_support.h"
#define fputs fputs_utf8
#define fprintf fprintf_utf8
#define remove(f) unlink_utf8(f)
#define rename(o,n) rename_utf8(o,n)
#define fopen(f,m) fopen_utf8(f,m)
#define strdup(x) _strdup(x)
#define stricmp(x,y) _stricmp(x,y)
#define strdup(x) _strdup(x)
#else
#define stricmp strcasecmp
#endif

///////////////////////////// local variable storage //////////////////////////

static const char *sign_on = "\n"
" WVTAG  WavPack Metadata Tagging Utility  %s Version %s\n"
" Copyright (c) 2018 - 2019 David Bryant.  All Rights Reserved.\n\n";

static const char *version_warning = "\n"
" WARNING: WVTAG using libwavpack version %s, expected %s (see README)\n\n";

static const char *help =
#if defined (_WIN32)
" Usage:\n"
"    WVTAG [-options] file[.wv] [...]\n\n"
"    Wildcard characters (*,?) may be included in the filenames. All options\n"
"    and operations specified are applied to each file in this order:\n"
"    clean, import, delete, write, extract, list.\n\n"
#else
" Usage:\n"
"    WVTAG [-options] file[.wv] [...]\n\n"
"    Multiple input files may be specified. All options and operations\n"
"    specified are applied to each file in this order: clean, import,\n"
"    delete, write, extract, list.\n\n"
#endif
" Options:\n"
"    --allow-huge-tags     allow tag data up to 16 MB (embedding > 1 MB is not\n"
"                           recommended for portable devices and may not work\n"
"                           with some programs including WavPack pre-4.70)\n"
"    -c                    extract cuesheet only to stdout\n"
"                           (note: equivalent to -x \"cuesheet\")\n"
"    -cc                   extract cuesheet file (.cue)\n"
"                           (note: equivalent to -xx \"cuesheet=%a.cue\")\n"
"    --clean or --clear    clean all items from tag (done first)\n"
"    -d \"Field\"            delete specified metadata item (text or binary)\n"
"    -h or --help          this help display\n"
"    --import-id3          import ID3v2 tags from the trailer of original file\n"
"                           (default for DSF files, optional for other formats,\n"
"                            add --allow-huge-tags option for > 1 MB images)\n"
"    -l or --list          list all tag items (done last)\n"
#ifdef _WIN32
"    --no-utf8-convert     assume tag values read from files are already UTF-8,\n"
"                           don't attempt to convert from local encoding\n"
#else
"    --no-utf8-convert     don't recode passed tags from local encoding to\n"
"                           UTF-8, assume they are in UTF-8 already\n"
#endif
#ifdef _WIN32
"    --pause               pause before exiting (if console window disappears)\n"
#endif
"    -q                    quiet (keep console output to a minimum)\n"
"    -v or --version       write the version to stdout\n"
"    -w \"Field=\"           delete specified metadata item (text or binary)\n"
"    -w \"Field=Value\"      write specified text metadata to APEv2 tag\n"
"    -w \"Field=@file.ext\"  write specified text metadata from file to APEv2\n"
"                           tag, normally used for embedded cuesheets and logs\n"
"                           (field names \"Cuesheet\" and \"Log\")\n"
"    --write-binary-tag \"Field=@file.ext\"\n"
"                          write the specified binary metadata file to APEv2\n"
"                           tag, normally used for cover art with the specified\n"
"                           field name \"Cover Art (Front)\"\n"
"    -x \"Field\"            extract specified tag field only to stdout\n"
"    -xx \"Field[=file]\"    extract specified tag field to file, optional\n"
"                           filename spec can include these replacement codes:\n"
"                             %a = source filename\n"
"                             %t = tag field name\n"
"                                 (note: comes from data for binary tags)\n"
"                             %e = extension from binary tag source file\n"
"                                 (or 'txt' for text tag)\n"
"    -y                    yes to overwrite warning (use with caution!)\n\n"
" Web:\n"
"     Visit www.wavpack.com for latest version and complete information\n";

// this global is used to indicate the special "debug" mode where extra debug messages
// are displayed and all messages are logged to the file \wavpack.log

int debug_logging_mode;

static int overwrite_all, clean_tags, list_tags, import_id3, quiet_mode, no_utf8_convert, allow_huge_tags;

// These two statics are used to keep track of tags that the user specifies on the
// command line. The "num_tag_strings" and "tag_strings" fields in the WavpackConfig
// structure are no longer used for anything (they should not have been there in
// the first place).

static int num_tag_items;

static struct tag_item {
    char *item, *value, *ext;
    int vsize, binary;
} *tag_items;

static char *tag_extract_stdout;    // extract single tag to stdout
static char **tag_extractions;      // extract multiple tags to named files
static int num_tag_extractions;

#if defined (_WIN32)
static int pause_mode;
#endif

int ImportID3v2 (WavpackContext *wpc, unsigned char *tag_data, int tag_size, char *error, int32_t *bytes_used); // import_id3.c

/////////////////////////// local function declarations ///////////////////////

static void add_tag_extraction_to_list (char *spec);
static void TextToUTF8 (void *string, int len);
static FILE *wild_fopen (char *filename, const char *mode);
static int process_file (char *infilename);

#define WAVPACK_NO_ERROR    0
#define WAVPACK_SOFT_ERROR  1
#define WAVPACK_HARD_ERROR  2

// The "main" function for the command-line WavPack metadata editor. Note that on Windows
// this is actually a static function that is called from the "real" main() defined
// immediately afterward that converts the wchar argument list into UTF-8 strings
// and sets the console to UTF-8 for better Unicode support.

#ifdef _WIN32
static int wvtag_main(int argc, char **argv)
#else
int main (int argc, char **argv)
#endif
{
#ifdef __EMX__ /* OS/2 */
    _wildcard (&argc, &argv);
#endif
    int num_files = 0, file_index;
    int error_count = 0, tag_next_arg = 0;
    int c_count = 0, x_count = 0;
    char **matches = NULL;
    int result, i;

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

    // loop through command-line arguments

    while (--argc)
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
                printf ("wvtag %s\n", PACKAGE_VERSION);
                printf ("libwavpack %s\n", WavpackGetLibraryVersionString ());
                return 0;
            }
#ifdef _WIN32
            else if (!strcmp (long_option, "pause"))                    // --pause
                pause_mode = 1;
#endif
            else if (!strcmp (long_option, "clean"))                    // --clean
                clean_tags = 1;
            else if (!strcmp (long_option, "clear"))                    // --clear
                clean_tags = 1;
            else if (!strcmp (long_option, "list"))                     // --list
                list_tags = 1;
            else if (!strcmp (long_option, "import-id3"))               // --import-id3
                import_id3 = 1;
            else if (!strcmp (long_option, "no-utf8-convert"))          // --no-utf8-convert
                no_utf8_convert = 1;
            else if (!strcmp (long_option, "allow-huge-tags"))          // --allow-huge-tags
                allow_huge_tags = 1;
            else if (!strcmp (long_option, "write-binary-tag"))         // --write-binary-tag
                tag_next_arg = 2;
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

                    case 'C': case 'c':
                        if (++c_count == 2) {
                            add_tag_extraction_to_list ("cuesheet=%a.cue");
                            c_count = 0;
                        }

                        break;

                    case 'D': case 'd':
                        tag_next_arg = -1;
                        break;

                    case 'H': case 'h':
                        printf ("%s", help);
                        return 0;

                    case 'L': case 'l':
                        list_tags = 1;
                        break;

                    case 'Q': case 'q':
                        quiet_mode = 1;
                        break;

                    case 'V': case 'v':
                        printf ("wvtag %s\n", PACKAGE_VERSION);
                        printf ("libwavpack %s\n", WavpackGetLibraryVersionString ());
                        return 0;

                    case 'W': case 'w':
                        tag_next_arg = 1;
                        break;

                    case 'X': case 'x':
                        if (++x_count == 3) {
                            error_line ("illegal option: %s !", *argv);
                            ++error_count;
                            x_count = 0;
                        }

                        break;

                    case 'Y': case 'y':
                        overwrite_all = 1;
                        break;

                    default:
                        error_line ("illegal option: %c !", **argv);
                        ++error_count;
                }
        else if (x_count) {
            if (x_count == 1) {
                if (tag_extract_stdout) {
                    error_line ("can't extract more than 1 tag item to stdout at a time!");
                    ++error_count;
                }
                else
                    tag_extract_stdout = *argv;
            }
            else if (x_count == 2)
                add_tag_extraction_to_list (*argv);

            x_count = 0;
        }
        else if (tag_next_arg > 0) {
            char *cp = strchr (*argv, '=');

            if (cp && cp > *argv) {
                int i = num_tag_items;

                tag_items = realloc (tag_items, ++num_tag_items * sizeof (*tag_items));
                tag_items [i].item = malloc (cp - *argv + 1);
                memcpy (tag_items [i].item, *argv, cp - *argv);
                tag_items [i].item [cp - *argv] = 0;
                tag_items [i].vsize = (int) strlen (cp + 1);
                tag_items [i].value = malloc (tag_items [i].vsize + 1);
                strcpy (tag_items [i].value, cp + 1);
                tag_items [i].binary = (tag_next_arg == 2);
                tag_items [i].ext = NULL;
            }
            else {
                error_line ("error in tag spec: %s !", *argv);
                ++error_count;
            }

            tag_next_arg = 0;
        }
        else if (tag_next_arg < 0) {
            int i = num_tag_items;

            if (strchr (*argv, '=')) {
                error_line ("error in tag spec: %s !", *argv);
                ++error_count;
            }

            tag_items = realloc (tag_items, ++num_tag_items * sizeof (*tag_items));
            tag_items [i].item = strdup (*argv);
            tag_items [i].binary = 0;
            tag_items [i].vsize = 0;
            tag_items [i].value = "";
            tag_next_arg = 0;
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

    setup_break ();     // set up console and detect ^C and ^Break

    // check for various command-line argument problems

    if (tag_next_arg) {
        error_line ("no tag specified with %s option!",
            tag_next_arg < 0 ? "-d" : (tag_next_arg == 1 ? "-w" : "--write-binary-tag"));
        ++error_count;
    }

    if (c_count == 1) {
        if (tag_extract_stdout) {
            error_line ("can't extract more than 1 tag item to stdout at a time!");
            error_count++;
        }
        else
            tag_extract_stdout = "cuesheet";
    }

    if (num_files && !clean_tags && !import_id3 && !num_tag_items && !num_tag_extractions && !tag_extract_stdout && !list_tags) {
        error_line ("no operations specified!");
        error_count++;
    }

    if (strcmp (WavpackGetLibraryVersionString (), PACKAGE_VERSION)) {
        fprintf (stderr, version_warning, WavpackGetLibraryVersionString (), PACKAGE_VERSION);
        fflush (stderr);
    }
    else if (!quiet_mode && !error_count) {
        fprintf (stderr, sign_on, VERSION_OS, WavpackGetLibraryVersionString ());
        fflush (stderr);
    }

    // Loop through any tag specification strings and check for file access, convert text
    // strings to UTF-8, and otherwise prepare for writing to APE tags. This is done here
    // rather than after encoding so that any errors can be reported to the user now.

    for (i = 0; i < num_tag_items; ++i) {
#ifdef _WIN32
        int tag_came_from_file = 0;
#endif
        int total_tag_size = 0;

        if (*tag_items [i].value == '@') {
            char *fn = tag_items [i].value + 1, *new_value = NULL;
            FILE *file = wild_fopen (fn, "rb");

            // if the file is not found, try using any input directory that the
            // user may have specified on the command line

            if (!file && num_files && filespec_name (matches [0]) && *matches [0] != '-') {
                char *temp = malloc (strlen (matches [0]) + PATH_MAX);

                strcpy (temp, matches [0]);
                strcpy (filespec_name (temp), fn);
                file = wild_fopen (temp, "rb");
                free (temp);
            }

            if (file) {
                uint32_t bcount;

                tag_items [i].vsize = (int) DoGetFileSize (file);

                if (filespec_ext (fn))
                    tag_items [i].ext = strdup (filespec_ext (fn));

                if (tag_items [i].vsize < 1048576 * (allow_huge_tags ? 16 : 1)) {
                    new_value = malloc (tag_items [i].vsize + 2);
                    memset (new_value, 0, tag_items [i].vsize + 2);

                    if (!DoReadFile (file, new_value, tag_items [i].vsize, &bcount) ||
                        bcount != tag_items [i].vsize) {
                            free (new_value);
                            new_value = NULL;
                        }
                }

                DoCloseHandle (file);
            }

            if (!new_value) {
                error_line ("error in tag spec: %s !", tag_items [i].value);
                ++error_count;
            }
            else {
                free (tag_items [i].value);
                tag_items [i].value = new_value;
#ifdef _WIN32
                tag_came_from_file = 1;
#endif
            }
        }
        else if (tag_items [i].binary) {
            error_line ("binary tags must be from files: %s !", tag_items [i].value);
            ++error_count;
        }

        if (tag_items [i].binary) {
            int isize = (int) strlen (tag_items [i].item);
            int esize = tag_items [i].ext ? (int) strlen (tag_items [i].ext) : 0;

            tag_items [i].value = realloc (tag_items [i].value, isize + esize + 1 + tag_items [i].vsize);
            memmove (tag_items [i].value + isize + esize + 1, tag_items [i].value, tag_items [i].vsize);
            strcpy (tag_items [i].value, tag_items [i].item);

            if (tag_items [i].ext)
                strcat (tag_items [i].value, tag_items [i].ext);

            tag_items [i].vsize += isize + esize + 1;
        }
        else if (tag_items [i].vsize) {
            tag_items [i].value = realloc (tag_items [i].value, tag_items [i].vsize * 2 + 1);

#ifdef _WIN32
            if (tag_came_from_file && !no_utf8_convert)
#else
            if (!no_utf8_convert)
#endif
                TextToUTF8 (tag_items [i].value, (int) tag_items [i].vsize * 2 + 1);

            // if a UTF8 BOM gets through to here, delete it now (redundant in APEv2 tags)

            if (tag_items [i].vsize >= 3 && (unsigned char) tag_items [i].value [0] == 0xEF &&
                (unsigned char) tag_items [i].value [1] == 0xBB && (unsigned char) tag_items [i].value [2] == 0xBF) {
                    memmove (tag_items [i].value, tag_items [i].value + 3, tag_items [i].vsize -= 3);
                    tag_items [i].value [tag_items [i].vsize] = 0;
            }

            tag_items [i].vsize = (int) strlen (tag_items [i].value);
        }

        if ((total_tag_size += tag_items [i].vsize) > 1048576 * (allow_huge_tags ? 16 : 1)) {
            error_line ("total APEv2 tag size exceeds %d MB !", allow_huge_tags ? 16 : 1);
            ++error_count;
            break;
        }
    }

    if (!num_files) {
        printf ("%s", help);
        return 1;
    }

    if (error_count)
        return 1;

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

        // loop through and process files in list

        for (file_index = 0; file_index < num_files; ++file_index) {
            if (check_break ())
                break;

            if (num_files > 1 && !quiet_mode) {
                fprintf (stderr, "\n%s:\n", matches [file_index]);
                fflush (stderr);
            }

            result = process_file (matches [file_index]);

            if (result != WAVPACK_NO_ERROR)
                ++error_count;

            if (result == WAVPACK_HARD_ERROR)
                break;

            // clean up in preparation for potentially another file

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

    return error_count ? 1 : 0;
}

#ifdef _WIN32

// On Windows, this "real" main() acts as a shell to our static wavpack_main().
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

    ret = wvtag_main(argc_utf8, argv_copy);

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

static int dump_tag_item_to_file (WavpackContext *wpc, const char *tag_item, FILE *dst, char *fname);
static void list_tags_to_file (WavpackContext *wpc, char *name, FILE *dst);
static int do_tag_extractions (WavpackContext *wpc, char *outfilename);
static int calculate_tag_size (WavpackContext *wpc);
static void clear_tag_items (WavpackContext *wpc);

static int process_file (char *infilename)
{
    int open_flags = OPEN_TAGS | OPEN_DSD_NATIVE, write_tag = 0, huge_tag = 0;
    WavpackContext *wpc;
    char error [80];

    if (clean_tags || num_tag_items || import_id3)
        open_flags |= OPEN_EDIT_TAGS;

#ifdef _WIN32
    open_flags |= OPEN_FILE_UTF8;
#endif

    if (import_id3)
        open_flags |= OPEN_WRAPPER | OPEN_ALT_TYPES;

    // use library to open WavPack file

    wpc = WavpackOpenFileInput (infilename, error, open_flags, 0);

    if (!wpc) {
        error_line (error);
        return WAVPACK_SOFT_ERROR;
    }

    huge_tag = calculate_tag_size (wpc) > 1048576;

    // if user wants to start fresh, clear any items that are already there

    if (clean_tags && (WavpackGetNumTagItems (wpc) || WavpackGetNumBinaryTagItems (wpc))) {
        clear_tag_items (wpc);
        write_tag = 1;
    }

    // enter here if the user wants to make tag changes (append, change, delete, import)

    if (num_tag_items || import_id3) {
        int i, res = TRUE;

        // if the existing tag was only an ID3v1, convert it now (unless user wanted to start fresh)

        if (!clean_tags && (WavpackGetMode (wpc) & (MODE_VALID_TAG | MODE_APETAG)) == MODE_VALID_TAG) {
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
            write_tag = 1;
        }

        // this is where we import from an ID3v2 tag that appears as the trailing wrapper of DSF files

        if (import_id3) {
            char error [80];

            WavpackFreeWrapper (wpc);               // free the header, then seek for the trailer
            WavpackSeekTrailingWrapper (wpc);

            if (WavpackGetWrapperBytes (wpc) > 10) {

                // first we do the "dry run", and if that shows applicable items (and no error), do real pass

                int res = ImportID3v2 (NULL, WavpackGetWrapperData (wpc), WavpackGetWrapperBytes (wpc), error, NULL);

                if (res > 0)
                    res = ImportID3v2 (wpc, WavpackGetWrapperData (wpc), WavpackGetWrapperBytes (wpc), error, NULL);

                if (res > 0) {
                    if (!quiet_mode)
                        error_line ("successfully imported %d items from ID3v2 tag", res);

                    write_tag = 1;
                }
                else if (res == 0)
                    error_line ("ID3v2 import: no applicable items found");
                else
                    error_line ("ID3v2 import: %s", error);
            }
        }

        // go though the user input, doing any deletions first

        for (i = 0; i < num_tag_items; ++i)
            if (!tag_items [i].vsize) {
                if (!WavpackDeleteTagItem (wpc, tag_items [i].item))
                    error_line ("warning: field \"%s\" not found, can't delete", tag_items [i].item);
                else
                    write_tag = 1;
            }

        // go though the user input again, doing any appends/changes this time

        for (i = 0; i < num_tag_items; ++i)
            if (tag_items [i].vsize) {
                if (tag_items [i].binary) 
                    res = WavpackAppendBinaryTagItem (wpc, tag_items [i].item, tag_items [i].value, tag_items [i].vsize);
                else
                    res = WavpackAppendTagItem (wpc, tag_items [i].item, tag_items [i].value, tag_items [i].vsize);

                if (!res) {
                    error_line ("%s", WavpackGetErrorMessage (wpc));
                    WavpackCloseFile (wpc);
                    return WAVPACK_HARD_ERROR;
                }

                write_tag = 1;
            }

        // if the tag exceeds 1 MB (and it didn't before) and the user didn't override, error out

        if (write_tag && !huge_tag && !allow_huge_tags && calculate_tag_size (wpc) > 1048576) {
            error_line ("APEv2 tag exceeds 1 MB, use --allow-huge-tags to override");
            WavpackCloseFile (wpc);
            return WAVPACK_SOFT_ERROR;
        }
    }

    // if we actually made some changes, write the tag back out to the file now

    if (write_tag && !WavpackWriteTag (wpc)) {
        error_line ("%s", WavpackGetErrorMessage (wpc));
        WavpackCloseFile (wpc);
        return WAVPACK_HARD_ERROR;
    }

    // finally, do any extractions and/or listings last

    if (tag_extract_stdout) {
        if (!dump_tag_item_to_file (wpc, tag_extract_stdout, stdout, NULL)) {
            error_line ("tag \"%s\" not found!", tag_extract_stdout);
            WavpackCloseFile (wpc);
            return WAVPACK_SOFT_ERROR;
        }
    }
    else if (num_tag_extractions) {
        int result = do_tag_extractions (wpc, infilename);

        if (result != WAVPACK_NO_ERROR) {
            WavpackCloseFile (wpc);
            return result;
        }
    }

    if (list_tags)
        list_tags_to_file (wpc, infilename, stdout);

    WavpackCloseFile (wpc);
    return WAVPACK_NO_ERROR;
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

static void clear_tag_items (WavpackContext *wpc)
{
    if ((WavpackGetMode (wpc) & MODE_VALID_TAG) && (WavpackGetMode (wpc) & MODE_APETAG)) {
        int item_len;
        char *item;

        while (WavpackGetNumTagItems (wpc)) {
            item_len = WavpackGetTagItemIndexed (wpc, 0, NULL, 0);
            item = malloc (item_len + 1);
            WavpackGetTagItemIndexed (wpc, 0, item, item_len + 1);
            while (WavpackDeleteTagItem (wpc, item));
            free (item);
        }

        while (WavpackGetNumBinaryTagItems (wpc)) {
            item_len = WavpackGetBinaryTagItemIndexed (wpc, 0, NULL, 0);
            item = malloc (item_len + 1);
            WavpackGetBinaryTagItemIndexed (wpc, 0, item, item_len + 1);
            while (WavpackDeleteTagItem (wpc, item));
            free (item);
        }
    }
}

static void dump_UTF8_string (char *string, FILE *dst);
static void UTF8ToAnsi (char *string, int len);

static void list_tags_to_file (WavpackContext *wpc, char *name, FILE *dst)
{
    if (WavpackGetMode (wpc) & MODE_VALID_TAG) {
        int ape_tag = WavpackGetMode (wpc) & MODE_APETAG;
        int num_binary_items = WavpackGetNumBinaryTagItems (wpc);
        int num_items = WavpackGetNumTagItems (wpc), i;
        char *spaces = "                  ";

        if (num_items + num_binary_items)
            fprintf (dst, "\n%s tag items:   %d (%d bytes used)\n",
                ape_tag ? "APEv2" : "ID3v1", num_items + num_binary_items, calculate_tag_size (wpc));

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

                MD5Init (&md5_context);
                value_len = WavpackGetBinaryTagItem (wpc, item, NULL, 0);
                value = malloc (value_len);
                value_len = WavpackGetBinaryTagItem (wpc, item, value, value_len);

                for (i = 0; i < value_len; ++i)
                    if (!value [i]) {
                        MD5Update (&md5_context, (unsigned char *) value + i + 1, value_len - i - 1);
                        MD5Final (md5_result, &md5_context);
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

// Dump the specified tag field to the specified stream. Both text and binary tags may be written,
// and in Windows the appropriate file mode will be set. If the tag is not found then 0 is returned,
// otherwise the length of the data is returned, and this is true even when the file pointer is NULL
// so this can be used to determine if the tag exists before further processing.
//
// The "fname" parameter can optionally be set to a character array that will accept the suggested
// filename. This is formed by the tag item name with the extension ".txt" for text fields; for
// binary fields this is supplied by convention as a NULL terminated string at the beginning of the
// data, so this is returned. The string should have 256 characters available.

static int dump_tag_item_to_file (WavpackContext *wpc, const char *tag_item, FILE *dst, char *fname)
{
    if (WavpackGetMode (wpc) & MODE_VALID_TAG) {
        if (WavpackGetTagItem (wpc, tag_item, NULL, 0)) {
            int value_len = WavpackGetTagItem (wpc, tag_item, NULL, 0);
            char *value;

            if (fname) {
                strcpy (fname, tag_item);
                strcat (fname, ".txt");
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
                        if (i < 256)
                            strcpy (fname, value);
                        else {
                            strcpy (fname, tag_item);
                            strcat (fname, ".bin");
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

// One missing piece of libwavpack functionality is the ability to obtain the size of
// an attached APETAG. Normally an application would not need this, but we want to be
// able to have some control over how big the tags get, not to mention wanting to
// report the size to the user. This function returns the total APETAG size, including
// both header and footer (the size field in the tag itself only has one of them). For
// ID3v1 tags it returns 128.

static int calculate_tag_size (WavpackContext *wpc)
{
    int mode = WavpackGetMode (wpc);

    if ((mode & MODE_VALID_TAG) && (mode & MODE_APETAG)) {
        int num_binary_items = WavpackGetNumBinaryTagItems (wpc);
        int num_items = WavpackGetNumTagItems (wpc), i;
        int ape_tag_size = 32 * 2;
        int item_len, value_len;
        char *item;

        if (num_items + num_binary_items == 0)
            return 0;

        for (i = 0; i < num_items; ++i) {
            item_len = WavpackGetTagItemIndexed (wpc, i, NULL, 0);
            item = malloc (item_len + 1);
            WavpackGetTagItemIndexed (wpc, i, item, item_len + 1);
            value_len = WavpackGetTagItem (wpc, item, NULL, 0);
            ape_tag_size += 8 + item_len + 1 + value_len;
            free (item);
        }

        for (i = 0; i < num_binary_items; ++i) {
            item_len = WavpackGetBinaryTagItemIndexed (wpc, i, NULL, 0);
            item = malloc (item_len + 1);
            WavpackGetBinaryTagItemIndexed (wpc, i, item, item_len + 1);
            value_len = WavpackGetBinaryTagItem (wpc, item, NULL, 0);
            ape_tag_size += 8 + item_len + 1 + value_len;
            free (item);
        }

        return ape_tag_size;
    }
    else if (mode & MODE_VALID_TAG)
        return 128;     // ID3v1 tag size
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

// Convert a Unicode UTF-8 format string into its Ansi equivalent. The
// conversion is done in-place so the maximum length of the string buffer must
// be specified because the string may become longer or shorter. If the
// resulting string will not fit in the specified buffer size then it is
// truncated.

#if defined (_WIN32)
static int UTF8ToWideChar (const unsigned char *pUTF8, wchar_t *pWide);
#endif

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

#if defined(_WIN32)

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

#else

static void TextToUTF8 (void *string, int len)
{
    char *temp = malloc (len);
    char *outp = temp;
    char *inp = string;
    size_t insize = 0;
    size_t outsize = len - 1;
    int err = 0;
    char *old_locale;
    iconv_t converter;

    // simple case: test for UTF8 BOM and if so, simply delete the BOM and return

    if (len > 3 && (unsigned char) inp [0] == 0xEF && (unsigned char) inp [1] == 0xBB &&
        (unsigned char) inp [2] == 0xBF) {
            memmove (inp, inp + 3, len - 3);
            inp [len - 3] = 0;
            return;
    }

    memset(temp, 0, len);
    old_locale = setlocale (LC_CTYPE, "");

    if ((unsigned char) inp [0] == 0xFF && (unsigned char) inp [1] == 0xFE) {
        uint16_t *utf16p = (uint16_t *) (inp += 2);

        while (*utf16p++)
            insize += 2;

        converter = iconv_open ("UTF-8", "UTF-16LE");
    }
    else {
        insize = strlen (string);
        converter = iconv_open ("UTF-8", "");
    }

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
    free (temp);
}

#endif

// Special version of fopen() that allows a wildcard specification for the
// filename. If a wildcard is specified, then it must match 1 and only 1
// file to be acceptable (i.e. it won't match just the "first" file).

#if defined (_WIN32)

static FILE *wild_fopen (char *filename, const char *mode)
{
    struct _wfinddata_t _wfinddata_t;
    char *matchname = NULL;
    wchar_t *wfilename;
    FILE *res = NULL;
    intptr_t file;

    if (!filespec_wild (filename) || !filespec_name (filename))
        return fopen (filename, mode);

    wfilename = utf8_to_utf16(filename);

    if (!wfilename)
        return NULL;

    if ((file = _wfindfirst (wfilename, &_wfinddata_t)) != (intptr_t) -1) {
        do {
            if (!(_wfinddata_t.attrib & _A_SUBDIR)) {
                char *name_utf8;

                if (matchname) {
                    free (matchname);
                    matchname = NULL;
                    break;
                }
                else if ((name_utf8 = utf16_to_utf8(_wfinddata_t.name))) {
                    matchname = malloc (strlen (filename) + strlen(name_utf8));
                    strcpy (matchname, filename);
                    strcpy (filespec_name (matchname), name_utf8);
                    free (name_utf8);
                }
            }
        } while (_wfindnext (file, &_wfinddata_t) == 0);

        _findclose (file);
    }

    if (matchname) {
        res = fopen (matchname, mode);
        free (matchname);
    }

    free (wfilename);
    return res;
}

#else

static FILE *wild_fopen (char *filename, const char *mode)
{
    char *matchname = NULL;
    struct stat statbuf;
    FILE *res = NULL;
    glob_t globbuf;
    int i;

    glob (filename, 0, NULL, &globbuf);

    for (i = 0; i < globbuf.gl_pathc; ++i) {
        if (stat (globbuf.gl_pathv [i], &statbuf) == -1 || S_ISDIR (statbuf.st_mode))
            continue;

        if (matchname) {
            free (matchname);
            matchname = NULL;
            break;
        }
        else {
            matchname = malloc (strlen (globbuf.gl_pathv [i]) + 10);
            strcpy (matchname, globbuf.gl_pathv [i]);
        }
    }

    globfree (&globbuf);

    if (matchname) {
        res = fopen (matchname, mode);
        free (matchname);
    }

    return res;
}

#endif

