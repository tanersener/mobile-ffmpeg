;******************************************************************************
;* be_blur.asm: SSE2 \be blur
;******************************************************************************
;* Copyright (C) 2013 Rodger Combs <rcombs@rcombs.me>
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

SECTION_RODATA 32
low_word_zero: dd 0xFFFF0000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF

SECTION .text

;------------------------------------------------------------------------------
; void be_blur_pass( uint8_t *buf, unsigned width,
;                    unsigned height, unsigned stride,
;                    uint16_t *tmp);
;------------------------------------------------------------------------------

INIT_XMM sse2
cglobal be_blur, 5,15,9
.skip_prologue:
    mov r6, 2 ; int x = 2;
    pxor xmm6, xmm6 ; __m128i temp3 = 0;
    mov r7, r0 ; unsigned char *src=buf;
    movzx r8, byte [r7 + 1] ; int old_pix = src[1];
    movzx r9, byte [r7] ; int old_sum = src[0];
    add r9, r8 ; old_sum += old_pix;
    lea r12, [r4 + r3 * 2] ; unsigned char *col_sum_buf = tmp + stride * 2;
    lea r14, [r1 - 2] ; tmpreg = (w-2);
    and r14, -8 ; tmpreg &= (~7);
.first_loop:
    movzx r10, byte [r7 + r6] ; int temp1 = src[x];
    lea r11, [r8 + r10] ; int temp2 = old_pix + temp1;
    mov r8, r10 ; old_pix = temp1;
    lea r10, [r9 + r11] ; temp1 = old_sum + temp2;
    mov r9, r11 ; old_sum = temp2;
    mov word [r4 + r6 * 2], r10w ; col_pix_buf[x] = temp1;
    inc r6 ; x++
    cmp r6, r1 ; x < w
    jl .first_loop
    mov r6, 2 ; int x = 2;
    lea r7, [r0 + r3] ; unsigned char *src=buf+stride;
    movzx r8, byte [r7 + 1] ; int old_pix = src[1];
    movzx r9, byte [r7] ; int old_sum = src[0];
    add r9, r8 ; old_sum += old_pix
.second_loop:
    movzx r10, byte [r7 + r6] ; int temp1 = src[x];
    lea r11, [r8 + r10] ; int temp2 = old_pix + temp1;
    mov r8, r10 ; old_pix = temp1;
    lea r10, [r9 + r11] ; temp1 = old_sum + temp2;
    mov r9, r11 ; old_sum = temp2;
    movzx r11, word [r4 + r6 * 2] ; temp2 = col_pix_buf[x];
    add r11, r10 ; temp2 += temp1;
    mov word [r4 + r6 * 2], r10w ; col_pix_buf[x] = temp1;
    mov word [r12 + r6 * 2], r11w ; col_sum_buf[x] = temp2;
    inc r6 ; x++
    cmp r6, r1 ; x < w
    jl .second_loop
    mov r5, 2 ; int y = 2;
.height_loop:
    mov r10, r5; int tmpreg = y;
    imul r10, r3; tmpreg *= stride;
    lea r7, [r0 + r10] ; unsigned char *src=buf+y*stride;
    sub r10, r3 ; tmpreg -= stride;
    lea r13, [r0 + r10]; unsigned char *dst=buf+(y-1)*stride;
    mov r6, 2 ; int x = 2;
    movzx r10, byte [r7] ; temp1 = src[0];
    movzx r11, byte [r7 + 1] ; temp2 = src[1];
    add r10, r11; temp1 += temp2
    movd xm0, r10d; __m128i old_pix_128 = temp2;
    movd xm1, r11d; __m128i old_sum_128 = temp1;
