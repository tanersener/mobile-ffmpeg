;******************************************************************************
;* blur.asm: SSE2/AVX2 cascade blur
;******************************************************************************
;* Copyright (C) 2015 Vabishchevich Nikolay <vabnick@gmail.com>
;*
;* This file is part of libass.
;*
;* Permission to use, copy, modify, and distribute this software for any
;* purpose with or without fee is hereby granted, provided that the above
;* copyright notice and this permission notice appear in all copies.
;*
;* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
;* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
;* MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
;* ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
;* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
;* ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
;* OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
;******************************************************************************

%include "x86/utils.asm"

SECTION_RODATA 32

words_zero: times 16 dw 0
words_one: times 16 dw 1
words_15_6: times 8 dw 15, 6
words_dither0: times 8 dw  8, 40
words_dither1: times 8 dw 56, 24
words_sign: times 16 dw 0x8000

dwords_two: times 8 dd 2
dwords_32: times 8 dd 32
dwords_round: times 8 dd 0x8000
dwords_lomask: times 8 dd 0xFFFF

SECTION .text

;------------------------------------------------------------------------------
; STRIPE_UNPACK
; void stripe_unpack(int16_t *dst, const uint8_t *src, ptrdiff_t src_stride,
;                    uintptr_t width, uintptr_t height);
;------------------------------------------------------------------------------

%macro STRIPE_UNPACK 0
cglobal stripe_unpack, 5,6,3
    lea r3, [2 * r3 + mmsize - 1]
    and r3, ~(mmsize - 1)
    mov r5, r3
    imul r3, r4
    shr r5, 1
    MUL r4, mmsize
    and r5, ~(mmsize - 1)
    sub r3, r4
    sub r2, r5
    xor r5, r5
    mova m2, [words_one]
    jmp .row_loop

.col_loop:
    mova m1, [r1]
%if mmsize == 32
    vpermq m1, m1, q3120
%endif
    punpcklbw m0, m1, m1
    punpckhbw m1, m1
    psrlw m0, 1
    psrlw m1, 1
    paddw m0, m2
    paddw m1, m2
    psrlw m0, 1
    psrlw m1, 1
    mova [r0 + r5], m0
    add r5, r4
    mova [r0 + r5], m1
    add r5, r4
    add r1, mmsize
.row_loop:
    cmp r5, r3
    jl .col_loop
    sub r5, r4
    cmp r5, r3
    jge .skip_odd

    add r5, r4
    mova m0, [r1]
%if mmsize == 32
    vpermq m0, m0, q3120
%endif
    punpcklbw m0, m0
    psrlw m0, 1
    paddw m0, m2
    psrlw m0, 1
    mova [r0 + r5], m0

.skip_odd:
    add r5, mmsize
    sub r5, r3
    add r1, r2
    cmp r5, r4
    jb .row_loop
    RET
%endmacro

INIT_XMM sse2
STRIPE_UNPACK
INIT_YMM avx2
STRIPE_UNPACK

;------------------------------------------------------------------------------
; STRIPE_PACK
; void stripe_pack(uint8_t *dst, ptrdiff_t dst_stride, const int16_t *src,
;                  uintptr_t width, uintptr_t height);
;------------------------------------------------------------------------------

%macro STRIPE_PACK 0
cglobal stripe_pack, 5,7,5
    lea r3, [2 * r3 + mmsize - 1]
    mov r6, r1
    and r3, ~(mmsize - 1)
    mov r5, mmsize
    imul r3, r4
    imul r6, r4
    add r3, r2
    MUL r4, mmsize
    sub r5, r6
    jmp .row_loop

.col_loop:
    mova m0, [r2]
    mova m2, m0
    psrlw m2, 8
    psubw m0, m2
    mova m1, [r2 + r4]
    mova m2, m1
    psrlw m2, 8
    psubw m1, m2
    paddw m0, m3
    paddw m1, m3
    psrlw m0, 6
    psrlw m1, 6
    packuswb m0, m1
%if mmsize == 32
    vpermq m0, m0, q3120
%endif
    mova [r0], m0
    mova m2, m3
    mova m3, m4
    mova m4, m2
    add r2, mmsize
    add r0, r1
    cmp r2, r6
    jb .col_loop
    add r0, r5
    add r2, r4
.row_loop:
    mova m3, [words_dither0]
    mova m4, [words_dither1]
    lea r6, [r2 + r4]
    cmp r6, r3
    jb .col_loop
    cmp r2, r3
    jb .odd_stripe
    RET

.odd_stripe:
    mova m0, [r2]
    mova m2, m0
    psrlw m2, 8
    psubw m0, m2
    pxor m1, m1
    paddw m0, m3
    psrlw m0, 6
    packuswb m0, m1
%if mmsize == 32
    vpermq m0, m0, q3120
%endif
    mova [r0], m0
    mova m2, m3
    mova m3, m4
    mova m4, m2
    add r2, mmsize
    add r0, r1
    cmp r2, r6
    jb .odd_stripe
    RET
%endmacro

INIT_XMM sse2
STRIPE_PACK
INIT_YMM avx2
STRIPE_PACK

