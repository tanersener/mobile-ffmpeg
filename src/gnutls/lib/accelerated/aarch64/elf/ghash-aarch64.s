# Copyright (c) 2011-2016, Andy Polyakov <appro@openssl.org>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 
#     * Redistributions of source code must retain copyright notices,
#      this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above
#      copyright notice, this list of conditions and the following
#      disclaimer in the documentation and/or other materials
#      provided with the distribution.
#
#     * Neither the name of the Andy Polyakov nor the names of its
#      copyright holder and contributors may be used to endorse or
#      promote products derived from this software without specific
#      prior written permission.
#
# ALTERNATIVELY, provided that this notice is retained in full, this
# product may be distributed under the terms of the GNU General Public
# License (GPL), in which case the provisions of the GPL apply INSTEAD OF
# those given above.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# *** This file is auto-generated ***
#
# 1 "lib/accelerated/aarch64/elf/ghash-aarch64.s.tmp.S"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "lib/accelerated/aarch64/elf/ghash-aarch64.s.tmp.S"
# 1 "lib/accelerated/aarch64/aarch64-common.h" 1
# 2 "lib/accelerated/aarch64/elf/ghash-aarch64.s.tmp.S" 2

.text
.arch armv8-a+crypto
.globl gcm_init_v8
.type gcm_init_v8,%function
.align 4
gcm_init_v8:
 ld1 {v17.2d},[x1]
 movi v19.16b,#0xe1
 shl v19.2d,v19.2d,#57
 ext v3.16b,v17.16b,v17.16b,#8
 ushr v18.2d,v19.2d,#63
 dup v17.4s,v17.s[1]
 ext v16.16b,v18.16b,v19.16b,#8
 ushr v18.2d,v3.2d,#63
 sshr v17.4s,v17.4s,#31
 and v18.16b,v18.16b,v16.16b
 shl v3.2d,v3.2d,#1
 ext v18.16b,v18.16b,v18.16b,#8
 and v16.16b,v16.16b,v17.16b
 orr v3.16b,v3.16b,v18.16b
 eor v20.16b,v3.16b,v16.16b
 st1 {v20.2d},[x0],#16


 ext v16.16b,v20.16b,v20.16b,#8
 pmull v0.1q,v20.1d,v20.1d
 eor v16.16b,v16.16b,v20.16b
 pmull2 v2.1q,v20.2d,v20.2d
 pmull v1.1q,v16.1d,v16.1d

 ext v17.16b,v0.16b,v2.16b,#8
 eor v18.16b,v0.16b,v2.16b
 eor v1.16b,v1.16b,v17.16b
 eor v1.16b,v1.16b,v18.16b
 pmull v18.1q,v0.1d,v19.1d

 ins v2.d[0],v1.d[1]
 ins v1.d[1],v0.d[0]
 eor v0.16b,v1.16b,v18.16b

 ext v18.16b,v0.16b,v0.16b,#8
 pmull v0.1q,v0.1d,v19.1d
 eor v18.16b,v18.16b,v2.16b
 eor v22.16b,v0.16b,v18.16b

 ext v17.16b,v22.16b,v22.16b,#8
 eor v17.16b,v17.16b,v22.16b
 ext v21.16b,v16.16b,v17.16b,#8
 st1 {v21.2d,v22.2d},[x0]

 ret
.size gcm_init_v8,.-gcm_init_v8
.globl gcm_gmult_v8
.type gcm_gmult_v8,%function
.align 4
gcm_gmult_v8:
 ld1 {v17.2d},[x0]
 movi v19.16b,#0xe1
 ld1 {v20.2d,v21.2d},[x1]
 shl v19.2d,v19.2d,#57

 rev64 v17.16b,v17.16b

 ext v3.16b,v17.16b,v17.16b,#8

 pmull v0.1q,v20.1d,v3.1d
 eor v17.16b,v17.16b,v3.16b
 pmull2 v2.1q,v20.2d,v3.2d
 pmull v1.1q,v21.1d,v17.1d

 ext v17.16b,v0.16b,v2.16b,#8
 eor v18.16b,v0.16b,v2.16b
 eor v1.16b,v1.16b,v17.16b
 eor v1.16b,v1.16b,v18.16b
 pmull v18.1q,v0.1d,v19.1d

 ins v2.d[0],v1.d[1]
 ins v1.d[1],v0.d[0]
 eor v0.16b,v1.16b,v18.16b

 ext v18.16b,v0.16b,v0.16b,#8
 pmull v0.1q,v0.1d,v19.1d
 eor v18.16b,v18.16b,v2.16b
 eor v0.16b,v0.16b,v18.16b


 rev64 v0.16b,v0.16b

 ext v0.16b,v0.16b,v0.16b,#8
 st1 {v0.2d},[x0]

 ret
