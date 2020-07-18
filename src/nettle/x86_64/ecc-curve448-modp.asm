C x86_64/ecc-curve448-modp.asm

ifelse(<
   Copyright (C) 2019 Niels Möller

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

	.file "ecc-curve448-modp.asm"

define(<RP>, <%rsi>)
define(<X0>, <%rax>)
define(<X1>, <%rbx>)
define(<X2>, <%rcx>)
define(<X3>, <%rdx>)
define(<X4>, <%rbp>)
define(<X5>, <%rdi>)
define(<X6>, <%r8>)
define(<X7>, <%r9>)
define(<T0>, <%r10>)
define(<T1>, <%r11>)
define(<T2>, <%r12>)

PROLOGUE(_nettle_ecc_curve448_modp)
	W64_ENTRY(2, 0)

	push	%rbx
	push	%rbp
	push	%r12

	C First load the values to be shifted by 32.
	mov 88(RP), X1
	mov X1, X0
	mov 96(RP), X2
	mov X1, T0
	mov 104(RP), X3
	mov X2, T1
	mov 56(RP), X4
	mov X3, T2
	mov 64(RP), X5
	mov 72(RP), X6
	mov 80(RP), X7

	C Multiply by 2^32
	shl $32, X0
	shrd $32, X2, X1
	shrd $32, X3, X2
	shrd $32, X4, X3
	shrd $32, X5, X4
	shrd $32, X6, X5
	shrd $32, X7, X6
	shr $32, X7

	C Multiply by 2
	add T0, T0
	adc T1, T1
	adc T2, T2
	adc $0, X7

	C Main additions
	add 56(RP), X0
	adc 64(RP), X1
	adc 72(RP), X2
	adc 80(RP), X3
	adc T0, X4
	adc T1, X5
	adc T2, X6
	adc $0, X7

	add (RP), X0
	adc 8(RP), X1
	adc 16(RP), X2
	adc 24(RP), X3
	adc 32(RP), X4
	adc 40(RP), X5
	adc 48(RP), X6
	adc $0, X7

	C X7 wraparound
	mov X7, T0
	mov X7, T1
	shl $32, T0
	shr $32, T1
	xor T2, T2
	add X7, X0
	adc $0, X1
	adc $0, X2
	adc T0, X3
	adc T1, X4
	adc $0, X5
	adc $0, X6
	adc $0, T2

	C Final carry wraparound. Carry T2 > 0 only if
	C X6 is zero, so carry is absorbed.
	mov T2, T0
	shl $32, T0

	add T2, X0
	mov X0, (RP)
	adc $0, X1
	mov X1, 8(RP)
	adc $0, X2
	mov X2, 16(RP)
	adc T0, X3
	mov X3, 24(RP)
	adc $0, X4
	mov X4, 32(RP)
	adc $0, X5
	mov X5, 40(RP)
	adc $0, X6
	mov X6, 48(RP)

	pop	%r12
	pop	%rbp
	pop	%rbx

	W64_EXIT(2, 0)
	ret
EPILOGUE(_nettle_ecc_curve448_modp)