;------------------------------------------------------------------------------
; LOAD_LINE 1:m_dst, 2:base, 3:max, 4:zero_offs,
;           5:offs(lea arg), 6:tmp, [7:left/right]
; LOAD_LINE_COMPACT 1:m_dst, 2:base, 3:max,
;                   4:offs(register), 5:tmp, [6:left/right]
; Load xmm/ymm register with correct source bitmap data
;------------------------------------------------------------------------------

%macro LOAD_LINE 6-7
    lea %6, [%5]
    cmp %6, %3
    cmovae %6, %4
%if (mmsize != 32) || (%0 < 7)
    mova m%1, [%2 + %6]
%elifidn %7, left
    mova xm%1, [%2 + %6]
%elifidn %7, right
    mova xm%1, [%2 + %6 + 16]
%else
    %error "left/right expected"
%endif
%endmacro

%macro LOAD_LINE_COMPACT 5-6
    lea %5, [words_zero]
    sub %5, %2
    cmp %4, %3
    cmovb %5, %4
%if (mmsize != 32) || (%0 < 6)
    mova m%1, [%2 + %5]
%elifidn %6, left
    mova xm%1, [%2 + %5]
%elifidn %6, right
    mova xm%1, [%2 + %5 + 16]
%else
    %error "left/right expected"
%endif
%endmacro

;------------------------------------------------------------------------------
; SHRINK_HORZ
; void shrink_horz(int16_t *dst, const int16_t *src,
;                  uintptr_t src_width, uintptr_t src_height);
;------------------------------------------------------------------------------

%macro SHRINK_HORZ 0
%if ARCH_X86_64
cglobal shrink_horz, 4,9,9
    DECLARE_REG_TMP 8
%else
cglobal shrink_horz, 4,7,8
    DECLARE_REG_TMP 6
%endif
    lea t0, [r2 + mmsize + 3]
    lea r2, [2 * r2 + mmsize - 1]
    and t0, ~(mmsize - 1)
    and r2, ~(mmsize - 1)
    imul t0, r3
    imul r2, r3
    add t0, r0
    xor r4, r4
    MUL r3, mmsize
    sub r4, r3
    mova m7, [dwords_lomask]
%if ARCH_X86_64
    mova m8, [dwords_two]
    lea r7, [words_zero]
    sub r7, r1
%else
    PUSH t0
%endif

    lea r5, [r0 + r3]
.main_loop:
%if ARCH_X86_64
    LOAD_LINE 0, r1,r2,r7, r4 + 0 * r3, r6, right
    LOAD_LINE 1, r1,r2,r7, r4 + 1 * r3, r6
    LOAD_LINE 2, r1,r2,r7, r4 + 2 * r3, r6
%else
    LOAD_LINE_COMPACT 0, r1,r2,r4, r6, right
    add r4, r3
    LOAD_LINE_COMPACT 1, r1,r2,r4, r6
    add r4, r3
    LOAD_LINE_COMPACT 2, r1,r2,r4, r6
    sub r4, r3
    sub r4, r3
%endif

%if mmsize == 32
    vperm2i128 m3, m0, m1, 0x20
    vperm2i128 m4, m1, m2, 0x21
%else
    mova m3, m0
    mova m4, m1
%endif
    psrldq m3, 10
    psrldq m4, 10
    pslldq m6, m1, 6
    por m3, m6
    pslldq m6, m2, 6
    por m4, m6
    paddw m3, m1
    paddw m4, m2
    pand m3, m7
    pand m4, m7

    psrld xm6, xm0, 16
    paddw xm0, xm6
    psrld m6, m1, 16
    paddw m1, m6
    psrld m6, m2, 16
    paddw m2, m6
    pand xm0, xm7
    pand m1, m7
    pand m2, m7

%if mmsize == 32
    vperm2i128 m0, m0, m1, 0x20
%endif
    psrldq m0, 8
    pslldq m6, m1, 8
    por m0, m6
    paddd m5, m0, m1
    psrld m5, 1
    psrldq m0, 4
    pslldq m6, m1, 4
    por m0, m6
    paddd m5, m0
    psrld m5, 1
    paddd m5, m3
    psrld m5, 1
    paddd m0, m5

%if mmsize == 32
    vperm2i128 m1, m1, m2, 0x21
%endif
    psrldq m1, 8
    pslldq m6, m2, 8
    por m1, m6
    paddd m5, m1, m2
    psrld m5, 1
    psrldq m1, 4
    pslldq m6, m2, 4
    por m1, m6
    paddd m5, m1
    psrld m5, 1
    paddd m5, m4
    psrld m5, 1
    paddd m1, m5

%if ARCH_X86_64
    paddd m0, m8
    paddd m1, m8
%else
    mova m6, [dwords_two]
    paddd m0, m6
    paddd m1, m6
%endif
    psrld m0, 2
    psrld m1, 2
    packssdw m0, m1
%if mmsize == 32
    vpermq m0, m0, q3120
%endif

    mova [r0], m0
    add r0, mmsize
    add r4, mmsize
    cmp r0, r5
    jb .main_loop
    add r4, r3
    add r5, r3
%if ARCH_X86_64
    cmp r0, t0
%else
    cmp r0, [rstk]
%endif
    jb .main_loop
%if ARCH_X86_64 == 0
    ADD rstk, 4
%endif
    RET
%endmacro

INIT_XMM sse2
SHRINK_HORZ
INIT_YMM avx2
SHRINK_HORZ

