/*
 * fontconfig/doc/edit-sgml.c
 *
 * Copyright © 2003 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the author(s) not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  The authors make no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * THE AUTHOR(S) DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void *
New (int size);

static void *
Reallocate (void *p, int size);

static void
Dispose (void *p);

typedef enum { False, True } Bool;

typedef struct {
    char    *buf;
    int	    size;
    int	    len;
} String;

static String *
StringNew (void);

static void
StringAdd (String *s, char c);

static void
StringAddString (String *s, char *buf);

static String *
StringMake (char *buf);

static void
StringDel (String *s);

static void
StringPut (FILE *f, String *s);

static void
StringDispose (String *s);

typedef struct {
    String  *tag;
    String  *text;
} Replace;

static Replace *
ReplaceNew (void);

static void
ReplaceDispose (Replace *r);

static void
Bail (const char *format, int line, const char *arg);

static Replace *
ReplaceRead (FILE *f, int *linep);

typedef struct _replaceList {
    struct _replaceList	*next;
    Replace		*r;
} ReplaceList;

static ReplaceList *
ReplaceListNew (Replace *r, ReplaceList *next);

static void
ReplaceListDispose (ReplaceList *l);

typedef struct {
    ReplaceList	*head;
} ReplaceSet;

static ReplaceSet *
ReplaceSetNew (void);

static void
ReplaceSetDispose (ReplaceSet *s);

static void
ReplaceSetAdd (ReplaceSet *s, Replace *r);

static Replace *
ReplaceSetFind (ReplaceSet *s, char *tag);

static ReplaceSet *
ReplaceSetRead (FILE *f, int *linep);

typedef struct _skipStack {
    struct _skipStack	*prev;
    int			skipping;
} SkipStack;

static SkipStack *
SkipStackPush (SkipStack *prev, int skipping);

static SkipStack *
SkipStackPop (SkipStack *prev);

typedef struct _loopStack {
    struct _loopStack	*prev;
    String		*tag;
    String		*extra;
    long		pos;
} LoopStack;

static LoopStack *
LoopStackPush (LoopStack *prev, FILE *f, char *tag);

static LoopStack *
LoopStackLoop (ReplaceSet *rs, LoopStack *ls, FILE *f);

static void
LineSkip (FILE *f, int *linep);

static void
DoReplace (FILE *f, int *linep, ReplaceSet *s);

#define STRING_INIT 128

static void *
New (int size)
{
    void    *m = malloc (size);
    if (!m)
	abort ();
    return m;
}

static void *
Reallocate (void *p, int size)
{
    void    *r = realloc (p, size);

    if (!r)
	abort ();
    return r;
}

static void
Dispose (void *p)
{
    free (p);
}

static String *
StringNew (void)
{
    String  *s;

    s = New (sizeof (String));
    s->buf = New (STRING_INIT);
    s->size = STRING_INIT - 1;
    s->buf[0] = '\0';
    s->len = 0;
    return s;
}

static void
StringAdd (String *s, char c)
{
    if (s->len == s->size)
	s->buf = Reallocate (s->buf, (s->size *= 2) + 1);
    s->buf[s->len++] = c;
    s->buf[s->len] = '\0';
}

static void
StringAddString (String *s, char *buf)
{
    while (*buf)
	StringAdd (s, *buf++);
}

static String *
StringMake (char *buf)
{
    String  *s = StringNew ();
    StringAddString (s, buf);
    return s;
}

static void
StringDel (String *s)
{
    if (s->len)
	s->buf[--s->len] = '\0';
}

static void
StringPut (FILE *f, String *s)
{
    char    *b = s->buf;

    while (*b)
	putc (*b++, f);
}

#define StringLast(s)	((s)->len ? (s)->buf[(s)->len - 1] : '\0')

static void
StringDispose (String *s)
{
    Dispose (s->buf);
    Dispose (s);
}

static Replace *
ReplaceNew (void)
{
    Replace *r = New (sizeof (Replace));
    r->tag = StringNew ();
    r->text = StringNew ();
    return r;
}

static void
ReplaceDispose (Replace *r)
{
    StringDispose (r->tag);
    StringDispose (r->text);
    Dispose (r);
}

static void
Bail (const char *format, int line, const char *arg)
{
    fprintf (stderr, "fatal: ");
    fprintf (stderr, format, line, arg);
    fprintf (stderr, "\n");
    exit (1);
}

static int
Getc (FILE *f, int *linep)
{
    int	c = getc (f);
    if (c == '\n')
	++(*linep);
    return c;
}

static void
Ungetc (int c, FILE *f, int *linep)
{
    if (c == '\n')
	--(*linep);
    ungetc (c, f);
}

static Replace *
ReplaceRead (FILE *f, int *linep)
{
    int	    c;
    Replace *r;

    while ((c = Getc (f, linep)) != '@')
    {
	if (c == EOF)
	    return 0;
    }
    r = ReplaceNew();
    while ((c = Getc (f, linep)) != '@')
    {
	if (c == EOF)
	{
	    ReplaceDispose (r);
	    return 0;
	}
	if (isspace (c))
	    Bail ("%d: invalid character after tag %s", *linep, r->tag->buf);
	StringAdd (r->tag, c);
    }
    if (r->tag->buf[0] == '\0')
    {
	ReplaceDispose (r);
	return 0;
    }
    while (isspace ((c = Getc (f, linep))))
	;
    Ungetc (c, f, linep);
    while ((c = Getc (f, linep)) != '@' && c != EOF)
	StringAdd (r->text, c);
    if (c == '@')
	Ungetc (c, f, linep);
    while (isspace (StringLast (r->text)))
	StringDel (r->text);
    if (StringLast(r->text) == '%')
    {
	StringDel (r->text);
	StringAdd (r->text, ' ');
    }
    return r;
}

static ReplaceList *
ReplaceListNew (Replace *r, ReplaceList *next)
{
    ReplaceList	*l = New (sizeof (ReplaceList));
    l->r = r;
    l->next = next;
    return l;
}

static void
ReplaceListDispose (ReplaceList *l)
{
    if (l)
    {
	ReplaceListDispose (l->next);
	ReplaceDispose (l->r);
	Dispose (l);
    }
}

static ReplaceSet *
ReplaceSetNew (void)
{
    ReplaceSet	*s = New (sizeof (ReplaceSet));
    s->head = 0;
    return s;
}

static void
ReplaceSetDispose (ReplaceSet *s)
{
    ReplaceListDispose (s->head);
    Dispose (s);
}

static void
ReplaceSetAdd (ReplaceSet *s, Replace *r)
{
    s->head = ReplaceListNew (r, s->head);
}

static Replace *
ReplaceSetFind (ReplaceSet *s, char *tag)
{
    ReplaceList	*l;

    for (l = s->head; l; l = l->next)
	if (!strcmp (tag, l->r->tag->buf))
	    return l->r;
    return 0;
}

static ReplaceSet *
ReplaceSetRead (FILE *f, int *linep)
{
    ReplaceSet	*s = ReplaceSetNew ();
    Replace	*r;

    while ((r = ReplaceRead (f, linep)))
    {
	while (ReplaceSetFind (s, r->tag->buf))
	    StringAdd (r->tag, '+');
	ReplaceSetAdd (s, r);
    }
    if (!s->head)
    {
	ReplaceSetDispose (s);
	s = 0;
    }
    return s;
}

static SkipStack *
SkipStackPush (SkipStack *prev, int skipping)
{
    SkipStack	*ss = New (sizeof (SkipStack));
    ss->prev = prev;
    ss->skipping = skipping;
    return ss;
}

static SkipStack *
SkipStackPop (SkipStack *prev)
{
    SkipStack	*ss = prev->prev;
    Dispose (prev);
    return ss;
}

static LoopStack *
LoopStackPush (LoopStack *prev, FILE *f, char *tag)
{
    LoopStack	*ls = New (sizeof (LoopStack));
    ls->prev = prev;
    ls->tag = StringMake (tag);
    ls->extra = StringNew ();
    ls->pos = ftell (f);
    return ls;
}

static LoopStack *
LoopStackLoop (ReplaceSet *rs, LoopStack *ls, FILE *f)
{
    String	*s = StringMake (ls->tag->buf);
    LoopStack	*ret = ls;
    Bool	loop;

    StringAdd (ls->extra, '+');
    StringAddString (s, ls->extra->buf);
    loop = ReplaceSetFind (rs, s->buf) != 0;
    StringDispose (s);
    if (loop)
	fseek (f, ls->pos, SEEK_SET);
    else
    {
	ret = ls->prev;
	StringDispose (ls->tag);
	StringDispose (ls->extra);
	Dispose (ls);
    }
    return ret;
}

static void
LineSkip (FILE *f, int *linep)
{
    int	c;

    while ((c = Getc (f, linep)) == '\n')
	;
    Ungetc (c, f, linep);
}

static void
DoReplace (FILE *f, int *linep, ReplaceSet *s)
{
    int		c;
    String	*tag;
    Replace	*r;
    SkipStack	*ss = 0;
    LoopStack	*ls = 0;
    int		skipping = 0;

    while ((c = Getc (f, linep)) != EOF)
    {
	if (c == '@')
	{
	    tag = StringNew ();
	    while ((c = Getc (f, linep)) != '@')
	    {
		if (c == EOF)
		    abort ();
		StringAdd (tag, c);
	    }
	    if (ls)
		StringAddString (tag, ls->extra->buf);
	    switch (tag->buf[0]) {
	    case '?':
		ss = SkipStackPush (ss, skipping);
		if (!ReplaceSetFind (s, tag->buf + 1))
		    skipping++;
		LineSkip (f, linep);
		break;
	    case ':':
		if (!ss)
		    abort ();
		if (ss->skipping == skipping)
		    ++skipping;
		else
		    --skipping;
		LineSkip (f, linep);
		break;
	    case ';':
		skipping = ss->skipping;
		ss = SkipStackPop (ss);
		LineSkip (f, linep);
		break;
	    case '{':
		ls = LoopStackPush (ls, f, tag->buf + 1);
		LineSkip (f, linep);
		break;
	    case '}':
		ls = LoopStackLoop (s, ls, f);
		LineSkip (f, linep);
		break;
	    default:
		r = ReplaceSetFind (s, tag->buf);
		if (r && !skipping)
		    StringPut (stdout, r->text);
		break;
	    }
	    StringDispose (tag);
	}
	else if (!skipping)
	    putchar (c);
    }
}

int
main (int argc, char **argv)
{
    FILE	*f;
    ReplaceSet	*s;
    int		iline, oline;

    if (!argv[1])
	Bail ("usage: %*s <template.sgml>", 0, argv[0]);
    f = fopen (argv[1], "r");
    if (!f)
    {
	Bail ("can't open file %s", 0, argv[1]);
	exit (1);
    }
    iline = 1;
    while ((s = ReplaceSetRead (stdin, &iline)))
    {
	oline = 1;
	DoReplace (f, &oline, s);
	ReplaceSetDispose (s);
	rewind (f);
    }
    if (ferror (stdout))
	Bail ("%s", 0, "error writing output");
    exit (0);
}
