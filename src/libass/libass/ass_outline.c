/*
 * Copyright (C) 2016 Vabishchevich Nikolay <vabnick@gmail.com>
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

#include "ass_utils.h"
#include "ass_outline.h"



bool outline_alloc(ASS_Outline *outline, size_t n_points, size_t n_segments)
{
    outline->points = malloc(sizeof(ASS_Vector) * n_points);
    outline->segments = malloc(n_segments);
    if (!outline->points || !outline->segments) {
        outline_free(outline);
        return false;
    }

    outline->max_points = n_points;
    outline->max_segments = n_segments;
    return true;
}

static void outline_clear(ASS_Outline *outline)
{
    outline->points = NULL;
    outline->segments = NULL;

    outline->n_points = outline->max_points = 0;
    outline->n_segments = outline->max_segments = 0;
}

bool outline_convert(ASS_Outline *outline, const FT_Outline *source)
{
    if (!source || !source->n_points) {
        outline_clear(outline);
        return true;
    }

    if (!outline_alloc(outline, 2 * source->n_points, source->n_points))
        return false;

    enum Status {
        S_ON, S_Q, S_C1, S_C2
    };

    outline->n_points = outline->n_segments = 0;
    for (size_t i = 0, j = 0; i < source->n_contours; i++) {
        ASS_Vector pt;
        bool skip_last = false;
        enum Status st;

        int last = source->contours[i];
        if (j > last || last >= source->n_points)
            goto fail;

        // skip degenerate 2-point contours from broken fonts
        if (last - j < 2) {
            j = last + 1;
            continue;
        }

        switch (FT_CURVE_TAG(source->tags[j])) {
        case FT_CURVE_TAG_ON:
            st = S_ON;
            break;

        case FT_CURVE_TAG_CONIC:
            pt.x =  source->points[last].x;
            pt.y = -source->points[last].y;
            switch (FT_CURVE_TAG(source->tags[last])) {
            case FT_CURVE_TAG_ON:
                skip_last = true;
                break;

            case FT_CURVE_TAG_CONIC:
                pt.x = (pt.x + source->points[j].x) >> 1;
                pt.y = (pt.y - source->points[j].y) >> 1;
                break;

            default:
                goto fail;
            }
            outline->points[outline->n_points++] = pt;
            st = S_Q;
            break;

        default:
            goto fail;
        }
        pt.x =  source->points[j].x;
        pt.y = -source->points[j].y;
        outline->points[outline->n_points++] = pt;

        for (j++; j <= last; j++) {
            switch (FT_CURVE_TAG(source->tags[j])) {
            case FT_CURVE_TAG_ON:
                switch (st) {
                case S_ON:
                    outline->segments[outline->n_segments++] = OUTLINE_LINE_SEGMENT;
                    break;

                case S_Q:
                    outline->segments[outline->n_segments++] = OUTLINE_QUADRATIC_SPLINE;
                    break;

                case S_C2:
                    outline->segments[outline->n_segments++] = OUTLINE_CUBIC_SPLINE;
                    break;

                default:
                    goto fail;
                }
                st = S_ON;
                break;

            case FT_CURVE_TAG_CONIC:
                switch (st) {
                case S_ON:
                    st = S_Q;
                    break;

                case S_Q:
                    outline->segments[outline->n_segments++] = OUTLINE_QUADRATIC_SPLINE;
                    pt.x = (pt.x + source->points[j].x) >> 1;
                    pt.y = (pt.y - source->points[j].y) >> 1;
                    outline->points[outline->n_points++] = pt;
                    break;

                default:
                    goto fail;
                }
                break;

            case FT_CURVE_TAG_CUBIC:
                switch (st) {
                case S_ON:
                    st = S_C1;
                    break;

                case S_C1:
                    st = S_C2;
                    break;

                default:
                    goto fail;
                }
                break;

            default:
                goto fail;
            }
            pt.x =  source->points[j].x;
            pt.y = -source->points[j].y;
            outline->points[outline->n_points++] = pt;
        }

        switch (st) {
        case S_ON:
            if (skip_last) {
                outline->n_points--;
                break;
            }
            outline->segments[outline->n_segments++] = OUTLINE_LINE_SEGMENT;
            break;

        case S_Q:
            outline->segments[outline->n_segments++] = OUTLINE_QUADRATIC_SPLINE;
            break;

        case S_C2:
            outline->segments[outline->n_segments++] = OUTLINE_CUBIC_SPLINE;
            break;

        default:
            goto fail;
        }
        outline->segments[outline->n_segments - 1] |= OUTLINE_CONTOUR_END;
    }
    return true;

fail:
    outline_free(outline);
    return false;
}

bool outline_copy(ASS_Outline *outline, const ASS_Outline *source)
{
    if (!source || !source->n_points) {
        outline_clear(outline);
        return true;
    }

    if (!outline_alloc(outline, source->n_points, source->n_segments))
        return false;

    memcpy(outline->points, source->points, sizeof(ASS_Vector) * source->n_points);
    memcpy(outline->segments, source->segments, source->n_segments);
    outline->n_points = source->n_points;
    outline->n_segments = source->n_segments;
    return true;
}

void outline_free(ASS_Outline *outline)
{
    if (!outline)
        return;

    free(outline->points);
    free(outline->segments);

    outline_clear(outline);
}


/*
 * \brief Add a single point to a contour.
 * Also adds outline segment if segment parameter is nonzero.
 */
bool outline_add_point(ASS_Outline *outline, ASS_Vector pt, char segment)
{
    if (outline->n_points >= outline->max_points) {
        size_t new_size = 2 * outline->max_points;
        if (!ASS_REALLOC_ARRAY(outline->points, new_size))
            return false;
        outline->max_points = new_size;
    }
    outline->points[outline->n_points] = pt;
    outline->n_points++;

    return !segment || outline_add_segment(outline, segment);
}

/*
 * \brief Add a segment to a contour.
 */
bool outline_add_segment(ASS_Outline *outline, char segment)
{
    if (outline->n_segments >= outline->max_segments) {
        size_t new_size = 2 * outline->max_segments;
        if (!ASS_REALLOC_ARRAY(outline->segments, new_size))
            return false;
        outline->max_segments = new_size;
    }
    outline->segments[outline->n_segments] = segment;
    outline->n_segments++;
    return true;
}