;------------------------------------------------------------------------------
; SHRINK_VERT
; void shrink_vert(int16_t *dst, const int16_t *src,
;                  uintptr_t src_width, uintptr_t src_height);
;------------------------------------------------------------------------------

%macro SHRINK_VERT 0
%if ARCH_X86_64
cglobal shrink_vert, 4,7,9
%else
cglobal shrink_vert, 4,7,8
%endif
    lea r2, [2 * r2 + mmsize - 1]
    lea r5, [r3 + 5]
    and r2, ~(mmsize - 1)
    shr r5, 1
    imul r2, r5
    MUL r3, mmsize
    add r2, r0
    mova m7, [words_one]
%if ARCH_X86_64
    mova m8, [words_sign]
%endif
    lea r6, [words_zero]
    sub r6, r1

.col_loop:
    mov r4, -4 * mmsize
    pxor m0, m0
    pxor m1, m1
    pxor m2, m2
    pxor m3, m3
.row_loop:
    LOAD_LINE 4, r1,r3,r6, r4 + 4 * mmsize, r5
    LOAD_LINE 5, r1,r3,r6, r4 + 5 * mmsize, r5

%if ARCH_X86_64
    mova m6, m8
%else
    psllw m6, m7, 15
%endif
    paddw m1, m4
    paddw m4, m5
    pand m6, m0
    pand m6, m4
    paddw m0, m4
    psrlw m0, 1
    por m0, m6
    pand m6, m2
    paddw m0, m2
    psrlw m0, 1
    por m0, m6
    pand m6, m1
    paddw m0, m1
    psrlw m0, 1
    por m0, m6
    paddw m0, m2
    psrlw m0, 1
    por m0, m6
    paddw m0, m7
    psrlw m0, 1

    mova [r0], m0
    add r4, 2 * mmsize
    add r0, mmsize
    mova m0, m2
    mova m1, m3
    mova m2, m4
    mova m3, m5
    cmp r4, r3
    jl .row_loop
    add r1, r3
    sub r6, r3
    cmp r0, r2
    jb .col_loop
    RET
%endmacro

INIT_XMM sse2
SHRINK_VERT
INIT_YMM avx2
SHRINK_VERT

;------------------------------------------------------------------------------
; EXPAND_HORZ
; void expand_horz(int16_t *dst, const int16_t *src,
;                  uintptr_t src_width, uintptr_t src_height);
;------------------------------------------------------------------------------

%macro EXPAND_HORZ 0
%if ARCH_X86_64
cglobal expand_horz, 4,9,5
    DECLARE_REG_TMP 8
%else
cglobal expand_horz, 4,7,5
    DECLARE_REG_TMP 6
%endif
    lea t0, [4 * r2 + 7]
    lea r2, [2 * r2 + mmsize - 1]
    and t0, ~(mmsize - 1)
    and r2, ~(mmsize - 1)
    imul t0, r3
    imul r2, r3
    add t0, r0
    xor r4, r4
    MUL r3, mmsize
    sub r4, r3
    mova m4, [words_one]
%if ARCH_X86_64
    lea r7, [words_zero]
    sub r7, r1
%endif

    lea r5, [r0 + r3]
    cmp r0, t0
    jae .odd_stripe
%if ARCH_X86_64 == 0
    PUSH t0
%endif
.main_loop:
%if ARCH_X86_64
    LOAD_LINE 0, r1,r2,r7, r4 + 0 * r3, r6, right
    LOAD_LINE 1, r1,r2,r7, r4 + 1 * r3, r6
%else
    LOAD_LINE_COMPACT 0, r1,r2,r4, r6, right
    add r4, r3
    LOAD_LINE_COMPACT 1, r1,r2,r4, r6
    sub r4, r3
%endif

%if mmsize == 32
    vperm2i128 m0, m0, m1, 0x20
%endif
    psrldq m0, 12
    pslldq m3, m1, 4
    por m0, m3
    psrldq m2, m0, 2
    pslldq m3, m1, 2
    por m2, m3

    paddw m3, m0, m1
    psrlw m3, 1
    paddw m3, m2
    psrlw m3, 1
    paddw m0, m3
    paddw m1, m3
    psrlw m0, 1
    psrlw m1, 1
    paddw m0, m2
    paddw m1, m2
    paddw m0, m4
    paddw m1, m4
    psrlw m0, 1
    psrlw m1, 1

%if mmsize == 32
    vpermq m0, m0, q3120
    vpermq m1, m1, q3120
%endif
    punpcklwd m2, m0, m1
    punpckhwd m0, m1
    mova [r0], m2
    mova [r0 + r3], m0
    add r0, mmsize
    add r4, mmsize
    cmp r0, r5
    jb .main_loop
    add r0, r3
    lea r5, [r0 + r3]
%if ARCH_X86_64 == 0
    mov t0, [rstk]
%endif
    cmp r0, t0
    jb .main_loop
    add t0, r3
%if ARCH_X86_64 == 0
    ADD rstk, 4
%endif
    cmp r0, t0
    jb .odd_stripe
    RET

.odd_stripe:
%if ARCH_X86_64
    LOAD_LINE 0, r1,r2,r7, r4 + 0 * r3, r6, right
    LOAD_LINE 1, r1,r2,r7, r4 + 1 * r3, r6, left
%else
    LOAD_LINE_COMPACT 0, r1,r2,r4, r6, right
    add r4, r3
    LOAD_LINE_COMPACT 1, r1,r2,r4, r6, left
    sub r4, r3
