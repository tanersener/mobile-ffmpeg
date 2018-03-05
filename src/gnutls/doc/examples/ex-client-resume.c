/* This example code is placed in the public domain. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gnutls/gnutls.h>

/* Those functions are defined in other examples.
 */
extern void check_alert(gnutls_session_t session, int ret);
extern int tcp_connect(void);
extern void tcp_close(int sd);

#define MAX_BUF 1024
#define CAFILE "/etc/ssl/certs/ca-certificates.crt"
#define MSG "GET / HTTP/1.0\r\n\r\n"

int main(void)
{
        int ret;
        int sd, ii;
        gnutls_session_t session;
        char buffer[MAX_BUF + 1];
        gnutls_certificate_credentials_t xcred;

        /* variables used in session resuming 
         */
        int t;
        char *session_data = NULL;
        size_t session_data_size = 0;

        gnutls_global_init();

        /* X509 stuff */
        gnutls_certificate_allocate_credentials(&xcred);

        gnutls_certificate_set_x509_trust_file(xcred, CAFILE,
                                               GNUTLS_X509_FMT_PEM);

        for (t = 0; t < 2; t++) {       /* connect 2 times to the server */

                sd = tcp_connect();

                gnutls_init(&session, GNUTLS_CLIENT);

                gnutls_priority_set_direct(session,
                                           "PERFORMANCE:!ARCFOUR-128",
                                           NULL);

                gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
                                       xcred);

                if (t > 0) {
                        /* if this is not the first time we connect */
                        gnutls_session_set_data(session, session_data,
                                                session_data_size);
                        free(session_data);
                }

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
                        printf("- Handshake was completed\n");
                }

                if (t == 0) {   /* the first time we connect */
                        /* get the session data size */
                        gnutls_session_get_data(session, NULL,
                                                &session_data_size);
                        session_data = malloc(session_data_size);

                        /* put session data to the session variable */
                        gnutls_session_get_data(session, session_data,
                                                &session_data_size);

                } else {        /* the second time we connect */

                        /* check if we actually resumed the previous session */
                        if (gnutls_session_is_resumed(session) != 0) {
                                printf("- Previous session was resumed\n");
                        } else {
                                fprintf(stderr,
                                        "*** Previous session was NOT resumed\n");
                        }
                }

                /* This function was defined in a previous example
                 */
                /* print_info(session); */

                gnutls_record_send(session, MSG, strlen(MSG));

                ret = gnutls_record_recv(session, buffer, MAX_BUF);
                if (ret == 0) {
                        printf("- Peer has closed the TLS connection\n");
                        goto end;
                } else if (ret < 0 && gnutls_error_is_fatal(ret) == 0) {
                        fprintf(stderr, "*** Warning: %s\n",
                                gnutls_strerror(ret));
                } else if (ret < 0) {
                        fprintf(stderr, "*** Error: %s\n",
                                gnutls_strerror(ret));
                        goto end;
                }

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

        }                       /* for() */

        gnutls_certificate_free_credentials(xcred);

        gnutls_global_deinit();

        return 0;
}