/*
 * \brief Close a contour.
 */
bool outline_close_contour(ASS_Outline *outline)
{
    assert(outline->n_segments);
    assert(!(outline->segments[outline->n_segments - 1] & ~OUTLINE_COUNT_MASK));
    outline->segments[outline->n_segments - 1] |= OUTLINE_CONTOUR_END;
    return true;
}


void outline_translate(const ASS_Outline *outline, int32_t dx, int32_t dy)
{
    for (size_t i = 0; i < outline->n_points; i++) {
        outline->points[i].x += dx;
        outline->points[i].y += dy;
    }
}

void outline_adjust(const ASS_Outline *outline, double scale_x, int32_t dx, int32_t dy)
{
    int32_t mul = lrint(scale_x * 0x10000);
    if (mul == 0x10000) {
        outline_translate(outline, dx, dy);
        return;
    }
    for (size_t i = 0; i < outline->n_points; i++) {
        int32_t x = (int64_t) outline->points[i].x * mul >> 16;
        outline->points[i].x = x + dx;
        outline->points[i].y += dy;
    }
}

void outline_get_cbox(const ASS_Outline *outline, ASS_Rect *cbox)
{
    if (!outline->n_points) {
        cbox->x_min = cbox->x_max = 0;
        cbox->y_min = cbox->y_max = 0;
        return;
    }
    cbox->x_min = cbox->x_max = outline->points[0].x;
    cbox->y_min = cbox->y_max = outline->points[0].y;
    for (size_t i = 1; i < outline->n_points; i++) {
        cbox->x_min = FFMIN(cbox->x_min, outline->points[i].x);
        cbox->x_max = FFMAX(cbox->x_max, outline->points[i].x);
        cbox->y_min = FFMIN(cbox->y_min, outline->points[i].y);
        cbox->y_max = FFMAX(cbox->y_max, outline->points[i].y);
    }
}


/*
 * Outline Stroke Algorithm
 *
 * Goal:
 * Given source outline, construct two border outlines, such as that
 * for any point inside any border outline (nonzero winding rule)
 * minimal distance to points of source outline (same rule)
 * is less than 1 with given precision, and for any point outside
 * both border outlines minimal distance is more than approximately 1.
 * Distance here is defined in normal space that is scaled by [1 / xbord, 1 / ybord].
 * Correspondingly distance is equal to hypot(dx / xbord, dy / ybord) and
 * approximate allowable error is eps / max(xbord, ybord).
 *
 * Two border outlines correspond to ±1 offset curves and
 * are required in case of self-intersecting source outline.
 *
 * Each of source segment that can be line segment, quadratic or cubic spline,
 * and also connection between them is stroked mostly independently.
 * Line segments can be offset quite straightforwardly.
 * For splines algorithm first tries to offset individual points,
 * then estimates error of such approximation and subdivide recursively if necessary.
 *
 * List of border cases handled by this algorithm:
 * 1) Too close points lead to random derivatives or even division by zero.
 *    Algorithm solves that by merging such points into one.
 * 2) Degenerate cases--near zero derivative at some spline points.
 *    Algorithm adds circular cap in such cases.
 * 3) Negative curvature--offset amount is larger than radius of curvature.
 *    Algorithm checks if produced splines can potentially have self-intersection
 *    and handles them accordingly. It's mostly done by skipping
 *    problematic spline and replacing it with polyline that covers only
 *    positive winging part of mathematical offset curve.
 *
 * Error estimation for splines is done by analyzing offset spline.
 * Offset spline is the difference between result and source spline in normal space.
 * Such spline should consist of vectors with length 1 and orthogonal to source spline.
 * Correspondingly error estimator have radial and angular part.
 *
 * Useful facts about B-splines.
 * 1) Derivative of B-spline of order N is B-spline of order N-1.
 * 2) Multiplication of B-splines of order N and M is B-spline of order N+M.
 * 3) B-spline is fully contained inside convex hull of its control points.
 *
 * So, for radial error it's possible to check only control points of
 * offset spline multiplication by itself. And for angular error it's
 * possible to check control points of cross and dot product between
 * offset spline and derivative spline.
 */


typedef struct {
    ASS_DVector v;
    double len;
} Normal;

typedef struct {
    ASS_Outline *result[2];   // result outlines
    size_t contour_first[2];  // start position of last contours
    double xbord, ybord;      // border sizes
    double xscale, yscale;    // inverse border sizes
    int eps;                  // allowable error in coordinate space

    // true if where are points in current contour
    bool contour_start;
    // skip flags for first and last point
    int first_skip, last_skip;
    // normal at first and last point
    ASS_DVector first_normal, last_normal;
    // first point of current contour
    ASS_Vector first_point;

    // cosinus of maximal angle that do not require cap
    double merge_cos;
    // cosinus of maximal angle of circular arc that can be approximated with quadratic spline
    double split_cos;
    // maximal distance between control points in normal space that triggers handling of degenerates
    double min_len;
    // constant that used in exact radial error checking in quadratic case
    double err_q;
    // constant that used in approximate radial error checking in cubic case
    double err_c;
    // tangent of maximal angular error
    double err_a;
} StrokerState;

/**
 * \brief 2D vector dot product
 */
static inline double vec_dot(ASS_DVector vec1, ASS_DVector vec2)
{
    return vec1.x * vec2.x + vec1.y * vec2.y;
}

/**
 * \brief 2D vector cross product
 */
static inline double vec_crs(ASS_DVector vec1, ASS_DVector vec2)
{
    return vec1.x * vec2.y - vec1.y * vec2.x;
}

/**
 * \brief 2D vector length
 */
static inline double vec_len(ASS_DVector vec)
{
    return sqrt(vec.x * vec.x + vec.y * vec.y);
}


/**
 * \brief Add point to one or two border outlines
 * \param str stroker state
 * \param pt source point
 * \param offs offset in normal space
 * \param segment outline segment type
 * \param dir destination outline flags
 * \return false on allocation failure
 */
