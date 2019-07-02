/* This example code is placed in the public domain. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <gnutls/gnutls.h>

extern void check_alert(gnutls_session_t session, int ret);
extern int tcp_connect(void);
extern void tcp_close(int sd);

/* A very basic TLS client, with X.509 authentication and server certificate
 * verification as well as session resumption.
 *
 * Note that error recovery is minimal for simplicity.
 */

#define CHECK(x) assert((x)>=0)
#define LOOP_CHECK(rval, cmd) \
        do { \
                rval = cmd; \
        } while(rval == GNUTLS_E_AGAIN || rval == GNUTLS_E_INTERRUPTED); \
        assert(rval >= 0)

#define MAX_BUF 1024
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
        gnutls_datum_t sdata;

        /* for backwards compatibility with gnutls < 3.3.0 */
        CHECK(gnutls_global_init());

        CHECK(gnutls_certificate_allocate_credentials(&xcred));
        CHECK(gnutls_certificate_set_x509_system_trust(xcred));

        for (t = 0; t < 2; t++) {       /* connect 2 times to the server */

                sd = tcp_connect();

                CHECK(gnutls_init(&session, GNUTLS_CLIENT));

                CHECK(gnutls_server_name_set(session, GNUTLS_NAME_DNS,
                                             "www.example.com",
                                             strlen("www.example.com")));
                gnutls_session_set_verify_cert(session, "www.example.com", 0);

                CHECK(gnutls_set_default_priority(session));

                gnutls_transport_set_int(session, sd);
                gnutls_handshake_set_timeout(session,
                                             GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT);

                gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
                                       xcred);

                if (t > 0) {
                        /* if this is not the first time we connect */
                        CHECK(gnutls_session_set_data(session, sdata.data,
                                                      sdata.size));
                        gnutls_free(sdata.data);
                }

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
                        /* get the session data */
                        CHECK(gnutls_session_get_data2(session, &sdata));
                } else { /* the second time we connect */

                        /* check if we actually resumed the previous session */
                        if (gnutls_session_is_resumed(session) != 0) {
                                printf("- Previous session was resumed\n");
                        } else {
                                fprintf(stderr,
                                        "*** Previous session was NOT resumed\n");
                        }
                }

                LOOP_CHECK(ret, gnutls_record_send(session, MSG, strlen(MSG)));

                LOOP_CHECK(ret, gnutls_record_recv(session, buffer, MAX_BUF));
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
