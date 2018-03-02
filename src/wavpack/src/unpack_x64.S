############################################################################
##                           **** WAVPACK ****                            ##
##                  Hybrid Lossless Wavefile Compressor                   ##
##              Copyright (c) 1998 - 2015 Conifer Software.               ##
##                          All Rights Reserved.                          ##
##      Distributed under the BSD Software License (see license.txt)      ##
############################################################################

        .intel_syntax noprefix
        .text

        .globl  _unpack_decorr_stereo_pass_cont_x64win
        .globl  _unpack_decorr_mono_pass_cont_x64win

        .globl  unpack_decorr_stereo_pass_cont_x64win
        .globl  unpack_decorr_mono_pass_cont_x64win

        .globl  _unpack_decorr_stereo_pass_cont_x64
        .globl  _unpack_decorr_mono_pass_cont_x64

        .globl  unpack_decorr_stereo_pass_cont_x64
        .globl  unpack_decorr_mono_pass_cont_x64

# This is an assembly optimized version of the following WavPack function:
#
# void unpack_decorr_stereo_pass_cont (struct decorr_pass *dpp,
#                                      int32_t *buffer,
#                                      int32_t sample_count,
#                                      int32_t long_math;
#
# It performs a single pass of stereo decorrelation on the provided buffer.
# Note that this version of the function requires that up to 8 previous
# stereo samples are visible and correct. In other words, it ignores the
# "samples_*" fields in the decorr_pass structure and gets the history data
# directly from the buffer. It does, however, return the appropriate history
# samples to the decorr_pass structure before returning.
#
# The "long_math" argument is used to specify that a 32-bit multiply is
# not enough for the "apply_weight" operation (although in this case it
# would only apply to the -1 and -2 terms because the MMX code does not have
# this limitation) but we ignore the parameter and use the overflow detection
# of the "imul" instruction to switch automatically to the "long_math" loop.
#
# This is written to work on an X86-64 processor (also called the AMD64)
# running in 64-bit mode and generally uses the MMX extensions to improve
# the performance by processing both stereo channels together. Unfortunately
# this is not easily used for terms -1 and -2, so these terms are handled
# sequentially with regular assembler code.
#
# This version has entry points for both the System V ABI and the Windows
# X64 ABI. It does not use the "red zone" or the "shadow area"; it saves the
# non-volatile registers for both ABIs on the stack and allocates another
# 8 bytes on the stack to store the dpp pointer. Note that it does NOT
# provide unwind data for the Windows ABI (the unpack_x64.asm module for
# MSVC does). The arguments are passed in registers:
#
# System V  Windows  
#   rdi       rcx      struct decorr_pass *dpp
#   rsi       rdx      int32_t *buffer
#   edx       r8       int32_t sample_count
#   ecx       r9       int32_t long_math
#
# registers after entry:
#
#   rdi         bptr
#   rsi         eptr
#
# stack usage:
#
# [rsp+0] = *dpp
#

_unpack_decorr_stereo_pass_cont_x64win:
unpack_decorr_stereo_pass_cont_x64win:
        push    rbp
        push    rbx
        push    rdi
        push    rsi
        sub     rsp, 8
        mov     rdi, rcx                    # copy params from win regs to Linux regs
        mov     rsi, rdx                    # so we can leave following code similar
        mov     rdx, r8
        mov     rcx, r9
        jmp     entry                       # jump into common portion

_unpack_decorr_stereo_pass_cont_x64:
unpack_decorr_stereo_pass_cont_x64:
        push    rbp
        push    rbx
        push    rdi
        push    rsi
        sub     rsp, 8

entry:  mov     [rsp], rdi                  # store dpp* at [rsp]
        and     edx, edx                    # if sample_count is zero, do nothing
        jz      done

        mov     rdi, rsi                    # rdi = bptr
        lea     rsi, [rdi+rdx*8]            # rsi = eptr

        mov     rax, [rsp]                  # get term from dpp struct & vector to handler
        mov     eax, [rax]
        cmp     al, 17
        je      term_17_entry
        cmp     al, 18
        je      term_18_entry
        cmp     al, -1
        je      term_minus_1_entry
        cmp     al, -2
        je      term_minus_2_entry
        cmp     al, -3
        je      term_minus_3_entry

