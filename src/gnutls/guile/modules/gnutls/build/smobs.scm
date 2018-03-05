;;; Help produce Guile wrappers for GnuTLS types.
;;;
;;; GnuTLS --- Guile bindings for GnuTLS.
;;; Copyright (C) 2007-2012, 2014 Free Software Foundation, Inc.
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

(define-module (gnutls build smobs)
  :use-module (srfi srfi-9)
  :use-module (srfi srfi-13)
  :use-module (gnutls build utils)
  :export (make-smob-type smob-type-tag smob-free-function
           smob-type-predicate-scheme-name
           smob-type-from-c-function smob-type-to-c-function

           output-smob-type-definition output-smob-type-declaration
           output-smob-type-predicate
           output-c->smob-converter output-smob->c-converter

           %gnutls-smobs))


;;;
;;; SMOB types.
;;;

(define-record-type <smob-type>
  (%make-smob-type c-name scm-name free-function)
  smob-type?
  (c-name         smob-type-c-name)
  (scm-name       smob-type-scheme-name)
  (free-function  smob-type-free-function))

(define (make-smob-type c-name scm-name . free-function)
  (%make-smob-type c-name scm-name
                   (if (null? free-function)
                       (string-append "gnutls_"
                                      (scheme-symbol->c-name scm-name)
                                      "_deinit")
                       (car free-function))))

(define (smob-type-tag type)
  ;; Return the name of the C variable holding the type tag for TYPE.
  (string-append "scm_tc16_gnutls_"
                 (scheme-symbol->c-name (smob-type-scheme-name type))))

(define (smob-type-predicate-scheme-name type)
  ;; Return a string denoting the Scheme name of TYPE's type predicate.
  (string-append (symbol->string (smob-type-scheme-name type)) "?"))

(define (smob-type-to-c-function type)
  ;; Return the name of the C `scm_to_' function for SMOB.
  (string-append "scm_to_gnutls_"
                 (scheme-symbol->c-name (smob-type-scheme-name type))))

(define (smob-type-from-c-function type)
  ;; Return the name of the C `scm_from_' function for SMOB.
  (string-append "scm_from_gnutls_"
                 (scheme-symbol->c-name (smob-type-scheme-name type))))


;;;
;;; C code generation.
;;;

(define (output-smob-type-definition type port)
  (format port "SCM_GLOBAL_SMOB (~a, \"~a\", 0);~%"
          (smob-type-tag type)
          (smob-type-scheme-name type))

  (format port "SCM_SMOB_FREE (~a, ~a_free, obj)~%{~%"
          (smob-type-tag type)
          (scheme-symbol->c-name (smob-type-scheme-name type)))
  (format port "  ~a c_obj;~%"
          (smob-type-c-name type))
  (format port "  c_obj = (~a) SCM_SMOB_DATA (obj);~%"
          (smob-type-c-name type))
  (format port "  ~a (c_obj);~%"
          (smob-type-free-function type))
  (format port "  return 0;~%")
  (format port "}~%"))

(define (output-smob-type-declaration type port)
  ;; Issue a header file declaration for the SMOB type tag of TYPE.
  (format port "SCM_API scm_t_bits ~a;~%"
          (smob-type-tag type)))

