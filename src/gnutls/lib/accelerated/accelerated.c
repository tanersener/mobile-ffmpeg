/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <config.h>
#include <accelerated.h>
#if defined(ASM_X86)
# include <x86/aes-x86.h>
# include <x86/x86-common.h>
#elif defined(ASM_AARCH64)
# include <aarch64/aarch64-common.h>
#endif

void _gnutls_register_accel_crypto(void)
{
#if defined(ASM_X86)
	register_x86_crypto();
#endif

#if defined(ASM_AARCH64)
	register_aarch64_crypto();
#endif

	return;
}