static bool emit_point(StrokerState *str, ASS_Vector pt,
                       ASS_DVector offs, char segment, int dir)
{
    int32_t dx = (int32_t) (str->xbord * offs.x);
    int32_t dy = (int32_t) (str->ybord * offs.y);

    if (dir & 1) {
        ASS_Vector res = { pt.x + dx, pt.y + dy };
        if (!outline_add_point(str->result[0], res, segment))
            return false;
    }
    if (dir & 2) {
        ASS_Vector res = { pt.x - dx, pt.y - dy };
        if (!outline_add_point(str->result[1], res, segment))
            return false;
    }
    return true;
}

/**
 * \brief Replace first point of current contour
 * \param str stroker state
 * \param pt source point
 * \param offs offset in normal space
 * \param dir destination outline flags
 */
static void fix_first_point(StrokerState *str, ASS_Vector pt,
                            ASS_DVector offs, int dir)
{
    int32_t dx = (int32_t) (str->xbord * offs.x);
    int32_t dy = (int32_t) (str->ybord * offs.y);

    if (dir & 1) {
        ASS_Vector res = { pt.x + dx, pt.y + dy };
        ASS_Outline *ol = str->result[0];
        ol->points[str->contour_first[0]] = res;
    }
    if (dir & 2) {
        ASS_Vector res = { pt.x - dx, pt.y - dy };
        ASS_Outline *ol = str->result[1];
        ol->points[str->contour_first[1]] = res;
    }
}

/**
 * \brief Helper function for circular arc construction
 * \param str stroker state
 * \param pt center point
 * \param normal0 first normal
 * \param normal1 last normal
 * \param mul precalculated coefficients
 * \param level subdivision level
 * \param dir destination outline flags
 * \return false on allocation failure
 */
static bool process_arc(StrokerState *str, ASS_Vector pt,
                        ASS_DVector normal0, ASS_DVector normal1,
                        const double *mul, int level, int dir)
{
    ASS_DVector center;
    center.x = (normal0.x + normal1.x) * mul[level];
    center.y = (normal0.y + normal1.y) * mul[level];
    if (level)
        return process_arc(str, pt, normal0, center, mul, level - 1, dir) &&
               process_arc(str, pt, center, normal1, mul, level - 1, dir);
    return emit_point(str, pt, normal0, OUTLINE_QUADRATIC_SPLINE, dir) &&
           emit_point(str, pt, center, 0, dir);
}

/**
 * \brief Construct circular arc
 * \param str stroker state
 * \param pt center point
 * \param normal0 first normal
 * \param normal1 last normal
 * \param c dot product between normal0 and normal1
 * \param dir destination outline flags
 * \return false on allocation failure
 */
static bool draw_arc(StrokerState *str, ASS_Vector pt,
                     ASS_DVector normal0, ASS_DVector normal1, double c, int dir)
{
    const int max_subdiv = 15;
    double mul[max_subdiv + 1];

    ASS_DVector center;
    bool small_angle = true;
    if (c < 0) {
        double mul = dir & 2 ? -sqrt(0.5) : sqrt(0.5);
        mul /= sqrt(1 - c);
        center.x = (normal1.y - normal0.y) * mul;
        center.y = (normal0.x - normal1.x) * mul;
        c = sqrt(FFMAX(0, 0.5 + 0.5 * c));
        small_angle = false;
    }

    int pos = max_subdiv;
    while (c < str->split_cos && pos) {
        mul[pos] = sqrt(0.5) / sqrt(1 + c);
        c = (1 + c) * mul[pos];
        pos--;
    }
    mul[pos] = 1 / (1 + c);
    return small_angle ?
        process_arc(str, pt, normal0, normal1, mul + pos, max_subdiv - pos, dir) :
        process_arc(str, pt, normal0, center,  mul + pos, max_subdiv - pos, dir) &&
        process_arc(str, pt, center,  normal1, mul + pos, max_subdiv - pos, dir);
}

/**
 * \brief Construct full circle
 * \param str stroker state
 * \param pt center point
 * \param dir destination outline flags
 * \return false on allocation failure
 */
static bool draw_circle(StrokerState *str, ASS_Vector pt, int dir)
{
    const int max_subdiv = 15;
    double mul[max_subdiv + 1], c = 0;

    int pos = max_subdiv;
    while (c < str->split_cos && pos) {
        mul[pos] = sqrt(0.5) / sqrt(1 + c);
        c = (1 + c) * mul[pos];
        pos--;
    }
    mul[pos] = 1 / (1 + c);

    ASS_DVector normal[4] = {
        { 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 }
    };
    return process_arc(str, pt, normal[0], normal[1], mul + pos, max_subdiv - pos, dir) &&
           process_arc(str, pt, normal[1], normal[2], mul + pos, max_subdiv - pos, dir) &&
           process_arc(str, pt, normal[2], normal[3], mul + pos, max_subdiv - pos, dir) &&
           process_arc(str, pt, normal[3], normal[0], mul + pos, max_subdiv - pos, dir);
}

/**
 * \brief Start new segment and add circular cap if necessary
 * \param str stroker state
 * \param pt start point of the new segment
 * \param normal normal at start of the new segment
 * \param dir destination outline flags
 * \return false on allocation failure
 */
static bool start_segment(StrokerState *str, ASS_Vector pt,
                          ASS_DVector normal, int dir)
{
    if (str->contour_start) {
        str->contour_start = false;
        str->first_skip = str->last_skip = 0;
        str->first_normal = str->last_normal = normal;
        str->first_point = pt;
        return true;
    }

    ASS_DVector prev = str->last_normal;
    double c = vec_dot(prev, normal);
    if (c > str->merge_cos) {  // merge without cap
        double mul = 1 / (1 + c);
        str->last_normal.x = (str->last_normal.x + normal.x) * mul;
        str->last_normal.y = (str->last_normal.y + normal.y) * mul;
        return true;
    }
    str->last_normal = normal;

    // check for negative curvature
    double s = vec_crs(prev, normal);
    int skip_dir = s < 0 ? 1 : 2;
    if (dir & skip_dir) {
        if (!emit_point(str, pt, prev, OUTLINE_LINE_SEGMENT, ~str->last_skip & skip_dir))
            return false;
        ASS_DVector zero_normal = {0, 0};
        if (!emit_point(str, pt, zero_normal, OUTLINE_LINE_SEGMENT, skip_dir))
            return false;
    }
    str->last_skip = skip_dir;

    dir &= ~skip_dir;
    return !dir || draw_arc(str, pt, prev, normal, c, dir);
}

