/*
 * Copyright (C) 2011-2018 Free Software Foundation, Inc.
 * Copyright (C) 2018 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * The GnuTLS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

/*
 * The following code is an implementation of the AES-128-CBC cipher
 * using intel's AES instruction set. 
 */

#include "errors.h"
#include "gnutls_int.h"
#include <gnutls/crypto.h>
#include "errors.h"
#include <aes-x86.h>
#include <sha-x86.h>
#include <x86-common.h>
#ifdef HAVE_LIBNETTLE
# include <nettle/aes.h>		/* for key generation in 192 and 256 bits */
# include <sha-padlock.h>
#endif
#include <aes-padlock.h>
#ifdef HAVE_CPUID_H
# include <cpuid.h>
#else
# define __get_cpuid(...) 0
# define __get_cpuid_count(...) 0
#endif

/* ebx, ecx, edx 
 * This is a format compatible with openssl's CPUID detection.
 */
#if defined(__GNUC__)
__attribute__((visibility("hidden")))
#elif defined(__SUNPRO_C)
__hidden
#endif
unsigned int _gnutls_x86_cpuid_s[4];

#ifndef bit_SHA
# define bit_SHA (1<<29)
#endif

/* ecx */
#ifndef bit_AVX512BITALG
# define bit_AVX512BITALG 0x4000
#endif

#ifndef bit_PCLMUL
# define bit_PCLMUL 0x2
#endif

#ifndef bit_SSSE3
/* ecx */
# define bit_SSSE3 0x0000200
#endif

#ifndef bit_AES
# define bit_AES 0x2000000
#endif

#ifndef bit_AVX
# define bit_AVX 0x10000000
#endif

#ifndef OSXSAVE_MASK
/* OSXSAVE|FMA|MOVBE */
# define OSXSAVE_MASK (0x8000000|0x1000|0x400000)
#endif

#ifndef bit_MOVBE
# define bit_MOVBE 0x00400000
#endif

#define via_bit_PADLOCK (0x3 << 6)
#define via_bit_PADLOCK_PHE (0x3 << 10)
#define via_bit_PADLOCK_PHE_SHA512 (0x3 << 25)

/* Our internal bit-string for cpu capabilities. Should be set
 * in GNUTLS_CPUID_OVERRIDE */
#define EMPTY_SET 1
#define INTEL_AES_NI (1<<1)
#define INTEL_SSSE3 (1<<2)
#define INTEL_PCLMUL (1<<3)
#define INTEL_AVX (1<<4)
#define INTEL_SHA (1<<5)
#define VIA_PADLOCK (1<<20)
#define VIA_PADLOCK_PHE (1<<21)
#define VIA_PADLOCK_PHE_SHA512 (1<<22)

#ifndef HAVE_GET_CPUID_COUNT
static inline void
get_cpuid_level7(unsigned int *eax, unsigned int *ebx,
		 unsigned int *ecx, unsigned int *edx)
{
	/* we avoid using __get_cpuid_count, because it is not available with gcc 4.8 */
	if (__get_cpuid_max(7, 0) < 7)
		return;

	__cpuid_count(7, 0, *eax, *ebx, *ecx, *edx);
	return;
}
#else
# define get_cpuid_level7(a,b,c,d) __get_cpuid_count(7, 0, a, b, c, d)
#endif

static unsigned read_cpuid_vals(unsigned int vals[4])
{
	unsigned t1, t2, t3;
	vals[0] = vals[1] = vals[2] = vals[3] = 0;

	if (!__get_cpuid(1, &t1, &vals[0], &vals[1], &t2))
		return 0;
	/* suppress AVX512; it works conditionally on certain CPUs on the original code */
	vals[1] &= 0xfffff7ff;

	get_cpuid_level7(&t1, &vals[2], &t2, &t3);

	return 1;
}

/* Based on the example in "How to detect New Instruction support in
 * the 4th generation Intel Core processor family.
 * https://software.intel.com/en-us/articles/how-to-detect-new-instruction-support-in-the-4th-generation-intel-core-processor-family
 */
static unsigned check_4th_gen_intel_features(unsigned ecx)
{
	uint32_t xcr0;

	if ((ecx & OSXSAVE_MASK) != OSXSAVE_MASK)
		return 0;

#if defined(_MSC_VER) && !defined(__clang__)
	xcr0 = _xgetbv(0);
#else
	__asm__ ("xgetbv" : "=a" (xcr0) : "c" (0) : "%edx");
#endif
	/* Check if xmm and ymm state are enabled in XCR0. */
	return (xcr0 & 6) == 6;
}

