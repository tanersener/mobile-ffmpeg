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

;KVZ_ZERO_EXTEND_WD
;zero extend all packed words in xmm to dwords in 2 xmm registers
;%1 source register
;%2 lower destination register
;%3 higher destination register

%macro KVZ_ZERO_EXTEND_WD 3

    ;Zero extend high 64 bits
    vmovhlps %3, %1
    vpmovzxwd %3, %3
    ;Zero extend low 64 bits
    vpmovzxwd %2, %1

%endmacro ; KVZ_ZERO_EXTEND_WD

; Use nondestructive horizontal add and sub to calculate both at the same time.
; TODO: It would probably be possible to do this with 3 registers (destructive vphsubw).
; args:
;	1, 2: input registers
;   3, 4: output registers

%macro SATD_HORIZONTAL_SUB_AND_ADD 4

    ; TODO: It might be possible to do this with 3 registers?
    
    ;First stage
    vphaddw %3, %1, %2
    vphsubw %4, %1, %2
    
    ;Second stage
    vphaddw %1, %3, %4
    vphsubw %2, %3, %4
    
    ;Third stage
    vphaddw %3, %1, %2
    vphsubw %4, %1, %2

%endmacro ; SATD_HORIZONTAL_SUB_AND_ADD

;KVZ_SATD_8X8_STRIDE
;Calculates SATD of a 8x8 block inside a frame with stride
;r0 address of the first value(reference)
;r1 address of the first value(current)
;r2 stride
;
;The Result is written in the register r4

