;******************************************************************************
;* utils.asm: helper macros
;******************************************************************************
;* Copyright (C) 2014 Vabishchevich Nikolay <vabnick@gmail.com>
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

%include "x86/x86inc.asm"

;------------------------------------------------------------------------------
; MUL 1:reg, 2:num
; Multiply by constant
;------------------------------------------------------------------------------

%macro MUL 2
%if (%2) == 0
    xor %1, %1
%elif (%2) == 1
%elif (%2) == 2
    add %1, %1  ; lea %1, [%1 + %1]
%elif (%2) == 3
    lea %1, [%1 + 2 * %1]
%elif (%2) == 4
    lea %1, [4 * %1]  ; shl %1, 2
%elif (%2) == 5
    lea %1, [%1 + 4 * %1]
%elif (%2) == 8
    lea %1, [8 * %1]  ; shl %1, 3
%elif (%2) == 9
    lea %1, [%1 + 8 * %1]
%elif (%2) == 16
    shl %1, 4
%elif (%2) == 32
    shl %1, 5
%elif (%2) == 64
    shl %1, 6
%elif (%2) == 128
    shl %1, 7
%elif (%2) == 256
    shl %1, 8
%else
    imul %1, %2
%endif
%endmacro

;------------------------------------------------------------------------------
; BCASTW 1:m_dst, 2:r_src
;------------------------------------------------------------------------------

%macro BCASTW 2
    movd xm%1, %2
%if mmsize == 32
    vpbroadcastw m%1, xm%1
%elif mmsize == 16
    punpcklwd m%1, m%1
    pshufd m%1, m%1, q0000
%endif
%endmacro

;------------------------------------------------------------------------------
; PABSW 1:m_reg, 2:m_tmp
;------------------------------------------------------------------------------

%macro PABSW 2
%if cpuflag(ssse3)
    pabsw m%1, m%1
%else
    pxor m%2, m%2
    psubw m%2, m%1
    pmaxsw m%1, m%2
%endif
%endmacro
