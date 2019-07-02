#ifndef GNUTLS_TESTS_X509SIGN_VERIFY_COMMON_H
#define GNUTLS_TESTS_X509SIGN_VERIFY_COMMON_H

static void tls_log_func(int level, const char *str)
{
	fprintf(stderr, "<%d> %s", level, str);
}

/* sha1 hash of "hello" string */
const gnutls_datum_t sha1_data = {
	(void *)
	    "\xaa\xf4\xc6\x1d\xdc\xc5\xe8\xa2\xda\xbe"
	    "\xde\x0f\x3b\x48\x2c\xd9\xae\xa9\x43\x4d",
	20
};

/* sha1 hash of "hello" string */
const gnutls_datum_t sha256_data = {
	(void *)
	    "\x2c\xf2\x4d\xba\x5f\xb0\xa3\x0e\x26\xe8"
	    "\x3b\x2a\xc5\xb9\xe2\x9e\x1b\x16\x1e\x5c"
	    "\x1f\xa7\x42\x5e\x73\x04\x33\x62\x93\x8b"
	    "\x98\x24",
	32
};

/* gost r 34.11-94 hash of "hello" string */
const gnutls_datum_t gostr94_data = {
	(void *)
	    "\x92\xea\x6d\xdb\xaf\x40\x02\x0d\xf3\x65"
	    "\x1f\x27\x8f\xd7\x15\x12\x17\xa2\x4a\xa8"
	    "\xd2\x2e\xbd\x25\x19\xcf\xd4\xd8\x9e\x64"
	    "\x50\xea",
	32
};

/* Streebog-256 hash of "hello" string */
const gnutls_datum_t streebog256_data = {
	(void *)
	    "\x3f\xb0\x70\x0a\x41\xce\x6e\x41\x41\x3b"
	    "\xa7\x64\xf9\x8b\xf2\x13\x5b\xa6\xde\xd5"
	    "\x16\xbe\xa2\xfa\xe8\x42\x9c\xc5\xbd\xd4"
	    "\x6d\x6d",
	32
};

/* Streebog-512 hash of "hello" string */
const gnutls_datum_t streebog512_data = {
	(void *)
	    "\x8d\xf4\x14\x26\x09\x66\xbe\xb7\xb3\x4d"
	    "\x92\x07\x63\x07\x9e\x15\xdf\x1f\x63\x29"
	    "\x7e\xb3\xdd\x43\x11\xe8\xb5\x85\xd4\xbf"
	    "\x2f\x59\x23\x21\x4f\x1d\xfe\xd3\xfd\xee"
	    "\x4a\xaf\x01\x83\x30\xa1\x2a\xcd\xe0\xef"
	    "\xcc\x33\x8e\xb5\x29\x22\xf3\xe5\x71\x21"
	    "\x2d\x42\xc8\xde",
	64
};

const gnutls_datum_t invalid_hash_data = {
	(void *)
	    "\xaa\xf4\xc6\x1d\xdc\xca\xe8\xa2\xda\xbe"
	    "\xde\x0f\x3b\x48\x2c\xb9\xae\xa9\x43\x4d",
	20
};

const gnutls_datum_t raw_data = {
	(void *)"hello",
	5
};


static void print_keys(gnutls_privkey_t privkey, gnutls_pubkey_t pubkey)
{
	gnutls_x509_privkey_t xkey;
	gnutls_datum_t out;
	int ret = gnutls_privkey_export_x509(privkey, &xkey);

	if (ret < 0)
		fail("error in privkey export\n");

	ret = gnutls_x509_privkey_export2(xkey, GNUTLS_X509_FMT_PEM, &out);
	if (ret < 0)
		fail("error in privkey export\n");

	fprintf(stderr, "%s\n", out.data);
	gnutls_free(out.data);

	ret = gnutls_pubkey_export2(pubkey, GNUTLS_X509_FMT_PEM, &out);
	if (ret < 0)
		fail("error in pubkey export\n");

	fprintf(stderr, "%s\n", out.data);
	gnutls_free(out.data);

	gnutls_x509_privkey_deinit(xkey);
}