%macro KVZ_SATD_8X8_STRIDE 0

    ;Calculate differences of the 8 rows into
    ;registers m0-m7
    vpmovzxbw m0, [r0]
    vpmovzxbw m7, [r2]
    vpsubw m0, m7

    vpmovzxbw m1, [r0+r1]
    vpmovzxbw m7, [r2+r3]
    vpsubw m1, m7

    ;Set r0 and r2 2 rows forward
    lea r0, [r0+r1*2]
    lea r2, [r2+r3*2]

    vpmovzxbw m2, [r0]
    vpmovzxbw m7, [r2]
    vpsubw m2, m7

    vpmovzxbw m3, [r0+r1]
    vpmovzxbw m7, [r2+r3]
    vpsubw m3, m7

    lea r0, [r0+r1*2]
    lea r2, [r2+r3*2]

    vpmovzxbw m4, [r0]
    vpmovzxbw m7, [r2]
    vpsubw m4, m7

    vpmovzxbw m5, [r0+r1]
    vpmovzxbw m7, [r2+r3]
    vpsubw m5, m7

    lea r0, [r0+r1*2]
    lea r2, [r2+r3*2]

    vpmovzxbw m6, [r0]
    vpmovzxbw m7, [r2]
    vpsubw m6, m7

    ;32-bit AVX doesn't have registers
    ;xmm8-xmm15, use stack instead
    
    %if ARCH_X86_64
        vpmovzxbw m7, [r0+r1]
        vpmovzxbw m8, [r2+r3]
        vpsubw m7, m8
    %else
        %define temp0 esp+16*3
        %define temp1 esp+16*2
        %define temp2 esp+16*1
        %define temp3 esp+16*0
        
        ;Reserve memory for 4 x 128 bits.
        sub esp, 16*4

        vpmovzxbw m7, [r2+r3]
        vmovdqu [temp0], m7
        vpmovzxbw m7, [r0+r1]
        vpsubw m7, [temp0]

        ;Put rows 5-8 to stack
        vmovdqu [temp0], m4
        vmovdqu [temp1], m5
        vmovdqu [temp2], m6
        vmovdqu [temp3], m7
    %endif

    ;Hadamard transform (FWHT algorithm)
    ;Horizontal transform

    %if ARCH_X86_64
        ;Calculate horizontal transform for each row.
        ;Transforms of two rows are interleaved in register pairs.
        ;(m8 and m9, m10 and m11,...)
        
        SATD_HORIZONTAL_SUB_AND_ADD m0, m1, m8, m9
        SATD_HORIZONTAL_SUB_AND_ADD m2, m3, m10, m11
        SATD_HORIZONTAL_SUB_AND_ADD m4, m5, m12, m13
        SATD_HORIZONTAL_SUB_AND_ADD m6, m7, m14, m15

    %else
        ;Calculate horizontal transforms for the first four rows.
        ;Then load the other four into the registers and store
        ;ready transforms in the stack.
        ;Input registers are m0-m3, results are written in
        ;registers m4-m7 (and memory).
        
        SATD_HORIZONTAL_SUB_AND_ADD m0, m1, m4, m5
        SATD_HORIZONTAL_SUB_AND_ADD m2, m3, m6, m7

        vmovdqu m3, [temp3]
        vmovdqu m2, [temp2]
        vmovdqu m1, [temp1]
        vmovdqu m0, [temp0]

        vmovdqu [temp3], m7
        vmovdqu [temp2], m6
        vmovdqu [temp1], m5
        vmovdqu [temp0], m4

        SATD_HORIZONTAL_SUB_AND_ADD m0, m1, m4, m5
        SATD_HORIZONTAL_SUB_AND_ADD m2, m3, m6, m7
    %endif


    ;Vertical transform
    ;Transform columns of the 8x8 block.
    ;First sum the interleaved horizontally
    ;transformed values with one horizontal add
    ;for each pair of rows. Then calculate
    ;with regular packed additions and
    ;subtractions.
    
    %if ARCH_X86_64
        ;Horizontally transformed values are in registers m8-m15
        ;Results are written in m0-m7

        ;First stage
        vphaddw m0, m8, m9
        vphsubw m1, m8, m9

        vphaddw m2, m10, m11
        vphsubw m3, m10, m11

        vphaddw m4, m12, m13
        vphsubw m5, m12, m13

        vphaddw m6, m14, m15
        vphsubw m7, m14, m15

        ;Second stage
        vpaddw m8, m0, m2
        vpaddw m9, m1, m3
        vpsubw m10, m0, m2
        vpsubw m11, m1, m3

        vpaddw m12, m4, m6
        vpaddw m13, m5, m7
        vpsubw m14, m4, m6
        vpsubw m15, m5, m7

        ;Third stage
        vpaddw m0, m8, m12
        vpaddw m1, m9, m13
        vpaddw m2, m10, m14
        vpaddw m3, m11, m15

        vpsubw m4, m8, m12
        vpsubw m5, m9, m13
        vpsubw m6, m10, m14
        vpsubw m7, m11, m15

    %else
        ;Transformed values are in registers m4-m7
        ;and in memory(temp0-temp3). Transformed values
        ;are written in m4-m7. Also calculate absolute
        ;values for them and accumulate into ymm0.

        ;First stage
        vphaddw m0, m4, m5
        vphsubw m1, m4, m5

        vphaddw m2, m6, m7
        vphsubw m3, m6, m7

        ;Second stage
        vpaddw m4, m0, m2
        vpaddw m5, m1, m3
        vpsubw m6, m0, m2
        vpsubw m7, m1, m3

        vmovdqu m3, [temp3]
        vmovdqu m2, [temp2]
        vmovdqu m1, [temp1]
        vmovdqu m0, [temp0]

        vmovdqu [temp3], m7
        vmovdqu [temp2], m6
        vmovdqu [temp1], m5
        vmovdqu [temp0], m4

        ;First stage (second half)
        vphaddw m4, m0, m1
        vphsubw m5, m0, m1

        vphaddw m6, m2, m3
        vphsubw m7, m2, m3

        ;Second stage (second half)
        vpaddw m0, m4, m6
        vpaddw m1, m5, m7
        vpsubw m2, m4, m6
        vpsubw m3, m5, m7

        ;Third stage
        vpaddw m4, m0, [temp0]
        vpaddw m5, m1, [temp1]
        vpsubw m6, m0, [temp0]
        vpsubw m7, m1, [temp1]

        ;Calculate the absolute values and
        ;zero extend 16-bit values to 32-bit
        ;values. Then sum the values.

        vpabsw m4, m4
        KVZ_ZERO_EXTEND_WD m4, m4, m1
        vpaddd m4, m1

        vpabsw m5, m5
        KVZ_ZERO_EXTEND_WD m5, m5, m1
        vpaddd m5, m1

        vpabsw m6, m6
        KVZ_ZERO_EXTEND_WD m6, m6, m1
        vpaddd m6, m1

        vpabsw m7, m7
        KVZ_ZERO_EXTEND_WD m7, m7, m1
        vpaddd m7, m1
      
        vpaddd m0, m4, m5
        vpaddd m0, m6
        vpaddd m0, m7

        ;Repeat for the rest
        vpaddw m4, m2, [temp2]
        vpaddw m5, m3, [temp3]
        vpsubw m6, m2, [temp2]
        vpsubw m7, m3, [temp3]

        vpabsw m4, m4
        KVZ_ZERO_EXTEND_WD m4, m4, m1
        vpaddd m4, m1

        vpabsw m5, m5
        KVZ_ZERO_EXTEND_WD m5, m5, m1
        vpaddd m5, m1

        vpabsw m6, m6
        KVZ_ZERO_EXTEND_WD m6, m6, m1
        vpaddd m6, m1

        vpabsw m7, m7
        KVZ_ZERO_EXTEND_WD m7, m7, m1
        vpaddd m7, m1

        ;Sum the other half of the packed results to ymm4
        vpaddd m4, m5
        vpaddd m4, m6
        vpaddd m4, m7

        ;Sum all packed results to ymm0
        vpaddd m0, m4

    %endif

    %if ARCH_X86_64
    
        ;Calculate the absolute values and
        ;zero extend 16-bit values to 32-bit
        ;values. In other words: extend xmm to
        ;corresponding ymm.

        vpabsw m0, m0
        KVZ_ZERO_EXTEND_WD m0, m0, m8
        vpaddd m0, m8

        vpabsw m1, m1
        KVZ_ZERO_EXTEND_WD m1, m1, m8
        vpaddd m1, m8

        vpabsw m2, m2
        KVZ_ZERO_EXTEND_WD m2, m2, m8
        vpaddd m1, m8

        vpabsw m3, m3
        KVZ_ZERO_EXTEND_WD m3, m3, m8
        vpaddd m3, m8

        vpabsw m4, m4
        KVZ_ZERO_EXTEND_WD m4, m4, m8
        vpaddd m4, m8

        vpabsw m5, m5
        KVZ_ZERO_EXTEND_WD m5, m5, m8
        vpaddd m5, m8

        vpabsw m6, m6
        KVZ_ZERO_EXTEND_WD m6, m6, m8
        vpaddd m6, m8

        vpabsw m7, m7
        KVZ_ZERO_EXTEND_WD m7, m7, m8
        vpaddd m7, m8

        ;Calculate packed sum of transformed values to ymm0
        vpaddd m0, m1
        vpaddd m0, m2
        vpaddd m0, m3
        vpaddd m0, m4
        vpaddd m0, m5
        vpaddd m0, m6
        vpaddd m0, m7
    %endif

    ;Sum the packed values to m0[32:0]
    vphaddd m0, m0
    vphaddd m0, m0

    ;The result is in the lowest 32 bits in m0
    vmovd r4d, m0

    ;8x8 Hadamard transform requires
    ;adding 2 and dividing by 4
    add r4, 2
    shr r4, 2

    ;Zero high 128 bits of ymm registers to
    ;prevent AVX-SSE transition penalty.
    vzeroupper

    %if ARCH_X86_64 == 0
        add esp, 16*4
    %endif