.width_loop:
    movq xmm2, [r7 + r6]; __m128i new_pix = (src+x);
    punpcklbw xmm2, xmm6 ; new_pix = _mm_unpacklo_epi8(new_pix, temp3);
    movdqa xmm3, xmm2 ; __m128i temp = new_pix;
    pslldq xmm3, 2 ; temp = temp << 2 * 8;
    paddw xmm3, xmm0 ; temp = _mm_add_epi16(temp, old_pix_128);
    paddw xmm3, xmm2 ; temp = _mm_add_epi16(temp, new_pix);
    movdqa xmm0, xmm2 ; old_pix_128 = new_pix;
    psrldq xmm0, 14 ; old_pix_128 = old_pix_128 >> 14 * 8;
    movdqa xmm2, xmm3 ; new_pix = temp;
    pslldq xmm2, 2 ; new_pix = new_pix << 2 * 8;
    paddw xmm2, xmm1 ; new_pix = _mm_add_epi16(new_pix, old_sum_128);
    paddw xmm2, xmm3 ; new_pix = _mm_add_epi16(new_pix, temp);
    movdqa xmm1, xmm3 ; old_sum_128 = temp;
    psrldq xmm1, 14 ; old_sum_128 = old_sum_128 >> 14 * 8;
    movdqu xmm4, [r4 + r6 * 2] ; __m128i old_col_pix = *(col_pix_buf+x);
    movdqu [r4 + r6 * 2], xmm2 ; *(col_pix_buf+x) = new_pix ;
    movdqu xmm5, [r12 + r6 * 2] ; __m128i old_col_sum = *(col_pix_sum+x);
    movdqa xmm3, xmm2 ; temp = new_pix;
    paddw xmm3, xmm4 ; temp = _mm_add_epi16(temp, old_col_pix);
    movdqu [r12 + r6 * 2], xmm3 ; *(col_sum_buf+x) = temp;
    paddw xmm5, xmm3 ; old_col_sum = _mm_add_epi16(old_col_sum, temp);
    psrlw xmm5, 4 ; old_col_sum = old_col_sum >> 4;
    packuswb xmm5, xmm5 ; old_col_sum = _mm_packus_epi16(old_col_sum, old_col_sum);
    movq qword [r13 + r6 - 1], xmm5 ; *(dst+x-1) = old_col_sum;
    add r6, 8; x += 8;
    cmp r6, r14; x < ((w - 2) & (~7));
    jl .width_loop
    movzx r8, byte [r7 + r6 - 1] ; old_pix = src[x-1];
    movzx r9, byte [r7 + r6 - 2] ; old_sum = old_pix + src[x-2];
    add r9, r8
    jmp .final_width_check
.final_width_loop:
    movzx r10, byte [r7 + r6] ; temp1 = src[x];
    lea r11, [r8 + r10] ; temp2 = old_pix + temp1;
    mov r8, r10 ; old_pix = temp1;
    lea r10, [r9 + r11] ; temp1 = old_sum + temp2;
    mov r9, r11 ; old_sum = temp2;
    movzx r11, word [r4 + r6 * 2] ; temp2 = col_pix_buf[x];
    add r11, r10 ; temp2 += temp1;
    mov word [r4 + r6 * 2], r10w ; col_pix_buf[x] = temp1;
    movzx r10, word [r12 + r6 * 2] ; temp1 = col_sum_buf[x];
    add r10, r11 ; temp1 += temp2;
    shr r10, 4 ; temp1 >>= 4;
    mov byte [r13 + r6 - 1], r10b ; dst[x-1] = temp1
    mov [r12 + r6 * 2], r11w ; col_sum_buf[x] = temp2;
    inc r6 ; x++
.final_width_check:
    cmp r6, r1 ; x < w
    jl .final_width_loop
    inc r5 ; y++;
    cmp r5, r2 ; y < h;
    jl .height_loop
    RET

INIT_YMM avx2
cglobal be_blur, 5,15,9
    cmp r1, 32
    jl be_blur_sse2.skip_prologue
    mov r6, 2 ; int x = 2;
    vpxor ymm6, ymm6 ; __m128i temp3 = 0;
    mov r7, r0 ; unsigned char *src=buf;
    movzx r8, byte [r7 + 1] ; int old_pix = src[1];
    movzx r9, byte [r7] ; int old_sum = src[0];
    add r9, r8 ; old_sum += old_pix;
    lea r12, [r4 + r3 * 2] ; unsigned char *col_sum_buf = tmp + stride * 2;
    lea r14, [r1 - 2] ; tmpreg = (w-2);
    and r14, -16 ; tmpreg &= (~15);
    vmovdqa ymm7, [low_word_zero]
.first_loop:
    movzx r10, byte [r7 + r6] ; int temp1 = src[x];
    lea r11, [r8 + r10] ; int temp2 = old_pix + temp1;
    mov r8, r10 ; old_pix = temp1;
    lea r10, [r9 + r11] ; temp1 = old_sum + temp2;
    mov r9, r11 ; old_sum = temp2;
    mov word [r4 + r6 * 2], r10w ; col_pix_buf[x] = temp1;
    inc r6 ; x++
    cmp r6, r1 ; x < w
    jl .first_loop
    mov r6, 2 ; int x = 2;
    lea r7, [r0 + r3] ; unsigned char *src=buf+stride;
    movzx r8, byte [r7 + 1] ; int old_pix = src[1];
    movzx r9, byte [r7] ; int old_sum = src[0];
    add r9, r8 ; old_sum += old_pix
.second_loop:
    movzx r10, byte [r7 + r6] ; int temp1 = src[x];
    lea r11, [r8 + r10] ; int temp2 = old_pix + temp1;
    mov r8, r10 ; old_pix = temp1;
    lea r10, [r9 + r11] ; temp1 = old_sum + temp2;
    mov r9, r11 ; old_sum = temp2;
    movzx r11, word [r4 + r6 * 2] ; temp2 = col_pix_buf[x];
    add r11, r10 ; temp2 += temp1;
    mov word [r4 + r6 * 2], r10w ; col_pix_buf[x] = temp1;
    mov word [r12 + r6 * 2], r11w ; col_sum_buf[x] = temp2;
    inc r6 ; x++
    cmp r6, r1 ; x < w
    jl .second_loop
    mov r5, 2 ; int y = 2;