%endif

    psrldq xm0, 12
    pslldq xm3, xm1, 4
    por xm0, xm3
    psrldq xm2, xm0, 2
    pslldq xm3, xm1, 2
    por xm2, xm3

    paddw xm3, xm0, xm1
    psrlw xm3, 1
    paddw xm3, xm2
    psrlw xm3, 1
    paddw xm0, xm3
    paddw xm1, xm3
    psrlw xm0, 1
    psrlw xm1, 1
    paddw xm0, xm2
    paddw xm1, xm2
    paddw xm0, xm4
    paddw xm1, xm4
    psrlw xm0, 1
    psrlw xm1, 1

%if mmsize == 32
    vpermq m0, m0, q3120
    vpermq m1, m1, q3120
%endif
    punpcklwd m0, m1
    mova [r0], m0
    add r0, mmsize
    add r4, mmsize
    cmp r0, r5
    jb .odd_stripe
    RET
%endmacro

INIT_XMM sse2
EXPAND_HORZ
INIT_YMM avx2
EXPAND_HORZ

;------------------------------------------------------------------------------
; EXPAND_VERT
; void expand_vert(int16_t *dst, const int16_t *src,
;                  uintptr_t src_width, uintptr_t src_height);
;------------------------------------------------------------------------------

%macro EXPAND_VERT 0
cglobal expand_vert, 4,7,5
    lea r2, [2 * r2 + mmsize - 1]
    lea r5, [2 * r3 + 4]
    and r2, ~(mmsize - 1)
    imul r2, r5
    MUL r3, mmsize
    add r2, r0
    mova m4, [words_one]
    lea r6, [words_zero]
    sub r6, r1

.col_loop:
    mov r4, -2 * mmsize
    pxor m0, m0
    pxor m1, m1
.row_loop:
    LOAD_LINE 2, r1,r3,r6, r4 + 2 * mmsize, r5

    paddw m3, m0, m2
    psrlw m3, 1
    paddw m3, m1
    psrlw m3, 1
    paddw m0, m3
    paddw m3, m2
    psrlw m0, 1
    psrlw m3, 1
    paddw m0, m1
    paddw m3, m1
    paddw m0, m4
    paddw m3, m4
    psrlw m0, 1
    psrlw m3, 1

    mova [r0], m0
    mova [r0 + mmsize], m3
    add r4, mmsize
    add r0, 2 * mmsize
    mova m0, m1
    mova m1, m2
    cmp r4, r3
    jl .row_loop
    add r1, r3
    sub r6, r3
    cmp r0, r2
    jb .col_loop
    RET
%endmacro

INIT_XMM sse2
EXPAND_VERT
INIT_YMM avx2
EXPAND_VERT

;------------------------------------------------------------------------------
; PRE_BLUR1_HORZ
; void pre_blur1_horz(int16_t *dst, const int16_t *src,
;                     uintptr_t src_width, uintptr_t src_height);
;------------------------------------------------------------------------------

%macro PRE_BLUR1_HORZ 0
%if ARCH_X86_64
cglobal pre_blur1_horz, 4,8,4
%else
cglobal pre_blur1_horz, 4,7,4
%endif
    lea r5, [2 * r2 + mmsize + 3]
    lea r2, [2 * r2 + mmsize - 1]
    and r5, ~(mmsize - 1)
    and r2, ~(mmsize - 1)
    imul r5, r3
    imul r2, r3
    add r5, r0
    xor r4, r4
    MUL r3, mmsize
    sub r4, r3
    mova m3, [words_one]
%if ARCH_X86_64
    lea r7, [words_zero]
    sub r7, r1
%endif

.main_loop:
%if ARCH_X86_64
    LOAD_LINE 0, r1,r2,r7, r4 + 0 * r3, r6, right
    LOAD_LINE 1, r1,r2,r7, r4 + 1 * r3, r6
%else
    LOAD_LINE_COMPACT 0, r1,r2,r4, r6, right
    add r4, r3
    LOAD_LINE_COMPACT 1, r1,r2,r4, r6
    sub r4, r3
%endif

%if mmsize == 32
    vperm2i128 m0, m0, m1, 0x20
%endif
    psrldq m0, 12
    pslldq m2, m1, 4
    por m0, m2
    psrldq m2, m0, 2
    paddw m0, m1
    pslldq m1, 2
    psrlw m0, 1
    por m1, m2
    paddw m0, m1
    paddw m0, m3
    psrlw m0, 1

    mova [r0], m0
    add r0, mmsize
    add r4, mmsize
    cmp r0, r5
    jb .main_loop
    RET
%endmacro

INIT_XMM sse2
PRE_BLUR1_HORZ
INIT_YMM avx2
PRE_BLUR1_HORZ

;------------------------------------------------------------------------------
; PRE_BLUR1_VERT
; void pre_blur1_vert(int16_t *dst, const int16_t *src,
;                     uintptr_t src_width, uintptr_t src_height);
;------------------------------------------------------------------------------

%macro PRE_BLUR1_VERT 0
cglobal pre_blur1_vert, 4,7,4
    lea r2, [2 * r2 + mmsize - 1]
    lea r5, [r3 + 2]
    and r2, ~(mmsize - 1)
    imul r2, r5
    MUL r3, mmsize
    add r2, r0
    mova m3, [words_one]
    lea r6, [words_zero]
    sub r6, r1