%endmacro ; KVZ_SATD_8X8_STRIDE

;KVZ_SATD_4X4
;Calculates SATD of the 16 consequtive bytes in memory
;r0 address of the first value(current)
;r1 address of the first value(reference)

cglobal satd_4x4, 2, 2, 6

    ;Load 8 bytes from memory and zero extend
    ;to 16-bit values. Calculate difference.
    vpmovzxbw m0, [r0]
    vpmovzxbw m2, [r1]
    vpsubw m0, m2

    vpmovzxbw m1, [r0+8]
    vpmovzxbw m3, [r1+8]
    vpsubw m1, m3

    ;Hadamard transform
    ;Horizontal phase
    ;First stage
    vphaddw m4, m0, m1
    vphsubw m5, m0, m1
    ;Second stage
    vphaddw m0, m4, m5
    vphsubw m1, m4, m5

    ;Vertical phase
    ;First stage
    vphaddw m4, m0, m1
    vphsubw m5, m0, m1
    ;Second stage
    vphaddw m0, m4, m5
    vphsubw m1, m4, m5

    ;Calculate absolute values
    vpabsw m0, m0
    vpabsw m1, m1

    ;Sum the all the transformed values
    vpaddw m0, m1

    vphaddw m0, m0
    vphaddw m0, m0
    vphaddw m0, m0

    ;Extract the lowest 16 bits of m0
    ;into eax
    vpextrw eax, m0, 0

    ;4x4 Hadamard transform requires
    ;Addition of 1 and division by 2
    add eax, 1
    shr eax, 1

    RET



