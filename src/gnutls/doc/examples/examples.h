#ifndef EXAMPLES_H
#define EXAMPLES_H

void check_alert(gnutls_session_t session, int ret);

int write_pkcs12(const gnutls_datum_t * cert,
                 const gnutls_datum_t * pkcs8_key, const char *password);

void verify_certificate(gnutls_session_t session, const char *hostname);

int print_info(gnutls_session_t session);

void print_x509_certificate_info(gnutls_session_t session);

int _ssh_verify_certificate_callback(gnutls_session_t session);

void
verify_certificate_chain(const char *hostname,
                         const gnutls_datum_t * cert_chain,
                         int cert_chain_length);

int verify_certificate_callback(gnutls_session_t session);

#endif                          /* EXAMPLES_H */
