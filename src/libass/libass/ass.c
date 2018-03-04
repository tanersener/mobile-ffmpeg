/*
 * Copyright (C) 2006 Evgeniy Stepanov <eugeni.stepanov@gmail.com>
 *
 * This file is part of libass.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "config.h"
#include "ass_compat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <inttypes.h>

#ifdef CONFIG_ICONV
#include <iconv.h>
#endif

#include "ass.h"
#include "ass_utils.h"
#include "ass_library.h"
#include "ass_string.h"

#define ass_atof(STR) (ass_strtod((STR),NULL))

typedef enum {
    PST_UNKNOWN = 0,
    PST_INFO,
    PST_STYLES,
    PST_EVENTS,
    PST_FONTS
} ParserState;

struct parser_priv {
    ParserState state;
    char *fontname;
    char *fontdata;
    int fontdata_size;
    int fontdata_used;

    // contains bitmap of ReadOrder IDs of all read events
    uint32_t *read_order_bitmap;
    int read_order_elems; // size in uint32_t units of read_order_bitmap
    int check_readorder;
};

#define ASS_STYLES_ALLOC 20

int ass_library_version(void)
{
    return LIBASS_VERSION;
}

void ass_free_track(ASS_Track *track)
{
    int i;

    if (track->parser_priv) {
        free(track->parser_priv->read_order_bitmap);
        free(track->parser_priv->fontname);
        free(track->parser_priv->fontdata);
        free(track->parser_priv);
    }
    free(track->style_format);
    free(track->event_format);
    free(track->Language);
    if (track->styles) {
        for (i = 0; i < track->n_styles; ++i)
            ass_free_style(track, i);
    }
    free(track->styles);
    if (track->events) {
        for (i = 0; i < track->n_events; ++i)
            ass_free_event(track, i);
    }
    free(track->events);
    free(track->name);
    free(track);
}

/// \brief Allocate a new style struct
/// \param track track
/// \return style id
int ass_alloc_style(ASS_Track *track)
{
    int sid;

    assert(track->n_styles <= track->max_styles);

    if (track->n_styles == track->max_styles) {
        track->max_styles += ASS_STYLES_ALLOC;
        track->styles =
            (ASS_Style *) realloc(track->styles,
                                  sizeof(ASS_Style) *
                                  track->max_styles);
    }

    sid = track->n_styles++;
    memset(track->styles + sid, 0, sizeof(ASS_Style));
    return sid;
}

/// \brief Allocate a new event struct
/// \param track track
/// \return event id
int ass_alloc_event(ASS_Track *track)
{
    int eid;

    assert(track->n_events <= track->max_events);

    if (track->n_events == track->max_events) {
        track->max_events = track->max_events * 2 + 1;
        track->events =
            (ASS_Event *) realloc(track->events,
                                  sizeof(ASS_Event) *
                                  track->max_events);
    }

    eid = track->n_events++;
    memset(track->events + eid, 0, sizeof(ASS_Event));
    return eid;
}

void ass_free_event(ASS_Track *track, int eid)
{
    ASS_Event *event = track->events + eid;

    free(event->Name);
    free(event->Effect);
    free(event->Text);
    free(event->render_priv);
}

void ass_free_style(ASS_Track *track, int sid)
{
    ASS_Style *style = track->styles + sid;

    free(style->Name);
    free(style->FontName);
}

static int resize_read_order_bitmap(ASS_Track *track, int max_id)
{
    // Don't allow malicious files to OOM us easily. Also avoids int overflows.
    if (max_id < 0 || max_id >= 10 * 1024 * 1024 * 8)
        goto fail;
    if (max_id >= track->parser_priv->read_order_elems * 32) {
        int oldelems = track->parser_priv->read_order_elems;
        int elems = ((max_id + 31) / 32 + 1) * 2;
        assert(elems >= oldelems);
        track->parser_priv->read_order_elems = elems;
        void *new_bitmap =
            realloc(track->parser_priv->read_order_bitmap, elems * 4);
        if (!new_bitmap)
            goto fail;
        track->parser_priv->read_order_bitmap = new_bitmap;
        memset(track->parser_priv->read_order_bitmap + oldelems, 0,
               (elems - oldelems) * 4);
    }
    return 0;

fail:
    free(track->parser_priv->read_order_bitmap);
    track->parser_priv->read_order_bitmap = NULL;
    track->parser_priv->read_order_elems = 0;
    return -1;
}

static int test_and_set_read_order_bit(ASS_Track *track, int id)
{
    if (resize_read_order_bitmap(track, id) < 0)
        return -1;
    int index = id / 32;
    int bit = 1 << (id % 32);
    if (track->parser_priv->read_order_bitmap[index] & bit)
        return 1;
    track->parser_priv->read_order_bitmap[index] |= bit;
    return 0;
}

// ==============================================================================================

/**
 * \brief Set up default style
 * \param style style to edit to defaults
 * The parameters are mostly taken directly from VSFilter source for
 * best compatibility.
 */
