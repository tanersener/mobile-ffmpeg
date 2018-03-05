/* This example code is placed in the public domain. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <gnutls/gnutls.h>

#define SRP_PASSWD "tpasswd"
#define SRP_PASSWD_CONF "tpasswd.conf"

#define KEYFILE "key.pem"
#define CERTFILE "cert.pem"
#define CAFILE "/etc/ssl/certs/ca-certificates.crt"

/* This is a sample TLS-SRP echo server.
 */

#define SOCKET_ERR(err,s) if(err==-1) {perror(s);return(1);}
#define MAX_BUF 1024
#define PORT 5556               /* listen to 5556 port */

int main(void)
{
        int err, listen_sd;
        int sd, ret;
        struct sockaddr_in sa_serv;
        struct sockaddr_in sa_cli;
        socklen_t client_len;
        char topbuf[512];
        gnutls_session_t session;
        gnutls_srp_server_credentials_t srp_cred;
        gnutls_certificate_credentials_t cert_cred;
        char buffer[MAX_BUF + 1];
        int optval = 1;
        char name[256];

        strcpy(name, "Echo Server");

        if (gnutls_check_version("3.1.4") == NULL) {
                fprintf(stderr, "GnuTLS 3.1.4 or later is required for this example\n");
                exit(1);
        }

        /* for backwards compatibility with gnutls < 3.3.0 */
        gnutls_global_init();

        /* SRP_PASSWD a password file (created with the included srptool utility) 
         */
        gnutls_srp_allocate_server_credentials(&srp_cred);
        gnutls_srp_set_server_credentials_file(srp_cred, SRP_PASSWD,
                                               SRP_PASSWD_CONF);

        gnutls_certificate_allocate_credentials(&cert_cred);
        gnutls_certificate_set_x509_trust_file(cert_cred, CAFILE,
                                               GNUTLS_X509_FMT_PEM);
        gnutls_certificate_set_x509_key_file(cert_cred, CERTFILE, KEYFILE,
                                             GNUTLS_X509_FMT_PEM);

        /* TCP socket operations
         */
        listen_sd = socket(AF_INET, SOCK_STREAM, 0);
        SOCKET_ERR(listen_sd, "socket");

        memset(&sa_serv, '\0', sizeof(sa_serv));
        sa_serv.sin_family = AF_INET;
        sa_serv.sin_addr.s_addr = INADDR_ANY;
        sa_serv.sin_port = htons(PORT); /* Server Port number */

        setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR, (void *) &optval,
                   sizeof(int));

        err =
            bind(listen_sd, (struct sockaddr *) &sa_serv, sizeof(sa_serv));
        SOCKET_ERR(err, "bind");
        err = listen(listen_sd, 1024);
        SOCKET_ERR(err, "listen");

        printf("%s ready. Listening to port '%d'.\n\n", name, PORT);

        client_len = sizeof(sa_cli);
        for (;;) {
                gnutls_init(&session, GNUTLS_SERVER);
                gnutls_priority_set_direct(session,
                                           "NORMAL"
                                           ":-KX-ALL:+SRP:+SRP-DSS:+SRP-RSA",
                                           NULL);
                gnutls_credentials_set(session, GNUTLS_CRD_SRP, srp_cred);
                /* for the certificate authenticated ciphersuites.
                 */
                gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
                                       cert_cred);

                /* We don't request any certificate from the client.
                 * If we did we would need to verify it. One way of
                 * doing that is shown in the "Verifying a certificate"
                 * example.
                 */
                gnutls_certificate_server_set_request(session,
                                                      GNUTLS_CERT_IGNORE);

                sd = accept(listen_sd, (struct sockaddr *) &sa_cli,
                            &client_len);

                printf("- connection from %s, port %d\n",
                       inet_ntop(AF_INET, &sa_cli.sin_addr, topbuf,
                                 sizeof(topbuf)), ntohs(sa_cli.sin_port));

                gnutls_transport_set_int(session, sd);

                do {
                        ret = gnutls_handshake(session);
                }
                while (ret < 0 && gnutls_error_is_fatal(ret) == 0);

                if (ret < 0) {
                        close(sd);
                        gnutls_deinit(session);
                        fprintf(stderr,
                                "*** Handshake has failed (%s)\n\n",
                                gnutls_strerror(ret));
                        continue;
                }
                printf("- Handshake was completed\n");
                printf("- User %s was connected\n",
                       gnutls_srp_server_get_username(session));

                /* print_info(session); */

                for (;;) {
                        ret = gnutls_record_recv(session, buffer, MAX_BUF);

                        if (ret == 0) {
                                printf
                                    ("\n- Peer has closed the GnuTLS connection\n");
                                break;
                        } else if (ret < 0
                                   && gnutls_error_is_fatal(ret) == 0) {
                                fprintf(stderr, "*** Warning: %s\n",
                                        gnutls_strerror(ret));
                        } else if (ret < 0) {
                                fprintf(stderr, "\n*** Received corrupted "
                                        "data(%d). Closing the connection.\n\n",
                                        ret);
                                break;
                        } else if (ret > 0) {
                                /* echo data back to the client
                                 */
                                gnutls_record_send(session, buffer, ret);
                        }
                }
                printf("\n");
                /* do not wait for the peer to close the connection. */
                gnutls_bye(session, GNUTLS_SHUT_WR);

                close(sd);
                gnutls_deinit(session);

        }
        close(listen_sd);

        gnutls_srp_free_server_credentials(srp_cred);
        gnutls_certificate_free_credentials(cert_cred);

        gnutls_global_deinit();

        return 0;

}
