extern int _gnutls_cryptodev_fd;

void _gnutls_cryptodev_deinit(void);
int _gnutls_cryptodev_init(void);
int _cryptodev_register_gcm_crypto(int cfd);
