/*
 * Copyright (C) 2009 Grigori Goronzy <greg@geekmind.org>
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
#include <math.h>

#include "ass_render.h"
#include "ass_parse.h"

#define MAX_VALID_NARGS 7
#define MAX_BE 127
#define NBSP 0xa0   // unicode non-breaking space character

struct arg {
    char *start, *end;
};

static inline int argtoi(struct arg arg)
{
    int value;
    mystrtoi(&arg.start, &value);
    return value;
}

static inline long long argtoll(struct arg arg)
{
    long long value;
    mystrtoll(&arg.start, &value);
    return value;
}

static inline double argtod(struct arg arg)
{
    double value;
    mystrtod(&arg.start, &value);
    return value;
}

static inline void push_arg(struct arg *args, int *nargs, char *start, char *end)
{
    if (*nargs <= MAX_VALID_NARGS) {
        rskip_spaces(&end, start);
        if (end > start) {
            args[*nargs] = (struct arg) {start, end};
            ++*nargs;
        }
    }
}

/**
 * \brief Check if starting part of (*p) matches sample.
 * If true, shift p to the first symbol after the matching part.
 */
static inline int mystrcmp(char **p, const char *sample)
{
    int len = strlen(sample);
    if (strncmp(*p, sample, len) == 0) {
        (*p) += len;
        return 1;
    } else
        return 0;
}

double ensure_font_size(ASS_Renderer *priv, double size)
{
    if (size < 1)
        size = 1;
    else if (size > priv->height * 2)
        size = priv->height * 2;

    return size;
}

static void change_font_size(ASS_Renderer *render_priv, double sz)
{
    render_priv->state.font_size = sz;
}

/**
 * \brief Change current font, using setting from render_priv->state.
 */
void update_font(ASS_Renderer *render_priv)
{
    unsigned val;
    ASS_FontDesc desc;

    if (render_priv->state.family[0] == '@') {
        desc.vertical = 1;
        desc.family = strdup(render_priv->state.family + 1);
    } else {
        desc.vertical = 0;
        desc.family = strdup(render_priv->state.family);
    }

    val = render_priv->state.bold;
    // 0 = normal, 1 = bold, >1 = exact weight
    if (val == 1 || val == -1)
        val = 700;               // bold
    else if (val <= 0)
        val = 400;               // normal
    desc.bold = val;

    val = render_priv->state.italic;
    if (val == 1)
        val = 100;              // italic
    else if (val <= 0)
        val = 0;                // normal
    desc.italic = val;

    ass_cache_dec_ref(render_priv->state.font);
    render_priv->state.font =
        ass_font_new(render_priv->cache.font_cache, render_priv->library,
                     render_priv->ftlibrary, render_priv->fontselect,
                     &desc);

    if (render_priv->state.font)
        change_font_size(render_priv, render_priv->state.font_size);
}

/**
 * \brief Calculate a weighted average of two colors
 * calculates c1*(1-a) + c2*a, but separately for each component except alpha
 */
static void change_color(uint32_t *var, uint32_t new, double pwr)
{
    (*var) = ((uint32_t) (_r(*var) * (1 - pwr) + _r(new) * pwr) << 24) |
        ((uint32_t) (_g(*var) * (1 - pwr) + _g(new) * pwr) << 16) |
        ((uint32_t) (_b(*var) * (1 - pwr) + _b(new) * pwr) << 8) | _a(*var);
}

// like change_color, but for alpha component only
inline void change_alpha(uint32_t *var, int32_t new, double pwr)
{
    *var = (*var & 0xFFFFFF00) | (uint8_t) (_a(*var) * (1 - pwr) + new * pwr);
}

/**
 * \brief Multiply two alpha values
 * \param a first value
 * \param b second value
 * \return result of multiplication
 * Parameters and result are limited by 0xFF.
 */
inline uint32_t mult_alpha(uint32_t a, uint32_t b)
{
    return 0xFF - (0xFF - a) * (0xFF - b) / 0xFF;
}

/**
 * \brief Calculate alpha value by piecewise linear function
 * Used for \fad, \fade implementation.
 */
static int
interpolate_alpha(long long now, long long t1, long long t2, long long t3,
                  long long t4, int a1, int a2, int a3)
{
    int a;
    double cf;

    if (now < t1) {
        a = a1;
    } else if (now < t2) {
        cf = ((double) (now - t1)) / (t2 - t1);
        a = a1 * (1 - cf) + a2 * cf;
    } else if (now < t3) {
        a = a2;
    } else if (now < t4) {
        cf = ((double) (now - t3)) / (t4 - t3);
        a = a2 * (1 - cf) + a3 * cf;
    } else {                    // now >= t4
        a = a3;
    }

    return a;
}