static void set_default_style(ASS_Style *style)
{
    style->Name             = strdup("Default");
    style->FontName         = strdup("Arial");
    style->FontSize         = 18;
    style->PrimaryColour    = 0xffffff00;
    style->SecondaryColour  = 0x00ffff00;
    style->OutlineColour    = 0x00000000;
    style->BackColour       = 0x00000080;
    style->Bold             = 200;
    style->ScaleX           = 1.0;
    style->ScaleY           = 1.0;
    style->Spacing          = 0;
    style->BorderStyle      = 1;
    style->Outline          = 2;
    style->Shadow           = 3;
    style->Alignment        = 2;
    style->MarginL = style->MarginR = style->MarginV = 20;
}

static long long string2timecode(ASS_Library *library, char *p)
{
    int h, m, s, ms;
    long long tm;
    int res = sscanf(p, "%d:%d:%d.%d", &h, &m, &s, &ms);
    if (res < 4) {
        ass_msg(library, MSGL_WARN, "Bad timestamp");
        return 0;
    }
    tm = ((h * 60LL + m) * 60 + s) * 1000 + ms * 10LL;
    return tm;
}

#define NEXT(str,token) \
	token = next_token(&str); \
	if (!token) break;


#define ALIAS(alias,name) \
        if (ass_strcasecmp(tname, #alias) == 0) {tname = #name;}

/* One section started with PARSE_START and PARSE_END parses a single token
 * (contained in the variable named token) for the header indicated by the
 * variable tname. It does so by chaining a number of else-if statements, each
 * of which checks if the tname variable indicates that this header should be
 * parsed. The first parameter of the macro gives the name of the header.
 *
 * The string that is passed is in str. str is advanced to the next token if
 * a header could be parsed. The parsed results are stored in the variable
 * target, which has the type ASS_Style* or ASS_Event*.
 */
#define PARSE_START if (0) {
#define PARSE_END   }

#define ANYVAL(name,func) \
	} else if (ass_strcasecmp(tname, #name) == 0) { \
		target->name = func(token);

#define STRVAL(name) \
	} else if (ass_strcasecmp(tname, #name) == 0) { \
		if (target->name != NULL) free(target->name); \
		target->name = strdup(token);

#define STARREDSTRVAL(name) \
    } else if (ass_strcasecmp(tname, #name) == 0) { \
        if (target->name != NULL) free(target->name); \
        while (*token == '*') ++token; \
        target->name = strdup(token);

#define COLORVAL(name) ANYVAL(name,parse_color_header)
#define INTVAL(name) ANYVAL(name,atoi)
#define FPVAL(name) ANYVAL(name,ass_atof)
#define TIMEVAL(name) \
	} else if (ass_strcasecmp(tname, #name) == 0) { \
		target->name = string2timecode(track->library, token);

#define STYLEVAL(name) \
	} else if (ass_strcasecmp(tname, #name) == 0) { \
		target->name = lookup_style(track, token);

static char *next_token(char **str)
{
    char *p = *str;
    char *start;
    skip_spaces(&p);
    if (*p == '\0') {
        *str = p;
        return 0;
    }
    start = p;                  // start of the token
    for (; (*p != '\0') && (*p != ','); ++p) {
    }
    if (*p == '\0') {
        *str = p;               // eos found, str will point to '\0' at exit
    } else {
        *p = '\0';
        *str = p + 1;           // ',' found, str will point to the next char (beginning of the next token)
    }
    rskip_spaces(&p, start);    // end of current token: the first space character, or '\0'
    *p = '\0';
    return start;
}

/**
 * \brief Parse the tail of Dialogue line
 * \param track track
 * \param event parsed data goes here
 * \param str string to parse, zero-terminated
 * \param n_ignored number of format options to skip at the beginning
*/
static int process_event_tail(ASS_Track *track, ASS_Event *event,
                              char *str, int n_ignored)
{
    char *token;
    char *tname;
    char *p = str;
    int i;
    ASS_Event *target = event;

    char *format = strdup(track->event_format);
    char *q = format;           // format scanning pointer

    if (track->n_styles == 0) {
        // add "Default" style to the end
        // will be used if track does not contain a default style (or even does not contain styles at all)
        int sid = ass_alloc_style(track);
        set_default_style(&track->styles[sid]);
        track->default_style = sid;
    }

    for (i = 0; i < n_ignored; ++i) {
        NEXT(q, tname);
    }

    while (1) {
        NEXT(q, tname);
        if (ass_strcasecmp(tname, "Text") == 0) {
            char *last;
            event->Text = strdup(p);
            if (*event->Text != 0) {
                last = event->Text + strlen(event->Text) - 1;
                if (last >= event->Text && *last == '\r')
                    *last = 0;
            }
            event->Duration -= event->Start;
            free(format);
            return 0;           // "Text" is always the last
        }
        NEXT(p, token);

        ALIAS(End, Duration)    // temporarily store end timecode in event->Duration
        PARSE_START
            INTVAL(Layer)
            STYLEVAL(Style)
            STRVAL(Name)
            STRVAL(Effect)
            INTVAL(MarginL)
            INTVAL(MarginR)
            INTVAL(MarginV)
            TIMEVAL(Start)
            TIMEVAL(Duration)
        PARSE_END
    }
    free(format);
    return 1;
}

/**
 * \brief Parse command line style overrides (--ass-force-style option)
 * \param track track to apply overrides to
 * The format for overrides is [StyleName.]Field=Value
 */
void ass_process_force_style(ASS_Track *track)
{
    char **fs, *eq, *dt, *style, *tname, *token;
    ASS_Style *target;
    int sid;
    char **list = track->library->style_overrides;

    if (!list)
        return;

    for (fs = list; *fs; ++fs) {
        eq = strrchr(*fs, '=');
        if (!eq)
            continue;
        *eq = '\0';
        token = eq + 1;

        if (!ass_strcasecmp(*fs, "PlayResX"))
            track->PlayResX = atoi(token);
        else if (!ass_strcasecmp(*fs, "PlayResY"))
            track->PlayResY = atoi(token);
        else if (!ass_strcasecmp(*fs, "Timer"))
            track->Timer = ass_atof(token);
        else if (!ass_strcasecmp(*fs, "WrapStyle"))
            track->WrapStyle = atoi(token);
        else if (!ass_strcasecmp(*fs, "ScaledBorderAndShadow"))
            track->ScaledBorderAndShadow = parse_bool(token);
        else if (!ass_strcasecmp(*fs, "Kerning"))
            track->Kerning = parse_bool(token);
        else if (!ass_strcasecmp(*fs, "YCbCr Matrix"))
            track->YCbCrMatrix = parse_ycbcr_matrix(token);

        dt = strrchr(*fs, '.');
        if (dt) {
            *dt = '\0';
            style = *fs;
            tname = dt + 1;
        } else {
            style = NULL;
            tname = *fs;
        }
        for (sid = 0; sid < track->n_styles; ++sid) {
            if (style == NULL
                || ass_strcasecmp(track->styles[sid].Name, style) == 0) {
                target = track->styles + sid;
                PARSE_START
                    STRVAL(FontName)
                    COLORVAL(PrimaryColour)
                    COLORVAL(SecondaryColour)
                    COLORVAL(OutlineColour)
                    COLORVAL(BackColour)
                    FPVAL(FontSize)
                    INTVAL(Bold)
                    INTVAL(Italic)
                    INTVAL(Underline)
                    INTVAL(StrikeOut)
                    FPVAL(Spacing)
                    FPVAL(Angle)
                    INTVAL(BorderStyle)
                    INTVAL(Alignment)
                    INTVAL(Justify)
                    INTVAL(MarginL)
                    INTVAL(MarginR)
                    INTVAL(MarginV)
                    INTVAL(Encoding)
                    FPVAL(ScaleX)
                    FPVAL(ScaleY)
                    FPVAL(Outline)
                    FPVAL(Shadow)
                    FPVAL(Blur)
                PARSE_END
            }
        }
        *eq = '=';
        if (dt)
            *dt = '.';
    }
}

/**
 * \brief Parse the Style line
 * \param track track
 * \param str string to parse, zero-terminated
 * Allocates a new style struct.
*/
static int process_style(ASS_Track *track, char *str)
{

    char *token;
    char *tname;
    char *p = str;
    char *format;
    char *q;                    // format scanning pointer
    int sid;
    ASS_Style *style;
    ASS_Style *target;

    if (!track->style_format) {
        // no style format header
        // probably an ancient script version
        if (track->track_type == TRACK_TYPE_SSA)
            track->style_format =
                strdup
                ("Name, Fontname, Fontsize, PrimaryColour, SecondaryColour,"
                 "TertiaryColour, BackColour, Bold, Italic, BorderStyle, Outline,"
                 "Shadow, Alignment, MarginL, MarginR, MarginV, AlphaLevel, Encoding");
        else
            track->style_format =
                strdup
                ("Name, Fontname, Fontsize, PrimaryColour, SecondaryColour,"
                 "OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut,"
                 "ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow,"
                 "Alignment, MarginL, MarginR, MarginV, Encoding");
    }

    q = format = strdup(track->style_format);

    // Add default style first
    if (track->n_styles == 0) {
        // will be used if track does not contain a default style (or even does not contain styles at all)
        int sid = ass_alloc_style(track);
        set_default_style(&track->styles[sid]);
        track->default_style = sid;
    }

    ass_msg(track->library, MSGL_V, "[%p] Style: %s", track, str);

    sid = ass_alloc_style(track);

    style = track->styles + sid;
    target = style;

    // fill style with some default values
    style->ScaleX = 100.;
    style->ScaleY = 100.;

    while (1) {
        NEXT(q, tname);
        NEXT(p, token);

        PARSE_START
            STARREDSTRVAL(Name)
            if (strcmp(target->Name, "Default") == 0)
                track->default_style = sid;
            STRVAL(FontName)
            COLORVAL(PrimaryColour)
            COLORVAL(SecondaryColour)
            COLORVAL(OutlineColour) // TertiaryColor
            COLORVAL(BackColour)
            // SSA uses BackColour for both outline and shadow
            // this will destroy SSA's TertiaryColour, but i'm not going to use it anyway
            if (track->track_type == TRACK_TYPE_SSA)
                target->OutlineColour = target->BackColour;
            FPVAL(FontSize)
            INTVAL(Bold)
            INTVAL(Italic)
            INTVAL(Underline)
            INTVAL(StrikeOut)
            FPVAL(Spacing)
            FPVAL(Angle)
            INTVAL(BorderStyle)
            INTVAL(Alignment)
            if (track->track_type == TRACK_TYPE_ASS)
                target->Alignment = numpad2align(target->Alignment);
            // VSFilter compatibility
            else if (target->Alignment == 8)
                target->Alignment = 3;
            else if (target->Alignment == 4)
                target->Alignment = 11;
            INTVAL(MarginL)
            INTVAL(MarginR)
            INTVAL(MarginV)
            INTVAL(Encoding)
            FPVAL(ScaleX)
            FPVAL(ScaleY)
            FPVAL(Outline)
            FPVAL(Shadow)
        PARSE_END
    }
    style->ScaleX = FFMAX(style->ScaleX, 0.) / 100.;
    style->ScaleY = FFMAX(style->ScaleY, 0.) / 100.;
    style->Spacing = FFMAX(style->Spacing, 0.);
    style->Outline = FFMAX(style->Outline, 0.);
    style->Shadow = FFMAX(style->Shadow, 0.);
    style->Bold = !!style->Bold;
    style->Italic = !!style->Italic;
    style->Underline = !!style->Underline;
    style->StrikeOut = !!style->StrikeOut;
    if (!style->Name)
        style->Name = strdup("Default");
    if (!style->FontName)
        style->FontName = strdup("Arial");
    free(format);
    return 0;

}

static int process_styles_line(ASS_Track *track, char *str)
{
    if (!strncmp(str, "Format:", 7)) {
        char *p = str + 7;
        skip_spaces(&p);
        free(track->style_format);
        track->style_format = strdup(p);
        ass_msg(track->library, MSGL_DBG2, "Style format: %s",
               track->style_format);
    } else if (!strncmp(str, "Style:", 6)) {
        char *p = str + 6;
        skip_spaces(&p);
        process_style(track, p);
    }
    return 0;
}

static int process_info_line(ASS_Track *track, char *str)
{
    if (!strncmp(str, "PlayResX:", 9)) {
        track->PlayResX = atoi(str + 9);
    } else if (!strncmp(str, "PlayResY:", 9)) {
        track->PlayResY = atoi(str + 9);
    } else if (!strncmp(str, "Timer:", 6)) {
        track->Timer = ass_atof(str + 6);
    } else if (!strncmp(str, "WrapStyle:", 10)) {
        track->WrapStyle = atoi(str + 10);
    } else if (!strncmp(str, "ScaledBorderAndShadow:", 22)) {
        track->ScaledBorderAndShadow = parse_bool(str + 22);
    } else if (!strncmp(str, "Kerning:", 8)) {
        track->Kerning = parse_bool(str + 8);
    } else if (!strncmp(str, "YCbCr Matrix:", 13)) {
        track->YCbCrMatrix = parse_ycbcr_matrix(str + 13);
    } else if (!strncmp(str, "Language:", 9)) {
        char *p = str + 9;
        while (*p && ass_isspace(*p)) p++;
        free(track->Language);
        track->Language = strndup(p, 2);
    }
    return 0;
}

static void event_format_fallback(ASS_Track *track)
{
    track->parser_priv->state = PST_EVENTS;
    if (track->track_type == TRACK_TYPE_SSA)
        track->event_format = strdup("Marked, Start, End, Style, "
            "Name, MarginL, MarginR, MarginV, Effect, Text");
    else
        track->event_format = strdup("Layer, Start, End, Style, "
            "Actor, MarginL, MarginR, MarginV, Effect, Text");
    ass_msg(track->library, MSGL_V,
            "No event format found, using fallback");
}

static int process_events_line(ASS_Track *track, char *str)
{
    if (!strncmp(str, "Format:", 7)) {
        char *p = str + 7;
        skip_spaces(&p);
        free(track->event_format);
        track->event_format = strdup(p);
        ass_msg(track->library, MSGL_DBG2, "Event format: %s", track->event_format);
    } else if (!strncmp(str, "Dialogue:", 9)) {
        // This should never be reached for embedded subtitles.
        // They have slightly different format and are parsed in ass_process_chunk,
        // called directly from demuxer
        int eid;
        ASS_Event *event;

        str += 9;
        skip_spaces(&str);

        eid = ass_alloc_event(track);
        event = track->events + eid;

        // We can't parse events with event_format
        if (!track->event_format)
            event_format_fallback(track);

        process_event_tail(track, event, str, 0);
    } else {
        ass_msg(track->library, MSGL_V, "Not understood: '%.30s'", str);
    }
    return 0;
}

static unsigned char *decode_chars(const unsigned char *src,
                                   unsigned char *dst, int cnt_in)
{
    uint32_t value = 0;
    for (int i = 0; i < cnt_in; i++)
        value |= (uint32_t) ((src[i] - 33u) & 63) << 6 * (3 - i);

    *dst++ = value >> 16;
    if (cnt_in >= 3)
        *dst++ = value >> 8 & 0xff;
    if (cnt_in >= 4)
        *dst++ = value & 0xff;
    return dst;
}

static int decode_font(ASS_Track *track)
{
    unsigned char *p;
    unsigned char *q;
    int i;
    int size;                   // original size
    int dsize;                  // decoded size
    unsigned char *buf = 0;

    ass_msg(track->library, MSGL_V, "Font: %d bytes encoded data",
            track->parser_priv->fontdata_used);
    size = track->parser_priv->fontdata_used;
    if (size % 4 == 1) {
        ass_msg(track->library, MSGL_ERR, "Bad encoded data size");
        goto error_decode_font;
    }
    buf = malloc(size / 4 * 3 + FFMAX(size % 4 - 1, 0));
    if (!buf)
        goto error_decode_font;
    q = buf;
    for (i = 0, p = (unsigned char *) track->parser_priv->fontdata;
         i < size / 4; i++, p += 4) {
        q = decode_chars(p, q, 4);
    }
    if (size % 4 == 2) {
        q = decode_chars(p, q, 2);
    } else if (size % 4 == 3) {
        q = decode_chars(p, q, 3);
    }
    dsize = q - buf;
    assert(dsize == size / 4 * 3 + FFMAX(size % 4 - 1, 0));

    if (track->library->extract_fonts) {
        ass_add_font(track->library, track->parser_priv->fontname,
                     (char *) buf, dsize);
    }

error_decode_font:
    free(buf);
    free(track->parser_priv->fontname);
    free(track->parser_priv->fontdata);
    track->parser_priv->fontname = 0;
    track->parser_priv->fontdata = 0;
    track->parser_priv->fontdata_size = 0;
    track->parser_priv->fontdata_used = 0;
    return 0;
}

static int process_fonts_line(ASS_Track *track, char *str)
{
    int len;

    if (!strncmp(str, "fontname:", 9)) {
        char *p = str + 9;
        skip_spaces(&p);
        if (track->parser_priv->fontname) {
            decode_font(track);
        }
        track->parser_priv->fontname = strdup(p);
        ass_msg(track->library, MSGL_V, "Fontname: %s",
               track->parser_priv->fontname);
        return 0;
    }

    if (!track->parser_priv->fontname) {
        ass_msg(track->library, MSGL_V, "Not understood: '%s'", str);
        return 0;
    }

    len = strlen(str);
    if (track->parser_priv->fontdata_used + len >
        track->parser_priv->fontdata_size) {
        track->parser_priv->fontdata_size += FFMAX(len, 100 * 1024);
        track->parser_priv->fontdata =
            realloc(track->parser_priv->fontdata,
                    track->parser_priv->fontdata_size);
    }
    memcpy(track->parser_priv->fontdata + track->parser_priv->fontdata_used,
           str, len);
    track->parser_priv->fontdata_used += len;

    return 0;
}

/**
 * \brief Parse a header line
 * \param track track
 * \param str string to parse, zero-terminated
*/
static int process_line(ASS_Track *track, char *str)
{
    if (!ass_strncasecmp(str, "[Script Info]", 13)) {
        track->parser_priv->state = PST_INFO;
    } else if (!ass_strncasecmp(str, "[V4 Styles]", 11)) {
        track->parser_priv->state = PST_STYLES;
        track->track_type = TRACK_TYPE_SSA;
    } else if (!ass_strncasecmp(str, "[V4+ Styles]", 12)) {
        track->parser_priv->state = PST_STYLES;
        track->track_type = TRACK_TYPE_ASS;
    } else if (!ass_strncasecmp(str, "[Events]", 8)) {
        track->parser_priv->state = PST_EVENTS;
    } else if (!ass_strncasecmp(str, "[Fonts]", 7)) {
        track->parser_priv->state = PST_FONTS;
    } else {
        switch (track->parser_priv->state) {
        case PST_INFO:
            process_info_line(track, str);
            break;
        case PST_STYLES:
            process_styles_line(track, str);
            break;
        case PST_EVENTS:
            process_events_line(track, str);
            break;
        case PST_FONTS:
            process_fonts_line(track, str);
            break;
        default:
            break;
        }
    }
    return 0;
}

static int process_text(ASS_Track *track, char *str)
{
    char *p = str;
    while (1) {
        char *q;
        while (1) {
            if ((*p == '\r') || (*p == '\n'))
                ++p;
            else if (p[0] == '\xef' && p[1] == '\xbb' && p[2] == '\xbf')
                p += 3;         // U+FFFE (BOM)
            else
                break;
        }
        for (q = p; ((*q != '\0') && (*q != '\r') && (*q != '\n')); ++q) {
        };
        if (q == p)
            break;
        if (*q != '\0')
            *(q++) = '\0';
        process_line(track, p);
        if (*q == '\0')
            break;
        p = q;
    }
    // there is no explicit end-of-font marker in ssa/ass
    if (track->parser_priv->fontname)
        decode_font(track);
    return 0;
}

/**
 * \brief Process a chunk of subtitle stream data.
 * \param track track
 * \param data string to parse
 * \param size length of data
*/
void ass_process_data(ASS_Track *track, char *data, int size)
{
    char *str = malloc(size + 1);
    if (!str)
        return;

    memcpy(str, data, size);
    str[size] = '\0';

    ass_msg(track->library, MSGL_V, "Event: %s", str);
    process_text(track, str);
    free(str);
}

/**
 * \brief Process CodecPrivate section of subtitle stream
 * \param track track
 * \param data string to parse
 * \param size length of data
 CodecPrivate section contains [Stream Info] and [V4+ Styles] ([V4 Styles] for SSA) sections
*/
void ass_process_codec_private(ASS_Track *track, char *data, int size)
{
    ass_process_data(track, data, size);

    // probably an mkv produced by ancient mkvtoolnix
    // such files don't have [Events] and Format: headers
    if (!track->event_format)
        event_format_fallback(track);

    ass_process_force_style(track);
}

static int check_duplicate_event(ASS_Track *track, int ReadOrder)
{
    if (track->parser_priv->read_order_bitmap)
        return test_and_set_read_order_bit(track, ReadOrder) > 0;
    // ignoring last event, it is the one we are comparing with
    for (int i = 0; i < track->n_events - 1; i++)
        if (track->events[i].ReadOrder == ReadOrder)
            return 1;
    return 0;
}

void ass_set_check_readorder(ASS_Track *track, int check_readorder)
{
    track->parser_priv->check_readorder = check_readorder == 1;
}

/**
 * \brief Process a chunk of subtitle stream data. In Matroska, this contains exactly 1 event (or a commentary).
 * \param track track
 * \param data string to parse
 * \param size length of data
 * \param timecode starting time of the event (milliseconds)
 * \param duration duration of the event (milliseconds)
*/
void ass_process_chunk(ASS_Track *track, char *data, int size,
                       long long timecode, long long duration)
{
    char *str;
    int eid;
    char *p;
    char *token;
    ASS_Event *event;
    int check_readorder = track->parser_priv->check_readorder;

    if (check_readorder && !track->parser_priv->read_order_bitmap) {
        for (int i = 0; i < track->n_events; i++) {
            if (test_and_set_read_order_bit(track, track->events[i].ReadOrder) < 0)
                break;
        }
    }

    if (!track->event_format) {
        ass_msg(track->library, MSGL_WARN, "Event format header missing");
        return;
    }

    str = malloc(size + 1);
    if (!str)
        return;
    memcpy(str, data, size);
    str[size] = '\0';
    ass_msg(track->library, MSGL_V, "Event at %" PRId64 ", +%" PRId64 ": %s",
           (int64_t) timecode, (int64_t) duration, str);

    eid = ass_alloc_event(track);
    event = track->events + eid;

    p = str;

    do {
        NEXT(p, token);
        event->ReadOrder = atoi(token);
        if (check_readorder && check_duplicate_event(track, event->ReadOrder))
            break;

        NEXT(p, token);
        event->Layer = atoi(token);

        process_event_tail(track, event, p, 3);

        event->Start = timecode;
        event->Duration = duration;

        free(str);
        return;
//              dump_events(tid);
    } while (0);
    // some error
    ass_free_event(track, eid);
    track->n_events--;
    free(str);
}

/**
 * \brief Flush buffered events.
 * \param track track
*/
void ass_flush_events(ASS_Track *track)
{
    if (track->events) {
        int eid;
        for (eid = 0; eid < track->n_events; eid++)
            ass_free_event(track, eid);
        track->n_events = 0;
    }
    free(track->parser_priv->read_order_bitmap);
    track->parser_priv->read_order_bitmap = NULL;
    track->parser_priv->read_order_elems = 0;
}

#ifdef CONFIG_ICONV
/** \brief recode buffer to utf-8
 * constraint: codepage != 0
 * \param data pointer to text buffer
 * \param size buffer size
 * \return a pointer to recoded buffer, caller is responsible for freeing it
**/
static char *sub_recode(ASS_Library *library, char *data, size_t size,
                        char *codepage)
{
    iconv_t icdsc;
    char *tocp = "UTF-8";
    char *outbuf;
    assert(codepage);

    if ((icdsc = iconv_open(tocp, codepage)) != (iconv_t) (-1)) {
        ass_msg(library, MSGL_V, "Opened iconv descriptor");
    } else {
        ass_msg(library, MSGL_ERR, "Error opening iconv descriptor");
        return NULL;
    }

    {
        size_t osize = size;
        size_t ileft = size;
        size_t oleft = size - 1;
        char *ip;
        char *op;
        size_t rc;
        int clear = 0;

        outbuf = malloc(osize);
        if (!outbuf)
            goto out;
        ip = data;
        op = outbuf;

        while (1) {
            if (ileft)
                rc = iconv(icdsc, &ip, &ileft, &op, &oleft);
            else {              // clear the conversion state and leave
                clear = 1;
                rc = iconv(icdsc, NULL, NULL, &op, &oleft);
            }
            if (rc == (size_t) (-1)) {
                if (errno == E2BIG) {
                    size_t offset = op - outbuf;
                    char *nbuf = realloc(outbuf, osize + size);
                    if (!nbuf) {
                        free(outbuf);
                        outbuf = 0;
                        goto out;
                    }
                    outbuf = nbuf;
                    op = outbuf + offset;
                    osize += size;
                    oleft += size;
                } else {
                    ass_msg(library, MSGL_WARN, "Error recoding file");
                    free(outbuf);
                    outbuf = NULL;
                    goto out;
                }
            } else if (clear)
                break;
        }
        outbuf[osize - oleft - 1] = 0;
    }

out:
    if (icdsc != (iconv_t) (-1)) {
        (void) iconv_close(icdsc);
        ass_msg(library, MSGL_V, "Closed iconv descriptor");
    }

    return outbuf;
}
#endif                          // ICONV

/**
 * \brief read file contents into newly allocated buffer
 * \param fname file name
 * \param bufsize out: file size
 * \return pointer to file contents. Caller is responsible for its deallocation.
 */
char *read_file(ASS_Library *library, char *fname, size_t *bufsize)
{
    int res;
    long sz;
    long bytes_read;
    char *buf;

    FILE *fp = fopen(fname, "rb");
    if (!fp) {
        ass_msg(library, MSGL_WARN,
                "ass_read_file(%s): fopen failed", fname);
        return 0;
    }
    res = fseek(fp, 0, SEEK_END);
    if (res == -1) {
        ass_msg(library, MSGL_WARN,
                "ass_read_file(%s): fseek failed", fname);
        fclose(fp);
        return 0;
    }

    sz = ftell(fp);
    rewind(fp);

    ass_msg(library, MSGL_V, "File size: %ld", sz);

    buf = sz < SIZE_MAX ? malloc(sz + 1) : NULL;
    if (!buf) {
        fclose(fp);
        return NULL;
    }
    assert(buf);
    bytes_read = 0;
    do {
        res = fread(buf + bytes_read, 1, sz - bytes_read, fp);
        if (res <= 0) {
            ass_msg(library, MSGL_INFO, "Read failed, %d: %s", errno,
                    strerror(errno));
            fclose(fp);
            free(buf);
            return 0;
        }
        bytes_read += res;
    } while (sz - bytes_read > 0);
    buf[sz] = '\0';
    fclose(fp);

    if (bufsize)
        *bufsize = sz;
    return buf;
}

/*
 * \param buf pointer to subtitle text in utf-8
 */
static ASS_Track *parse_memory(ASS_Library *library, char *buf)
{
    ASS_Track *track;
    int i;

    track = ass_new_track(library);

    // process header
    process_text(track, buf);

    // external SSA/ASS subs does not have ReadOrder field
    for (i = 0; i < track->n_events; ++i)
        track->events[i].ReadOrder = i;

    if (track->track_type == TRACK_TYPE_UNKNOWN) {
        ass_free_track(track);
        return 0;
    }

    ass_process_force_style(track);

    return track;
}

/**
 * \brief Read subtitles from memory.
 * \param library libass library object
 * \param buf pointer to subtitles text
 * \param bufsize size of buffer
 * \param codepage recode buffer contents from given codepage
 * \return newly allocated track
*/
ASS_Track *ass_read_memory(ASS_Library *library, char *buf,
                           size_t bufsize, char *codepage)
{
    ASS_Track *track;
    int copied = 0;

    if (!buf)
        return 0;

#ifdef CONFIG_ICONV
    if (codepage) {
        buf = sub_recode(library, buf, bufsize, codepage);
        if (!buf)
            return 0;
        else
            copied = 1;
    }
#endif
    if (!copied) {
        char *newbuf = malloc(bufsize + 1);
        if (!newbuf)
            return 0;
        memcpy(newbuf, buf, bufsize);
        newbuf[bufsize] = '\0';
        buf = newbuf;
    }
    track = parse_memory(library, buf);
    free(buf);
    if (!track)
        return 0;

    ass_msg(library, MSGL_INFO, "Added subtitle file: "
            "<memory> (%d styles, %d events)",
            track->n_styles, track->n_events);
    return track;
}

static char *read_file_recode(ASS_Library *library, char *fname,
                              char *codepage, size_t *size)
{
    char *buf;
    size_t bufsize;

    buf = read_file(library, fname, &bufsize);
    if (!buf)
        return 0;
#ifdef CONFIG_ICONV
    if (codepage) {
        char *tmpbuf = sub_recode(library, buf, bufsize, codepage);
        free(buf);
        buf = tmpbuf;
    }
    if (!buf)
        return 0;
#endif
    *size = bufsize;
    return buf;
}

/**
 * \brief Read subtitles from file.
 * \param library libass library object
 * \param fname file name
 * \param codepage recode buffer contents from given codepage
 * \return newly allocated track
*/
ASS_Track *ass_read_file(ASS_Library *library, char *fname,
                         char *codepage)
{
    char *buf;
    ASS_Track *track;
    size_t bufsize;

    buf = read_file_recode(library, fname, codepage, &bufsize);
    if (!buf)
        return 0;
    track = parse_memory(library, buf);
    free(buf);
    if (!track)
        return 0;

    track->name = strdup(fname);

    ass_msg(library, MSGL_INFO,
            "Added subtitle file: '%s' (%d styles, %d events)",
            fname, track->n_styles, track->n_events);

    return track;
}

/**
 * \brief read styles from file into already initialized track
 */
int ass_read_styles(ASS_Track *track, char *fname, char *codepage)
{
    char *buf;
    ParserState old_state;
    size_t sz;

    buf = read_file(track->library, fname, &sz);
    if (!buf)
        return 1;
#ifdef CONFIG_ICONV
    if (codepage) {
        char *tmpbuf;
        tmpbuf = sub_recode(track->library, buf, sz, codepage);
        free(buf);
        buf = tmpbuf;
    }
    if (!buf)
        return 1;
#endif

    old_state = track->parser_priv->state;
    track->parser_priv->state = PST_STYLES;
    process_text(track, buf);
    free(buf);
    track->parser_priv->state = old_state;

    return 0;
}

long long ass_step_sub(ASS_Track *track, long long now, int movement)
{
    int i;
    ASS_Event *best = NULL;
    long long target = now;
    int direction = (movement > 0 ? 1 : -1) * !!movement;

    if (track->n_events == 0)
        return 0;

    do {
        ASS_Event *closest = NULL;
        long long closest_time = now;
        for (i = 0; i < track->n_events; i++) {
            if (direction < 0) {
                long long end =
                    track->events[i].Start + track->events[i].Duration;
                if (end < target) {
                    if (!closest || end > closest_time) {
                        closest = &track->events[i];
                        closest_time = end;
                    }
                }
            } else if (direction > 0) {
                long long start = track->events[i].Start;
                if (start > target) {
                    if (!closest || start < closest_time) {
                        closest = &track->events[i];
                        closest_time = start;
                    }
                }
            } else {
                long long start = track->events[i].Start;
                if (start < target) {
                    if (!closest || start >= closest_time) {
                        closest = &track->events[i];
                        closest_time = start;
                    }
                }
            }
        }
        target = closest_time + direction;
        movement -= direction;
        if (closest)
            best = closest;
    } while (movement);

    return best ? best->Start - now : 0;
}

ASS_Track *ass_new_track(ASS_Library *library)
{
    ASS_Track *track = calloc(1, sizeof(ASS_Track));
    if (!track)
        return NULL;
    track->library = library;
    track->ScaledBorderAndShadow = 1;
    track->parser_priv = calloc(1, sizeof(ASS_ParserPriv));
    if (!track->parser_priv) {
        free(track);
        return NULL;
    }
    track->parser_priv->check_readorder = 1;
    return track;
}

/**
 * \brief Prepare track for rendering
 */
void ass_lazy_track_init(ASS_Library *lib, ASS_Track *track)
{
    if (track->PlayResX > 0 && track->PlayResY > 0)
        return;
    if (track->PlayResX <= 0 && track->PlayResY <= 0) {
        ass_msg(lib, MSGL_WARN,
               "Neither PlayResX nor PlayResY defined. Assuming 384x288");
        track->PlayResX = 384;
        track->PlayResY = 288;
    } else {
        if (track->PlayResY <= 0 && track->PlayResX == 1280) {
            track->PlayResY = 1024;
            ass_msg(lib, MSGL_WARN,
                   "PlayResY undefined, setting to %d", track->PlayResY);
        } else if (track->PlayResY <= 0) {
            track->PlayResY = FFMAX(1, track->PlayResX * 3 / 4);
            ass_msg(lib, MSGL_WARN,
                   "PlayResY undefined, setting to %d", track->PlayResY);
        } else if (track->PlayResX <= 0 && track->PlayResY == 1024) {
            track->PlayResX = 1280;
            ass_msg(lib, MSGL_WARN,
                   "PlayResX undefined, setting to %d", track->PlayResX);
        } else if (track->PlayResX <= 0) {
            track->PlayResX = FFMAX(1, track->PlayResY * 4 / 3);
            ass_msg(lib, MSGL_WARN,
                   "PlayResX undefined, setting to %d", track->PlayResX);
        }
    }
}