/**
 * \brief Same as emit_point() but also updates skip flags
 */
static bool emit_first_point(StrokerState *str, ASS_Vector pt, char segment, int dir)
{
    str->last_skip &= ~dir;
    return emit_point(str, pt, str->last_normal, segment, dir);
}

/**
 * \brief Prepare to skip part of curve
 * \param str stroker state
 * \param pt start point of the skipped part
 * \param dir destination outline flags
 * \param first true if the skipped part is at start of the segment
 * \return false on allocation failure
 */
static bool prepare_skip(StrokerState *str, ASS_Vector pt, int dir, bool first)
{
    if (first)
        str->first_skip |= dir;
    else if (!emit_point(str, pt, str->last_normal, OUTLINE_LINE_SEGMENT, ~str->last_skip & dir))
        return false;
    str->last_skip |= dir;
    return true;
}

/**
 * \brief Process source line segment
 * \param str stroker state
 * \param pt0 start point of the line segment
 * \param pt1 end point of the line segment
 * \param dir destination outline flags
 * \return false on allocation failure
 */
static bool add_line(StrokerState *str, ASS_Vector pt0, ASS_Vector pt1, int dir)
{
    int32_t dx = pt1.x - pt0.x;
    int32_t dy = pt1.y - pt0.y;
    if (dx > -str->eps && dx < str->eps && dy > -str->eps && dy < str->eps)
        return true;

    ASS_DVector deriv = { dy * str->yscale, -dx * str->xscale };
    double scale = 1 / vec_len(deriv);
    ASS_DVector normal = { deriv.x * scale, deriv.y * scale };
    if (!start_segment(str, pt0, normal, dir))
        return false;
    if (!emit_first_point(str, pt0, OUTLINE_LINE_SEGMENT, dir))
        return false;
    str->last_normal = normal;
    return true;
}


/**
 * \brief Estimate error for quadratic spline
 * \param str stroker state
 * \param c dot product between normal[0] and normal[1]
 * \param s cross product between normal[0] and normal[1]
 * \param normal first and last spline normal
 * \param result best offset for central spline point
 * \return false if error is too large
 */
static bool estimate_quadratic_error(StrokerState *str, double c, double s,
                                     const Normal *normal, ASS_DVector *result)
{
    // check radial error
    if (!((3 + c) * (3 + c) < str->err_q * (1 + c)))
        return false;

    double mul = 1 / (1 + c);
    double l0 = 2 * normal[0].len, l1 = 2 * normal[1].len;
    double dot0 = l0 + normal[1].len * c, crs0 = (l0 * mul - normal[1].len) * s;
    double dot1 = l1 + normal[0].len * c, crs1 = (l1 * mul - normal[0].len) * s;
    // check angular error
    if (!(fabs(crs0) < str->err_a * dot0 && fabs(crs1) < str->err_a * dot1))
        return false;

    result->x = (normal[0].v.x + normal[1].v.x) * mul;
    result->y = (normal[0].v.y + normal[1].v.y) * mul;
    return true;
}

/**
 * \brief Helper function for quadratic spline construction
 * \param str stroker state
 * \param pt array of 3 source spline points
 * \param deriv array of 2 differences between adjacent points in normal space
 * \param normal first and last spline normal
 * \param dir destination outline flags
 * \param first true if the current part is at start of the segment
 * \return false on allocation failure
 */
static bool process_quadratic(StrokerState *str, const ASS_Vector *pt,
                              const ASS_DVector *deriv, const Normal *normal,
                              int dir, bool first)
{
    double c = vec_dot(normal[0].v, normal[1].v);
    double s = vec_crs(normal[0].v, normal[1].v);
    int check_dir = dir, skip_dir = s < 0 ? 1 : 2;
    if (dir & skip_dir) {
        double abs_s = fabs(s);
        double f0 = normal[0].len * c + normal[1].len;
        double f1 = normal[1].len * c + normal[0].len;
        double g0 = normal[0].len * abs_s;
        double g1 = normal[1].len * abs_s;
        // check for self-intersection
        if (f0 < abs_s && f1 < abs_s) {
            double d2 = (f0 * normal[1].len + f1 * normal[0].len) / 2;
            if (d2 < g0 && d2 < g1) {
                if (!prepare_skip(str, pt[0], skip_dir, first))
                    return false;
                if (f0 < 0 || f1 < 0) {
                    ASS_DVector zero_normal = {0, 0};
                    if (!emit_point(str, pt[0], zero_normal, OUTLINE_LINE_SEGMENT, skip_dir) ||
                        !emit_point(str, pt[2], zero_normal, OUTLINE_LINE_SEGMENT, skip_dir))
                        return false;
                } else {
                    double mul = f0 / abs_s;
                    ASS_DVector offs = { normal[0].v.x * mul, normal[0].v.y * mul };
                    if (!emit_point(str, pt[0], offs, OUTLINE_LINE_SEGMENT, skip_dir))
                        return false;
                }
                dir &= ~skip_dir;
                if (!dir) {
                    str->last_normal = normal[1].v;
                    return true;
                }
            }
            check_dir ^= skip_dir;
        } else if (c + g0 < 1 && c + g1 < 1)
            check_dir ^= skip_dir;
    }

    ASS_DVector result;
    if (check_dir && estimate_quadratic_error(str, c, s, normal, &result)) {
        if (!emit_first_point(str, pt[0], OUTLINE_QUADRATIC_SPLINE, check_dir))
            return false;
        if (!emit_point(str, pt[1], result, 0, check_dir))
            return false;
        dir &= ~check_dir;
        if (!dir) {
            str->last_normal = normal[1].v;
            return true;
        }
    }

    ASS_Vector next[5];
    next[1].x = pt[0].x + pt[1].x;
    next[1].y = pt[0].y + pt[1].y;
    next[3].x = pt[1].x + pt[2].x;
    next[3].y = pt[1].y + pt[2].y;
    next[2].x = (next[1].x + next[3].x + 2) >> 2;
    next[2].y = (next[1].y + next[3].y + 2) >> 2;
    next[1].x >>= 1;
    next[1].y >>= 1;
    next[3].x >>= 1;
    next[3].y >>= 1;
    next[0] = pt[0];
    next[4] = pt[2];

    ASS_DVector next_deriv[3];
    next_deriv[0].x = deriv[0].x / 2;
    next_deriv[0].y = deriv[0].y / 2;
    next_deriv[2].x = deriv[1].x / 2;
    next_deriv[2].y = deriv[1].y / 2;
    next_deriv[1].x = (next_deriv[0].x + next_deriv[2].x) / 2;
    next_deriv[1].y = (next_deriv[0].y + next_deriv[2].y) / 2;

    double len = vec_len(next_deriv[1]);
    if (len < str->min_len) {  // check degenerate case
        if (!emit_first_point(str, next[0], OUTLINE_LINE_SEGMENT, dir))
            return false;
        if (!start_segment(str, next[2], normal[1].v, dir))
            return false;
        str->last_skip &= ~dir;
        return emit_point(str, next[2], normal[1].v, OUTLINE_LINE_SEGMENT, dir);
    }

    double scale = 1 / len;
    Normal next_normal[3] = {
        { normal[0].v, normal[0].len / 2 },
        { { next_deriv[1].x * scale, next_deriv[1].y * scale }, len },
        { normal[1].v, normal[1].len / 2 }
    };
    return process_quadratic(str, next + 0, next_deriv + 0, next_normal + 0, dir, first) &&
           process_quadratic(str, next + 2, next_deriv + 1, next_normal + 1, dir, false);
}

