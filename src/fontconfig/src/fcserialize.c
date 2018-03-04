/*
 * Copyright © 2006 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include "fcint.h"

intptr_t
FcAlignSize (intptr_t size)
{
    intptr_t	rem = size % sizeof (FcAlign);
    if (rem)
	size += sizeof (FcAlign) - rem;
    return size;
}

/*
 * Serialization helper object -- allocate space in the
 * yet-to-be-created linear array for a serialized font set
 */

FcSerialize *
FcSerializeCreate (void)
{
    FcSerialize	*serialize;

    serialize = malloc (sizeof (FcSerialize));
    if (!serialize)
	return NULL;
    serialize->size = 0;
    serialize->linear = NULL;
    serialize->cs_freezer = NULL;
    memset (serialize->buckets, '\0', sizeof (serialize->buckets));
    return serialize;
}

void
FcSerializeDestroy (FcSerialize *serialize)
{
    uintptr_t	bucket;

    for (bucket = 0; bucket < FC_SERIALIZE_HASH_SIZE; bucket++)
    {
	FcSerializeBucket   *buck, *next;

	for (buck = serialize->buckets[bucket]; buck; buck = next) {
	    next = buck->next;
	    free (buck);
	}
    }
    if (serialize->cs_freezer)
	FcCharSetFreezerDestroy (serialize->cs_freezer);
    free (serialize);
}

/*
 * Allocate space for an object in the serialized array. Keep track
 * of where the object is placed and only allocate one copy of each object
 */

FcBool
FcSerializeAlloc (FcSerialize *serialize, const void *object, int size)
{
    uintptr_t	bucket = ((uintptr_t) object) % FC_SERIALIZE_HASH_SIZE;
    FcSerializeBucket  *buck;

    for (buck = serialize->buckets[bucket]; buck; buck = buck->next)
	if (buck->object == object)
	    return FcTrue;
    buck = malloc (sizeof (FcSerializeBucket));
    if (!buck)
	return FcFalse;
    buck->object = object;
    buck->offset = serialize->size;
    buck->next = serialize->buckets[bucket];
    serialize->buckets[bucket] = buck;
    serialize->size += FcAlignSize (size);
    return FcTrue;
}

/*
 * Reserve space in the serialization array
 */
intptr_t
FcSerializeReserve (FcSerialize *serialize, int size)
{
    intptr_t	offset = serialize->size;
    serialize->size += FcAlignSize (size);
    return offset;
}

/*
 * Given an object, return the offset in the serialized array where
 * the serialized copy of the object is stored
 */
intptr_t
FcSerializeOffset (FcSerialize *serialize, const void *object)
{
    uintptr_t	bucket = ((uintptr_t) object) % FC_SERIALIZE_HASH_SIZE;
    FcSerializeBucket  *buck;

    for (buck = serialize->buckets[bucket]; buck; buck = buck->next)
	if (buck->object == object)
	    return buck->offset;
    return 0;
}

/*
 * Given a cache and an object, return a pointer to where
 * the serialized copy of the object is stored
 */
void *
FcSerializePtr (FcSerialize *serialize, const void *object)
{
    intptr_t	offset = FcSerializeOffset (serialize, object);

    if (!offset)
	return NULL;
    return (void *) ((char *) serialize->linear + offset);
}

FcBool
FcStrSerializeAlloc (FcSerialize *serialize, const FcChar8 *str)
{
    return FcSerializeAlloc (serialize, str, strlen ((const char *) str) + 1);
}

FcChar8 *
FcStrSerialize (FcSerialize *serialize, const FcChar8 *str)
{
    FcChar8 *str_serialize = FcSerializePtr (serialize, str);
    if (!str_serialize)
	return NULL;
    strcpy ((char *) str_serialize, (const char *) str);
    return str_serialize;
}
#include "fcaliastail.h"
#undef __fcserialize__
