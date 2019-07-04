#ifndef GNUTLS_LIB_PIN_H
#define GNUTLS_LIB_PIN_H

extern gnutls_pin_callback_t _gnutls_pin_func;
extern void *_gnutls_pin_data;

int
_gnutls_retrieve_pin(struct pin_info_st *pin_info, const char *url, const char *label,
		     unsigned pin_flags, char *pin, unsigned pin_size);

#endif /* GNUTLS_LIB_PIN_H */
