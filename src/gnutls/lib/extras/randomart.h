#ifndef GNUTLS_LIB_EXTRAS_RANDOMART_H
#define GNUTLS_LIB_EXTRAS_RANDOMART_H

char *_gnutls_key_fingerprint_randomart(uint8_t * dgst_raw,
					u_int dgst_raw_len,
					const char *key_type,
					unsigned int key_size,
					const char *prefix);

#endif /* GNUTLS_LIB_EXTRAS_RANDOMART_H */