/**
 * \brief Process source quadratic spline
 * \param str stroker state
 * \param pt array of 3 source spline points
 * \param dir destination outline flags
 * \return false on allocation failure
 */
static bool add_quadratic(StrokerState *str, const ASS_Vector *pt, int dir)
{
    int32_t dx0 = pt[1].x - pt[0].x;
    int32_t dy0 = pt[1].y - pt[0].y;
    if (dx0 > -str->eps && dx0 < str->eps && dy0 > -str->eps && dy0 < str->eps)
        return add_line(str, pt[0], pt[2], dir);

    int32_t dx1 = pt[2].x - pt[1].x;
    int32_t dy1 = pt[2].y - pt[1].y;
    if (dx1 > -str->eps && dx1 < str->eps && dy1 > -str->eps && dy1 < str->eps)
        return add_line(str, pt[0], pt[2], dir);

    ASS_DVector deriv[2] = {
        { dy0 * str->yscale, -dx0 * str->xscale },
        { dy1 * str->yscale, -dx1 * str->xscale }
    };
    double len0 = vec_len(deriv[0]), scale0 = 1 / len0;
    double len1 = vec_len(deriv[1]), scale1 = 1 / len1;
    Normal normal[2] = {
        { { deriv[0].x * scale0, deriv[0].y * scale0 }, len0 },
        { { deriv[1].x * scale1, deriv[1].y * scale1 }, len1 }
    };

    bool first = str->contour_start;
    return start_segment(str, pt[0], normal[0].v, dir) &&
        process_quadratic(str, pt, deriv, normal, dir, first);
}


enum {
    FLAG_INTERSECTION =  1,
    FLAG_ZERO_0       =  2,
    FLAG_ZERO_1       =  4,
    FLAG_CLIP_0       =  8,
    FLAG_CLIP_1       = 16,
    FLAG_DIR_2        = 32,

    FLAG_COUNT        =  6,

    MASK_INTERSECTION = FLAG_INTERSECTION << FLAG_COUNT,
    MASK_ZERO_0       = FLAG_ZERO_0       << FLAG_COUNT,
    MASK_ZERO_1       = FLAG_ZERO_1       << FLAG_COUNT,
    MASK_CLIP_0       = FLAG_CLIP_0       << FLAG_COUNT,
    MASK_CLIP_1       = FLAG_CLIP_1       << FLAG_COUNT,
};

/**
 * \brief Estimate error for cubic spline
 * \param str stroker state
 * \param c dot product between normal[0] and normal[1]
 * \param s cross product between normal[0] and normal[1]
 * \param dc dot products between normals and central points difference in normal space
 * \param dc cross products between normals and central points difference in normal space
 * \param normal first and last spline normal
 * \param result best offsets for central spline points (second & third)
 * \return flags for destination outlines that do not require subdivision
 */