#
# registers in default term loop:
#
#   rbx         term * -8 (for indexing correlation sample)
#   rdi         bptr
#   rsi         eptr
#
#   mm0, mm1    scratch
#   mm2         original sample values
#   mm3         correlation sample
#   mm4         zero (for pcmpeqd)
#   mm5         weights
#   mm6         delta
#   mm7         512 (for rounding)
#

default_term_entry:
        imul    rbx, rax, -8                # set RBX to term * -8
        mov     eax, 512
        movd    mm7, eax
        punpckldq mm7, mm7                  # mm7 = round (512)
        mov     rdx, [rsp]                  # set RDX to *dpp
        mov     eax, [rdx+4]
        movd    mm6, eax
        punpckldq mm6, mm6                  # mm6 = delta (0-7)
        mov     eax, 0xFFFF                 # mask high weights to zero for PMADDWD
        movd    mm5, eax
        punpckldq mm5, mm5                  # mm5 = weight mask 0x0000FFFF0000FFFF
        pand    mm5, [rdx+8]                # mm5 = weight_AB masked to 16 bits
        pxor    mm4, mm4                    # mm4 = zero (for pcmpeqd)
        jmp     default_term_loop

        .balign  64
default_term_loop:
        movq    mm3, [rdi+rbx]              # mm3 = sam_AB
        movq    mm1, mm3
        movq    mm0, mm3
        paddd   mm1, mm1
        psrld   mm0, 15
        psrlw   mm1, 1
        pmaddwd mm0, mm5
        pmaddwd mm1, mm5
        movq    mm2, [rdi]                  # mm2 = left_right
        pslld   mm0, 5
        paddd   mm1, mm7                    # add 512 for rounding
        psrad   mm1, 10
        paddd   mm0, mm2
        paddd   mm0, mm1                    # add shifted sums
        movq    [rdi], mm0                  # store result
        movq    mm0, mm3
        pxor    mm0, mm2
        psrad   mm0, 31                     # mm0 = sign (sam_AB ^ left_right)
        add     rdi, 8
        pcmpeqd mm2, mm4                    # mm2 = 1s if left_right was zero
        pcmpeqd mm3, mm4                    # mm3 = 1s if sam_AB was zero
        por     mm2, mm3                    # mm2 = 1s if either was zero
        pandn   mm2, mm6                    # mask delta with zeros check
        pxor    mm5, mm0
        paddw   mm5, mm2                    # and add to weight_AB
        pxor    mm5, mm0
        cmp     rdi, rsi                    # compare bptr and eptr to see if we're done
        jb      default_term_loop

        pslld   mm5, 16                     # sign-extend 16-bit weights back to dwords
        psrad   mm5, 16
        mov     rdx, [rsp]                  # point to dpp
        movq    [rdx+8], mm5                # put weight_AB back
        emms

        mov     ecx, [rdx]                  # ecx = dpp->term

default_store_samples:
        dec     ecx
        sub     rdi, 8                      # back up one full sample
        mov     eax, [rdi+4]
        mov     [rdx+rcx*4+48], eax         # store samples_B [ecx]
        mov     eax, [rdi]
        mov     [rdx+rcx*4+16], eax         # store samples_A [ecx]
        test    ecx, ecx
        jnz     default_store_samples
        jmp     done

#
# registers in term 17 & 18 loops:
#
#   rdi         bptr
#   rsi         eptr
#
#   mm0, mm1    scratch
#   mm2         original sample values
#   mm3         correlation samples
#   mm4         last calculated values (so we don't need to reload)
#   mm5         weights
#   mm6         delta
#   mm7         512 (for rounding)
#