;KVZ_SATD_8X8
;Calculates SATD of a 8x8 block inside a frame with stride
;r0 address of the first value(reference)
;r1 address of the first value(current)
;r2 stride

%if ARCH_X86_64
    cglobal satd_8x8, 4, 5, 16
%else
    cglobal satd_8x8, 4, 5, 8
%endif
    
    ;Set arguments
    mov r2, r1
    mov r1, 8
    mov r3, 8
    
    ;Calculate 8x8 SATD. Result is written
    ;in the register r4.
    KVZ_SATD_8X8_STRIDE
    mov rax, r4
    RET

;KVZ_SATD_NXN
;Calculates SATD of a NxN block inside a frame with stride
;r0 address of the first value(reference)
;r1 address of the first value(current)

%macro KVZ_SATD_NXN 1

    %if ARCH_X86_64
        cglobal satd_%1x%1, 2, 7, 16
    %else
        cglobal satd_%1x%1, 2, 7, 8
    %endif
    
    ;Set arguments
    mov r2, r1
    mov r1, %1
    mov r3, %1
    
    ;Zero r5 and r6
    xor r5, r5
    xor r6, r6

    ;Calculate SATDs of each 8x8 sub-blocks
    ;and accumulate the results in r6. Repeat yloop
    ;N times. Repeat xloop N times. r4 and r5 are counters
    ;for the loops.
    
    .yloop
        
        ;zero r4
        xor r4, r4

        .xloop
            push r4
        
            ;Calculate SATD of the sub-block. Result is
            ;written in the register r4.
            KVZ_SATD_8X8_STRIDE
            add r6, r4

            ;Set r2 and r0 to the next sub-block
            ;on the same row
            sub r2, 6*%1-8
            sub r0, 6*%1-8

            pop r4
            add r4, 8
            cmp r4, %1
        jne .xloop

        ;Set r2 and r0 to the first sub-block
        ;on the next row(of 8x8 sub-blocks)
        add r2, 7*%1
        add r0, 7*%1

        add r5, 8
        cmp r5, %1
    jne .yloop

    mov rax, r6
    RET

%endmacro ; KVZ_SATD_NXN

KVZ_SATD_NXN 16
KVZ_SATD_NXN 32
KVZ_SATD_NXN 64