static int estimate_cubic_error(StrokerState *str, double c, double s,
                                const double *dc, const double *ds,
                                const Normal *normal, ASS_DVector *result,
                                int check_flags, int dir)
{
    double t = (ds[0] + ds[1]) / (dc[0] + dc[1]), c1 = 1 + c, ss = s * s;
    double ts = t * s, tt = t * t, ttc = tt * c1, ttcc = ttc * c1;

    const double w = 0.4;
    double f0[] = {
        10 * w * (c - 1) + 9 * w * tt * c,
        2 * (c - 1) + 3 * tt + 2 * ts,
        2 * (c - 1) + 3 * tt - 2 * ts,
    };
    double f1[] = {
        18 * w * (ss - ttc * c),
        2 * ss - 6 * ttc - 2 * ts * (c + 4),
        2 * ss - 6 * ttc + 2 * ts * (c + 4),
    };
    double f2[] = {
        9 * w * (ttcc - ss) * c,
        3 * ss + 3 * ttcc + 6 * ts * c1,
        3 * ss + 3 * ttcc - 6 * ts * c1,
    };

    double aa = 0, ab = 0;
    double ch = sqrt(c1 / 2);
    double inv_ro0 = 1.5 * ch * (ch + 1);
    for (int i = 0; i < 3; i++) {
        double a = 2 * f2[i] + f1[i] * inv_ro0;
        double b = f2[i] - f0[i] * inv_ro0 * inv_ro0;
        aa += a * a;  ab += a * b;
    }
    double ro = ab / (aa * inv_ro0 + 1e-9);  // best fit

    double err2 = 0;
    for (int i = 0; i < 3; i++) {
        double err = f0[i] + ro * (f1[i] + ro * f2[i]);
        err2 += err * err;
    }
    if (!(err2 < str->err_c))  // check radial error
        return 0;

    double r = ro * c1 - 1;
    double ro0 = t * r - ro * s;
    double ro1 = t * r + ro * s;

    int check_dir = check_flags & FLAG_DIR_2 ? 2 : 1;
    if (dir & check_dir) {
        double test_s = s, test0 = ro0, test1 = ro1;
        if (check_flags & FLAG_DIR_2) {
            test_s = -test_s;
            test0 = -test0;
            test1 = -test1;
        }
        int flags = 0;
        if (2 * test_s * r < dc[0] + dc[1]) flags |= FLAG_INTERSECTION;
        if (normal[0].len - test0 < 0) flags |= FLAG_ZERO_0;
        if (normal[1].len + test1 < 0) flags |= FLAG_ZERO_1;
        if (normal[0].len + dc[0] + test_s - test1 * c < 0) flags |= FLAG_CLIP_0;
        if (normal[1].len + dc[1] + test_s + test0 * c < 0) flags |= FLAG_CLIP_1;
        if ((flags ^ check_flags) & (check_flags >> FLAG_COUNT)) {
            dir &= ~check_dir;
            if (!dir)
                return 0;
        }
    }

    double d0c = 2 * dc[0], d0s = 2 * ds[0];
    double d1c = 2 * dc[1], d1s = 2 * ds[1];
    double dot0 = d0c + 3 * normal[0].len, crs0 = d0s + 3 * ro0 * normal[0].len;
    double dot1 = d1c + 3 * normal[1].len, crs1 = d1s + 3 * ro1 * normal[1].len;
    // check angular error (stage 1)
    if (!(fabs(crs0) < str->err_a * dot0 && fabs(crs1) < str->err_a * dot1))
        return 0;

    double cl0 = c * normal[0].len, sl0 = +s * normal[0].len;
    double cl1 = c * normal[1].len, sl1 = -s * normal[1].len;
    dot0 = d0c - ro0 * d0s + cl0 + ro1 * sl0 + cl1 / 3;
    dot1 = d1c - ro1 * d1s + cl1 + ro0 * sl1 + cl0 / 3;
    crs0 = d0s + ro0 * d0c - sl0 + ro1 * cl0 - sl1 / 3;
    crs1 = d1s + ro1 * d1c - sl1 + ro0 * cl1 - sl0 / 3;
    // check angular error (stage 2)
    if (!(fabs(crs0) < str->err_a * dot0 && fabs(crs1) < str->err_a * dot1))
        return 0;

    result[0].x = normal[0].v.x + normal[0].v.y * ro0;
    result[0].y = normal[0].v.y - normal[0].v.x * ro0;
    result[1].x = normal[1].v.x + normal[1].v.y * ro1;
    result[1].y = normal[1].v.y - normal[1].v.x * ro1;
    return dir;
}

/**
 * \brief Helper function for cubic spline construction
 * \param str stroker state
 * \param pt array of 4 source spline points
 * \param deriv array of 3 differences between adjacent points in normal space
 * \param normal first and last spline normal
 * \param dir destination outline flags
 * \param first true if the current part is at start of the segment
 * \return false on allocation failure
 */