term_17_entry:
        mov     eax, 512
        movd    mm7, eax
        punpckldq mm7, mm7                  # mm7 = round (512)
        mov     rdx, [rsp]                  # set RDX to *dpp
        mov     eax, [rdx+4]
        movd    mm6, eax
        punpckldq mm6, mm6                  # mm6 = delta (0-7)
        mov     eax, 0xFFFF                 # mask high weights to zero for PMADDWD
        movd    mm5, eax
        punpckldq mm5, mm5                  # mm5 = weight mask 0x0000FFFF0000FFFF
        pand    mm5, [rdx+8]                # mm5 = weight_AB masked to 16 bits
        movq    mm4, [rdi-8]                # preload last calculated values in mm4
        jmp     term_17_loop

        .balign  64
term_17_loop:
        paddd   mm4, mm4
        psubd   mm4, [rdi-16]               # mm3 = sam_AB
        movq    mm3, mm4
        movq    mm1, mm3
        paddd   mm1, mm1
        psrld   mm4, 15
        psrlw   mm1, 1
        pmaddwd mm4, mm5
        pmaddwd mm1, mm5
        movq    mm2, [rdi]                  # mm2 = left_right
        pslld   mm4, 5
        paddd   mm1, mm7                    # add 512 for rounding
        psrad   mm1, 10
        paddd   mm4, mm2
        paddd   mm4, mm1                    # add shifted sums
        movq    mm0, mm3
        movq    [rdi], mm4                  # store result
        pxor    mm0, mm2
        psrad   mm0, 31                     # mm0 = sign (sam_AB ^ left_right)
        add     rdi, 8
        pxor    mm1, mm1                    # mm1 = zero
        pcmpeqd mm2, mm1                    # mm2 = 1s if left_right was zero
        pcmpeqd mm3, mm1                    # mm3 = 1s if sam_AB was zero
        por     mm2, mm3                    # mm2 = 1s if either was zero
        pandn   mm2, mm6                    # mask delta with zeros check
        pxor    mm5, mm0
        paddw   mm5, mm2                    # and add to weight_AB
        pxor    mm5, mm0
        cmp     rdi, rsi                    # compare bptr and eptr to see if we're done
        jb      term_17_loop
        jmp     term_1718_exit              # terms 17 & 18 treat samples_AB[] the same

term_18_entry:
        mov     eax, 512
        movd    mm7, eax
        punpckldq mm7, mm7                  # mm7 = round (512)
        mov     rdx, [rsp]                  # set RDX to *dpp
        mov     eax, [rdx+4]
        movd    mm6, eax
        punpckldq mm6, mm6                  # mm6 = delta (0-7)
        mov     eax, 0xFFFF                 # mask high weights to zero for PMADDWD
        movd    mm5, eax
        punpckldq mm5, mm5                  # mm5 = weight mask 0x0000FFFF0000FFFF
        pand    mm5, [rdx+8]                # mm5 = weight_AB masked to 16 bits
        movq    mm4, [rdi-8]                # preload last calculated values in mm4
        jmp     term_18_loop

        .balign  64
term_18_loop:
        movq    mm3, mm4
        psubd   mm3, [rdi-16]
        psrad   mm3, 1
        paddd   mm3, mm4                    # mm3 = sam_AB
        movq    mm1, mm3
        movq    mm4, mm3
        paddd   mm1, mm1
        psrld   mm4, 15
        psrlw   mm1, 1
        pmaddwd mm4, mm5
        pmaddwd mm1, mm5
        movq    mm2, [rdi]                  # mm2 = left_right
        pslld   mm4, 5
        paddd   mm1, mm7                    # add 512 for rounding
        psrad   mm1, 10
        paddd   mm4, mm2
        paddd   mm4, mm1                    # add shifted sums
        movq    mm0, mm3
        movq    [rdi], mm4                  # store result
        pxor    mm0, mm2
        psrad   mm0, 31                     # mm0 = sign (sam_AB ^ left_right)
        add     rdi, 8
        pxor    mm1, mm1                    # mm1 = zero
        pcmpeqd mm2, mm1                    # mm2 = 1s if left_right was zero
        pcmpeqd mm3, mm1                    # mm3 = 1s if sam_AB was zero
        por     mm2, mm3                    # mm2 = 1s if either was zero
        pandn   mm2, mm6                    # mask delta with zeros check
        pxor    mm5, mm0
        paddw   mm5, mm2                    # and add to weight_AB
        pxor    mm5, mm0
        cmp     rdi, rsi                    # compare bptr and eptr to see if we're done
        jb      term_18_loop

