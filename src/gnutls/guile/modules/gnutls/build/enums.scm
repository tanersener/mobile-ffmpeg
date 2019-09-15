;;; GnuTLS --- Guile bindings for GnuTLS.
;;; Copyright (C) 2007-2012, 2014, 2019 Free Software Foundation, Inc.
;;;
;;; GnuTLS is free software; you can redistribute it and/or
;;; modify it under the terms of the GNU Lesser General Public
;;; License as published by the Free Software Foundation; either
;;; version 2.1 of the License, or (at your option) any later version.
;;;
;;; GnuTLS is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;;; Lesser General Public License for more details.
;;;
;;; You should have received a copy of the GNU Lesser General Public
;;; License along with GnuTLS; if not, write to the Free Software
;;; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

;;; Written by Ludovic Courtès <ludo@chbouib.org>

(define-module (gnutls build enums)
  :use-module (srfi srfi-1)
  :use-module (srfi srfi-9)
  :use-module (gnutls build utils)

  :export (make-enum-type enum-type-subsystem enum-type-value-alist
           enum-type-c-type enum-type-get-name-function
           enum-type-automatic-get-name-function
           enum-type-smob-name
           enum-type-to-c-function enum-type-from-c-function

           output-enum-smob-definitions output-enum-definitions
           output-enum-declarations
           output-enum-definition-function output-c->enum-converter
           output-enum->c-converter

           %cipher-enum %mac-enum %compression-method-enum %kx-enum
           %protocol-enum %certificate-type-enum

           %gnutls-enums))

;;;
;;; This module helps with the creation of bindings for the C enumerate
;;; types.  It aims at providing strong typing (i.e., one cannot use an
;;; enumerate value of the wrong type) along with authenticity checks (i.e.,
;;; values of a given enumerate type cannot be forged---for instance, one
;;; cannot use some random integer as an enumerate value).  Additionally,
;;; Scheme enums representing the same C enum value should be `eq?'.
;;;
;;; To that end, Scheme->C conversions are optimized (a simple
;;; `SCM_SMOB_DATA'), since that is the most common usage pattern.
;;; Conversely, C->Scheme conversions take time proportional to the number of
;;; value in the enum type.
;;;


;;;
;;; Enumeration tools.
;;;

(define-record-type <enum-type>
  (%make-enum-type subsystem c-type enum-map get-name value-prefix)
  enum-type?
  (subsystem    enum-type-subsystem)
  (enum-map     enum-type-value-alist)
  (c-type       enum-type-c-type)
  (get-name     enum-type-get-name-function)
  (value-prefix enum-type-value-prefix))


(define (make-enum-type subsystem c-type values get-name . value-prefix)
  ;; Return a new enumeration type.
  (let ((value-prefix (if (null? value-prefix)
                          #f
                          (car value-prefix))))
    (%make-enum-type subsystem c-type
                     (make-enum-map subsystem values value-prefix)
                     get-name value-prefix)))


