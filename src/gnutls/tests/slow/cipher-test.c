#include <config.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <utils.h>
#include <stdlib.h>
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#include <gnutls/self-test.h>
#include <signal.h>

/* This does check the AES and SHA implementation against test vectors.
 * This should not run under valgrind in order to use the native
 * cpu instructions (AES-NI or padlock).
 */

#if defined(WIN32)
int main(int argc, char **argv)
{
	exit(77);
}
#else
# include <unistd.h>

static void handle_sigill(int sig)
{
	_exit(0);
}

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}

#ifndef ENABLE_SELF_CHECKS
# define AVOID_INTERNALS
# include "../../lib/crypto-selftests.c"
# include "../../lib/crypto-selftests-pk.c"
#endif

int main(int argc, char **argv)
{
	gnutls_global_set_log_function(tls_log_func);
	if (argc > 1)
		gnutls_global_set_log_level(4711);

	global_init();
	signal(SIGILL, handle_sigill);

	/* ciphers */
	if (gnutls_cipher_self_test(1, 0) < 0)
		return 1;

	/* message digests */
	if (gnutls_digest_self_test(1, 0) < 0)
		return 1;

	/* MAC */
	if (gnutls_mac_self_test(1, 0) < 0)
		return 1;

	/* PK */
	if (gnutls_pk_self_test(1, 0) < 0)
		return 1;

	gnutls_global_deinit();
	return 0;
}

#endif