term_1718_exit:
        pslld   mm5, 16                     # sign-extend 16-bit weights back to dwords
        psrad   mm5, 16
        mov     rdx, [rsp]                  # point to dpp
        movq    [rdx+8], mm5                # put weight_AB back
        emms

        mov     eax, [rdi-4]                # dpp->samples_B [0] = bptr [-1];
        mov     [rdx+48], eax
        mov     eax, [rdi-8]                # dpp->samples_A [0] = bptr [-2];
        mov     [rdx+16], eax
        mov     eax, [rdi-12]               # dpp->samples_B [1] = bptr [-3];
        mov     [rdx+52], eax
        mov     eax, [rdi-16]               # dpp->samples_A [1] = bptr [-4];
        mov     [rdx+20], eax
        jmp     done

#
# registers in term -1 & -2 loops:
#
#   eax,ebx,edx scratch
#   ecx         weight_A
#   ebp         weight_B
#   rdi         bptr
#   rsi         eptr
#   r8d         delta
#

term_minus_1_entry:
        cld
        mov     rdx, [rsp]                  # point to dpp
        mov     ecx, [rdx+8]                # ecx = weight_A
        mov     ebp, [rdx+12]               # ebp = weight_B
        mov     r8d, [rdx+4]                # r8d = delta
        mov     eax, [rdi-4]
        jmp     term_minus_1_loop

        .balign  64
term_minus_1_loop:
        mov     ebx, eax
        imul    eax, ecx
        mov     edx, [rdi]
        jo      OV11
        sar     eax, 10
        adc     eax, edx
        stosd
        test    ebx, ebx
        je      L182
        test    edx, edx
        je      L182
        xor     ebx, edx
        sar     ebx, 31
        xor     ecx, ebx
        add     ecx, r8d
        mov     edx, 1024
        add     edx, ebx
        cmp     ecx, edx
        jle     L183
        mov     ecx, edx
L183:   xor     ecx, ebx
L182:   mov     ebx, eax
        imul    eax, ebp
        mov     edx, [rdi]
        jo      OV12
        sar     eax, 10
        adc     eax, edx
        stosd
        test    ebx, ebx
        je      L187
        test    edx, edx
        je      L187
        xor     ebx, edx
        sar     ebx, 31
        xor     ebp, ebx
        add     ebp, r8d
        mov     edx, 1024
        add     edx, ebx
        cmp     ebp, edx
        jle     L188
        mov     ebp, edx
L188:   xor     ebp, ebx
L187:   cmp     rdi, rsi                    # compare bptr and eptr to see if we're done
        jb      term_minus_1_loop
        jmp     term_minus_1_done

OV11:   mov     eax, ebx                    # restore previous sample into eax
        jmp     long_term_minus_1_loop

OV12:   mov     eax, ebx                    # restore previous sample into eax
        jmp     L282

        .balign  64
long_term_minus_1_loop:
        mov     ebx, eax
        imul    ecx
        shl     edx, 22
        shr     eax, 10
        adc     eax, edx
        mov     edx, [rdi]
        add     eax, edx
        stosd
        test    ebx, ebx
        je      L282
        test    edx, edx
        je      L282
        xor     ebx, edx
        sar     ebx, 31
        xor     ecx, ebx
        add     ecx, r8d
        mov     edx, 1024
        add     edx, ebx
        cmp     ecx, edx
        jle     L283
        mov     ecx, edx
L283:   xor     ecx, ebx
L282:   mov     ebx, eax
        imul    ebp
        shl     edx, 22
        shr     eax, 10
        adc     eax, edx
        mov     edx, [rdi]
        add     eax, edx
        stosd
        test    ebx, ebx
        je      L287
        test    edx, edx
        je      L287
        xor     ebx, edx
        sar     ebx, 31
        xor     ebp, ebx
        add     ebp, r8d
        mov     edx, 1024
        add     edx, ebx
        cmp     ebp, edx
        jle     L288
        mov     ebp, edx
