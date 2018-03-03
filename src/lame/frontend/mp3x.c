/* $Id: mp3x.c,v 1.28 2010/04/08 11:07:50 robert Exp $ */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>

#include "lame.h"
#include "machine.h"
#include "encoder.h"
#include "lame-analysis.h"
#include <gtk/gtk.h>
#include "parse.h"
#include "get_audio.h"
#include "gtkanal.h"
#include "lametime.h"

#include "main.h"
#include "console.h"


/************************************************************************
*
* main
*
* PURPOSE:  MPEG-1,2 Layer III encoder with GPSYCHO
* psychoacoustic model.
*
************************************************************************/
int
lame_main(lame_t gf, int argc, char **argv)
{
    unsigned char mp3buffer[LAME_MAXMP3BUFFER];
    char    outPath[PATH_MAX + 1];
    char    inPath[PATH_MAX + 1];
    int     ret;

    lame_set_errorf(gf, &frontend_errorf);
    lame_set_debugf(gf, &frontend_debugf);
    lame_set_msgf(gf, &frontend_msgf);
    if (argc <= 1) {
        usage(stderr, argv[0]); /* no command-line args  */
        return -1;
    }
    ret = parse_args(gf, argc, argv, inPath, outPath, NULL, NULL);
    if (ret < 0) {
        return ret == -2 ? 0 : 1;
    }
    (void) lame_set_analysis(gf, 1);

    if (init_infile(gf, inPath) < 0) {
        error_printf("Can't init infile '%s'\n", inPath);
        return 1;
    }
    lame_init_params(gf);
    lame_print_config(gf);

    gtk_init(&argc, &argv);
    gtkcontrol(gf, inPath);

    lame_encode_flush(gf, mp3buffer, sizeof(mp3buffer));
    close_infile();
    return 0;
}