(define (make-enum-map subsystem values value-prefix)
  ;; Return an alist mapping C enum values (strings) to Scheme symbols.
  (define (value-symbol->string value)
    (string-upcase (scheme-symbol->c-name value)))

  (define (make-c-name value)
    (case value-prefix
      ((#f)
       ;; automatically derive the C value name.
       (string-append "GNUTLS_" (string-upcase (symbol->string subsystem))
                      "_" (value-symbol->string value)))
      (else
       (string-append value-prefix (value-symbol->string value)))))

  (map (lambda (value)
         (cons (make-c-name value) value))
       values))

(define (enum-type-smob-name enum)
  ;; Return the C name of the smob type for ENUM.
  (string-append "scm_tc16_gnutls_"
                 (scheme-symbol->c-name (enum-type-subsystem enum))
                 "_enum"))

(define (enum-type-smob-list enum)
  ;; Return the name of the C variable holding a list of value (SMOBs) for
  ;; ENUM.  This list is used when converting from C to Scheme.
  (string-append "scm_gnutls_"
                 (scheme-symbol->c-name (enum-type-subsystem enum))
                 "_enum_values"))

(define (enum-type-to-c-function enum)
  ;; Return the name of the C `scm_to_' function for ENUM.
  (string-append "scm_to_gnutls_"
                 (scheme-symbol->c-name (enum-type-subsystem enum))))

(define (enum-type-from-c-function enum)
  ;; Return the name of the C `scm_from_' function for ENUM.
  (string-append "scm_from_gnutls_"
                 (scheme-symbol->c-name (enum-type-subsystem enum))))

(define (enum-type-automatic-get-name-function enum)
  ;; Return the name of an automatically-generated C function that returns a
  ;; string describing the given enum value of type ENUM.
  (string-append "scm_gnutls_"
                 (scheme-symbol->c-name (enum-type-subsystem enum))
                 "_to_c_string"))


;;;
;;; C code generation.
;;;

(define (output-enum-smob-definitions enum port)
  (let ((smob     (enum-type-smob-name enum))
        (get-name (enum-type-get-name-function enum)))
    (format port "SCM_GLOBAL_SMOB (~a, \"~a\", 0);~%"
            smob (enum-type-subsystem enum))
    (format port "SCM ~a = SCM_EOL;~%"
            (enum-type-smob-list enum))

    (if (not (string? get-name))
        ;; Generate a "get name" function.
        (output-enum-get-name-function enum port))

    ;; Generate the printer and `->string' function.
    (let ((get-name (or get-name
                        (enum-type-automatic-get-name-function enum))))
      (let ((subsystem (scheme-symbol->c-name (enum-type-subsystem enum))))
        ;; SMOB printer.
        (format port "SCM_SMOB_PRINT (~a, ~a_print, obj, port, pstate)~%{~%"
                smob subsystem)
        (format port "  scm_puts (\"#<gnutls-~a-enum \", port);~%"
                (enum-type-subsystem enum))
        (format port "  scm_puts (~a (~a (obj, 1, \"~a_print\")), port);~%"
                get-name (enum-type-to-c-function enum) subsystem)
        (format port "  scm_puts (\">\", port);~%")
        (format port "  return 1;~%")
        (format port "}~%")

        ;; Enum-to-string.
        (format port "SCM_DEFINE (scm_gnutls_~a_to_string, \"~a->string\", "
                subsystem (enum-type-subsystem enum))
        (format port "1, 0, 0,~%")
        (format port "            (SCM enumval),~%")
        (format port "            \"Return a string describing ")
        (format port "@var{enumval}, a @code{~a} value.\")~%"
                (enum-type-subsystem enum))
        (format port "#define FUNC_NAME s_scm_gnutls_~a_to_string~%"
                subsystem)
        (format port "{~%")
        (format port "  ~a c_enum;~%"
                (enum-type-c-type enum))
        (format port "  const char *c_string;~%")
        (format port "  c_enum = ~a (enumval, 1, FUNC_NAME);~%"
                (enum-type-to-c-function enum))
        (format port "  c_string = ~a (c_enum);~%"
                get-name)
        (format port "  return (scm_from_locale_string (c_string));~%")
        (format port "}~%")
        (format port "#undef FUNC_NAME~%")))))

(define (output-enum-definitions enum port)
  ;; Output to PORT the Guile C code that defines the values of ENUM-ALIST.
  (let ((subsystem (scheme-symbol->c-name (enum-type-subsystem enum))))
    (format port "  enum_values = SCM_EOL;~%")
    (for-each (lambda (c+scheme)
                (format port "  SCM_NEWSMOB (enum_smob, ~a, "
                        (enum-type-smob-name enum))
                (format port "(scm_t_bits) ~a);~%"
                        (car c+scheme))
                (format port "  enum_values = scm_cons (enum_smob, ")
                (format port "enum_values);~%")
                (format port "  scm_c_define (\"~a\", enum_smob);~%"
                        (symbol-append (enum-type-subsystem enum) '/
                                       (cdr c+scheme))))
              (enum-type-value-alist enum))
    (format port "  ~a = scm_permanent_object (enum_values);~%"
            (enum-type-smob-list enum))))

(define (output-enum-declarations enum port)
  ;; Issue header file declarations needed for the inline functions that
  ;; handle ENUM values.
  (format port "SCM_API scm_t_bits ~a;~%"
          (enum-type-smob-name enum))
  (format port "SCM_API SCM ~a;~%"
          (enum-type-smob-list enum)))

(define (output-enum-definition-function enums port)
  ;; Output a C function that does all the `scm_c_define ()' for the enums
  ;; listed in ENUMS.
  (format port "static inline void~%scm_gnutls_define_enums (void)~%{~%")
  (format port "  SCM enum_values, enum_smob;~%")
  (for-each (lambda (enum)
              (output-enum-definitions enum port))
            enums)
  (format port "}~%"))

(define (output-c->enum-converter enum port)
  ;; Output a C->Scheme converted for ENUM.  This works by walking the list
  ;; of available enum values (SMOBs) for ENUM and then returning the
  ;; matching SMOB, so that users can then compare enums using `eq?'.  While
  ;; this may look inefficient, this shouldn't be a problem since (i)
  ;; conversion in that direction is rarely needed and (ii) the number of
  ;; values per enum is expected to be small.
  (format port "static inline SCM~%~a (~a c_obj)~%{~%"
          (enum-type-from-c-function enum)
          (enum-type-c-type enum))
  (format port "  SCM pair, result = SCM_BOOL_F;~%")
  (format port "  for (pair = ~a; scm_is_pair (pair); "
          (enum-type-smob-list enum))
  (format port "pair = SCM_CDR (pair))~%")
  (format port "    {~%")
  (format port "      SCM enum_smob;~%")
  (format port "      enum_smob = SCM_CAR (pair);~%")
  (format port "      if ((~a) SCM_SMOB_DATA (enum_smob) == c_obj)~%"
          (enum-type-c-type enum))
  (format port "        {~%")
  (format port "          result = enum_smob;~%")
  (format port "          break;~%")
  (format port "        }~%")
  (format port "    }~%")
  (format port "  return result;~%")
  (format port "}~%"))

(define (output-enum->c-converter enum port)
  (let* ((c-type-name (enum-type-c-type enum))
         (subsystem   (scheme-symbol->c-name (enum-type-subsystem enum))))

    (format port
            "static inline ~a~%~a (SCM obj, unsigned pos, const char *func)~%"
            c-type-name (enum-type-to-c-function enum))
    (format port "#define FUNC_NAME func~%")
    (format port "{~%")
    (format port "  SCM_VALIDATE_SMOB (pos, obj, ~a);~%"
            (string-append "gnutls_" subsystem "_enum"))
    (format port "  return ((~a) SCM_SMOB_DATA (obj));~%"
            c-type-name)
    (format port "}~%")
    (format port "#undef FUNC_NAME~%")))

(define (output-enum-get-name-function enum port)
  ;; Output a C function that, when passed a C ENUM value, returns a C string
  ;; representing that value.
  (let ((function (enum-type-automatic-get-name-function enum)))
    (format port
            "static const char *~%~a (~a c_obj)~%"
            function (enum-type-c-type enum))
    (format port "{~%")
    (format port "  static const struct ")
    (format port "{ ~a value; const char *name; } "
            (enum-type-c-type enum))
    (format port "table[] =~%")
    (format port "    {~%")
    (for-each (lambda (c+scheme)
                (format port "       { ~a, \"~a\" },~%"
                        (car c+scheme) (cdr c+scheme)))
              (enum-type-value-alist enum))
    (format port "    };~%")
    (format port "  unsigned i;~%")
    (format port "  const char *name = NULL;~%")
    (format port "  for (i = 0; i < ~a; i++)~%"
            (length (enum-type-value-alist enum)))
    (format port "    {~%")
    (format port "      if (table[i].value == c_obj)~%")
    (format port "        {~%")
    (format port "          name = table[i].name;~%")
    (format port "          break;~%")
    (format port "        }~%")
    (format port "    }~%")
    (format port "  return (name);~%")
    (format port "}~%")))


;;;
;;; Actual enumerations.
;;;

(define %cipher-enum
  (make-enum-type 'cipher "gnutls_cipher_algorithm_t"
                  '(null arcfour 3des-cbc aes-128-cbc aes-256-cbc
                    arcfour-40 rc2-40-cbc des-cbc)
                  "gnutls_cipher_get_name"))

(define %kx-enum
  (make-enum-type 'kx "gnutls_kx_algorithm_t"
                  '(rsa dhe-dss dhe-rsa anon-dh srp rsa-export
                    srp-rsa srp-dss psk dhe-dss)
                  "gnutls_kx_get_name"))

(define %params-enum
  (make-enum-type 'params "gnutls_params_type_t"
                  '(rsa-export dh)
                  #f))

(define %credentials-enum
  (make-enum-type 'credentials "gnutls_credentials_type_t"
                  '(certificate anon srp psk ia)
                  #f
                  "GNUTLS_CRD_"))

(define %mac-enum
  (make-enum-type 'mac "gnutls_mac_algorithm_t"
                  '(unknown null md5 sha1 rmd160 md2)
                  "gnutls_mac_get_name"))

(define %digest-enum
  (make-enum-type 'digest "gnutls_digest_algorithm_t"
                  '(null md5 sha1 rmd160 md2)
                  #f
                  "GNUTLS_DIG_"))

(define %compression-method-enum
  (make-enum-type 'compression-method "gnutls_compression_method_t"
                  '(null deflate)
                  "gnutls_compression_get_name"
                  "GNUTLS_COMP_"))

(define %connection-end-enum
  (make-enum-type 'connection-end "gnutls_connection_end_t"
                  '(server client)
                  #f
                  "GNUTLS_"))

(define %connection-flag-enum
  (make-enum-type 'connection-flag "gnutls_init_flags_t"
                  '(datagram
                    nonblock
                    no-extensions
                    no-replay-protection
                    no-signal
                    allow-id-change
                    enable-false-start
                    force-client-cert
                    no-tickets
                    key-share-top
                    key-share-top2
                    key-share-top3
                    post-handshake-auth
                    no-auto-rekey
                    safe-padding-check
                    enable-early-start
                    enable-rawpk
                    auto-reauth
                    enable-early-data)
                  #f
                  "GNUTLS_"))

(define %alert-level-enum
  (make-enum-type 'alert-level "gnutls_alert_level_t"
                  '(warning fatal)
                  #f
                  "GNUTLS_AL_"))

(define %alert-description-enum
  (make-enum-type 'alert-description "gnutls_alert_description_t"
                  '(close-notify unexpected-message bad-record-mac
decryption-failed record-overflow decompression-failure handshake-failure
ssl3-no-certificate bad-certificate unsupported-certificate
certificate-revoked certificate-expired certificate-unknown illegal-parameter
unknown-ca access-denied decode-error decrypt-error export-restriction
protocol-version insufficient-security internal-error user-canceled
no-renegotiation unsupported-extension certificate-unobtainable
unrecognized-name unknown-psk-identity)
                  #f
                  "GNUTLS_A_"))

(define %handshake-description-enum
  (make-enum-type 'handshake-description "gnutls_handshake_description_t"
                  '(hello-request client-hello server-hello certificate-pkt
                    server-key-exchange certificate-request server-hello-done
                    certificate-verify client-key-exchange finished)
                  #f
                  "GNUTLS_HANDSHAKE_"))

(define %certificate-status-enum
  (make-enum-type 'certificate-status "gnutls_certificate_status_t"
                  '(invalid revoked signer-not-found signer-not-ca
                    insecure-algorithm)
                  #f
                  "GNUTLS_CERT_"))

(define %certificate-request-enum
  (make-enum-type 'certificate-request "gnutls_certificate_request_t"
                  '(ignore request require)
                  #f
                  "GNUTLS_CERT_"))

;; XXX: Broken naming convention.
; (define %openpgp-key-status-enum
;   (make-enum-type 'openpgp-key-status "gnutls_openpgp_key_status_t"
;                   '(key fingerprint)
;                   #f
;                   "GNUTLS_OPENPGP_"))

(define %close-request-enum
  (make-enum-type 'close-request "gnutls_close_request_t"
                  '(rdwr wr) ;; FIXME: Check the meaning and rename
                  #f
                  "GNUTLS_SHUT_"))

(define %protocol-enum
  (make-enum-type 'protocol "gnutls_protocol_t"
                  '(ssl3 tls1-0 tls1-1 version-unknown)
                  #f
                  "GNUTLS_"))

(define %certificate-type-enum
  (make-enum-type 'certificate-type "gnutls_certificate_type_t"
                  '(x509 openpgp)
                  "gnutls_certificate_type_get_name"
                  "GNUTLS_CRT_"))

(define %x509-certificate-format-enum
  (make-enum-type 'x509-certificate-format "gnutls_x509_crt_fmt_t"
                  '(der pem)
                  #f
                  "GNUTLS_X509_FMT_"))

(define %x509-subject-alternative-name-enum
  (make-enum-type 'x509-subject-alternative-name
                  "gnutls_x509_subject_alt_name_t"
                  '(dnsname rfc822name uri ipaddress)
                  #f
                  "GNUTLS_SAN_"))

(define %pk-algorithm-enum
  (make-enum-type 'pk-algorithm "gnutls_pk_algorithm_t"
                  '(unknown rsa dsa)
                  "gnutls_pk_algorithm_get_name"
                  "GNUTLS_PK_"))

(define %sign-algorithm-enum
  (make-enum-type 'sign-algorithm "gnutls_sign_algorithm_t"
                  '(unknown rsa-sha1 dsa-sha1 rsa-md5 rsa-md2
                    rsa-rmd160)
                  "gnutls_sign_algorithm_get_name"
                  "GNUTLS_SIGN_"))

(define %psk-key-format-enum
  (make-enum-type 'psk-key-format "gnutls_psk_key_flags"
                  '(raw hex)
                  #f
                  "GNUTLS_PSK_KEY_"))

(define %key-usage-enum
  ;; Not actually an enum on the C side.
  (make-enum-type 'key-usage "int"
                  '(digital-signature non-repudiation key-encipherment
                    data-encipherment key-agreement key-cert-sign
                    crl-sign encipher-only decipher-only)
                  #f
                  "GNUTLS_KEY_"))

(define %certificate-verify-enum
  (make-enum-type 'certificate-verify "gnutls_certificate_verify_flags"
                  '(disable-ca-sign allow-x509-v1-ca-crt
                    do-not-allow-same allow-any-x509-v1-ca-crt
                    allow-sign-rsa-md2 allow-sign-rsa-md5)
                  #f
                  "GNUTLS_VERIFY_"))

(define %error-enum
  (make-enum-type 'error "int"
                  '(
;; FIXME: Automate this:
;; grep '^#define GNUTLS_E_' ../../../lib/includes/gnutls/gnutls.h.in \
;;  | sed -r -e 's/^#define GNUTLS_E_([^ ]+).*$/\1/' | tr A-Z_ a-z-
success
unsupported-version-packet
tls-packet-decoding-error
unexpected-packet-length
invalid-session
fatal-alert-received
unexpected-packet
warning-alert-received
error-in-finished-packet
unexpected-handshake-packet
decryption-failed
memory-error
decompression-failed
compression-failed
again
expired
db-error
srp-pwd-error
keyfile-error
insufficient-credentials
insuficient-credentials
insufficient-cred
insuficient-cred
hash-failed
base64-decoding-error
rehandshake
got-application-data
record-limit-reached
encryption-failed
pk-encryption-failed
pk-decryption-failed
pk-sign-failed
x509-unsupported-critical-extension
key-usage-violation
no-certificate-found
invalid-request
short-memory-buffer
interrupted
push-error
pull-error
received-illegal-parameter
requested-data-not-available
pkcs1-wrong-pad
received-illegal-extension
internal-error
dh-prime-unacceptable
file-error
too-many-empty-packets
unknown-pk-algorithm
too-many-handshake-packets
received-disallowed-name
certificate-required
no-temporary-rsa-params
no-compression-algorithms
no-cipher-suites
openpgp-getkey-failed
pk-sig-verify-failed
illegal-srp-username
srp-pwd-parsing-error
keyfile-parsing-error
no-temporary-dh-params
asn1-element-not-found
asn1-identifier-not-found
asn1-der-error
asn1-value-not-found
asn1-generic-error
asn1-value-not-valid
asn1-tag-error
asn1-tag-implicit
asn1-type-any-error
asn1-syntax-error
asn1-der-overflow
openpgp-uid-revoked
certificate-error
x509-certificate-error
certificate-key-mismatch
unsupported-certificate-type
x509-unknown-san
openpgp-fingerprint-unsupported
x509-unsupported-attribute
unknown-hash-algorithm
unknown-pkcs-content-type
unknown-pkcs-bag-type
invalid-password
mac-verify-failed
constraint-error
warning-ia-iphf-received
warning-ia-fphf-received
ia-verify-failed
unknown-algorithm
unsupported-signature-algorithm
safe-renegotiation-failed
unsafe-renegotiation-denied
unknown-srp-username
premature-termination
malformed-cidr
base64-encoding-error
incompatible-gcrypt-library
incompatible-crypto-library
incompatible-libtasn1-library
openpgp-keyring-error
x509-unsupported-oid
random-failed
base64-unexpected-header-error
openpgp-subkey-error
crypto-already-registered
already-registered
handshake-too-large
cryptodev-ioctl-error
cryptodev-device-error
channel-binding-not-available
bad-cookie
openpgp-preferred-key-error
incompat-dsa-key-with-tls-protocol
insufficient-security
heartbeat-pong-received
heartbeat-ping-received
unrecognized-name
pkcs11-error
pkcs11-load-error
parsing-error
pkcs11-pin-error
pkcs11-slot-error
locking-error
pkcs11-attribute-error
pkcs11-device-error
pkcs11-data-error
pkcs11-unsupported-feature-error
pkcs11-key-error
pkcs11-pin-expired
pkcs11-pin-locked
pkcs11-session-error
pkcs11-signature-error
pkcs11-token-error
pkcs11-user-error
crypto-init-failed
timedout
user-error
ecc-no-supported-curves
ecc-unsupported-curve
pkcs11-requested-object-not-availble
certificate-list-unsorted
illegal-parameter
no-priorities-were-set
x509-unsupported-extension
session-eof
tpm-error
tpm-key-password-error
tpm-srk-password-error
tpm-session-error
tpm-key-not-found
tpm-uninitialized
tpm-no-lib
no-certificate-status
ocsp-response-error
random-device-error
auth-error
no-application-protocol
sockets-init-error
key-import-failed
inappropriate-fallback
certificate-verification-error
privkey-verification-error
unexpected-extensions-length
asn1-embedded-null-in-string
self-test-error
no-self-test
lib-in-error-state
pk-generation-error
idna-error
need-fallback
session-user-id-changed
handshake-during-false-start
unavailable-during-handshake
pk-invalid-pubkey
pk-invalid-privkey
not-yet-activated
invalid-utf8-string
no-embedded-data
invalid-utf8-email
invalid-password-string
certificate-time-error
record-overflow
asn1-time-error
incompatible-sig-with-key
pk-invalid-pubkey-params
pk-no-validation-params
ocsp-mismatch-with-certs
no-common-key-share
reauth-request
too-many-matches
crl-verification-error
missing-extension
db-entry-exists
early-data-rejected
unimplemented-feature
int-ret-0
int-check-again
application-error-max
application-error-min
)
                  "gnutls_strerror"
                  "GNUTLS_E_"))


(define %openpgp-certificate-format-enum
  (make-enum-type 'openpgp-certificate-format "gnutls_openpgp_crt_fmt_t"
                  '(raw base64)
                  #f
                  "GNUTLS_OPENPGP_FMT_"))

(define %server-name-type-enum
  (make-enum-type 'server-name-type "gnutls_server_name_type_t"
                  '(dns)
                  #f
                  "GNUTLS_NAME_"))

(define %gnutls-enums
  ;; All enums.
  (list %cipher-enum %kx-enum %params-enum %credentials-enum %mac-enum
        %digest-enum %compression-method-enum
        %connection-end-enum %connection-flag-enum
        %alert-level-enum %alert-description-enum %handshake-description-enum
        %certificate-status-enum %certificate-request-enum
        %close-request-enum %protocol-enum %certificate-type-enum
        %x509-certificate-format-enum %x509-subject-alternative-name-enum
        %pk-algorithm-enum %sign-algorithm-enum %server-name-type-enum
        %psk-key-format-enum %key-usage-enum %certificate-verify-enum
        %error-enum

        %openpgp-certificate-format-enum))

;;; Local Variables:
;;; mode: scheme
;;; coding: latin-1
;;; End:

;;; arch-tag: 9e3eb6bb-61a5-4e85-861f-1914ab9677b0