L288:   xor     ebp, ebx
L287:   cmp     rdi, rsi                    # compare bptr and eptr to see if we're done
        jb      long_term_minus_1_loop

term_minus_1_done:
        mov     rdx, [rsp]                  # point to dpp
        mov     [rdx+8], ecx                # store weights back
        mov     [rdx+12], ebp
        mov     eax, [rdi-4]                # dpp->samples_A [0] = bptr [-1];
        mov     [rdx+16], eax
        jmp     done

term_minus_2_entry:
        mov     rdx, [rsp]                  # point to dpp
        mov     ecx, [rdx+8]                # ecx = weight_A
        mov     ebp, [rdx+12]               # ebp = weight_B
        mov     r8d, [rdx+4]                # r8d = delta
        mov     eax, [rdi-8]
        jmp     term_minus_2_loop

        .balign  64
term_minus_2_loop:
        mov     ebx, eax
        imul    eax, ebp
        mov     edx, [rdi+4]
        jo      OV21
        sar     eax, 10
        adc     eax, edx
        mov     [rdi+4], eax
        test    ebx, ebx
        je      L194
        test    edx, edx
        je      L194
        xor     ebx, edx
        sar     ebx, 31
        xor     ebp, ebx
        add     ebp, r8d
        mov     edx, 1024
        add     edx, ebx
        cmp     ebp, edx
        jle     L195
        mov     ebp, edx
L195:   xor     ebp, ebx
L194:   mov     ebx, eax
        imul    eax, ecx
        mov     edx, [rdi]
        jo      OV22
        sar     eax, 10
        adc     eax, edx
        mov     [rdi], eax
        test    ebx, ebx
        je      L199
        test    edx, edx
        je      L199
        xor     ebx, edx
        sar     ebx, 31
        xor     ecx, ebx
        add     ecx, r8d
        mov     edx, 1024
        add     edx, ebx
        cmp     ecx, edx
        jle     L200
        mov     ecx, edx
L200:   xor     ecx, ebx
L199:   add     rdi, 8
        cmp     rdi, rsi                    # compare bptr and eptr to see if we're done
        jb      term_minus_2_loop
        jmp     term_minus_2_done

OV21:   mov     eax, ebx                    # restore previous sample into eax
        jmp     long_term_minus_2_loop

OV22:   mov     eax, ebx                    # restore previous sample into eax
        jmp     L294

        .balign  64
long_term_minus_2_loop:
        mov     ebx, eax
        imul    ebp
        shl     edx, 22
        shr     eax, 10
        adc     eax, edx
        mov     edx, [rdi+4]
        add     eax, edx
        mov     [rdi+4], eax
        test    ebx, ebx
        je      L294
        test    edx, edx
        je      L294
        xor     ebx, edx
        sar     ebx, 31
        xor     ebp, ebx
        add     ebp, r8d
        mov     edx, 1024
        add     edx, ebx
        cmp     ebp, edx
        jle     L295
        mov     ebp, edx
L295:   xor     ebp, ebx
L294:   mov     ebx, eax
        imul    ecx
        shl     edx, 22
        shr     eax, 10
        adc     eax, edx
        mov     edx, [rdi]
        add     eax, edx
        mov     [rdi], eax
        test    ebx, ebx
        je      L299
        test    edx, edx
        je      L299
        xor     ebx, edx
        sar     ebx, 31
        xor     ecx, ebx
        add     ecx, r8d
        mov     edx, 1024
        add     edx, ebx
        cmp     ecx, edx
        jle     L300
        mov     ecx, edx
L300:   xor     ecx, ebx
L299:   add     rdi, 8
        cmp     rdi, rsi                    # compare bptr and eptr to see if we're done
        jb      long_term_minus_2_loop

