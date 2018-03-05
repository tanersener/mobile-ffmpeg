/* This example code is placed in the public domain. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#include <gnutls/ocsp.h>
#ifndef NO_LIBCURL
#include <curl/curl.h>
#endif
#include "read-file.h"

size_t get_data(void *buffer, size_t size, size_t nmemb, void *userp);
static gnutls_x509_crt_t load_cert(const char *cert_file);
static void _response_info(const gnutls_datum_t * data);
static void
_generate_request(gnutls_datum_t * rdata, gnutls_x509_crt_t cert,
                  gnutls_x509_crt_t issuer, gnutls_datum_t *nonce);
static int
_verify_response(gnutls_datum_t * data, gnutls_x509_crt_t cert,
                 gnutls_x509_crt_t signer, gnutls_datum_t *nonce);

/* This program queries an OCSP server.
   It expects three files. argv[1] containing the certificate to
   be checked, argv[2] holding the issuer for this certificate,
   and argv[3] holding a trusted certificate to verify OCSP's response.
   argv[4] is optional and should hold the server host name.
   
   For simplicity the libcurl library is used.
 */

int main(int argc, char *argv[])
{
        gnutls_datum_t ud, tmp;
        int ret;
        gnutls_datum_t req;
        gnutls_x509_crt_t cert, issuer, signer;
#ifndef NO_LIBCURL
        CURL *handle;
        struct curl_slist *headers = NULL;
#endif
        int v, seq;
        const char *cert_file = argv[1];
        const char *issuer_file = argv[2];
        const char *signer_file = argv[3];
        char *hostname = NULL;
        unsigned char noncebuf[23];
        gnutls_datum_t nonce = { noncebuf, sizeof(noncebuf) };

        gnutls_global_init();

        if (argc > 4)
                hostname = argv[4];

        ret = gnutls_rnd(GNUTLS_RND_NONCE, nonce.data, nonce.size);
        if (ret < 0)
                exit(1);

        cert = load_cert(cert_file);
        issuer = load_cert(issuer_file);
        signer = load_cert(signer_file);

        if (hostname == NULL) {

                for (seq = 0;; seq++) {
                        ret =
                            gnutls_x509_crt_get_authority_info_access(cert,
                                                                      seq,
                                                                      GNUTLS_IA_OCSP_URI,
                                                                      &tmp,
                                                                      NULL);
                        if (ret == GNUTLS_E_UNKNOWN_ALGORITHM)
                                continue;
                        if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
                                fprintf(stderr,
                                        "No URI was found in the certificate.\n");
                                exit(1);
                        }
                        if (ret < 0) {
                                fprintf(stderr, "error: %s\n",
                                        gnutls_strerror(ret));
                                exit(1);
                        }

                        printf("CA issuers URI: %.*s\n", tmp.size,
                               tmp.data);

                        hostname = malloc(tmp.size + 1);
                        memcpy(hostname, tmp.data, tmp.size);
                        hostname[tmp.size] = 0;

                        gnutls_free(tmp.data);
                        break;
                }

        }

        /* Note that the OCSP servers hostname might be available
         * using gnutls_x509_crt_get_authority_info_access() in the issuer's
         * certificate */

        memset(&ud, 0, sizeof(ud));
        fprintf(stderr, "Connecting to %s\n", hostname);

        _generate_request(&req, cert, issuer, &nonce);

#ifndef NO_LIBCURL
        curl_global_init(CURL_GLOBAL_ALL);

        handle = curl_easy_init();
        if (handle == NULL)
                exit(1);

        headers =
            curl_slist_append(headers,
                              "Content-Type: application/ocsp-request");

        curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(handle, CURLOPT_POSTFIELDS, (void *) req.data);
        curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, req.size);
        curl_easy_setopt(handle, CURLOPT_URL, hostname);
        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, get_data);
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, &ud);

        ret = curl_easy_perform(handle);
        if (ret != 0) {
                fprintf(stderr, "curl[%d] error %d\n", __LINE__, ret);
                exit(1);
        }

        curl_easy_cleanup(handle);
#endif

        _response_info(&ud);

        v = _verify_response(&ud, cert, signer, &nonce);

        gnutls_x509_crt_deinit(cert);
        gnutls_x509_crt_deinit(issuer);
        gnutls_x509_crt_deinit(signer);
        gnutls_global_deinit();

        return v;
}