.height_loop:
    mov r10, r5; int tmpreg = y;
    imul r10, r3; tmpreg *= stride;
    lea r7, [r0 + r10] ; unsigned char *src=buf+y*stride;
    sub r10, r3 ; tmpreg -= stride;
    lea r13, [r0 + r10]; unsigned char *dst=buf+(y-1)*stride;
    mov r6, 2 ; int x = 2;
    movzx r10, byte [r7] ; temp1 = src[0];
    movzx r11, byte [r7 + 1] ; temp2 = src[1];
    add r10, r11; temp1 += temp2
    vmovd xmm0, r10d; __m128i old_pix_128 = temp2;
    vmovd xmm1, r11d; __m128i old_sum_128 = temp1;
.width_loop:
    vpermq ymm2, [r7 + r6], 0x10
    vpunpcklbw ymm2, ymm2, ymm6 ; new_pix = _mm_unpacklo_epi8(new_pix, temp3);
    vpermq ymm8, ymm2, 0x4e
    vpalignr ymm3, ymm2, ymm8, 14
    vpand ymm3, ymm3, ymm7
    vpaddw ymm3, ymm0 ; temp = _mm_add_epi16(temp, old_pix_128);
    vpaddw ymm3, ymm2 ; temp = _mm_add_epi16(temp, new_pix);
    vperm2i128 ymm0, ymm2, ymm6, 0x21
    vpsrldq ymm0, ymm0, 14; temp = temp >> 14 * 8;
    vpermq ymm8, ymm3, 0x4e
    vpand ymm8, ymm8, ymm7;
    vpalignr ymm2, ymm3, ymm8, 14
    vpand ymm2, ymm2, ymm7
    vpaddw ymm2, ymm1 ; new_pix = _mm_add_epi16(new_pix, old_sum_128);
    vpaddw ymm2, ymm3 ; new_pix = _mm_add_epi16(new_pix, temp);
    vperm2i128 ymm1, ymm3, ymm6, 0x21
    vpsrldq ymm1, ymm1, 14; temp = temp << 2 * 8;
    vmovdqu ymm4, [r4 + r6 * 2] ; __m128i old_col_pix = *(col_pix_buf+x);
    vmovdqu [r4 + r6 * 2], ymm2 ; *(col_pix_buf+x) = new_pix ;
    vmovdqu ymm5, [r12 + r6 * 2] ; __m128i old_col_sum = *(col_pix_sum+x);
    vpaddw ymm3, ymm2, ymm4
    vmovdqu [r12 + r6 * 2], ymm3 ; *(col_sum_buf+x) = temp;
    vpaddw ymm5, ymm3 ; old_col_sum = _mm_add_epi16(old_col_sum, temp);
    vpsrlw ymm5, 4 ; old_col_sum = old_col_sum >> 4;
    vpackuswb ymm5, ymm5 ; old_col_sum = _mm_packus_epi16(old_col_sum, old_col_sum);
    vpermq ymm5, ymm5, 11_01_10_00b
    vmovdqu [r13 + r6 - 1], xmm5 ; *(dst+x-1) = old_col_sum;
    add r6, 16; x += 16;
    cmp r6, r14; x < ((w - 2) & (~15));
    jl .width_loop
    movzx r8, byte [r7 + r6 - 1] ; old_pix = src[x-1];
    movzx r9, byte [r7 + r6 - 2] ; old_sum = old_pix + src[x-2];
    add r9, r8
    jmp .final_width_check
.final_width_loop:
    movzx r10, byte [r7 + r6] ; temp1 = src[x];
    lea r11, [r8 + r10] ; temp2 = old_pix + temp1;
    mov r8, r10 ; old_pix = temp1;
    lea r10, [r9 + r11] ; temp1 = old_sum + temp2;
    mov r9, r11 ; old_sum = temp2;
    movzx r11, word [r4 + r6 * 2] ; temp2 = col_pix_buf[x];
    add r11, r10 ; temp2 += temp1;
    mov word [r4 + r6 * 2], r10w ; col_pix_buf[x] = temp1;
    movzx r10, word [r12 + r6 * 2] ; temp1 = col_sum_buf[x];
    add r10, r11 ; temp1 += temp2;
    shr r10, 4 ; temp1 >>= 4;
    mov byte [r13 + r6 - 1], r10b ; dst[x-1] = temp1
    mov [r12 + r6 * 2], r11w ; col_sum_buf[x] = temp2;
    inc r6 ; x++
.final_width_check:
    cmp r6, r1 ; x < w
    jl .final_width_loop
    inc r5 ; y++;
    cmp r5, r2 ; y < h;
    jl .height_loop
    RET