(define (output-smob-type-predicate type port)
  (define (texi-doc-string)
    (string-append "Return true if @var{obj} is of type @code{"
                   (symbol->string (smob-type-scheme-name type))
                   "}."))

  (let ((c-name (string-append "scm_gnutls_"
                               (string-map (lambda (chr)
                                             (if (char=? chr #\-)
                                                 #\_
                                                 chr))
                                           (symbol->string
                                            (smob-type-scheme-name type)))
                               "_p")))
    (format port "SCM_DEFINE (~a, \"~a\", 1, 0, 0,~%"
            c-name (smob-type-predicate-scheme-name type))
    (format port "            (SCM obj),~%")
    (format port "            \"~a\")~%"
            (texi-doc-string))
    (format port "#define FUNC_NAME s_~a~%"
            c-name)
    (format port "{~%")
    (format port "  return (scm_from_bool (SCM_SMOB_PREDICATE (~a, obj)));~%"
            (smob-type-tag type))
    (format port "}~%#undef FUNC_NAME~%")))

(define (output-c->smob-converter type port)
  (format port "static inline SCM~%~a (~a c_obj)~%{~%"
          (smob-type-from-c-function type)
          (smob-type-c-name type))
  (format port "  SCM_RETURN_NEWSMOB (~a, (scm_t_bits) c_obj);~%"
          (smob-type-tag type))
  (format port "}~%"))

(define (output-smob->c-converter type port)
  (format port "static inline ~a~%~a (SCM obj, "
          (smob-type-c-name type)
          (smob-type-to-c-function type))
  (format port "unsigned pos, const char *func)~%")
  (format port "#define FUNC_NAME func~%")
  (format port "{~%")
  (format port "  SCM_VALIDATE_SMOB (pos, obj, ~a);~%"
          (string-append "gnutls_"
                         (scheme-symbol->c-name (smob-type-scheme-name type))))
  (format port "  return ((~a) SCM_SMOB_DATA (obj));~%"
          (smob-type-c-name type))
  (format port "}~%")
  (format port "#undef FUNC_NAME~%"))


;;;
;;; Actual SMOB types.
;;;

(define %session-smob
  (make-smob-type "gnutls_session_t" 'session
                  "gnutls_deinit"))

(define %anonymous-client-credentials-smob
  (make-smob-type "gnutls_anon_client_credentials_t" 'anonymous-client-credentials
                  "gnutls_anon_free_client_credentials"))

(define %anonymous-server-credentials-smob
  (make-smob-type "gnutls_anon_server_credentials_t" 'anonymous-server-credentials
                  "gnutls_anon_free_server_credentials"))

(define %dh-parameters-smob
  (make-smob-type "gnutls_dh_params_t" 'dh-parameters
                  "gnutls_dh_params_deinit"))

(define %certificate-credentials-smob
  (make-smob-type "gnutls_certificate_credentials_t" 'certificate-credentials
                  "gnutls_certificate_free_credentials"))

(define %srp-server-credentials-smob
  (make-smob-type "gnutls_srp_server_credentials_t" 'srp-server-credentials
                  "gnutls_srp_free_server_credentials"))

(define %srp-client-credentials-smob
  (make-smob-type "gnutls_srp_client_credentials_t" 'srp-client-credentials
                  "gnutls_srp_free_client_credentials"))

(define %psk-server-credentials-smob
  (make-smob-type "gnutls_psk_server_credentials_t" 'psk-server-credentials
                  "gnutls_psk_free_server_credentials"))

(define %psk-client-credentials-smob
  (make-smob-type "gnutls_psk_client_credentials_t" 'psk-client-credentials
                  "gnutls_psk_free_client_credentials"))

(define %x509-certificate-smob
  (make-smob-type "gnutls_x509_crt_t" 'x509-certificate
                  "gnutls_x509_crt_deinit"))

(define %x509-private-key-smob
  (make-smob-type "gnutls_x509_privkey_t" 'x509-private-key
                  "gnutls_x509_privkey_deinit"))

(define %openpgp-certificate-smob
  (make-smob-type "gnutls_openpgp_crt_t" 'openpgp-certificate
                  "gnutls_openpgp_crt_deinit"))

(define %openpgp-private-key-smob
  (make-smob-type "gnutls_openpgp_privkey_t" 'openpgp-private-key
                  "gnutls_openpgp_privkey_deinit"))

(define %openpgp-keyring-smob
  (make-smob-type "gnutls_openpgp_keyring_t" 'openpgp-keyring
                  "gnutls_openpgp_keyring_deinit"))


(define %gnutls-smobs
  ;; All SMOB types.
  (list %session-smob %anonymous-client-credentials-smob
        %anonymous-server-credentials-smob %dh-parameters-smob
        %certificate-credentials-smob
        %srp-server-credentials-smob %srp-client-credentials-smob
        %psk-server-credentials-smob %psk-client-credentials-smob
        %x509-certificate-smob %x509-private-key-smob

        %openpgp-certificate-smob %openpgp-private-key-smob
        %openpgp-keyring-smob))


;;; Local Variables:
;;; mode: scheme
;;; coding: latin-1
;;; End:

;;; arch-tag: 26bf79ef-6dee-45f2-9e9d-2d209c518278
