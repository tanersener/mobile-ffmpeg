extern int _gnutls_cryptodev_fd;

#define CHECK_AES_KEYSIZE(s) \
	if (s != 16 && s != 24 && s != 32) \
		return GNUTLS_E_INVALID_REQUEST

void _gnutls_cryptodev_deinit(void);
int _gnutls_cryptodev_init(void);
int _cryptodev_register_gcm_crypto(int cfd);