term_minus_2_done:
        mov     rdx, [rsp]                  # point to dpp
        mov     [rdx+8], ecx                # store weights back
        mov     [rdx+12], ebp
        mov     eax, [rdi-8]                # dpp->samples_B [0] = bptr [-2];
        mov     [rdx+48], eax
        jmp     done

#
# registers in term -3 loop:
#
#   rdi         bptr
#   rsi         eptr
#
#   mm0, mm1    scratch
#   mm2         original sample values
#   mm3         correlation samples
#   mm4         last calculated values (so we don't need to reload)
#   mm5         weights
#   mm6         delta
#   mm7         512 (for rounding)
#

term_minus_3_entry:
        mov     eax, 512
        movd    mm7, eax
        punpckldq mm7, mm7                  # mm7 = round (512)
        mov     rdx, [rsp]                  # set RDX to *dpp
        mov     eax, [rdx+4]
        movd    mm6, eax
        punpckldq mm6, mm6                  # mm6 = delta (0-7)
        mov     eax, 0xFFFF                 # mask high weights to zero for PMADDWD
        movd    mm5, eax
        punpckldq mm5, mm5                  # mm5 = weight mask 0x0000FFFF0000FFFF
        pand    mm5, [rdx+8]                # mm5 = weight_AB masked to 16 bits
        movq    mm4, [rdi-8]
        jmp     term_minus_3_loop

        .balign  64
term_minus_3_loop:
        movq    mm3, mm4
        psrlq   mm3, 32
        punpckldq mm3, mm4                  # mm3 = sam_AB
        movq    mm1, mm3
        movq    mm4, mm3
        pslld   mm1, 1
        psrld   mm4, 15
        psrlw   mm1, 1
        pmaddwd mm4, mm5
        pmaddwd mm1, mm5
        movq    mm2, [rdi]                  # mm2 = left_right
        pslld   mm4, 5
        paddd   mm1, mm7                    # add 512 for rounding
        psrad   mm1, 10
        paddd   mm4, mm2
        paddd   mm4, mm1                    # add shifted sums
        movq    [rdi], mm4                  # store result
        movq    mm0, mm3
        pxor    mm0, mm2
        psrad   mm0, 31                     # mm0 = sign (sam_AB ^ left_right)
        add     rdi, 8
        pxor    mm1, mm1                    # mm1 = zero
        pcmpeqd mm2, mm1                    # mm2 = 1s if left_right was zero
        pcmpeqd mm3, mm1                    # mm3 = 1s if sam_AB was zero
        por     mm2, mm3                    # mm2 = 1s if either was zero
        pandn   mm2, mm6                    # mask delta with zeros check
        pcmpeqd mm1, mm1
        psubd   mm1, mm7
        psubd   mm1, mm7
        psubd   mm1, mm0
        pxor    mm5, mm0
        paddw   mm5, mm1
        paddusw mm5, mm2                    # and add to weight_AB
        psubw   mm5, mm1
        pxor    mm5, mm0
        cmp     rdi, rsi                    # compare bptr and eptr to see if we're done
        jb      term_minus_3_loop

        pslld   mm5, 16                     # sign-extend 16-bit weights back to dwords
        psrad   mm5, 16
        mov     rdx, [rsp]                  # point to dpp
        movq    [rdx+8], mm5                # put weight_AB back
        emms

        mov     edx, [rdi-4]                # dpp->samples_A [0] = bptr [-1];
        mov     rax, [rsp] 
        mov     [rax+16], edx
        mov     edx, [rdi-8]                # dpp->samples_B [0] = bptr [-2];
        mov     [rax+48], edx

done:   add     rsp, 8
        pop     rsi
        pop     rdi
        pop     rbx
        pop     rbp
        ret

#######################################################################################################################
#
# This is the mono version of the above function. It does not use MMX and does not handle negative terms.
#
# void unpack_decorr_mono_pass_cont (struct decorr_pass *dpp,
#                                    int32_t *buffer,
#                                    int32_t sample_count,
#                                    int32_t long_math;
# arguments on entry:
#
# System V  Windows  
#   rdi       rcx      struct decorr_pass *dpp
#   rsi       rdx      int32_t *buffer
#   edx       r8       int32_t sample_count
#   ecx       r9       int32_t long_math
#
# registers after entry:
#
#   rdi         bptr
#   rsi         eptr
#
# stack usage:
#
# [rsp+0] = *dpp
#