.col_loop:
    mov r4, -2 * mmsize
    pxor m0, m0
    pxor m1, m1
.row_loop:
    LOAD_LINE 2, r1,r3,r6, r4 + 2 * mmsize, r5

    paddw m0, m2
    psrlw m0, 1
    paddw m0, m1
    paddw m0, m3
    psrlw m0, 1

    mova [r0], m0
    add r4, mmsize
    add r0, mmsize
    mova m0, m1
    mova m1, m2
    cmp r4, r3
    jl .row_loop
    add r1, r3
    sub r6, r3
    cmp r0, r2
    jb .col_loop
    RET
%endmacro

INIT_XMM sse2
PRE_BLUR1_VERT
INIT_YMM avx2
PRE_BLUR1_VERT

;------------------------------------------------------------------------------
; PRE_BLUR2_HORZ
; void pre_blur2_horz(int16_t *dst, const int16_t *src,
;                     uintptr_t src_width, uintptr_t src_height);
;------------------------------------------------------------------------------

%macro PRE_BLUR2_HORZ 0
%if ARCH_X86_64
cglobal pre_blur2_horz, 4,8,7
%else
cglobal pre_blur2_horz, 4,7,7
%endif
    lea r5, [2 * r2 + mmsize + 7]
    lea r2, [2 * r2 + mmsize - 1]
    and r5, ~(mmsize - 1)
    and r2, ~(mmsize - 1)
    imul r5, r3
    imul r2, r3
    add r5, r0
    xor r4, r4
    MUL r3, mmsize
    sub r4, r3
    mova m5, [words_one]
    mova m6, [words_sign]
%if ARCH_X86_64
    lea r7, [words_zero]
    sub r7, r1
%endif

.main_loop:
%if ARCH_X86_64
    LOAD_LINE 0, r1,r2,r7, r4 + 0 * r3, r6, right
    LOAD_LINE 1, r1,r2,r7, r4 + 1 * r3, r6
%else
    LOAD_LINE_COMPACT 0, r1,r2,r4, r6, right
    add r4, r3
    LOAD_LINE_COMPACT 1, r1,r2,r4, r6
    sub r4, r3
%endif

%if mmsize == 32
    vperm2i128 m0, m0, m1, 0x20
%endif
    psrldq m0, 8
    pslldq m2, m1, 8
    por m2, m0
    paddw m2, m1
    psrlw m2, 1
    psrldq m0, 2
    pslldq m3, m1, 6
    por m3, m0
    psrldq m0, 2
    pslldq m4, m1, 4
    por m4, m0
    paddw m2, m4
    psrlw m2, 1
    paddw m2, m4
    psrldq m0, 2
    pslldq m1, 2
    por m0, m1
    paddw m0, m3
    mova m1, m6
    pand m1, m0
    pand m1, m2
    paddw m0, m2
    psrlw m0, 1
    por m0, m1
    paddw m0, m5
    psrlw m0, 1

    mova [r0], m0
    add r0, mmsize
    add r4, mmsize
    cmp r0, r5
    jb .main_loop
    RET
%endmacro

INIT_XMM sse2
PRE_BLUR2_HORZ
INIT_YMM avx2
PRE_BLUR2_HORZ

;------------------------------------------------------------------------------
; PRE_BLUR2_VERT
; void pre_blur2_vert(int16_t *dst, const int16_t *src,
;                     uintptr_t src_width, uintptr_t src_height);
;------------------------------------------------------------------------------

%macro PRE_BLUR2_VERT 0
%if ARCH_X86_64
cglobal pre_blur2_vert, 4,7,9
%else
cglobal pre_blur2_vert, 4,7,8
%endif
    lea r2, [2 * r2 + mmsize - 1]
    lea r5, [r3 + 4]
    and r2, ~(mmsize - 1)
    imul r2, r5
    MUL r3, mmsize
    add r2, r0
    mova m7, [words_one]
%if ARCH_X86_64
    mova m8, [words_sign]
%endif
    lea r6, [words_zero]
    sub r6, r1

.col_loop:
    mov r4, -4 * mmsize
    pxor m0, m0
    pxor m1, m1
    pxor m2, m2
    pxor m3, m3
.row_loop:
    LOAD_LINE 4, r1,r3,r6, r4 + 4 * mmsize, r5

%if ARCH_X86_64
    mova m6, m8
%else
    psllw m6, m7, 15
%endif
    paddw m0, m4
    psrlw m0, 1
    paddw m0, m2
    psrlw m0, 1
    paddw m0, m2
    paddw m5, m1, m3
    pand m6, m0
    pand m6, m5
    paddw m0, m5
    psrlw m0, 1
    por m0, m6
    paddw m0, m7
    psrlw m0, 1

    mova [r0], m0
    add r4, mmsize
    add r0, mmsize
    mova m0, m1
    mova m1, m2
    mova m2, m3
    mova m3, m4
    cmp r4, r3
    jl .row_loop
    add r1, r3
    sub r6, r3
    cmp r0, r2
    jb .col_loop
    RET
%endmacro

INIT_XMM sse2
PRE_BLUR2_VERT
INIT_YMM avx2
PRE_BLUR2_VERT