static bool process_cubic(StrokerState *str, const ASS_Vector *pt,
                          const ASS_DVector *deriv, const Normal *normal,
                          int dir, bool first)
{
    double c = vec_dot(normal[0].v, normal[1].v);
    double s = vec_crs(normal[0].v, normal[1].v);
    double dc[] = { vec_dot(normal[0].v, deriv[1]), vec_dot(normal[1].v, deriv[1]) };
    double ds[] = { vec_crs(normal[0].v, deriv[1]), vec_crs(normal[1].v, deriv[1]) };
    double f0 = normal[0].len * c + normal[1].len + dc[1];
    double f1 = normal[1].len * c + normal[0].len + dc[0];
    double g0 = normal[0].len * s - ds[1];
    double g1 = normal[1].len * s + ds[0];

    double abs_s = s;
    int check_dir = dir, skip_dir = 2;
    int flags = FLAG_INTERSECTION | FLAG_DIR_2;
    if (s < 0) {
        abs_s = -s;
        skip_dir = 1;
        flags = 0;
        g0 = -g0;
        g1 = -g1;
    }

    if (!(dc[0] + dc[1] > 0))
        check_dir = 0;
    else if (dir & skip_dir) {
        if (f0 < abs_s && f1 < abs_s) {  // check for self-intersection
            double d2 = (f0 + dc[1]) * normal[1].len + (f1 + dc[0]) * normal[0].len;
            d2 = (d2 + vec_dot(deriv[1], deriv[1])) / 2;
            if (d2 < g0 && d2 < g1) {
                double q = sqrt(d2 / (2 - d2));
                double h0 = (f0 * q + g0) * normal[1].len;
                double h1 = (f1 * q + g1) * normal[0].len;
                q *= (4.0 / 3) * d2;
                if (h0 > q && h1 > q) {
                    if (!prepare_skip(str, pt[0], skip_dir, first))
                        return false;
                    if (f0 < 0 || f1 < 0) {
                        ASS_DVector zero_normal = {0, 0};
                        if (!emit_point(str, pt[0], zero_normal, OUTLINE_LINE_SEGMENT, skip_dir) ||
                            !emit_point(str, pt[3], zero_normal, OUTLINE_LINE_SEGMENT, skip_dir))
                            return false;
                    } else {
                        double mul = f0 / abs_s;
                        ASS_DVector offs = { normal[0].v.x * mul, normal[0].v.y * mul };
                        if (!emit_point(str, pt[0], offs, OUTLINE_LINE_SEGMENT, skip_dir))
                            return false;
                    }
                    dir &= ~skip_dir;
                    if (!dir) {
                        str->last_normal = normal[1].v;
                        return true;
                    }
                }
            }
            check_dir ^= skip_dir;
        } else {
            if (ds[0] < 0)
                flags ^= MASK_INTERSECTION;
            if (ds[1] < 0)
                flags ^= MASK_INTERSECTION | FLAG_INTERSECTION;
            bool parallel = flags & MASK_INTERSECTION;
            int badness = parallel ? 0 : 1;
            if (c + g0 < 1) {
                if (parallel) {
                    flags ^= MASK_ZERO_0 | FLAG_ZERO_0;
                    if (c < 0)
                        flags ^= MASK_CLIP_0;
                    if (f0 > abs_s)
                        flags ^= FLAG_ZERO_0 | FLAG_CLIP_0;
                }
                badness++;
            } else {
                flags ^= MASK_INTERSECTION | FLAG_INTERSECTION;
                if (!parallel) {
                    flags ^= MASK_ZERO_0;
                    if (c > 0)
                        flags ^= MASK_CLIP_0;
                }
            }
            if (c + g1 < 1) {
                if (parallel) {
                    flags ^= MASK_ZERO_1 | FLAG_ZERO_1;
                    if (c < 0)
                        flags ^= MASK_CLIP_1;
                    if (f1 > abs_s)
                        flags ^= FLAG_ZERO_1 | FLAG_CLIP_1;
                }
                badness++;
            } else {
                flags ^= MASK_INTERSECTION;
                if (!parallel) {
                    flags ^= MASK_ZERO_1;
                    if (c > 0)
                        flags ^= MASK_CLIP_1;
                }
            }
            if (badness > 2)
                check_dir ^= skip_dir;
        }
    }

    ASS_DVector result[2];
    if (check_dir)
        check_dir = estimate_cubic_error(str, c, s, dc, ds,
                                         normal, result, flags, check_dir);
    if (check_dir) {
        if (!emit_first_point(str, pt[0], OUTLINE_CUBIC_SPLINE, check_dir))
            return false;
        if (!emit_point(str, pt[1], result[0], 0, check_dir) ||
            !emit_point(str, pt[2], result[1], 0, check_dir))
            return false;
        dir &= ~check_dir;
        if (!dir) {
            str->last_normal = normal[1].v;
            return true;
        }
    }

    ASS_Vector next[7], center;
    next[1].x = pt[0].x + pt[1].x;
    next[1].y = pt[0].y + pt[1].y;
    center.x = pt[1].x + pt[2].x + 2;
    center.y = pt[1].y + pt[2].y + 2;
    next[5].x = pt[2].x + pt[3].x;
    next[5].y = pt[2].y + pt[3].y;
    next[2].x = next[1].x + center.x;
    next[2].y = next[1].y + center.y;
    next[4].x = center.x + next[5].x;
    next[4].y = center.y + next[5].y;
    next[3].x = (next[2].x + next[4].x - 1) >> 3;
    next[3].y = (next[2].y + next[4].y - 1) >> 3;
    next[2].x >>= 2;
    next[2].y >>= 2;
    next[4].x >>= 2;
    next[4].y >>= 2;
    next[1].x >>= 1;
    next[1].y >>= 1;
    next[5].x >>= 1;
    next[5].y >>= 1;
    next[0] = pt[0];
    next[6] = pt[3];

    ASS_DVector next_deriv[5], center_deriv;
    next_deriv[0].x = deriv[0].x / 2;
    next_deriv[0].y = deriv[0].y / 2;
    center_deriv.x = deriv[1].x / 2;
    center_deriv.y = deriv[1].y / 2;
    next_deriv[4].x = deriv[2].x / 2;
    next_deriv[4].y = deriv[2].y / 2;
    next_deriv[1].x = (next_deriv[0].x + center_deriv.x) / 2;
    next_deriv[1].y = (next_deriv[0].y + center_deriv.y) / 2;
    next_deriv[3].x = (center_deriv.x + next_deriv[4].x) / 2;
    next_deriv[3].y = (center_deriv.y + next_deriv[4].y) / 2;
    next_deriv[2].x = (next_deriv[1].x + next_deriv[3].x) / 2;
    next_deriv[2].y = (next_deriv[1].y + next_deriv[3].y) / 2;

    double len = vec_len(next_deriv[2]);
    if (len < str->min_len) {  // check degenerate case
        Normal next_normal[4];
        next_normal[0].v = normal[0].v;
        next_normal[0].len = normal[0].len / 2;
        next_normal[3].v = normal[1].v;
        next_normal[3].len = normal[1].len / 2;

        next_deriv[1].x += next_deriv[2].x;
        next_deriv[1].y += next_deriv[2].y;
        next_deriv[3].x += next_deriv[2].x;
        next_deriv[3].y += next_deriv[2].y;
        next_deriv[2].x = next_deriv[2].y = 0;

        double len1 = vec_len(next_deriv[1]);
        if (len1 < str->min_len) {
            next_normal[1] = normal[0];
        } else {
            double scale = 1 / len1;
            next_normal[1].v.x = next_deriv[1].x * scale;
            next_normal[1].v.y = next_deriv[1].y * scale;
            next_normal[1].len = len1;
        }

        double len2 = vec_len(next_deriv[3]);
        if (len2 < str->min_len) {
            next_normal[2] = normal[1];
        } else {
            double scale = 1 / len2;
            next_normal[2].v.x = next_deriv[3].x * scale;
            next_normal[2].v.y = next_deriv[3].y * scale;
            next_normal[2].len = len2;
        }

        if (len1 < str->min_len) {
            if (!emit_first_point(str, next[0], OUTLINE_LINE_SEGMENT, dir))
                return false;
        } else {
            if (!process_cubic(str, next + 0, next_deriv + 0, next_normal + 0, dir, first))
                return false;
        }
        if (!start_segment(str, next[2], next_normal[2].v, dir))
            return false;
        if (len2 < str->min_len) {
            if (!emit_first_point(str, next[3], OUTLINE_LINE_SEGMENT, dir))
                return false;
        } else {
            if (!process_cubic(str, next + 3, next_deriv + 2, next_normal + 2, dir, false))
                return false;
        }
        return true;
    }

    double scale = 1 / len;
    Normal next_normal[3] = {
        { normal[0].v, normal[0].len / 2 },
        { { next_deriv[2].x * scale, next_deriv[2].y * scale }, len },
        { normal[1].v, normal[1].len / 2 }
    };
    return process_cubic(str, next + 0, next_deriv + 0, next_normal + 0, dir, first) &&
           process_cubic(str, next + 3, next_deriv + 2, next_normal + 1, dir, false);
}

/**
 * \brief Process source cubic spline
 * \param str stroker state
 * \param pt array of 4 source spline points
 * \param dir destination outline flags
 * \return false on allocation failure
 */