/**
 * Parse a vector clip into an outline, using the proper scaling
 * parameters.  Translate it to correct for screen borders, if needed.
 */
static int parse_vector_clip(ASS_Renderer *render_priv,
                             struct arg *args, int nargs)
{
    int scale = 1;
    ASS_Drawing *drawing = render_priv->state.clip_drawing;

    if (nargs != 1 && nargs != 2)
        return 0;
    if (nargs == 2)
        scale = argtoi(args[0]);
    struct arg text = args[nargs - 1];

    ass_drawing_free(drawing);
    render_priv->state.clip_drawing = ass_drawing_new(render_priv->library);
    drawing = render_priv->state.clip_drawing;
    if (drawing) {
        drawing->scale = scale;
        drawing->scale_x = render_priv->font_scale_x * render_priv->font_scale;
        drawing->scale_y = render_priv->font_scale;
        ass_drawing_set_text(drawing, text.start, text.end - text.start);
    }

    return 1;
}

/**
 * \brief Parse style override tag.
 * \param p string to parse
 * \param end end of string to parse, which must be '}', ')', or the first
 *            of a number of spaces immediately preceding '}' or ')'
 * \param pwr multiplier for some tag effects (comes from \t tags)
 */
char *parse_tag(ASS_Renderer *render_priv, char *p, char *end, double pwr)
{
    while (*p != '\\' && p != end)
        ++p;
    if (*p != '\\')
        return p;
    ++p;
    if (p != end)
        skip_spaces(&p);

    char *q = p;
    while (*q != '(' && *q != '\\' && q != end)
        ++q;
    if (q == p)
        return q;

    char *name_end = q;

    // Store one extra element to be able to detect excess arguments
    struct arg args[MAX_VALID_NARGS + 1];
    int nargs = 0;
    for (int i = 0; i <= MAX_VALID_NARGS; ++i)
        args[i].start = args[i].end = "";

    // Split parenthesized arguments. Do this for all tags and before
    // any non-parenthesized argument because that's what VSFilter does.
    if (*q == '(') {
        ++q;
        while (1) {
            if (q != end)
                skip_spaces(&q);

            // Split on commas. If there is a backslash, ignore any
            // commas following it and lump everything starting from
            // the last comma, through the backslash and all the way
            // to the end of the argument string into a single argument.

            char *r = q;
            while (*r != ',' && *r != '\\' && *r != ')' && r != end)
                ++r;

            if (*r == ',') {
                push_arg(args, &nargs, q, r);
                q = r + 1;
            } else {
                // Swallow the rest of the parenthesized string. This could
                // be either a backslash-argument or simply the last argument.
                while (*r != ')' && r != end)
                    ++r;
                push_arg(args, &nargs, q, r);
                q = r;
                // The closing parenthesis could be missing.
                if (q != end)
                    ++q;
                break;
            }
        }
    }

#define tag(name) (mystrcmp(&p, (name)) && (push_arg(args, &nargs, p, name_end), 1))
#define complex_tag(name) mystrcmp(&p, (name))

    // New tags introduced in vsfilter 2.39
    if (tag("xbord")) {
        double val;
        if (nargs) {
            val = argtod(*args);
            val = render_priv->state.border_x * (1 - pwr) + val * pwr;
            val = (val < 0) ? 0 : val;
        } else
            val = render_priv->state.style->Outline;
        render_priv->state.border_x = val;
    } else if (tag("ybord")) {
        double val;
        if (nargs) {
            val = argtod(*args);
            val = render_priv->state.border_y * (1 - pwr) + val * pwr;
            val = (val < 0) ? 0 : val;
        } else
            val = render_priv->state.style->Outline;
        render_priv->state.border_y = val;
    } else if (tag("xshad")) {
        double val;
        if (nargs) {
            val = argtod(*args);
            val = render_priv->state.shadow_x * (1 - pwr) + val * pwr;
        } else
            val = render_priv->state.style->Shadow;
        render_priv->state.shadow_x = val;
    } else if (tag("yshad")) {
        double val;
        if (nargs) {
            val = argtod(*args);
            val = render_priv->state.shadow_y * (1 - pwr) + val * pwr;
        } else
            val = render_priv->state.style->Shadow;
        render_priv->state.shadow_y = val;
    } else if (tag("fax")) {
        double val;
        if (nargs) {
            val = argtod(*args);
            render_priv->state.fax =
                val * pwr + render_priv->state.fax * (1 - pwr);
        } else
            render_priv->state.fax = 0.;
    } else if (tag("fay")) {
        double val;
        if (nargs) {
            val = argtod(*args);
            render_priv->state.fay =
                val * pwr + render_priv->state.fay * (1 - pwr);
        } else
            render_priv->state.fay = 0.;
    } else if (complex_tag("iclip")) {
        if (nargs == 4) {
            int x0, y0, x1, y1;
            x0 = argtoi(args[0]);
            y0 = argtoi(args[1]);
            x1 = argtoi(args[2]);
            y1 = argtoi(args[3]);
            render_priv->state.clip_x0 =
                render_priv->state.clip_x0 * (1 - pwr) + x0 * pwr;
            render_priv->state.clip_x1 =
                render_priv->state.clip_x1 * (1 - pwr) + x1 * pwr;
            render_priv->state.clip_y0 =
                render_priv->state.clip_y0 * (1 - pwr) + y0 * pwr;
            render_priv->state.clip_y1 =
                render_priv->state.clip_y1 * (1 - pwr) + y1 * pwr;
            render_priv->state.clip_mode = 1;
        } else if (!render_priv->state.clip_drawing) {
            if (parse_vector_clip(render_priv, args, nargs))
                render_priv->state.clip_drawing_mode = 1;
        }
    } else if (tag("blur")) {
        double val;
        if (nargs) {
            val = argtod(*args);
            val = render_priv->state.blur * (1 - pwr) + val * pwr;
            val = (val < 0) ? 0 : val;
            val = (val > BLUR_MAX_RADIUS) ? BLUR_MAX_RADIUS : val;
            render_priv->state.blur = val;
        } else
            render_priv->state.blur = 0.0;
        // ASS standard tags
    } else if (tag("fscx")) {
        double val;
        if (nargs) {
            val = argtod(*args) / 100;
            val = render_priv->state.scale_x * (1 - pwr) + val * pwr;
            val = (val < 0) ? 0 : val;
        } else
            val = render_priv->state.style->ScaleX;
        render_priv->state.scale_x = val;
    } else if (tag("fscy")) {
        double val;
        if (nargs) {
            val = argtod(*args) / 100;
            val = render_priv->state.scale_y * (1 - pwr) + val * pwr;
            val = (val < 0) ? 0 : val;
        } else
            val = render_priv->state.style->ScaleY;
        render_priv->state.scale_y = val;
    } else if (tag("fsc")) {
        render_priv->state.scale_x = render_priv->state.style->ScaleX;
        render_priv->state.scale_y = render_priv->state.style->ScaleY;
    } else if (tag("fsp")) {
        double val;
        if (nargs) {
            val = argtod(*args);
            render_priv->state.hspacing =
                render_priv->state.hspacing * (1 - pwr) + val * pwr;
        } else
            render_priv->state.hspacing = render_priv->state.style->Spacing;
    } else if (tag("fs")) {
        double val = 0;
        if (nargs) {
            val = argtod(*args);
            if (*args->start == '+' || *args->start == '-')
                val = render_priv->state.font_size * (1 + pwr * val / 10);
            else
                val = render_priv->state.font_size * (1 - pwr) + val * pwr;
        }
        if (val <= 0)
            val = render_priv->state.style->FontSize;
        if (render_priv->state.font)
            change_font_size(render_priv, val);
    } else if (tag("bord")) {
        double val, xval, yval;
        if (nargs) {
            val = argtod(*args);
            xval = render_priv->state.border_x * (1 - pwr) + val * pwr;
            yval = render_priv->state.border_y * (1 - pwr) + val * pwr;
            xval = (xval < 0) ? 0 : xval;
            yval = (yval < 0) ? 0 : yval;
        } else
            xval = yval = render_priv->state.style->Outline;
        render_priv->state.border_x = xval;
        render_priv->state.border_y = yval;
    } else if (complex_tag("move")) {
        double x1, x2, y1, y2;
        long long t1, t2, delta_t, t;
        double x, y;
        double k;
        if (nargs == 4 || nargs == 6) {
            x1 = argtod(args[0]);
            y1 = argtod(args[1]);
            x2 = argtod(args[2]);
            y2 = argtod(args[3]);
            t1 = t2 = 0;
            if (nargs == 6) {
                t1 = argtoll(args[4]);
                t2 = argtoll(args[5]);
                if (t1 > t2) {
                    long long tmp = t2;
                    t2 = t1;
                    t1 = tmp;
                }
            }
        } else
            return q;
        if (t1 <= 0 && t2 <= 0) {
            t1 = 0;
            t2 = render_priv->state.event->Duration;
        }
        delta_t = t2 - t1;
        t = render_priv->time - render_priv->state.event->Start;
        if (t <= t1)
            k = 0.;
        else if (t >= t2)
            k = 1.;
        else
            k = ((double) (t - t1)) / delta_t;
        x = k * (x2 - x1) + x1;
        y = k * (y2 - y1) + y1;
        if (render_priv->state.evt_type != EVENT_POSITIONED) {
            render_priv->state.pos_x = x;
            render_priv->state.pos_y = y;
            render_priv->state.detect_collisions = 0;
            render_priv->state.evt_type = EVENT_POSITIONED;
        }
    } else if (tag("frx")) {
        double val;
        if (nargs) {
            val = argtod(*args);
            val *= M_PI / 180;
            render_priv->state.frx =
                val * pwr + render_priv->state.frx * (1 - pwr);
        } else
            render_priv->state.frx = 0.;
    } else if (tag("fry")) {
        double val;
        if (nargs) {
            val = argtod(*args);
            val *= M_PI / 180;
            render_priv->state.fry =
                val * pwr + render_priv->state.fry * (1 - pwr);
        } else
            render_priv->state.fry = 0.;
    } else if (tag("frz") || tag("fr")) {
        double val;
        if (nargs) {
            val = argtod(*args);
            val *= M_PI / 180;
            render_priv->state.frz =
                val * pwr + render_priv->state.frz * (1 - pwr);
        } else
            render_priv->state.frz =
                M_PI * render_priv->state.style->Angle / 180.;
    } else if (tag("fn")) {
        char *family;
        char *start = args->start;
        end = args->end;
        if (nargs && strncmp(start, "0", end - start)) {
            skip_spaces(&start);
            family = strndup(start, end - start);
        } else
            family = strdup(render_priv->state.style->FontName);
        free(render_priv->state.family);
        render_priv->state.family = family;
        update_font(render_priv);
    } else if (tag("alpha")) {
        int i;
        if (nargs) {
            int32_t a = parse_alpha_tag(args->start);
            for (i = 0; i < 4; ++i)
                change_alpha(&render_priv->state.c[i], a, pwr);
        } else {
            change_alpha(&render_priv->state.c[0],
                         _a(render_priv->state.style->PrimaryColour), 1);
            change_alpha(&render_priv->state.c[1],
                         _a(render_priv->state.style->SecondaryColour), 1);
            change_alpha(&render_priv->state.c[2],
                         _a(render_priv->state.style->OutlineColour), 1);
            change_alpha(&render_priv->state.c[3],
                         _a(render_priv->state.style->BackColour), 1);
        }
        // FIXME: simplify
    } else if (tag("an")) {
        int val = argtoi(*args);
        if ((render_priv->state.parsed_tags & PARSED_A) == 0) {
            if (val >= 1 && val <= 9)
                render_priv->state.alignment = numpad2align(val);
            else
                render_priv->state.alignment =
                    render_priv->state.style->Alignment;
            render_priv->state.parsed_tags |= PARSED_A;
        }
    } else if (tag("a")) {
        int val = argtoi(*args);
        if ((render_priv->state.parsed_tags & PARSED_A) == 0) {
            if (val >= 1 && val <= 11)
                // take care of a vsfilter quirk:
                // handle illegal \a8 and \a4 like \a5
                render_priv->state.alignment = ((val & 3) == 0) ? 5 : val;
            else
                render_priv->state.alignment =
                    render_priv->state.style->Alignment;
            render_priv->state.parsed_tags |= PARSED_A;
        }
    } else if (complex_tag("pos")) {
        double v1, v2;
        if (nargs == 2) {
            v1 = argtod(args[0]);
            v2 = argtod(args[1]);
        } else
            return q;
        if (render_priv->state.evt_type == EVENT_POSITIONED) {
            ass_msg(render_priv->library, MSGL_V, "Subtitle has a new \\pos "
                   "after \\move or \\pos, ignoring");
        } else {
            render_priv->state.evt_type = EVENT_POSITIONED;
            render_priv->state.detect_collisions = 0;
            render_priv->state.pos_x = v1;
            render_priv->state.pos_y = v2;
        }
    } else if (complex_tag("fade") || complex_tag("fad")) {
        int a1, a2, a3;
        long long t1, t2, t3, t4;
        if (nargs == 2) {
            // 2-argument version (\fad, according to specs)
            a1 = 0xFF;
            a2 = 0;
            a3 = 0xFF;
            t1 = -1;
            t2 = argtoll(args[0]);
            t3 = argtoll(args[1]);
            t4 = -1;
        } else if (nargs == 7) {
            // 7-argument version (\fade)
            a1 = argtoi(args[0]);
            a2 = argtoi(args[1]);
            a3 = argtoi(args[2]);
            t1 = argtoll(args[3]);
            t2 = argtoll(args[4]);
            t3 = argtoll(args[5]);
            t4 = argtoll(args[6]);
        } else
            return q;
        if (t1 == -1 && t4 == -1) {
            t1 = 0;
            t4 = render_priv->state.event->Duration;
            t3 = t4 - t3;
        }
        if ((render_priv->state.parsed_tags & PARSED_FADE) == 0) {
            render_priv->state.fade =
                interpolate_alpha(render_priv->time -
                        render_priv->state.event->Start, t1, t2,
                        t3, t4, a1, a2, a3);
            render_priv->state.parsed_tags |= PARSED_FADE;
        }
    } else if (complex_tag("org")) {
        double v1, v2;
        if (nargs == 2) {
            v1 = argtod(args[0]);
            v2 = argtod(args[1]);
        } else
            return q;
        if (!render_priv->state.have_origin) {
            render_priv->state.org_x = v1;
            render_priv->state.org_y = v2;
            render_priv->state.have_origin = 1;
            render_priv->state.detect_collisions = 0;
        }
    } else if (complex_tag("t")) {
        double accel;
        int cnt = nargs - 1;
        long long t1, t2, t, delta_t;
        double k;
        if (cnt == 3) {
            t1 = argtoll(args[0]);
            t2 = argtoll(args[1]);
            accel = argtod(args[2]);
        } else if (cnt == 2) {
            t1 = argtoll(args[0]);
            t2 = argtoll(args[1]);
            accel = 1.;
        } else if (cnt == 1) {
            t1 = 0;
            t2 = 0;
            accel = argtod(args[0]);
        } else if (cnt == 0) {
            t1 = 0;
            t2 = 0;
            accel = 1.;
        } else
            return q;
        render_priv->state.detect_collisions = 0;
        if (t2 == 0)
            t2 = render_priv->state.event->Duration;
        delta_t = t2 - t1;
        t = render_priv->time - render_priv->state.event->Start;        // FIXME: move to render_context
        if (t <= t1)
            k = 0.;
        else if (t >= t2)
            k = 1.;
        else {
            assert(delta_t != 0.);
            k = pow(((double) (t - t1)) / delta_t, accel);
        }
        p = args[cnt].start;
        while (p < args[cnt].end)
            p = parse_tag(render_priv, p, args[cnt].end, k);    // maybe k*pwr ? no, specs forbid nested \t's
    } else if (complex_tag("clip")) {
        if (nargs == 4) {
            int x0, y0, x1, y1;
            x0 = argtoi(args[0]);
            y0 = argtoi(args[1]);
            x1 = argtoi(args[2]);
            y1 = argtoi(args[3]);
            render_priv->state.clip_x0 =
                render_priv->state.clip_x0 * (1 - pwr) + x0 * pwr;
            render_priv->state.clip_x1 =
                render_priv->state.clip_x1 * (1 - pwr) + x1 * pwr;
            render_priv->state.clip_y0 =
                render_priv->state.clip_y0 * (1 - pwr) + y0 * pwr;
            render_priv->state.clip_y1 =
                render_priv->state.clip_y1 * (1 - pwr) + y1 * pwr;
            render_priv->state.clip_mode = 0;
        } else if (!render_priv->state.clip_drawing) {
            if (parse_vector_clip(render_priv, args, nargs))
                render_priv->state.clip_drawing_mode = 0;
        }
    } else if (tag("c") || tag("1c")) {
        if (nargs) {
            uint32_t val = parse_color_tag(args->start);
            change_color(&render_priv->state.c[0], val, pwr);
        } else
            change_color(&render_priv->state.c[0],
                         render_priv->state.style->PrimaryColour, 1);
    } else if (tag("2c")) {
        if (nargs) {
            uint32_t val = parse_color_tag(args->start);
            change_color(&render_priv->state.c[1], val, pwr);
        } else
            change_color(&render_priv->state.c[1],
                         render_priv->state.style->SecondaryColour, 1);
    } else if (tag("3c")) {
        if (nargs) {
            uint32_t val = parse_color_tag(args->start);
            change_color(&render_priv->state.c[2], val, pwr);
        } else
            change_color(&render_priv->state.c[2],
                         render_priv->state.style->OutlineColour, 1);
    } else if (tag("4c")) {
        if (nargs) {
            uint32_t val = parse_color_tag(args->start);
            change_color(&render_priv->state.c[3], val, pwr);
        } else
            change_color(&render_priv->state.c[3],
                         render_priv->state.style->BackColour, 1);
    } else if (tag("1a")) {
        if (nargs) {
            uint32_t val = parse_alpha_tag(args->start);
            change_alpha(&render_priv->state.c[0], val, pwr);
        } else
            change_alpha(&render_priv->state.c[0],
                         _a(render_priv->state.style->PrimaryColour), 1);
    } else if (tag("2a")) {
        if (nargs) {
            uint32_t val = parse_alpha_tag(args->start);
            change_alpha(&render_priv->state.c[1], val, pwr);
        } else
            change_alpha(&render_priv->state.c[1],
                         _a(render_priv->state.style->SecondaryColour), 1);
    } else if (tag("3a")) {
        if (nargs) {
            uint32_t val = parse_alpha_tag(args->start);
            change_alpha(&render_priv->state.c[2], val, pwr);
        } else
            change_alpha(&render_priv->state.c[2],
                         _a(render_priv->state.style->OutlineColour), 1);
    } else if (tag("4a")) {
        if (nargs) {
            uint32_t val = parse_alpha_tag(args->start);
            change_alpha(&render_priv->state.c[3], val, pwr);
        } else
            change_alpha(&render_priv->state.c[3],
                         _a(render_priv->state.style->BackColour), 1);
    } else if (tag("r")) {
        if (nargs) {
            int len = args->end - args->start;
            reset_render_context(render_priv,
                    lookup_style_strict(render_priv->track, args->start, len));
        } else
            reset_render_context(render_priv, NULL);
    } else if (tag("be")) {
        double dval;
        if (nargs) {
            int val;
            dval = argtod(*args);
            // VSFilter always adds +0.5, even if the value is negative
            val = (int) (render_priv->state.be * (1 - pwr) + dval * pwr + 0.5);
            // Clamp to a safe upper limit, since high values need excessive CPU
            val = (val < 0) ? 0 : val;
            val = (val > MAX_BE) ? MAX_BE : val;
            render_priv->state.be = val;
        } else
            render_priv->state.be = 0;
    } else if (tag("b")) {
        int val = argtoi(*args);
        if (!nargs || !(val == 0 || val == 1 || val >= 100))
            val = render_priv->state.style->Bold;
        render_priv->state.bold = val;
        update_font(render_priv);
    } else if (tag("i")) {
        int val = argtoi(*args);
        if (!nargs || !(val == 0 || val == 1))
            val = render_priv->state.style->Italic;
        render_priv->state.italic = val;
        update_font(render_priv);
    } else if (tag("kf") || tag("K")) {
        double val = 100;
        if (nargs)
            val = argtod(*args);
        render_priv->state.effect_type = EF_KARAOKE_KF;
        if (render_priv->state.effect_timing)
            render_priv->state.effect_skip_timing +=
                render_priv->state.effect_timing;
        render_priv->state.effect_timing = val * 10;
    } else if (tag("ko")) {
        double val = 100;
        if (nargs)
            val = argtod(*args);
        render_priv->state.effect_type = EF_KARAOKE_KO;
        if (render_priv->state.effect_timing)
            render_priv->state.effect_skip_timing +=
                render_priv->state.effect_timing;
        render_priv->state.effect_timing = val * 10;
    } else if (tag("k")) {
        double val = 100;
        if (nargs)
            val = argtod(*args);
        render_priv->state.effect_type = EF_KARAOKE;
        if (render_priv->state.effect_timing)
            render_priv->state.effect_skip_timing +=
                render_priv->state.effect_timing;
        render_priv->state.effect_timing = val * 10;
    } else if (tag("shad")) {
        double val, xval, yval;
        if (nargs) {
            val = argtod(*args);
            xval = render_priv->state.shadow_x * (1 - pwr) + val * pwr;
            yval = render_priv->state.shadow_y * (1 - pwr) + val * pwr;
            // VSFilter compatibility: clip for \shad but not for \[xy]shad
            xval = (xval < 0) ? 0 : xval;
            yval = (yval < 0) ? 0 : yval;
        } else
            xval = yval = render_priv->state.style->Shadow;
        render_priv->state.shadow_x = xval;
        render_priv->state.shadow_y = yval;
    } else if (tag("s")) {
        int val = argtoi(*args);
        if (!nargs || !(val == 0 || val == 1))
            val = render_priv->state.style->StrikeOut;
        if (val)
            render_priv->state.flags |= DECO_STRIKETHROUGH;
        else
            render_priv->state.flags &= ~DECO_STRIKETHROUGH;
    } else if (tag("u")) {
        int val = argtoi(*args);
        if (!nargs || !(val == 0 || val == 1))
            val = render_priv->state.style->Underline;
        if (val)
            render_priv->state.flags |= DECO_UNDERLINE;
        else
            render_priv->state.flags &= ~DECO_UNDERLINE;
    } else if (tag("pbo")) {
        double val = argtod(*args);
        render_priv->state.pbo = val;
    } else if (tag("p")) {
        int val = argtoi(*args);
        val = (val < 0) ? 0 : val;
        render_priv->state.drawing_scale = val;
    } else if (tag("q")) {
        int val = argtoi(*args);
        if (!nargs || !(val >= 0 && val <= 3))
            val = render_priv->track->WrapStyle;
        render_priv->state.wrap_style = val;
    } else if (tag("fe")) {
        int val;
        if (nargs)
            val = argtoi(*args);
        else
            val = render_priv->state.style->Encoding;
        render_priv->state.font_encoding = val;
    }

    return q;
}

