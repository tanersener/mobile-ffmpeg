#include <config.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <utils.h>
#include <stdlib.h>
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#include <gnutls/abstract.h>
#include <gnutls/x509.h>
#include <assert.h>

unsigned audit_called = 0;

/* This does check the FIPS140 override support with
 * gnutls_fips140_set_mode().
 */

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d>| %s", level, str);
}

static void audit_log_func(gnutls_session_t session, const char *str)
{
	audit_called = 1;
}


static void try_crypto(void)
{
	static uint8_t key16[16];
	static uint8_t iv16[16];
	gnutls_datum_t key = { key16, sizeof(key16) };
	gnutls_datum_t iv = { iv16, sizeof(iv16) };
	gnutls_cipher_hd_t ch;
	gnutls_hmac_hd_t mh;
	int ret;
	gnutls_x509_privkey_t privkey;

	ret =
	    gnutls_cipher_init(&ch, GNUTLS_CIPHER_ARCFOUR_128, &key, &iv);
	if (ret < 0) {
		fail("gnutls_cipher_init failed\n");
	}
	gnutls_cipher_deinit(ch);

	ret =
	    gnutls_cipher_init(&ch, GNUTLS_CIPHER_AES_128_CBC, &key, &iv);
	if (ret < 0) {
		fail("gnutls_cipher_init failed\n");
	}
	gnutls_cipher_deinit(ch);

	ret = gnutls_hmac_init(&mh, GNUTLS_MAC_MD5, key.data, key.size);
	if (ret < 0) {
		fail("gnutls_hmac_init failed\n");
	}
	gnutls_hmac_deinit(mh, NULL);

	ret = gnutls_hmac_init(&mh, GNUTLS_MAC_SHA1, key.data, key.size);
	if (ret < 0) {
		fail("gnutls_hmac_init failed\n");
	}
	gnutls_hmac_deinit(mh, NULL);

	ret = gnutls_rnd(GNUTLS_RND_NONCE, key16, sizeof(key16));
	if (ret < 0) {
		fail("gnutls_rnd failed\n");
	}

	assert(gnutls_x509_privkey_init(&privkey) == 0);
	ret = gnutls_x509_privkey_generate(privkey, GNUTLS_PK_RSA, 512, 0);
	if (ret < 0) {
		fail("gnutls_x509_privkey_generate failed for 512-bit key\n");
	}
	gnutls_x509_privkey_deinit(privkey);
}

void doit(void)
{
	int ret;
	unsigned int mode;

	fprintf(stderr,
		"Please note that if in FIPS140 mode, you need to assure the library's integrity prior to running this test\n");

	gnutls_global_set_log_function(tls_log_func);
	gnutls_global_set_audit_log_function(audit_log_func);
	if (debug)
		gnutls_global_set_log_level(4711);

	mode = gnutls_fips140_mode_enabled();
	if (mode == 0) {
		success("We are not in FIPS140 mode\n");
		exit(77);
	}

	ret = global_init();
	if (ret < 0) {
		fail("Cannot initialize library\n");
	}

	/* switch to lax mode and check whether forbidden algorithms are accessible */
	gnutls_fips140_set_mode(GNUTLS_FIPS140_LAX, 0);

	try_crypto();

	/* check whether audit log was called */
	if (audit_called) {
		fail("the audit function was called in lax mode!\n");
	}

	gnutls_fips140_set_mode(GNUTLS_FIPS140_LOG, 0);

	try_crypto();

	/* check whether audit log was called */
	if (!audit_called) {
		fail("the audit function was not called in log mode!\n");
	}

	gnutls_fips140_set_mode(GNUTLS_FIPS140_SELFTESTS, 0);
	if (gnutls_fips140_mode_enabled() != GNUTLS_FIPS140_STRICT)
		fail("switching to selftests didn't switch the lib to the expected mode\n");

	gnutls_fips140_set_mode(532, 0);
	if (gnutls_fips140_mode_enabled() != GNUTLS_FIPS140_STRICT)
		fail("switching to unknown mode didn't switch the lib to the expected mode\n");

	GNUTLS_FIPS140_SET_LAX_MODE();
	if (gnutls_fips140_mode_enabled() != GNUTLS_FIPS140_LAX)
		fail("switching to lax mode did not succeed!\n");

	GNUTLS_FIPS140_SET_STRICT_MODE();
	if (gnutls_fips140_mode_enabled() != GNUTLS_FIPS140_STRICT)
		fail("switching to strict mode did not succeed!\n");

	gnutls_global_deinit();
	return;
}