;------------------------------------------------------------------------------
; ADD_LINE 1:m_acc1, 2:m_acc2, 3:m_line, 4-5:m_tmp
; Calculate acc += line
;------------------------------------------------------------------------------

%macro ADD_LINE 5
    psraw m%4, m%3, 15
    punpcklwd m%5, m%3, m%4
    punpckhwd m%3, m%4
%ifidn %1, %5
    paddd m%1, m%2
%else
    paddd m%1, m%5
%endif
    paddd m%2, m%3
%endmacro

;------------------------------------------------------------------------------
; FILTER_PAIR 1:m_acc1, 2:m_acc2, 3:m_line1, 4:m_line2,
;             5:m_tmp, 6:m_mul64, [7:m_mul32, 8:swizzle]
; Calculate acc += line1 * mul[odd] + line2 * mul[even]
;------------------------------------------------------------------------------

%macro FILTER_PAIR 6-8
    punpcklwd m%5, m%4, m%3
    punpckhwd m%4, m%3
%if ARCH_X86_64 || (%0 < 8)
    pmaddwd m%5, m%6
    pmaddwd m%4, m%6
%else
    pshufd m%3, m%7, %8
    pmaddwd m%5, m%3
    pmaddwd m%4, m%3
%endif
%ifidn %1, %5
    paddd m%1, m%2
%else
    paddd m%1, m%5
%endif
    paddd m%2, m%4
%endmacro

;------------------------------------------------------------------------------
; PRE_BLUR3_HORZ
; void pre_blur3_horz(int16_t *dst, const int16_t *src,
;                     uintptr_t src_width, uintptr_t src_height);
;------------------------------------------------------------------------------

%macro PRE_BLUR3_HORZ 0
%if ARCH_X86_64
cglobal pre_blur3_horz, 4,8,9
%else
cglobal pre_blur3_horz, 4,7,8
%endif
    lea r5, [2 * r2 + mmsize + 11]
    lea r2, [2 * r2 + mmsize - 1]
    and r5, ~(mmsize - 1)
    and r2, ~(mmsize - 1)
    imul r5, r3
    imul r2, r3
    add r5, r0
    xor r4, r4
    MUL r3, mmsize
    sub r4, r3
    mova m5, [words_15_6]
%if ARCH_X86_64
    mova m8, [dwords_32]
    lea r7, [words_zero]
    sub r7, r1
%endif

.main_loop:
%if ARCH_X86_64
    LOAD_LINE 0, r1,r2,r7, r4 + 0 * r3, r6, right
    LOAD_LINE 1, r1,r2,r7, r4 + 1 * r3, r6
%else
    LOAD_LINE_COMPACT 0, r1,r2,r4, r6, right
    add r4, r3
    LOAD_LINE_COMPACT 1, r1,r2,r4, r6
    sub r4, r3
%endif

%if ARCH_X86_64
    mova m7, m8
%else
    mova m7, [dwords_32]
%endif
%if mmsize == 32
    vperm2i128 m0, m0, m1, 0x20
%endif
    psrldq m2, m0, 10
    pslldq m3, m1, 6
    por m2, m3

    psrldq m0, 4
    pslldq m3, m2, 6
    por m3, m0
    psubw m3, m2
    ADD_LINE 6,7, 3,4, 6

    psrldq m0, 2
    pslldq m3, m2, 4
    por m3, m0
    psubw m3, m2
    psrldq m0, 2
    pslldq m4, m2, 2
    por m4, m0
    psubw m4, m2
    FILTER_PAIR 6,7, 3,4, 0, 5

    psubw m3, m1, m2
    ADD_LINE 6,7, 3,4, 0

    pslldq m1, 2
    psrldq m3, m2, 4
    por m3, m1
    psubw m3, m2
    pslldq m1, 2
    psrldq m4, m2, 2
    por m4, m1
    psubw m4, m2
    FILTER_PAIR 6,7, 3,4, 0, 5

    psrad m6, 6
    psrad m7, 6
    packssdw m6, m7
    paddw m2, m6
    mova [r0], m2
    add r0, mmsize
    add r4, mmsize
    cmp r0, r5
    jb .main_loop
    RET
%endmacro

INIT_XMM sse2
PRE_BLUR3_HORZ
INIT_YMM avx2
PRE_BLUR3_HORZ

;------------------------------------------------------------------------------
; PRE_BLUR3_VERT
; void pre_blur3_vert(int16_t *dst, const int16_t *src,
;                     uintptr_t src_width, uintptr_t src_height);
;------------------------------------------------------------------------------

%macro PRE_BLUR3_VERT 0
%if ARCH_X86_64
cglobal pre_blur3_vert, 4,7,8
%else
cglobal pre_blur3_vert, 4,7,8
%endif
    lea r2, [2 * r2 + mmsize - 1]
    lea r5, [r3 + 6]
    and r2, ~(mmsize - 1)
    imul r2, r5
    MUL r3, mmsize
    add r2, r0
    mova m4, [dwords_32]
    mova m5, [words_15_6]
    lea r6, [words_zero]
    sub r6, r1

.col_loop:
    mov r4, -6 * mmsize