.size gcm_gmult_v8,.-gcm_gmult_v8
.globl gcm_ghash_v8
.type gcm_ghash_v8,%function
.align 4
gcm_ghash_v8:
 ld1 {v0.2d},[x0]





 subs x3,x3,#32
 mov x12,#16
# 116 "lib/accelerated/aarch64/elf/ghash-aarch64.s.tmp.S"
 ld1 {v20.2d,v21.2d},[x1],#32
 movi v19.16b,#0xe1
 ld1 {v22.2d},[x1]
 csel x12,xzr,x12,eq
 ext v0.16b,v0.16b,v0.16b,#8
 ld1 {v16.2d},[x2],#16
 shl v19.2d,v19.2d,#57

 rev64 v16.16b,v16.16b
 rev64 v0.16b,v0.16b

 ext v3.16b,v16.16b,v16.16b,#8
 b.lo .Lodd_tail_v8
 ld1 {v17.2d},[x2],x12

 rev64 v17.16b,v17.16b

 ext v7.16b,v17.16b,v17.16b,#8
 eor v3.16b,v3.16b,v0.16b
 pmull v4.1q,v20.1d,v7.1d
 eor v17.16b,v17.16b,v7.16b
 pmull2 v6.1q,v20.2d,v7.2d
 b .Loop_mod2x_v8

.align 4
.Loop_mod2x_v8:
 ext v18.16b,v3.16b,v3.16b,#8
 subs x3,x3,#32
 pmull v0.1q,v22.1d,v3.1d
 csel x12,xzr,x12,lo

 pmull v5.1q,v21.1d,v17.1d
 eor v18.16b,v18.16b,v3.16b
 pmull2 v2.1q,v22.2d,v3.2d
 eor v0.16b,v0.16b,v4.16b
 pmull2 v1.1q,v21.2d,v18.2d
 ld1 {v16.2d},[x2],x12

 eor v2.16b,v2.16b,v6.16b
 csel x12,xzr,x12,eq
 eor v1.16b,v1.16b,v5.16b

 ext v17.16b,v0.16b,v2.16b,#8
 eor v18.16b,v0.16b,v2.16b
 eor v1.16b,v1.16b,v17.16b
 ld1 {v17.2d},[x2],x12

 rev64 v16.16b,v16.16b

 eor v1.16b,v1.16b,v18.16b
 pmull v18.1q,v0.1d,v19.1d


 rev64 v17.16b,v17.16b

 ins v2.d[0],v1.d[1]
 ins v1.d[1],v0.d[0]
 ext v7.16b,v17.16b,v17.16b,#8
 ext v3.16b,v16.16b,v16.16b,#8
 eor v0.16b,v1.16b,v18.16b
 pmull v4.1q,v20.1d,v7.1d
 eor v3.16b,v3.16b,v2.16b

 ext v18.16b,v0.16b,v0.16b,#8
 pmull v0.1q,v0.1d,v19.1d
 eor v3.16b,v3.16b,v18.16b
 eor v17.16b,v17.16b,v7.16b
 eor v3.16b,v3.16b,v0.16b
 pmull2 v6.1q,v20.2d,v7.2d
 b.hs .Loop_mod2x_v8

 eor v2.16b,v2.16b,v18.16b
 ext v3.16b,v16.16b,v16.16b,#8
 adds x3,x3,#32
 eor v0.16b,v0.16b,v2.16b
 b.eq .Ldone_v8
.Lodd_tail_v8:
 ext v18.16b,v0.16b,v0.16b,#8
 eor v3.16b,v3.16b,v0.16b
 eor v17.16b,v16.16b,v18.16b

 pmull v0.1q,v20.1d,v3.1d
 eor v17.16b,v17.16b,v3.16b
 pmull2 v2.1q,v20.2d,v3.2d
 pmull v1.1q,v21.1d,v17.1d

 ext v17.16b,v0.16b,v2.16b,#8
 eor v18.16b,v0.16b,v2.16b
 eor v1.16b,v1.16b,v17.16b
 eor v1.16b,v1.16b,v18.16b
 pmull v18.1q,v0.1d,v19.1d

 ins v2.d[0],v1.d[1]
 ins v1.d[1],v0.d[0]
 eor v0.16b,v1.16b,v18.16b

 ext v18.16b,v0.16b,v0.16b,#8
 pmull v0.1q,v0.1d,v19.1d
 eor v18.16b,v18.16b,v2.16b
 eor v0.16b,v0.16b,v18.16b

.Ldone_v8:

 rev64 v0.16b,v0.16b

 ext v0.16b,v0.16b,v0.16b,#8
 st1 {v0.2d},[x0]

 ret
.size gcm_ghash_v8,.-gcm_ghash_v8
.byte 71,72,65,83,72,32,102,111,114,32,65,82,77,118,56,44,32,67,82,89,80,84,79,71,65,77,83,32,98,121,32,60,97,112,112,114,111,64,111,112,101,110,115,115,108,46,111,114,103,62,0
.align 2
.align 2
.section .note.GNU-stack,"",%progbits