void apply_transition_effects(ASS_Renderer *render_priv, ASS_Event *event)
{
    int v[4];
    int cnt;
    char *p = event->Effect;

    if (!p || !*p)
        return;

    cnt = 0;
    while (cnt < 4 && (p = strchr(p, ';'))) {
        v[cnt++] = atoi(++p);
    }

    if (strncmp(event->Effect, "Banner;", 7) == 0) {
        int delay;
        if (cnt < 1) {
            ass_msg(render_priv->library, MSGL_V,
                    "Error parsing effect: '%s'", event->Effect);
            return;
        }
        if (cnt >= 2 && v[1] == 0)      // right-to-left
            render_priv->state.scroll_direction = SCROLL_RL;
        else                    // left-to-right
            render_priv->state.scroll_direction = SCROLL_LR;

        delay = v[0];
        if (delay == 0)
            delay = 1;          // ?
        render_priv->state.scroll_shift =
            (render_priv->time - render_priv->state.event->Start) / delay;
        render_priv->state.evt_type = EVENT_HSCROLL;
        return;
    }

    if (strncmp(event->Effect, "Scroll up;", 10) == 0) {
        render_priv->state.scroll_direction = SCROLL_BT;
    } else if (strncmp(event->Effect, "Scroll down;", 12) == 0) {
        render_priv->state.scroll_direction = SCROLL_TB;
    } else {
        ass_msg(render_priv->library, MSGL_DBG2,
                "Unknown transition effect: '%s'", event->Effect);
        return;
    }
    // parse scroll up/down parameters
    {
        int delay;
        int y0, y1;
        if (cnt < 3) {
            ass_msg(render_priv->library, MSGL_V,
                    "Error parsing effect: '%s'", event->Effect);
            return;
        }
        delay = v[2];
        if (delay == 0)
            delay = 1;          // ?
        render_priv->state.scroll_shift =
            (render_priv->time - render_priv->state.event->Start) / delay;
        if (v[0] < v[1]) {
            y0 = v[0];
            y1 = v[1];
        } else {
            y0 = v[1];
            y1 = v[0];
        }
        if (y1 == 0)
            y1 = render_priv->track->PlayResY;  // y0=y1=0 means fullscreen scrolling
        render_priv->state.clip_y0 = y0;
        render_priv->state.clip_y1 = y1;
        render_priv->state.evt_type = EVENT_VSCROLL;
        render_priv->state.detect_collisions = 0;
    }

}