static void capabilities_to_intel_cpuid(unsigned capabilities)
{
	unsigned a[4];

	if (capabilities & EMPTY_SET) {
		return;
	}

	if (!read_cpuid_vals(a))
		return;

	if (capabilities & INTEL_AES_NI) {
		if (a[1] & bit_AES) {
			_gnutls_x86_cpuid_s[1] |= bit_AES;
		} else {
			_gnutls_debug_log
			    ("AESNI acceleration requested but not available\n");
		}
	}

	if (capabilities & INTEL_SSSE3) {
		if (a[1] & bit_SSSE3) {
			_gnutls_x86_cpuid_s[1] |= bit_SSSE3;
		} else {
			_gnutls_debug_log
			    ("SSSE3 acceleration requested but not available\n");
		}
	}

	if (capabilities & INTEL_AVX) {
		if ((a[1] & bit_AVX) && check_4th_gen_intel_features(a[1])) {
			_gnutls_x86_cpuid_s[1] |= bit_AVX|bit_MOVBE;
		} else {
			_gnutls_debug_log
			    ("AVX acceleration requested but not available\n");
		}
	}

	if (capabilities & INTEL_PCLMUL) {
		if (a[1] & bit_PCLMUL) {
			_gnutls_x86_cpuid_s[1] |= bit_PCLMUL;
		} else {
			_gnutls_debug_log
			    ("PCLMUL acceleration requested but not available\n");
		}
	}

	if (capabilities & INTEL_SHA) {
		if (a[2] & bit_SHA) {
			_gnutls_x86_cpuid_s[2] |= bit_SHA;
		} else {
			_gnutls_debug_log
			    ("SHA acceleration requested but not available\n");
		}
	}
}


static unsigned check_optimized_aes(void)
{
	return (_gnutls_x86_cpuid_s[1] & bit_AES);
}

static unsigned check_ssse3(void)
{
	return (_gnutls_x86_cpuid_s[1] & bit_SSSE3);
}

static unsigned check_sha(void)
{
	return (_gnutls_x86_cpuid_s[2] & bit_SHA);
}

#ifdef ASM_X86_64
static unsigned check_avx_movbe(void)
{
	if (check_4th_gen_intel_features(_gnutls_x86_cpuid_s[1]) == 0)
		return 0;

	return ((_gnutls_x86_cpuid_s[1] & bit_AVX));
}

static unsigned check_pclmul(void)
{
	return (_gnutls_x86_cpuid_s[1] & bit_PCLMUL);
}
#endif

#ifdef ENABLE_PADLOCK
static unsigned capabilities_to_via_edx(unsigned capabilities)
{
	unsigned a,b,c,t;

	memset(_gnutls_x86_cpuid_s, 0, sizeof(_gnutls_x86_cpuid_s));

	if (capabilities & EMPTY_SET) {
		return 0;
	}

	if (!__get_cpuid(1, &t, &a, &b, &c))
		return 0;
	if (capabilities & VIA_PADLOCK) {
		if (c & via_bit_PADLOCK) {
			_gnutls_x86_cpuid_s[2] |= via_bit_PADLOCK;
		} else {
			_gnutls_debug_log
			    ("Padlock acceleration requested but not available\n");
		}
	}

	if (capabilities & VIA_PADLOCK_PHE) {
		if (c & via_bit_PADLOCK_PHE) {
			_gnutls_x86_cpuid_s[2] |= via_bit_PADLOCK_PHE;
		} else {
			_gnutls_debug_log
			    ("Padlock-PHE acceleration requested but not available\n");
		}
	}

	if (capabilities & VIA_PADLOCK_PHE_SHA512) {
		if (c & via_bit_PADLOCK_PHE_SHA512) {
			_gnutls_x86_cpuid_s[2] |= via_bit_PADLOCK_PHE_SHA512;
		} else {
			_gnutls_debug_log
			    ("Padlock-PHE-SHA512 acceleration requested but not available\n");
		}
	}

	return _gnutls_x86_cpuid_s[2];
}

static int check_padlock(unsigned edx)
{
	return ((edx & via_bit_PADLOCK) == via_bit_PADLOCK);
}

static int check_phe(unsigned edx)
{
	return ((edx & via_bit_PADLOCK_PHE) == via_bit_PADLOCK_PHE);
}

