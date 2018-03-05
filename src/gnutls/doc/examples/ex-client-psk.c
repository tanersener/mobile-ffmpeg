/* This example code is placed in the public domain. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <assert.h>
#include <gnutls/gnutls.h>

/* A very basic TLS client, with PSK authentication.
 */

#define CHECK(x) assert((x)>=0)

#define MAX_BUF 1024
#define MSG "GET / HTTP/1.0\r\n\r\n"

extern int tcp_connect(void);
extern void tcp_close(int sd);

int main(void)
{
        int ret, sd, ii;
        gnutls_session_t session;
        char buffer[MAX_BUF + 1];
        const char *err;
        gnutls_psk_client_credentials_t pskcred;
        const gnutls_datum_t key = { (void *) "DEADBEEF", 8 };

        CHECK(gnutls_global_init());

        CHECK(gnutls_psk_allocate_client_credentials(&pskcred));
        CHECK(gnutls_psk_set_client_credentials(pskcred, "test", &key,
                                                GNUTLS_PSK_KEY_HEX));

        /* Initialize TLS session
         */
        CHECK(gnutls_init(&session, GNUTLS_CLIENT));

        /* Use default priorities */
        ret =
            gnutls_priority_set_direct(session,
                                       "PERFORMANCE:+ECDHE-PSK:+DHE-PSK:+PSK",
                                       &err);
        if (ret < 0) {
                if (ret == GNUTLS_E_INVALID_REQUEST) {
                        fprintf(stderr, "Syntax error at: %s\n", err);
                }
                exit(1);
        }

        /* put the x509 credentials to the current session
         */
        CHECK(gnutls_credentials_set(session, GNUTLS_CRD_PSK, pskcred));

        /* connect to the peer
         */
        sd = tcp_connect();

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

        CHECK(gnutls_record_send(session, MSG, strlen(MSG)));

        ret = gnutls_record_recv(session, buffer, MAX_BUF);
        if (ret == 0) {
                printf("- Peer has closed the TLS connection\n");
                goto end;
        } else if (ret < 0 && gnutls_error_is_fatal(ret) == 0) {
                fprintf(stderr, "*** Warning: %s\n", gnutls_strerror(ret));
        } else if (ret < 0) {
                fprintf(stderr, "*** Error: %s\n", gnutls_strerror(ret));
                goto end;
        }

        if (ret > 0) {
                printf("- Received %d bytes: ", ret);
                for (ii = 0; ii < ret; ii++) {
                        fputc(buffer[ii], stdout);
                }
                fputs("\n", stdout);
        }

        CHECK(gnutls_bye(session, GNUTLS_SHUT_RDWR));

      end:

        tcp_close(sd);

        gnutls_deinit(session);

        gnutls_psk_free_client_credentials(pskcred);

        gnutls_global_deinit();

        return 0;
}