_unpack_decorr_mono_pass_cont_x64win:
unpack_decorr_mono_pass_cont_x64win:
        push    rbp
        push    rbx
        push    rdi
        push    rsi
        sub     rsp, 8

        mov     rdi, rcx                    # copy params from win regs to Linux regs
        mov     rsi, rdx                    # so we can leave following code similar
        mov     rdx, r8
        mov     rcx, r9
        jmp     mentry                      # jump into common portion

_unpack_decorr_mono_pass_cont_x64:
unpack_decorr_mono_pass_cont_x64:
        push    rbp
        push    rbx
        push    rdi
        push    rsi
        sub     rsp, 8

mentry: mov     [rsp], rdi                  # store dpp* into [rsp]
        and     edx, edx                    # if sample_count is zero, do nothing
        jz      mono_done

        cld                                 # we use stosd
        mov     rdi, rsi                    # rdi = bptr
        lea     rsi, [rdi+rdx*4]            # rsi = eptr

        mov     rax, [rsp]                  # get term from dpp struct & vector to handler
        mov     eax, [rax]
        cmp     al, 17
        je      mono_17_entry
        cmp     al, 18
        je      mono_18_entry

#
# registers during default term processing loop:
#   rdi         active buffer pointer
#   rsi         end of buffer pointer
#   r8d         delta
#   ecx         weight_A
#   ebx         term * -4
#   eax,edx     scratch
#

default_mono_entry:
        imul    rbx, rax, -4                # set rbx to term * -4 for decorrelation index
        mov     rdx, [rsp]
        mov     ecx, [rdx+8]                # ecx = weight, r8d = delta
        mov     r8d, [rdx+4]
        jmp     default_mono_loop

#
# registers during processing loop for terms 17 & 18:
#   rdi         active buffer pointer
#   rsi         end of buffer pointer
#   r8d         delta
#   ecx         weight_A
#   ebp         previously calculated value
#   ebx         calculated correlation sample
#   eax,edx     scratch
#

mono_17_entry:
        mov     rdx, [rsp]                  # rdx = dpp*
        mov     ecx, [rdx+8]                # ecx = weight, r8d = delta
        mov     r8d, [rdx+4]
        mov     ebp, [rdi-4]
        jmp     mono_17_loop

mono_18_entry:
        mov     rdx, [rsp]                  # rdx = dpp*
        mov     ecx, [rdx+8]                # ecx = weight, r8d = delta
        mov     r8d, [rdx+4]
        mov     ebp, [rdi-4]
        jmp     mono_18_loop

        .balign  64
default_mono_loop:
        mov     eax, [rdi+rbx]
        imul    eax, ecx
        mov     edx, [rdi]
        jo      long_default_mono_loop
        sar     eax, 10
        adc     eax, edx
        mov     [rdi], eax
        mov     eax, [rdi+rbx]
        add     rdi, 4
        test    edx, edx
        je      L100
        test    eax, eax
        je      L100
        xor     eax, edx
        cdq
        xor     ecx, edx
        add     ecx, r8d
        xor     ecx, edx
L100:   cmp     rdi, rsi                    # compare bptr and eptr to see if we're done
        jb      default_mono_loop
        jmp     default_mono_done

        .balign  64
long_default_mono_loop:
        mov     eax, [rdi+rbx]
        imul    ecx
        shl     edx, 22
        shr     eax, 10
        adc     eax, edx
        mov     edx, [rdi]
        add     eax, edx
        mov     [rdi], eax
        mov     eax, [rdi+rbx]
        add     rdi, 4
        test    edx, edx
        je      L101
        test    eax, eax
        je      L101
        xor     eax, edx
        cdq
        xor     ecx, edx
        add     ecx, r8d
        xor     ecx, edx
L101:   cmp     rdi, rsi                    # compare bptr and eptr to see if we're done
        jb      long_default_mono_loop