/* We are actually checking for SHA512 */
static int check_phe_sha512(unsigned edx)
{
	return ((edx & via_bit_PADLOCK_PHE_SHA512) == via_bit_PADLOCK_PHE_SHA512);
}

static int check_phe_partial(void)
{
	const char *text = "test and test";
	uint32_t iv[5] = { 0x67452301UL, 0xEFCDAB89UL,
		0x98BADCFEUL, 0x10325476UL, 0xC3D2E1F0UL
	};

	padlock_sha1_blocks(iv, text, sizeof(text) - 1);
	padlock_sha1_blocks(iv, text, sizeof(text) - 1);

	if (iv[0] == 0x9096E2D8UL && iv[1] == 0xA33074EEUL &&
	    iv[2] == 0xCDBEE447UL && iv[3] == 0xEC7979D2UL &&
	    iv[4] == 0x9D3FF5CFUL)
		return 1;
	else
		return 0;
}

static unsigned check_via(void)
{
	unsigned int a, b, c, d;

	if (!__get_cpuid(0, &a, &b, &c, &d))
		return 0;

	if ((memcmp(&b, "Cent", 4) == 0 &&
	     memcmp(&d, "aurH", 4) == 0 && memcmp(&c, "auls", 4) == 0)) {
		return 1;
	}

	return 0;
}