/**
 * \brief determine karaoke effects
 * Karaoke effects cannot be calculated during parse stage (get_next_char()),
 * so they are done in a separate step.
 * Parse stage: when karaoke style override is found, its parameters are stored in the next glyph's
 * (the first glyph of the karaoke word)'s effect_type and effect_timing.
 * This function:
 * 1. sets effect_type for all glyphs in the word (_karaoke_ word)
 * 2. sets effect_timing for all glyphs to x coordinate of the border line between the left and right karaoke parts
 * (left part is filled with PrimaryColour, right one - with SecondaryColour).
 */
void process_karaoke_effects(ASS_Renderer *render_priv)
{
    GlyphInfo *cur, *cur2;
    GlyphInfo *s1, *e1;      // start and end of the current word
    GlyphInfo *s2;           // start of the next word
    int i;
    int timing;                 // current timing
    int tm_start, tm_end;       // timings at start and end of the current word
    int tm_current;
    double dt;
    int x;
    int x_start, x_end;

    tm_current = render_priv->time - render_priv->state.event->Start;
    timing = 0;
    s1 = s2 = 0;
    for (i = 0; i <= render_priv->text_info.length; ++i) {
        cur = render_priv->text_info.glyphs + i;
        if ((i == render_priv->text_info.length)
            || (cur->effect_type != EF_NONE)) {
            s1 = s2;
            s2 = cur;
            if (s1) {
                e1 = s2 - 1;
                tm_start = timing + s1->effect_skip_timing;
                tm_end = tm_start + s1->effect_timing;
                timing = tm_end;
                x_start = 1000000;
                x_end = -1000000;
                for (cur2 = s1; cur2 <= e1; ++cur2) {
                    x_start = FFMIN(x_start, d6_to_int(cur2->bbox.x_min + cur2->pos.x));
                    x_end = FFMAX(x_end, d6_to_int(cur2->bbox.x_max + cur2->pos.x));
                }

                dt = (tm_current - tm_start);
                if ((s1->effect_type == EF_KARAOKE)
                    || (s1->effect_type == EF_KARAOKE_KO)) {
                    if (dt >= 0)
                        x = x_end + 1;
                    else
                        x = x_start;
                } else if (s1->effect_type == EF_KARAOKE_KF) {
                    dt /= (tm_end - tm_start);
                    x = x_start + (x_end - x_start) * dt;
                } else {
                    ass_msg(render_priv->library, MSGL_ERR,
                            "Unknown effect type");
                    continue;
                }

                for (cur2 = s1; cur2 <= e1; ++cur2) {
                    cur2->effect_type = s1->effect_type;
                    cur2->effect_timing = x - d6_to_int(cur2->pos.x);
                }
                s1->effect = 1;
            }
        }
    }
}