default_mono_done:
        mov     rdx, [rsp]                  # edx = dpp*
        mov     [rdx+8], ecx                # store weight_A back
        mov     ecx, [rdx]                  # ecx = dpp->term

default_mono_store_samples:
        dec     ecx
        sub     rdi, 4                      # back up one full sample
        mov     eax, [rdi]
        mov     [rdx+rcx*4+16], eax         # store samples_A [ecx]
        test    ecx, ecx
        jnz     default_mono_store_samples
        jmp     mono_done

        .balign  64
mono_17_loop:
        lea     ebx, [ebp+ebp]
        sub     ebx, [rdi-8]
        mov     eax, ecx
        imul    eax, ebx
        mov     edx, [rdi]
        jo      long_mono_17_loop
        sar     eax, 10
        adc     eax, edx
        stosd
        test    ebx, ebx
        mov     ebp, eax
        je      L117
        test    edx, edx
        je      L117
        xor     ebx, edx
        sar     ebx, 31
        xor     ecx, ebx
        add     ecx, r8d
        xor     ecx, ebx
L117:   cmp     rdi, rsi                    # compare bptr and eptr to see if we're done
        jb      mono_17_loop
        jmp     mono_1718_exit

        .balign  64
long_mono_17_loop:
        lea     ebx, [ebp+ebp]
        sub     ebx, [rdi-8]
        mov     eax, ecx
        imul    ebx
        shl     edx, 22
        shr     eax, 10
        adc     eax, edx
        mov     edx, [rdi]
        add     eax, edx
        stosd
        test    ebx, ebx
        mov     ebp, eax
        je      L217
        test    edx, edx
        je      L217
        xor     ebx, edx
        sar     ebx, 31
        xor     ecx, ebx
        add     ecx, r8d
        xor     ecx, ebx
L217:   cmp     rdi, rsi                    # compare bptr and eptr to see if we're done
        jb      long_mono_17_loop
        jmp     mono_1718_exit

        .balign  64
mono_18_loop:
        lea     ebx, [ebp+ebp*2]
        sub     ebx, [rdi-8]
        sar     ebx, 1
        mov     eax, ecx
        imul    eax, ebx
        mov     edx, [rdi]
        jo      long_mono_18_loop
        sar     eax, 10
        adc     eax, edx
        stosd
        test    ebx, ebx
        mov     ebp, eax
        je      L118
        test    edx, edx
        je      L118
        xor     ebx, edx
        sar     ebx, 31
        xor     ecx, ebx
        add     ecx, r8d
        xor     ecx, ebx
L118:   cmp     rdi, rsi                    # compare bptr and eptr to see if we're done
        jb      mono_18_loop
        jmp     mono_1718_exit

        .balign  64
long_mono_18_loop:
        lea     ebx, [ebp+ebp*2]
        sub     ebx, [rdi-8]
        sar     ebx, 1
        mov     eax, ecx
        imul    ebx
        shl     edx, 22
        shr     eax, 10
        adc     eax, edx
        mov     edx, [rdi]
        add     eax, edx
        stosd
        test    ebx, ebx
        mov     ebp, eax
        je      L218
        test    edx, edx
        je      L218
        xor     ebx, edx
        sar     ebx, 31
        xor     ecx, ebx
        add     ecx, r8d
        xor     ecx, ebx
L218:   cmp     rdi, rsi                    # compare bptr and eptr to see if we're done
        jb      long_mono_18_loop

mono_1718_exit:
        mov     rdx, [rsp]                  # edx = dpp*
        mov     [rdx+8], ecx                # store weight_A back
        mov     eax, [rdi-4]                # dpp->samples_A [0] = bptr [-1];
        mov     [rdx+16], eax
        mov     eax, [rdi-8]                # dpp->samples_A [1] = bptr [-2];
        mov     [rdx+20], eax

mono_done:
        add     rsp, 8
        pop     rsi
        pop     rdi
        pop     rbx
        pop     rbp
        ret

#ifdef __ELF__
        .section .note.GNU-stack,"",@progbits
#endif