#define ERR fail("Failure at: %s (%s-%s) (iter: %d)\n", gnutls_sign_get_name(sign_algo), gnutls_pk_get_name(pk), gnutls_digest_get_name(hash), j);
static
void test_sig(gnutls_pk_algorithm_t pk, unsigned hash, unsigned bits)
{
	gnutls_pubkey_t pubkey;
	gnutls_privkey_t privkey;
	gnutls_sign_algorithm_t sign_algo;
	gnutls_datum_t signature;
	const gnutls_datum_t *hash_data;
	int ret;
	unsigned j;
	unsigned vflags = 0;

	if (hash == GNUTLS_DIG_SHA1) {
		hash_data = &sha1_data;
		vflags |= GNUTLS_VERIFY_ALLOW_SIGN_WITH_SHA1;
	} else if (hash == GNUTLS_DIG_SHA256)
		hash_data = &sha256_data;
	else if (hash == GNUTLS_DIG_GOSTR_94)
		hash_data = &gostr94_data;
	else if (hash == GNUTLS_DIG_STREEBOG_256)
		hash_data = &streebog256_data;
	else if (hash == GNUTLS_DIG_STREEBOG_512)
		hash_data = &streebog512_data;
	else
		abort();

	sign_algo =
	    gnutls_pk_to_sign(pk, hash);

	for (j = 0; j < 100; j++) {
		ret = gnutls_pubkey_init(&pubkey);
		if (ret < 0)
			ERR;

		ret = gnutls_privkey_init(&privkey);
		if (ret < 0)
			ERR;

		ret = gnutls_privkey_generate(privkey, pk, bits, 0);
		if (ret < 0)
			ERR;

		ret =
		    gnutls_privkey_sign_hash(privkey, hash,
					     0, hash_data,
					     &signature);
		if (ret < 0)
			ERR;

		ret = gnutls_pubkey_import_privkey(pubkey, privkey, GNUTLS_KEY_DIGITAL_SIGNATURE, 0);
		if (ret < 0)
			ERR;

		ret =
		    gnutls_pubkey_verify_hash2(pubkey,
						sign_algo, vflags,
						hash_data, &signature);
		if (ret < 0) {
			print_keys(privkey, pubkey);
			ERR;
		}

		/* should fail */
		ret =
		    gnutls_pubkey_verify_hash2(pubkey,
						sign_algo, vflags,
						&invalid_hash_data,
						&signature);
		if (ret != GNUTLS_E_PK_SIG_VERIFY_FAILED) {
			print_keys(privkey, pubkey);
			ERR;
		}

		sign_algo =
		    gnutls_pk_to_sign(gnutls_pubkey_get_pk_algorithm
				      (pubkey, NULL), hash);

		ret =
		    gnutls_pubkey_verify_hash2(pubkey, sign_algo, vflags,
						hash_data, &signature);
		if (ret < 0)
			ERR;

		/* should fail */
		ret =
		    gnutls_pubkey_verify_hash2(pubkey, sign_algo, vflags,
						&invalid_hash_data,
						&signature);
		if (ret != GNUTLS_E_PK_SIG_VERIFY_FAILED) {
			print_keys(privkey, pubkey);
			ERR;
		}

		/* test the raw interface */
		gnutls_free(signature.data);
		signature.data = NULL;

		if (pk == GNUTLS_PK_RSA) {
			ret =
			    gnutls_privkey_sign_hash(privkey,
						     hash,
						     GNUTLS_PRIVKEY_SIGN_FLAG_TLS1_RSA,
						     hash_data,
						     &signature);
			if (ret < 0)
				ERR;

			sign_algo =
			    gnutls_pk_to_sign
			    (gnutls_pubkey_get_pk_algorithm
			     (pubkey, NULL), hash);

			ret =
			    gnutls_pubkey_verify_hash2(pubkey,
							sign_algo,
							vflags|GNUTLS_PUBKEY_VERIFY_FLAG_TLS1_RSA,
							hash_data,
							&signature);
			if (ret < 0) {
				print_keys(privkey, pubkey);
				ERR;
			}

		}
		gnutls_free(signature.data);
		gnutls_privkey_deinit(privkey);
		gnutls_pubkey_deinit(pubkey);
	}
}

#endif /* GNUTLS_TESTS_X509SIGN_VERIFY_COMMON_H */
