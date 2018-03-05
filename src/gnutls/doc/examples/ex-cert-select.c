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
#include <gnutls/x509.h>
#include <gnutls/abstract.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* A TLS client that loads the certificate and key.
 */

#define CHECK(x) assert((x)>=0)

#define MAX_BUF 1024
#define MSG "GET / HTTP/1.0\r\n\r\n"

#define CERT_FILE "cert.pem"
#define KEY_FILE "key.pem"
#define CAFILE "/etc/ssl/certs/ca-certificates.crt"

extern int tcp_connect(void);
extern void tcp_close(int sd);

static int
cert_callback(gnutls_session_t session,
              const gnutls_datum_t * req_ca_rdn, int nreqs,
              const gnutls_pk_algorithm_t * sign_algos,
              int sign_algos_length, gnutls_pcert_st ** pcert,
              unsigned int *pcert_length, gnutls_privkey_t * pkey);

gnutls_pcert_st pcrt;
gnutls_privkey_t key;

/* Load the certificate and the private key.
 */
static void load_keys(void)
{
        gnutls_datum_t data;

        CHECK(gnutls_load_file(CERT_FILE, &data));

        CHECK(gnutls_pcert_import_x509_raw(&pcrt, &data,
                                           GNUTLS_X509_FMT_PEM, 0));

        gnutls_free(data.data);

        CHECK(gnutls_load_file(KEY_FILE, &data));

        CHECK(gnutls_privkey_init(&key));

        CHECK(gnutls_privkey_import_x509_raw(key, &data,
                                             GNUTLS_X509_FMT_PEM,
                                             NULL, 0));
        gnutls_free(data.data);
}

int main(void)
{
        int ret, sd, ii;
        gnutls_session_t session;
        char buffer[MAX_BUF + 1];
        gnutls_certificate_credentials_t xcred;
        
        if (gnutls_check_version("3.1.4") == NULL) {
                fprintf(stderr, "GnuTLS 3.1.4 or later is required for this example\n");
                exit(1);
        }

        /* for backwards compatibility with gnutls < 3.3.0 */
        CHECK(gnutls_global_init());

        load_keys();

        /* X509 stuff */
        CHECK(gnutls_certificate_allocate_credentials(&xcred));

        /* sets the trusted cas file
         */
        CHECK(gnutls_certificate_set_x509_trust_file(xcred, CAFILE,
                                                     GNUTLS_X509_FMT_PEM));

        gnutls_certificate_set_retrieve_function2(xcred, cert_callback);

        /* Initialize TLS session 
         */
        CHECK(gnutls_init(&session, GNUTLS_CLIENT));

        /* Use default priorities */
        CHECK(gnutls_set_default_priority(session));

        /* put the x509 credentials to the current session
         */
        CHECK(gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, xcred));

        /* connect to the peer
         */
        sd = tcp_connect();

        gnutls_transport_set_int(session, sd);

        /* Perform the TLS handshake
         */
        ret = gnutls_handshake(session);

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
        } else if (ret < 0) {
                fprintf(stderr, "*** Error: %s\n", gnutls_strerror(ret));
                goto end;
        }

        printf("- Received %d bytes: ", ret);
        for (ii = 0; ii < ret; ii++) {
                fputc(buffer[ii], stdout);
        }
        fputs("\n", stdout);

        CHECK(gnutls_bye(session, GNUTLS_SHUT_RDWR));

      end:

        tcp_close(sd);

        gnutls_deinit(session);

        gnutls_certificate_free_credentials(xcred);

        gnutls_global_deinit();

        return 0;
}



/* This callback should be associated with a session by calling
 * gnutls_certificate_client_set_retrieve_function( session, cert_callback),
 * before a handshake.
 */

static int
cert_callback(gnutls_session_t session,
              const gnutls_datum_t * req_ca_rdn, int nreqs,
              const gnutls_pk_algorithm_t * sign_algos,
              int sign_algos_length, gnutls_pcert_st ** pcert,
              unsigned int *pcert_length, gnutls_privkey_t * pkey)
{
        char issuer_dn[256];
        int i, ret;
        size_t len;
        gnutls_certificate_type_t type;

        /* Print the server's trusted CAs
         */
        if (nreqs > 0)
                printf("- Server's trusted authorities:\n");
        else
                printf
                    ("- Server did not send us any trusted authorities names.\n");

        /* print the names (if any) */
        for (i = 0; i < nreqs; i++) {
                len = sizeof(issuer_dn);
                ret = gnutls_x509_rdn_get(&req_ca_rdn[i], issuer_dn, &len);
                if (ret >= 0) {
                        printf("   [%d]: ", i);
                        printf("%s\n", issuer_dn);
                }
        }

        /* Select a certificate and return it.
         * The certificate must be of any of the "sign algorithms"
         * supported by the server.
         */
        type = gnutls_certificate_type_get(session);
        if (type == GNUTLS_CRT_X509) {
                *pcert_length = 1;
                *pcert = &pcrt;
                *pkey = key;
        } else {
                return -1;
        }

        return 0;

}