static
void register_x86_padlock_crypto(unsigned capabilities)
{
	int ret, phe;
	unsigned edx;

	if (check_via() == 0)
		return;

	if (capabilities == 0)
		edx = padlock_capability();
	else
		edx = capabilities_to_via_edx(capabilities);

	if (check_padlock(edx)) {
		_gnutls_debug_log
		    ("Padlock AES accelerator was detected\n");
		ret =
		    gnutls_crypto_single_cipher_register
		    (GNUTLS_CIPHER_AES_128_CBC, 80, &_gnutls_aes_padlock, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		/* register GCM ciphers */
		ret =
		    gnutls_crypto_single_cipher_register
		    (GNUTLS_CIPHER_AES_128_GCM, 80,
		     &_gnutls_aes_gcm_padlock, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_cipher_register
		    (GNUTLS_CIPHER_AES_256_CBC, 80, &_gnutls_aes_padlock, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_cipher_register
		    (GNUTLS_CIPHER_AES_256_GCM, 80,
		     &_gnutls_aes_gcm_padlock, 0);
		if (ret < 0) {
			gnutls_assert();
		}
	} else {
		_gnutls_priority_update_non_aesni();
	}

#ifdef HAVE_LIBNETTLE
	phe = check_phe(edx);

	if (phe && check_phe_partial()) {
		_gnutls_debug_log
		    ("Padlock SHA1 and SHA256 (partial) accelerator was detected\n");
		if (check_phe_sha512(edx)) {
			_gnutls_debug_log
			    ("Padlock SHA512 (partial) accelerator was detected\n");
			ret =
			    gnutls_crypto_single_digest_register
			    (GNUTLS_DIG_SHA384, 80,
			     &_gnutls_sha_padlock_nano, 0);
			if (ret < 0) {
				gnutls_assert();
			}

			ret =
			    gnutls_crypto_single_digest_register
			    (GNUTLS_DIG_SHA512, 80,
			     &_gnutls_sha_padlock_nano, 0);
			if (ret < 0) {
				gnutls_assert();
			}

			ret =
			    gnutls_crypto_single_mac_register
			    (GNUTLS_MAC_SHA384, 80,
			     &_gnutls_hmac_sha_padlock_nano, 0);
			if (ret < 0) {
				gnutls_assert();
			}

			ret =
			    gnutls_crypto_single_mac_register
			    (GNUTLS_MAC_SHA512, 80,
			     &_gnutls_hmac_sha_padlock_nano, 0);
			if (ret < 0) {
				gnutls_assert();
			}
		}

		ret =
		    gnutls_crypto_single_digest_register(GNUTLS_DIG_SHA1,
							 80,
							 &_gnutls_sha_padlock_nano, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_digest_register(GNUTLS_DIG_SHA224,
							 80,
							 &_gnutls_sha_padlock_nano, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_digest_register(GNUTLS_DIG_SHA256,
							 80,
							 &_gnutls_sha_padlock_nano, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_mac_register(GNUTLS_MAC_SHA1,
						      80,
						      &_gnutls_hmac_sha_padlock_nano, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		/* we don't register MAC_SHA224 because it is not used by TLS */

		ret =
		    gnutls_crypto_single_mac_register(GNUTLS_MAC_SHA256,
						      80,
						      &_gnutls_hmac_sha_padlock_nano, 0);
		if (ret < 0) {
			gnutls_assert();
		}
	} else if (phe) {
		/* Original padlock PHE. Does not support incremental operations.
		 */
		_gnutls_debug_log
		    ("Padlock SHA1 and SHA256 accelerator was detected\n");
		ret =
		    gnutls_crypto_single_digest_register(GNUTLS_DIG_SHA1,
							 80,
							 &_gnutls_sha_padlock, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_digest_register(GNUTLS_DIG_SHA256,
							 80,
							 &_gnutls_sha_padlock, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_mac_register(GNUTLS_MAC_SHA1,
						      80,
						      &_gnutls_hmac_sha_padlock, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_mac_register(GNUTLS_MAC_SHA256,
						      80,
						      &_gnutls_hmac_sha_padlock, 0);
		if (ret < 0) {
			gnutls_assert();
		}
	}
#endif

	return;
}
#endif

static unsigned check_intel_or_amd(void)
{
	unsigned int a, b, c, d;

	if (!__get_cpuid(0, &a, &b, &c, &d))
		return 0;

	if ((memcmp(&b, "Genu", 4) == 0 &&
	     memcmp(&d, "ineI", 4) == 0 &&
	     memcmp(&c, "ntel", 4) == 0) ||
	    (memcmp(&b, "Auth", 4) == 0 &&
	     memcmp(&d, "enti", 4) == 0 && memcmp(&c, "cAMD", 4) == 0)) {
		return 1;
	}

	return 0;
}

static
void register_x86_intel_crypto(unsigned capabilities)
{
	int ret;

	memset(_gnutls_x86_cpuid_s, 0, sizeof(_gnutls_x86_cpuid_s));

	if (check_intel_or_amd() == 0)
		return;

	if (capabilities == 0) {
		if (!read_cpuid_vals(_gnutls_x86_cpuid_s))
			return;
	} else {
		capabilities_to_intel_cpuid(capabilities);
	}

	if (check_ssse3()) {
		_gnutls_debug_log("Intel SSSE3 was detected\n");

		ret =
		    gnutls_crypto_single_cipher_register
		    (GNUTLS_CIPHER_AES_128_GCM, 90,
		     &_gnutls_aes_gcm_x86_ssse3, 0);
			if (ret < 0) {
				gnutls_assert();
			}

		ret =
		    gnutls_crypto_single_cipher_register
		    (GNUTLS_CIPHER_AES_256_GCM, 90,
		     &_gnutls_aes_gcm_x86_ssse3, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_cipher_register
		    (GNUTLS_CIPHER_AES_128_CBC, 90, &_gnutls_aes_ssse3, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_cipher_register
		    (GNUTLS_CIPHER_AES_192_CBC, 90, &_gnutls_aes_ssse3, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_cipher_register
		    (GNUTLS_CIPHER_AES_256_CBC, 90, &_gnutls_aes_ssse3, 0);
		if (ret < 0) {
			gnutls_assert();
		}
	}

	if (check_sha() || check_ssse3()) {
		if (check_sha())
			_gnutls_debug_log("Intel SHA was detected\n");

		ret =
		    gnutls_crypto_single_digest_register(GNUTLS_DIG_SHA1,
							 80,
							 &_gnutls_sha_x86_ssse3, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_digest_register(GNUTLS_DIG_SHA224,
							 80,
							 &_gnutls_sha_x86_ssse3, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_digest_register(GNUTLS_DIG_SHA256,
							 80,
							 &_gnutls_sha_x86_ssse3, 0);
		if (ret < 0) {
			gnutls_assert();
		}


		ret =
		    gnutls_crypto_single_mac_register(GNUTLS_MAC_SHA1,
							 80,
							 &_gnutls_hmac_sha_x86_ssse3, 0);
		if (ret < 0)
			gnutls_assert();

		ret =
		    gnutls_crypto_single_mac_register(GNUTLS_MAC_SHA224,
							 80,
							 &_gnutls_hmac_sha_x86_ssse3, 0);
		if (ret < 0)
			gnutls_assert();

		ret =
		    gnutls_crypto_single_mac_register(GNUTLS_MAC_SHA256,
							 80,
							 &_gnutls_hmac_sha_x86_ssse3, 0);
		if (ret < 0)
			gnutls_assert();

		ret =
		    gnutls_crypto_single_digest_register(GNUTLS_DIG_SHA384,
							 80,
							 &_gnutls_sha_x86_ssse3, 0);
		if (ret < 0)
			gnutls_assert();

		ret =
		    gnutls_crypto_single_digest_register(GNUTLS_DIG_SHA512,
							 80,
							 &_gnutls_sha_x86_ssse3, 0);
		if (ret < 0)
			gnutls_assert();
		ret =
		    gnutls_crypto_single_mac_register(GNUTLS_MAC_SHA384,
							 80,
							 &_gnutls_hmac_sha_x86_ssse3, 0);
		if (ret < 0)
			gnutls_assert();

		ret =
		    gnutls_crypto_single_mac_register(GNUTLS_MAC_SHA512,
							 80,
							 &_gnutls_hmac_sha_x86_ssse3, 0);
		if (ret < 0)
			gnutls_assert();
	}

	if (check_optimized_aes()) {
		_gnutls_debug_log("Intel AES accelerator was detected\n");
		ret =
		    gnutls_crypto_single_cipher_register
		    (GNUTLS_CIPHER_AES_128_CBC, 80, &_gnutls_aesni_x86, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_cipher_register
		    (GNUTLS_CIPHER_AES_192_CBC, 80, &_gnutls_aesni_x86, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_cipher_register
		    (GNUTLS_CIPHER_AES_256_CBC, 80, &_gnutls_aesni_x86, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_cipher_register
		    (GNUTLS_CIPHER_AES_128_CCM, 80,
		     &_gnutls_aes_ccm_x86_aesni, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_cipher_register
		    (GNUTLS_CIPHER_AES_256_CCM, 80,
		     &_gnutls_aes_ccm_x86_aesni, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_cipher_register
		    (GNUTLS_CIPHER_AES_128_CCM_8, 80,
		     &_gnutls_aes_ccm_x86_aesni, 0);
		if (ret < 0) {
			gnutls_assert();
		}

		ret =
		    gnutls_crypto_single_cipher_register
		    (GNUTLS_CIPHER_AES_256_CCM_8, 80,
		     &_gnutls_aes_ccm_x86_aesni, 0);
		if (ret < 0) {
			gnutls_assert();
		}

#ifdef ASM_X86_64
		if (check_pclmul()) {
			/* register GCM ciphers */
			if (check_avx_movbe()) {
				_gnutls_debug_log
				    ("Intel GCM accelerator (AVX) was detected\n");
				ret =
				    gnutls_crypto_single_cipher_register
				    (GNUTLS_CIPHER_AES_128_GCM, 80,
				     &_gnutls_aes_gcm_pclmul_avx, 0);
				if (ret < 0) {
					gnutls_assert();
				}

				ret =
				    gnutls_crypto_single_cipher_register
				    (GNUTLS_CIPHER_AES_256_GCM, 80,
				     &_gnutls_aes_gcm_pclmul_avx, 0);
				if (ret < 0) {
					gnutls_assert();
				}
			} else {
				_gnutls_debug_log
				    ("Intel GCM accelerator was detected\n");
				ret =
				    gnutls_crypto_single_cipher_register
				    (GNUTLS_CIPHER_AES_128_GCM, 80,
				     &_gnutls_aes_gcm_pclmul, 0);
				if (ret < 0) {
					gnutls_assert();
				}

				ret =
				    gnutls_crypto_single_cipher_register
				    (GNUTLS_CIPHER_AES_256_GCM, 80,
				     &_gnutls_aes_gcm_pclmul, 0);
				if (ret < 0) {
					gnutls_assert();
				}
			}
		} else
#endif
		{
			ret =
			    gnutls_crypto_single_cipher_register
			    (GNUTLS_CIPHER_AES_128_GCM, 80,
			     &_gnutls_aes_gcm_x86_aesni, 0);
			if (ret < 0) {
				gnutls_assert();
			}

			ret =
			    gnutls_crypto_single_cipher_register
			    (GNUTLS_CIPHER_AES_256_GCM, 80,
			     &_gnutls_aes_gcm_x86_aesni, 0);
			if (ret < 0) {
				gnutls_assert();
			}
		}
	} else {
		_gnutls_priority_update_non_aesni();
	}

	return;
}


void register_x86_crypto(void)
{
	unsigned capabilities = 0;
	char *p;
	p = secure_getenv("GNUTLS_CPUID_OVERRIDE");
	if (p) {
		capabilities = strtol(p, NULL, 0);
	}

	register_x86_intel_crypto(capabilities);
#ifdef ENABLE_PADLOCK
	register_x86_padlock_crypto(capabilities);
#endif
}