static bool add_cubic(StrokerState *str, const ASS_Vector *pt, int dir)
{
    int flags = 9;

    int32_t dx0 = pt[1].x - pt[0].x;
    int32_t dy0 = pt[1].y - pt[0].y;
    if (dx0 > -str->eps && dx0 < str->eps && dy0 > -str->eps && dy0 < str->eps) {
        dx0 = pt[2].x - pt[0].x;
        dy0 = pt[2].y - pt[0].y;
        if (dx0 > -str->eps && dx0 < str->eps && dy0 > -str->eps && dy0 < str->eps)
            return add_line(str, pt[0], pt[3], dir);
        flags ^= 1;
    }

    int32_t dx2 = pt[3].x - pt[2].x;
    int32_t dy2 = pt[3].y - pt[2].y;
    if (dx2 > -str->eps && dx2 < str->eps && dy2 > -str->eps && dy2 < str->eps) {
        dx2 = pt[3].x - pt[1].x;
        dy2 = pt[3].y - pt[1].y;
        if (dx2 > -str->eps && dx2 < str->eps && dy2 > -str->eps && dy2 < str->eps)
            return add_line(str, pt[0], pt[3], dir);
        flags ^= 4;
    }

    if (flags == 12)
        return add_line(str, pt[0], pt[3], dir);

    int32_t dx1 = pt[flags >> 2].x - pt[flags & 3].x;
    int32_t dy1 = pt[flags >> 2].y - pt[flags & 3].y;

    ASS_DVector deriv[3] = {
        { dy0 * str->yscale, -dx0 * str->xscale },
        { dy1 * str->yscale, -dx1 * str->xscale },
        { dy2 * str->yscale, -dx2 * str->xscale }
    };
    double len0 = vec_len(deriv[0]), scale0 = 1 / len0;
    double len2 = vec_len(deriv[2]), scale2 = 1 / len2;
    Normal normal[2] = {
        { { deriv[0].x * scale0, deriv[0].y * scale0 }, len0 },
        { { deriv[2].x * scale2, deriv[2].y * scale2 }, len2 }
    };

    bool first = str->contour_start;
    return start_segment(str, pt[0], normal[0].v, dir) &&
        process_cubic(str, pt, deriv, normal, dir, first);
}


/**
 * \brief Process contour closing
 * \param str stroker state
 * \param last_point last contour point
 * \param dir destination outline flags
 * \return false on allocation failure
 */
static bool close_contour(StrokerState *str, ASS_Vector last_point, int dir)
{
    if (str->contour_start) {
        if ((dir & 3) == 3)
            dir = 1;
        if (!draw_circle(str, last_point, dir))
            return false;
    } else {
        if (!add_line(str, last_point, str->first_point, dir))
            return false;
        if (!start_segment(str, str->first_point, str->first_normal, dir))
            return false;
        if (!emit_point(str, str->first_point, str->first_normal, OUTLINE_LINE_SEGMENT,
                       ~str->last_skip & dir & str->first_skip))
            return false;
        if (str->last_normal.x != str->first_normal.x ||
            str->last_normal.y != str->first_normal.y)
            fix_first_point(str, str->first_point, str->last_normal,
                           ~str->last_skip & dir & ~str->first_skip);
        str->contour_start = true;
    }
    if ((dir & 1) && !outline_close_contour(str->result[0]))
        return false;
    if ((dir & 2) && !outline_close_contour(str->result[1]))
        return false;
    str->contour_first[0] = str->result[0]->n_points;
    str->contour_first[1] = str->result[1]->n_points;
    return true;
}


/*
 * Stroke an outline glyph in x/y direction.
 * \param result first result outline
 * \param result1 second result outline
 * \param path source outline
 * \param xbord border size in X direction
 * \param ybord border size in Y direction
 * \param eps approximate allowable error
 * \return false on allocation failure
 */
bool outline_stroke(ASS_Outline *result, ASS_Outline *result1,
                    const ASS_Outline *path, int xbord, int ybord, int eps)
{
    const int dir = 3;
    int rad = FFMAX(xbord, ybord);
    assert(rad >= eps);

    result->n_points = result->n_segments = 0;
    result1->n_points = result1->n_segments = 0;

    StrokerState str;
    str.result[0] = result;
    str.result[1] = result1;
    str.contour_first[0] = 0;
    str.contour_first[1] = 0;
    str.xbord = xbord;
    str.ybord = ybord;
    str.xscale = 1.0 / FFMAX(eps, xbord);
    str.yscale = 1.0 / FFMAX(eps, ybord);
    str.eps = eps;

    str.contour_start = true;
    double rel_err = (double) eps / rad;
    str.merge_cos = 1 - rel_err;
    double e = sqrt(2 * rel_err);
    str.split_cos = 1 + 8 * rel_err - 4 * (1 + rel_err) * e;
    str.min_len = rel_err / 4;
    str.err_q = 8 * (1 + rel_err) * (1 + rel_err);
    str.err_c = 390 * rel_err * rel_err;
    str.err_a = e;

    for (size_t i = 0; i < path->n_points; i++) {
        if (path->points[i].x < OUTLINE_MIN || path->points[i].x > OUTLINE_MAX)
            return false;
        if (path->points[i].y < OUTLINE_MIN || path->points[i].y > OUTLINE_MAX)
            return false;
    }

    ASS_Vector *start = path->points, *cur = start;
    for (size_t i = 0; i < path->n_segments; i++) {
        int n = path->segments[i] & OUTLINE_COUNT_MASK;
        cur += n;

        ASS_Vector *end = cur, p[4];
        if (path->segments[i] & OUTLINE_CONTOUR_END) {
            end = start;
            start = cur;
        }

        switch (n) {
        case OUTLINE_LINE_SEGMENT:
            if (!add_line(&str, cur[-1], *end, dir))
                return false;
            break;

        case OUTLINE_QUADRATIC_SPLINE:
            p[0] = cur[-2];
            p[1] = cur[-1];
            p[2] = *end;
            if (!add_quadratic(&str, p, dir))
                return false;
            break;

        case OUTLINE_CUBIC_SPLINE:
            p[0] = cur[-3];
            p[1] = cur[-2];
            p[2] = cur[-1];
            p[3] = *end;
            if (!add_cubic(&str, p, dir))
                return false;
            break;

        default:
            return false;
        }

        if (start == cur && !close_contour(&str, *end, dir))
            return false;
    }
    assert(start == cur && cur == path->points + path->n_points);
    return true;
}