.row_loop:
    mova m6, m4
    mova m7, m4
    LOAD_LINE 0, r1,r3,r6, r4 + 3 * mmsize, r5

    LOAD_LINE 1, r1,r3,r6, r4 + 0 * mmsize, r5
    psubw m1, m0
    ADD_LINE 6,7, 1,2, 3

    LOAD_LINE 1, r1,r3,r6, r4 + 1 * mmsize, r5
    LOAD_LINE 2, r1,r3,r6, r4 + 2 * mmsize, r5
    psubw m1, m0
    psubw m2, m0
    FILTER_PAIR 6,7, 1,2, 3, 5

    LOAD_LINE 1, r1,r3,r6, r4 + 6 * mmsize, r5
    psubw m1, m0
    ADD_LINE 6,7, 1,2, 3

    LOAD_LINE 1, r1,r3,r6, r4 + 5 * mmsize, r5
    LOAD_LINE 2, r1,r3,r6, r4 + 4 * mmsize, r5
    psubw m1, m0
    psubw m2, m0
    FILTER_PAIR 6,7, 1,2, 3, 5

    psrad m6, 6
    psrad m7, 6
    packssdw m6, m7
    paddw m0, m6
    mova [r0], m0
    add r4, mmsize
    add r0, mmsize
    cmp r4, r3
    jl .row_loop
    add r1, r3
    sub r6, r3
    cmp r0, r2
    jb .col_loop
    RET
%endmacro

INIT_XMM sse2
PRE_BLUR3_VERT
INIT_YMM avx2
PRE_BLUR3_VERT

;------------------------------------------------------------------------------
; LOAD_MULTIPLIER 1:m_mul1, 2:m_mul2, 3:src, 4:tmp
; Load blur parameters into xmm/ymm registers
;------------------------------------------------------------------------------

%macro LOAD_MULTIPLIER 4
    mov %4, [%3]
    movd xm%1, %4d
%if ARCH_X86_64
    shr %4, 32
%else
    mov %4, [%3 + 4]
%endif
    movd xm%2, %4d
%if ARCH_X86_64 == 0
    punpckldq xm%1, xm%2
%if mmsize == 32
    vpbroadcastq m%1, xm%1
%endif
%elif mmsize == 32
    vpbroadcastd m%1, xm%1
    vpbroadcastd m%2, xm%2
%else
    pshufd m%1, m%1, q0000
    pshufd m%2, m%2, q0000
%endif
%endmacro

;------------------------------------------------------------------------------
; BLUR_HORZ 1:pattern
; void blurNNNN_horz(int16_t *dst, const int16_t *src,
;                    uintptr_t src_width, uintptr_t src_height,
;                    const int16_t *param);
;------------------------------------------------------------------------------

%macro BLUR_HORZ 1
    %assign %%i1 %1 / 1000 % 10
    %assign %%i2 %1 / 100 % 10
    %assign %%i3 %1 / 10 % 10
    %assign %%i4 %1 / 1 % 10
%if ARCH_X86_64
cglobal blur%1_horz, 5,8,10
%else
cglobal blur%1_horz, 5,7,8
%endif
%if ARCH_X86_64
    LOAD_MULTIPLIER 8,9, r4, r5
%else
    LOAD_MULTIPLIER 5,0, r4, r5
%endif
    lea r5, [2 * r2 + mmsize + 4 * %%i4 - 1]
    lea r2, [2 * r2 + mmsize - 1]
    and r5, ~(mmsize - 1)
    and r2, ~(mmsize - 1)
    imul r5, r3
    imul r2, r3
    add r5, r0
    xor r4, r4
    MUL r3, mmsize
%if (mmsize != 32) && (%%i4 > 4)
    sub r4, r3
%endif
    sub r4, r3
%if ARCH_X86_64
    mova m5, [dwords_round]
    lea r7, [words_zero]
    sub r7, r1
%endif

.main_loop:
%if ARCH_X86_64
%if %%i4 > 4
    LOAD_LINE 0, r1,r2,r7, r4 + 0 * r3, r6
%else
    LOAD_LINE 0, r1,r2,r7, r4 + 0 * r3, r6, right
%endif
    LOAD_LINE 1, r1,r2,r7, r4 + 1 * r3, r6
%if (mmsize != 32) && (%%i4 > 4)
    LOAD_LINE 2, r1,r2,r7, r4 + 2 * r3, r6
    SWAP 1, 2
%endif
%else
%if %%i4 > 4
    LOAD_LINE_COMPACT 0, r1,r2,r4, r6
%else
    LOAD_LINE_COMPACT 0, r1,r2,r4, r6, right
%endif
    add r4, r3
    LOAD_LINE_COMPACT 1, r1,r2,r4, r6
%if (mmsize != 32) && (%%i4 > 4)
    add r4, r3
    LOAD_LINE_COMPACT 2, r1,r2,r4, r6
    SWAP 1, 2
    sub r4, r3
%endif
    sub r4, r3
%endif

%if ARCH_X86_64
    mova m7, m5
%else
    mova m7, [dwords_round]
%endif
%if %%i4 > 4
%if mmsize == 32
    vperm2i128 m2, m0, m1, 0x21
%endif
    psrldq m0, 32 - 4 * %%i4
    pslldq m3, m2, 4 * %%i4 - 16
    por m0, m3
    psrldq m2, 16 - 2 * %%i4
%else
%if mmsize == 32
    vperm2i128 m0, m0, m1, 0x20
%endif
    psrldq m2, m0, 16 - 2 * %%i4
