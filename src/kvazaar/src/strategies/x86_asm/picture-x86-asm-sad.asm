;/****************************************************************************
; * This file is part of Kvazaar HEVC encoder.
; *
; * Copyright (C) 2013-2015 Tampere University of Technology and others (see
; * COPYING file).
; *
; * Kvazaar is free software: you can redistribute it and/or modify it under
; * the terms of the GNU Lesser General Public License as published by the
; * Free Software Foundation; either version 2.1 of the License, or (at your
; * option) any later version.
; *
; * Kvazaar is distributed in the hope that it will be useful, but WITHOUT ANY
; * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
; * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
; * more details.
; *
; * You should have received a copy of the GNU General Public License along
; * with Kvazaar.  If not, see <http://www.gnu.org/licenses/>.
; ***************************************************************************/

%include "x86inc.asm"

;cglobal and RET macros are from the x86.inc
;they push and pop the necessary registers to
;stack depending on the operating system

;Usage: cglobal name, %1, %2, %3
;1%: Number of arguments
;2%: Number of registers used
;3%: Number of xmm registers used.
;More info in x86inc.asm

SECTION .text

;Set x86inc.asm macros to use avx and xmm registers
INIT_XMM avx

;KVZ_SAD_4X4
;Calculates SAD of the 16 consequtive bytes in memory
;r0 address of the first value(current frame)
;r1 address of the first value(reference frame)

cglobal sad_4x4, 2, 2, 2

    ;Load 16 bytes of both frames
    vmovdqu m0, [r0]
    vmovdqu m1, [r1]

    ;Calculate SAD. The results are written in
    ;m0[15:0] and m0[79:64]
    vpsadbw m0, m1

    ;Sum the results
    vmovhlps m1, m0
    vpaddw m0, m1

    ;Write the result to eax
    vmovd eax, m0

    RET


;KVZ_SAD_4X4_STRIDE
;Calculates SAD of a 4x4 block inside a frame with stride
;r0 address of the first value(current)
;r1 address of the first value(reference)
;r2 stride

cglobal sad_4x4_stride, 3, 3, 2

    ;Load 4 times 4 bytes of both frames
    vpinsrd m0, [r0], 0
    add r0, r2
    vpinsrd m0, [r0], 1
    vpinsrd m0, [r0+r2], 2
    vpinsrd m0, [r0+r2*2], 3

    vpinsrd m1, [r1], 0
    add r1, r2
    vpinsrd m1, [r1], 1
    vpinsrd m1, [r1+r2], 2
    vpinsrd m1, [r1+r2*2], 3

    vpsadbw m0, m1

    vmovhlps m1, m0
    vpaddw m0, m1

    vmovd eax, m0

    RET


;KVZ_SAD_8X8
;Calculates SAD of the 64 consequtive bytes in memory
;r0 address of the first value(current)
;r1 address of the first value(reference)

cglobal sad_8x8, 2, 2, 5

    ;Load the first half of both frames
    vmovdqu m0, [r0]
    vmovdqu m2, [r0+16]

    vmovdqu m1, [r1]
    vmovdqu m3, [r1+16]

    ;Calculate SADs for both
    vpsadbw m0, m1
    vpsadbw m2, m3

    ;Sum
    vpaddw m0, m2

    ;Repeat for the latter half
    vmovdqu m1, [r0+16*2]
    vmovdqu m3, [r0+16*3]

    vmovdqu m2, [r1+16*2]
    vmovdqu m4, [r1+16*3]

    vpsadbw m1, m2
    vpsadbw m3, m4

    vpaddw m1, m3

    ;Sum all the SADs
    vpaddw m0, m1

    vmovhlps m1, m0
    vpaddw m0, m1

    vmovd eax, m0

    RET


;KVZ_SAD_8X8_STRIDE
;Calculates SAD of a 8x8 block inside a frame with stride
;r0 address of the first value(current)
;r1 address of the first value(reference)
;r2 stride

cglobal sad_8x8_stride, 3, 3, 5

    ;Zero m0 register
    vpxor m0, m0

    ;Load the first half to m1 and m3 registers(cur)
    ;Current frame
    ;Load to the high 64 bits of xmm
    vmovhpd m1, [r0]
    add r0, r2
    ;Load to the low 64 bits
    vmovlpd m1, [r0] 

    vmovhpd m3, [r0+r2]
    vmovlpd m3, [r0+r2*2] 
    ;lea calculates the address to r0,
    ;but doesn't load anything from
    ;the memory. Equivalent for
    ;two add r0, r2 instructions.
    lea r0, [r0+r2*2]
    add r0, r2

    ;Reference frame
    vmovhpd m2, [r1]
    add r1, r2
    vmovlpd m2, [r1] 

    vmovhpd m4, [r1+r2]
    vmovlpd m4, [r1+r2*2] 
    lea r1, [r1+r2*2]
    add r1, r2

    vpsadbw m1, m2
    vpsadbw m3, m4

    vpaddw m0, m1
    vpaddw m0, m3

    ;Repeat for the other half
    vmovhpd m1, [r0]
    add r0, r2
    vmovlpd m1, [r0] 

    vmovhpd m3, [r0+r2]
    vmovlpd m3, [r0+r2*2] 
    lea r0, [r0+r2*2]
    add r0, r2

    vmovhpd m2, [r1]
    add r1, r2
    vmovlpd m2, [r1] 

    vmovhpd m4, [r1+r2]
    vmovlpd m4, [r1+r2*2] 
    lea r1, [r1+r2*2]
    add r1, r2

    vpsadbw m1, m2
    vpsadbw m3, m4

    vpaddw m0, m1
    vpaddw m0, m3

    vmovhlps m1, m0
    vpaddw m0, m1

    vmovd eax, m0

    RET


