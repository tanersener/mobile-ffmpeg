#ifndef VASPRINTF_H
#define VASPRINTF_H
#include <config.h>

#ifndef HAVE_VASPRINTF

int _gnutls_vasprintf(char **strp, const char *fmt, va_list ap);
#define vasprintf _gnutls_vasprintf

#endif

#endif