%endif
    pslldq m3, m1, 2 * %%i4
    por m2, m3

    psubw m3, m1, m2
    pslldq m1, 2 * (%%i4 - %%i3)
    psrldq m4, m2, 2 * %%i3
    por m4, m1
    psubw m4, m2
    FILTER_PAIR 6,7, 3,4, 6, 9,5,q1111

    pslldq m1, 2 * (%%i3 - %%i2)
    psrldq m3, m2, 2 * %%i2
    por m3, m1
    psubw m3, m2
    pslldq m1, 2 * (%%i2 - %%i1)
    psrldq m4, m2, 2 * %%i1
    por m4, m1
    psubw m4, m2
    FILTER_PAIR 6,7, 3,4, 1, 8,5,q0000

    psubw m3, m0, m2
    psrldq m0, 2 * (%%i4 - %%i3)
    pslldq m4, m2, 2 * %%i3
    por m4, m0
    psubw m4, m2
    FILTER_PAIR 6,7, 3,4, 1, 9,5,q1111

    psrldq m0, 2 * (%%i3 - %%i2)
    pslldq m3, m2, 2 * %%i2
    por m3, m0
    psubw m3, m2
    psrldq m0, 2 * (%%i2 - %%i1)
    pslldq m4, m2, 2 * %%i1
    por m4, m0
    psubw m4, m2
    FILTER_PAIR 6,7, 3,4, 1, 8,5,q0000

    psrad m6, 16
    psrad m7, 16
    packssdw m6, m7
    paddw m2, m6
    mova [r0], m2
    add r0, mmsize
    add r4, mmsize
    cmp r0, r5
    jb .main_loop
    RET
%endmacro

INIT_XMM sse2
BLUR_HORZ 1234
BLUR_HORZ 1235
BLUR_HORZ 1246
INIT_YMM avx2
BLUR_HORZ 1234
BLUR_HORZ 1235
BLUR_HORZ 1246

;------------------------------------------------------------------------------
; BLUR_VERT 1:pattern
; void blurNNNN_vert(int16_t *dst, const int16_t *src,
;                    uintptr_t src_width, uintptr_t src_height,
;                    const int16_t *param);
;------------------------------------------------------------------------------

%macro BLUR_VERT 1
    %assign %%i1 %1 / 1000 % 10
    %assign %%i2 %1 / 100 % 10
    %assign %%i3 %1 / 10 % 10
    %assign %%i4 %1 / 1 % 10
%if ARCH_X86_64
cglobal blur%1_vert, 5,7,9
%else
cglobal blur%1_vert, 5,7,8
%endif
%if ARCH_X86_64
    LOAD_MULTIPLIER 4,5, r4, r5
%else
    LOAD_MULTIPLIER 5,0, r4, r5
    SWAP 4, 8
%endif
    lea r2, [2 * r2 + mmsize - 1]
    lea r5, [r3 + 2 * %%i4]
    and r2, ~(mmsize - 1)
    imul r2, r5
    MUL r3, mmsize
    add r2, r0
    mova m8, [dwords_round]
    lea r6, [words_zero]
    sub r6, r1

.col_loop:
    mov r4, -2 * %%i4 * mmsize
.row_loop:
    mova m6, m8
    mova m7, m8
    LOAD_LINE 0, r1,r3,r6, r4 + %%i4 * mmsize, r5

    LOAD_LINE 1, r1,r3,r6, r4 + (%%i4 - %%i4) * mmsize, r5
    LOAD_LINE 2, r1,r3,r6, r4 + (%%i4 - %%i3) * mmsize, r5
    psubw m1, m0
    psubw m2, m0
    FILTER_PAIR 6,7, 1,2, 3, 5,5,q1111

    LOAD_LINE 1, r1,r3,r6, r4 + (%%i4 - %%i2) * mmsize, r5
    LOAD_LINE 2, r1,r3,r6, r4 + (%%i4 - %%i1) * mmsize, r5
    psubw m1, m0
    psubw m2, m0
    FILTER_PAIR 6,7, 1,2, 3, 4,5,q0000

    LOAD_LINE 1, r1,r3,r6, r4 + (%%i4 + %%i4) * mmsize, r5
    LOAD_LINE 2, r1,r3,r6, r4 + (%%i4 + %%i3) * mmsize, r5
    psubw m1, m0
    psubw m2, m0
    FILTER_PAIR 6,7, 1,2, 3, 5,5,q1111

    LOAD_LINE 1, r1,r3,r6, r4 + (%%i4 + %%i2) * mmsize, r5
    LOAD_LINE 2, r1,r3,r6, r4 + (%%i4 + %%i1) * mmsize, r5
    psubw m1, m0
    psubw m2, m0
    FILTER_PAIR 6,7, 1,2, 3, 4,5,q0000

    psrad m6, 16
    psrad m7, 16
    packssdw m6, m7
    paddw m0, m6
    mova [r0], m0
    add r4, mmsize
    add r0, mmsize
    cmp r4, r3
    jl .row_loop
    add r1, r3
    sub r6, r3
    cmp r0, r2
    jb .col_loop
    RET
%endmacro

INIT_XMM sse2
BLUR_VERT 1234
BLUR_VERT 1235
BLUR_VERT 1246
INIT_YMM avx2
BLUR_VERT 1234
BLUR_VERT 1235
BLUR_VERT 1246