/**
 * \brief Get next ucs4 char from string, parsing UTF-8 and escapes
 * \param str string pointer
 * \return ucs4 code of the next char
 * On return str points to the unparsed part of the string
 */
unsigned get_next_char(ASS_Renderer *render_priv, char **str)
{
    char *p = *str;
    unsigned chr;
    if (*p == '\t') {
        ++p;
        *str = p;
        return ' ';
    }
    if (*p == '\\') {
        if ((p[1] == 'N') || ((p[1] == 'n') &&
                              (render_priv->state.wrap_style == 2))) {
            p += 2;
            *str = p;
            return '\n';
        } else if (p[1] == 'n') {
            p += 2;
            *str = p;
            return ' ';
        } else if (p[1] == 'h') {
            p += 2;
            *str = p;
            return NBSP;
        } else if (p[1] == '{') {
            p += 2;
            *str = p;
            return '{';
        } else if (p[1] == '}') {
            p += 2;
            *str = p;
            return '}';
        }
    }
    chr = ass_utf8_get_char((char **) &p);
    *str = p;
    return chr;
}

// Return 1 if the event contains tags that will apply overrides the selective
// style override code should not touch. Return 0 otherwise.
int event_has_hard_overrides(char *str)
{
    // look for \pos and \move tags inside {...}
    // mirrors get_next_char, but is faster and doesn't change any global state
    while (*str) {
        if (str[0] == '\\' && str[1] != '\0') {
            str += 2;
        } else if (str[0] == '{') {
            str++;
            while (*str && *str != '}') {
                if (*str == '\\') {
                    char *p = str + 1;
                    if (mystrcmp(&p, "pos") || mystrcmp(&p, "move") ||
                        mystrcmp(&p, "clip") || mystrcmp(&p, "iclip") ||
                        mystrcmp(&p, "org") || mystrcmp(&p, "pbo") ||
                        mystrcmp(&p, "p"))
                        return 1;
                }
                str++;
            }
        } else {
            str++;
        }
    }
    return 0;
}