;KVZ_SAD_16X16
;Calculates SAD of the 256 consequtive bytes in memory
;r0 address of the first value(current)
;r1 address of the first value(reference)

cglobal sad_16x16, 2, 2, 5

    ;Zero m4
    vpxor m4, m4

    %assign i 0

    ;Repeat 8 times.
    %rep 8

        ;Load the next to rows of the current frame
        vmovdqu m0, [r0 + 16 * i]
        vmovdqu m2, [r0 + 16 * (i + 1)]

        ;Load the next to rows of the reference frame
        vmovdqu m1, [r1 + 16 * i]
        vmovdqu m3, [r1 + 16 * (i + 1)]

        vpsadbw m0, m1
        vpsadbw m2, m3

        ;Accumulate SADs to m4
        vpaddw m4, m0
        vpaddw m4, m2

        %assign i i+2

    %endrep

    ;Calculate the final sum
    vmovhlps m0, m4
    vpaddw m4, m0

    vmovd eax, m4

    RET


;KVZ_SAD_16X16_STRIDE
;Calculates SAD of a 16x16 block inside a frame with stride
;r0 address of the first value(current)
;r1 address of the first value(reference)
;r2 stride

cglobal sad_16x16_stride, 3, 3, 5

    vpxor m4, m4

    %rep 8

        ; Load the next 2 rows from rec_buf to m0 and m2
        vmovdqu m0, [r0]
        vmovdqu m2, [r0 + r2]
        lea r0, [r0 + r2*2]

        ; Load the next 2 rows from ref_buf to m1 and m3
        vmovdqu m1, [r1]
        vmovdqu m3, [r1 + r2]
        lea r1, [r1 + r2*2]
 
        vpsadbw m0, m1
        vpsadbw m2, m3

        vpaddw m4, m0
        vpaddw m4, m2

    %endrep

    vmovhlps m0, m4
    vpaddw m4, m0

    vmovd eax, m4

    RET


;KVZ_SAD_32x32_STRIDE
;Calculates SAD of a 32x32 block inside a frame with stride
;r0 address of the first value(current)
;r1 address of the first value(reference)
;r2 stride
cglobal sad_32x32_stride, 3, 3, 5
    vpxor m4, m4

	; Handle 2 lines per iteration
    %rep 16
        vmovdqu m0, [r0]
        vmovdqu m1, [r0 + 16]
        vmovdqu m2, [r0 + r2]
        vmovdqu m3, [r0 + r2 + 16]
        lea r0, [r0 + 2 * r2]

        vpsadbw m0, [r1]
        vpsadbw m1, [r1 + 16]
        vpsadbw m2, [r1 + r2]
        vpsadbw m3, [r1 + r2 + 16]
        lea r1, [r1 + 2 * r2]
 
        vpaddd m4, m0
        vpaddd m4, m1
        vpaddd m4, m2
        vpaddd m4, m3
    %endrep

    vmovhlps m0, m4
    vpaddd m4, m0

    vmovd eax, m4

    RET


;KVZ_SAD_64x64_STRIDE
;Calculates SAD of a 64x64 block inside a frame with stride
;r0 address of the first value(current)
;r1 address of the first value(reference)
;r2 stride
cglobal sad_64x64_stride, 3, 4, 5
    vpxor m4, m4 ; sum accumulation register
	mov r3, 4 ; number of iterations in the loop

Process16Lines:
	; Intel optimization manual says to not unroll beyond 500 instructions.
	; Didn't seem to have much of an affect on Ivy Bridge or Haswell, but
	; smaller is better, when speed is the same, right?
    %rep 16
        vmovdqu m0, [r0]
        vmovdqu m1, [r0 + 1*16]
        vmovdqu m2, [r0 + 2*16]
        vmovdqu m3, [r0 + 3*16]

        vpsadbw m0, [r1]
        vpsadbw m1, [r1 + 1*16]
        vpsadbw m2, [r1 + 2*16]
        vpsadbw m3, [r1 + 3*16]

        lea r0, [r0 + r2]
        lea r1, [r1 + r2]
 
        vpaddd m4, m0
        vpaddd m4, m1
        vpaddd m4, m2
        vpaddd m4, m3
    %endrep

	dec r3
	jnz Process16Lines

    vmovhlps m0, m4
    vpaddd m4, m0

    vmovd eax, m4

    RET