static void _response_info(const gnutls_datum_t * data)
{
        gnutls_ocsp_resp_t resp;
        int ret;
        gnutls_datum buf;

        ret = gnutls_ocsp_resp_init(&resp);
        if (ret < 0)
                exit(1);

        ret = gnutls_ocsp_resp_import(resp, data);
        if (ret < 0)
                exit(1);

        ret = gnutls_ocsp_resp_print(resp, GNUTLS_OCSP_PRINT_FULL, &buf);
        if (ret != 0)
                exit(1);

        printf("%.*s", buf.size, buf.data);
        gnutls_free(buf.data);

        gnutls_ocsp_resp_deinit(resp);
}

static gnutls_x509_crt_t load_cert(const char *cert_file)
{
        gnutls_x509_crt_t crt;
        int ret;
        gnutls_datum_t data;
        size_t size;

        ret = gnutls_x509_crt_init(&crt);
        if (ret < 0)
                exit(1);

        data.data = (void *) read_binary_file(cert_file, &size);
        data.size = size;

        if (!data.data) {
                fprintf(stderr, "Cannot open file: %s\n", cert_file);
                exit(1);
        }

        ret = gnutls_x509_crt_import(crt, &data, GNUTLS_X509_FMT_PEM);
        free(data.data);
        if (ret < 0) {
                fprintf(stderr, "Cannot import certificate in %s: %s\n",
                        cert_file, gnutls_strerror(ret));
                exit(1);
        }

        return crt;
}

static void
_generate_request(gnutls_datum_t * rdata, gnutls_x509_crt_t cert,
                  gnutls_x509_crt_t issuer, gnutls_datum_t *nonce)
{
        gnutls_ocsp_req_t req;
        int ret;

        ret = gnutls_ocsp_req_init(&req);
        if (ret < 0)
                exit(1);

        ret = gnutls_ocsp_req_add_cert(req, GNUTLS_DIG_SHA1, issuer, cert);
        if (ret < 0)
                exit(1);


        ret = gnutls_ocsp_req_set_nonce(req, 0, nonce);
        if (ret < 0)
                exit(1);

        ret = gnutls_ocsp_req_export(req, rdata);
        if (ret != 0)
                exit(1);

        gnutls_ocsp_req_deinit(req);

        return;
}

static int
_verify_response(gnutls_datum_t * data, gnutls_x509_crt_t cert,
                 gnutls_x509_crt_t signer, gnutls_datum_t *nonce)
{
        gnutls_ocsp_resp_t resp;
        int ret;
        unsigned verify;
        gnutls_datum_t rnonce;

        ret = gnutls_ocsp_resp_init(&resp);
        if (ret < 0)
                exit(1);

        ret = gnutls_ocsp_resp_import(resp, data);
        if (ret < 0)
                exit(1);

        ret = gnutls_ocsp_resp_check_crt(resp, 0, cert);
        if (ret < 0)
                exit(1);

	ret = gnutls_ocsp_resp_get_nonce(resp, NULL, &rnonce);
	if (ret < 0)
		exit(1);

	if (rnonce.size != nonce->size || memcmp(nonce->data, rnonce.data,
		nonce->size) != 0) {
		exit(1);
	}

        ret = gnutls_ocsp_resp_verify_direct(resp, signer, &verify, 0);
        if (ret < 0)
                exit(1);

        printf("Verifying OCSP Response: ");
        if (verify == 0)
                printf("Verification success!\n");
        else
                printf("Verification error!\n");

        if (verify & GNUTLS_OCSP_VERIFY_SIGNER_NOT_FOUND)
                printf("Signer cert not found\n");

        if (verify & GNUTLS_OCSP_VERIFY_SIGNER_KEYUSAGE_ERROR)
                printf("Signer cert keyusage error\n");

        if (verify & GNUTLS_OCSP_VERIFY_UNTRUSTED_SIGNER)
                printf("Signer cert is not trusted\n");

        if (verify & GNUTLS_OCSP_VERIFY_INSECURE_ALGORITHM)
                printf("Insecure algorithm\n");

        if (verify & GNUTLS_OCSP_VERIFY_SIGNATURE_FAILURE)
                printf("Signature failure\n");

        if (verify & GNUTLS_OCSP_VERIFY_CERT_NOT_ACTIVATED)
                printf("Signer cert not yet activated\n");

        if (verify & GNUTLS_OCSP_VERIFY_CERT_EXPIRED)
                printf("Signer cert expired\n");

        gnutls_free(rnonce.data);
        gnutls_ocsp_resp_deinit(resp);

        return verify;
}

size_t get_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
        gnutls_datum_t *ud = userp;

        size *= nmemb;

        ud->data = realloc(ud->data, size + ud->size);
        if (ud->data == NULL) {
                fprintf(stderr, "Not enough memory for the request\n");
                exit(1);
        }

        memcpy(&ud->data[ud->size], buffer, size);
        ud->size += size;

        return size;
}
