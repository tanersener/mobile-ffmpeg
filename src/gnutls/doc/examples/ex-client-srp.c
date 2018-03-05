/* This example code is placed in the public domain. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gnutls/gnutls.h>

/* Those functions are defined in other examples.
 */
extern void check_alert(gnutls_session_t session, int ret);
extern int tcp_connect(void);
extern void tcp_close(int sd);

#define MAX_BUF 1024
#define USERNAME "user"
#define PASSWORD "pass"
#define CAFILE "/etc/ssl/certs/ca-certificates.crt"
#define MSG "GET / HTTP/1.0\r\n\r\n"

int main(void)
{
        int ret;
        int sd, ii;
        gnutls_session_t session;
        char buffer[MAX_BUF + 1];
        gnutls_srp_client_credentials_t srp_cred;
        gnutls_certificate_credentials_t cert_cred;

        if (gnutls_check_version("3.1.4") == NULL) {
                fprintf(stderr, "GnuTLS 3.1.4 or later is required for this example\n");
                exit(1);
        }

        /* for backwards compatibility with gnutls < 3.3.0 */
        gnutls_global_init();

        gnutls_srp_allocate_client_credentials(&srp_cred);
        gnutls_certificate_allocate_credentials(&cert_cred);

        gnutls_certificate_set_x509_trust_file(cert_cred, CAFILE,
                                               GNUTLS_X509_FMT_PEM);
        gnutls_srp_set_client_credentials(srp_cred, USERNAME, PASSWORD);

        /* connects to server
         */
        sd = tcp_connect();

        /* Initialize TLS session
         */
        gnutls_init(&session, GNUTLS_CLIENT);


        /* Set the priorities.
         */
        gnutls_priority_set_direct(session,
                                   "NORMAL:+SRP:+SRP-RSA:+SRP-DSS",
                                   NULL);

        /* put the SRP credentials to the current session
         */
        gnutls_credentials_set(session, GNUTLS_CRD_SRP, srp_cred);
        gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, cert_cred);

        gnutls_transport_set_int(session, sd);
        gnutls_handshake_set_timeout(session,
                                     GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT);

        /* Perform the TLS handshake
         */
        do {
                ret = gnutls_handshake(session);
        }
        while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

        if (ret < 0) {
                fprintf(stderr, "*** Handshake failed\n");
                gnutls_perror(ret);
                goto end;
        } else {
                char *desc;

                desc = gnutls_session_get_desc(session);
                printf("- Session info: %s\n", desc);
                gnutls_free(desc);
        }

        gnutls_record_send(session, MSG, strlen(MSG));

        ret = gnutls_record_recv(session, buffer, MAX_BUF);
        if (gnutls_error_is_fatal(ret) != 0 || ret == 0) {
                if (ret == 0) {
                        printf
                            ("- Peer has closed the GnuTLS connection\n");
                        goto end;
                } else {
                        fprintf(stderr, "*** Error: %s\n",
                                gnutls_strerror(ret));
                        goto end;
                }
        } else
                check_alert(session, ret);

        if (ret > 0) {
                printf("- Received %d bytes: ", ret);
                for (ii = 0; ii < ret; ii++) {
                        fputc(buffer[ii], stdout);
                }
                fputs("\n", stdout);
        }
        gnutls_bye(session, GNUTLS_SHUT_RDWR);

      end:

        tcp_close(sd);

        gnutls_deinit(session);

        gnutls_srp_free_client_credentials(srp_cred);
        gnutls_certificate_free_credentials(cert_cred);

        gnutls_global_deinit();

        return 0;
}
