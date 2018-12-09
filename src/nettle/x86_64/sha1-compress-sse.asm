C x86_64/sha1-compress.asm

ifelse(<
   Copyright (C) 2004, 2008, 2013, 2018 Niels Möller

   This file is part of GNU Nettle.

   GNU Nettle is free software: you can redistribute it and/or
   modify it under the terms of either:

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at your
       option) any later version.

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at your
       option) any later version.

   or both in parallel, as here.

   GNU Nettle is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see http://www.gnu.org/licenses/.
>)

C Register usage.

C Arguments
define(<STATE>,<%rdi>)dnl
define(<INPUT>,<%rsi>)dnl

C Constants
define(<K1VALUE>, <0x5A827999>)dnl		C  Rounds  0-19
define(<K2VALUE>, <0x6ED9EBA1>)dnl		C  Rounds 20-39
define(<K3VALUE>, <0x8F1BBCDC>)dnl		C  Rounds 40-59
define(<K4VALUE>, <0xCA62C1D6>)dnl		C  Rounds 60-79

	.file "sha1-compress.asm"

	C _nettle_sha1_compress(uint32_t *state, uint8_t *input)
	
	.text
	ALIGN(16)
PROLOGUE(_nettle_sha1_compress)
	C save all registers that need to be saved XXX
	movups (INPUT), W0
	movups 16(INPUT), W1
	movups 32(INPUT), W2
	movups 48(INPUT), W3

	ret
EPILOGUE(_nettle_sha1_compress)

