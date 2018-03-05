# Copyright (c) 2011-2013, Andy Polyakov <appro@openssl.org>
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
.text	
.globl	padlock_capability
.def	padlock_capability;	.scl 2;	.type 32;	.endef
.p2align	4
padlock_capability:
	movq	%rbx,%r8
	xorl	%eax,%eax
	cpuid
	xorl	%eax,%eax
	cmpl	$1953391939,%ebx
	jne	.Lnoluck
	cmpl	$1215460705,%edx
	jne	.Lnoluck
	cmpl	$1936487777,%ecx
	jne	.Lnoluck
	movl	$3221225472,%eax
	cpuid
	movl	%eax,%edx
	xorl	%eax,%eax
	cmpl	$3221225473,%edx
	jb	.Lnoluck
	movl	$3221225473,%eax
	cpuid
	movl	%edx,%eax
	andl	$4294967279,%eax
	orl	$16,%eax
.Lnoluck:
	movq	%r8,%rbx
	.byte	0xf3,0xc3


.globl	padlock_key_bswap
.def	padlock_key_bswap;	.scl 2;	.type 32;	.endef
.p2align	4
padlock_key_bswap:
	movl	240(%rcx),%edx
.Lbswap_loop:
	movl	(%rcx),%eax
	bswapl	%eax
	movl	%eax,(%rcx)
	leaq	4(%rcx),%rcx
	subl	$1,%edx
	jnz	.Lbswap_loop
	.byte	0xf3,0xc3


.globl	padlock_verify_context
.def	padlock_verify_context;	.scl 2;	.type 32;	.endef
.p2align	4
padlock_verify_context:
	movq	%rcx,%rdx
	pushf
	leaq	.Lpadlock_saved_context(%rip),%rax
	call	_padlock_verify_ctx
	leaq	8(%rsp),%rsp
	.byte	0xf3,0xc3


.def	_padlock_verify_ctx;	.scl 3;	.type 32;	.endef
.p2align	4
_padlock_verify_ctx:
	movq	8(%rsp),%r8
	btq	$30,%r8
	jnc	.Lverified
	cmpq	(%rax),%rdx
	je	.Lverified
	pushf
	popf
.Lverified:
	movq	%rdx,(%rax)
	.byte	0xf3,0xc3


.globl	padlock_reload_key
.def	padlock_reload_key;	.scl 2;	.type 32;	.endef
.p2align	4
padlock_reload_key:
	pushf
	popf
	.byte	0xf3,0xc3


.globl	padlock_aes_block
.def	padlock_aes_block;	.scl 2;	.type 32;	.endef
.p2align	4
padlock_aes_block:
	movq	%rdi,8(%rsp)
	movq	%rsi,16(%rsp)
	movq	%rsp,%rax
.LSEH_begin_padlock_aes_block:
	movq	%rcx,%rdi
	movq	%rdx,%rsi
	movq	%r8,%rdx

	movq	%rbx,%r8
	movq	$1,%rcx
	leaq	32(%rdx),%rbx
	leaq	16(%rdx),%rdx
.byte	0xf3,0x0f,0xa7,0xc8	
	movq	%r8,%rbx
	movq	8(%rsp),%rdi
	movq	16(%rsp),%rsi
	.byte	0xf3,0xc3
.LSEH_end_padlock_aes_block:

.globl	padlock_xstore
.def	padlock_xstore;	.scl 2;	.type 32;	.endef
.p2align	4
padlock_xstore:
	movq	%rdi,8(%rsp)
	movq	%rsi,16(%rsp)
	movq	%rsp,%rax
.LSEH_begin_padlock_xstore:
	movq	%rcx,%rdi
	movq	%rdx,%rsi

	movl	%esi,%edx
.byte	0x0f,0xa7,0xc0		
	movq	8(%rsp),%rdi
	movq	16(%rsp),%rsi
	.byte	0xf3,0xc3
.LSEH_end_padlock_xstore:

.globl	padlock_sha1_oneshot
.def	padlock_sha1_oneshot;	.scl 2;	.type 32;	.endef
.p2align	4
padlock_sha1_oneshot:
	movq	%rdi,8(%rsp)
	movq	%rsi,16(%rsp)
	movq	%rsp,%rax
.LSEH_begin_padlock_sha1_oneshot:
	movq	%rcx,%rdi
	movq	%rdx,%rsi
	movq	%r8,%rdx

	movq	%rdx,%rcx
	movq	%rdi,%rdx
	movups	(%rdi),%xmm0
	subq	$128+8,%rsp
	movl	16(%rdi),%eax
	movaps	%xmm0,(%rsp)
	movq	%rsp,%rdi
	movl	%eax,16(%rsp)
	xorq	%rax,%rax
.byte	0xf3,0x0f,0xa6,0xc8	
	movaps	(%rsp),%xmm0
	movl	16(%rsp),%eax
	addq	$128+8,%rsp
	movups	%xmm0,(%rdx)
	movl	%eax,16(%rdx)
	movq	8(%rsp),%rdi
	movq	16(%rsp),%rsi
	.byte	0xf3,0xc3
.LSEH_end_padlock_sha1_oneshot:

.globl	padlock_sha1_blocks
.def	padlock_sha1_blocks;	.scl 2;	.type 32;	.endef
.p2align	4
padlock_sha1_blocks:
	movq	%rdi,8(%rsp)
	movq	%rsi,16(%rsp)
	movq	%rsp,%rax
.LSEH_begin_padlock_sha1_blocks:
	movq	%rcx,%rdi
	movq	%rdx,%rsi
	movq	%r8,%rdx

	movq	%rdx,%rcx
	movq	%rdi,%rdx
	movups	(%rdi),%xmm0
	subq	$128+8,%rsp
	movl	16(%rdi),%eax
	movaps	%xmm0,(%rsp)
	movq	%rsp,%rdi
	movl	%eax,16(%rsp)
	movq	$-1,%rax
.byte	0xf3,0x0f,0xa6,0xc8	
	movaps	(%rsp),%xmm0
	movl	16(%rsp),%eax
	addq	$128+8,%rsp
	movups	%xmm0,(%rdx)
	movl	%eax,16(%rdx)
	movq	8(%rsp),%rdi
	movq	16(%rsp),%rsi
	.byte	0xf3,0xc3
.LSEH_end_padlock_sha1_blocks:

.globl	padlock_sha256_oneshot
.def	padlock_sha256_oneshot;	.scl 2;	.type 32;	.endef
.p2align	4
padlock_sha256_oneshot:
	movq	%rdi,8(%rsp)
	movq	%rsi,16(%rsp)
	movq	%rsp,%rax
.LSEH_begin_padlock_sha256_oneshot:
	movq	%rcx,%rdi
	movq	%rdx,%rsi
	movq	%r8,%rdx

	movq	%rdx,%rcx
	movq	%rdi,%rdx
	movups	(%rdi),%xmm0
	subq	$128+8,%rsp
	movups	16(%rdi),%xmm1
	movaps	%xmm0,(%rsp)
	movq	%rsp,%rdi
	movaps	%xmm1,16(%rsp)
	xorq	%rax,%rax
.byte	0xf3,0x0f,0xa6,0xd0	
	movaps	(%rsp),%xmm0
	movaps	16(%rsp),%xmm1
	addq	$128+8,%rsp
	movups	%xmm0,(%rdx)
	movups	%xmm1,16(%rdx)
	movq	8(%rsp),%rdi
	movq	16(%rsp),%rsi
	.byte	0xf3,0xc3
.LSEH_end_padlock_sha256_oneshot:

.globl	padlock_sha256_blocks
.def	padlock_sha256_blocks;	.scl 2;	.type 32;	.endef
.p2align	4
padlock_sha256_blocks:
	movq	%rdi,8(%rsp)
	movq	%rsi,16(%rsp)
	movq	%rsp,%rax
.LSEH_begin_padlock_sha256_blocks:
	movq	%rcx,%rdi
	movq	%rdx,%rsi
	movq	%r8,%rdx

	movq	%rdx,%rcx
	movq	%rdi,%rdx
	movups	(%rdi),%xmm0
	subq	$128+8,%rsp
	movups	16(%rdi),%xmm1
	movaps	%xmm0,(%rsp)
	movq	%rsp,%rdi
	movaps	%xmm1,16(%rsp)
	movq	$-1,%rax
.byte	0xf3,0x0f,0xa6,0xd0	
	movaps	(%rsp),%xmm0
	movaps	16(%rsp),%xmm1
	addq	$128+8,%rsp
	movups	%xmm0,(%rdx)
	movups	%xmm1,16(%rdx)
	movq	8(%rsp),%rdi
	movq	16(%rsp),%rsi
	.byte	0xf3,0xc3
.LSEH_end_padlock_sha256_blocks:

.globl	padlock_sha512_blocks
.def	padlock_sha512_blocks;	.scl 2;	.type 32;	.endef
.p2align	4
padlock_sha512_blocks:
	movq	%rdi,8(%rsp)
	movq	%rsi,16(%rsp)
	movq	%rsp,%rax
.LSEH_begin_padlock_sha512_blocks:
	movq	%rcx,%rdi
	movq	%rdx,%rsi
	movq	%r8,%rdx

	movq	%rdx,%rcx
	movq	%rdi,%rdx
	movups	(%rdi),%xmm0
	subq	$128+8,%rsp
	movups	16(%rdi),%xmm1
	movups	32(%rdi),%xmm2
	movups	48(%rdi),%xmm3
	movaps	%xmm0,(%rsp)
	movq	%rsp,%rdi
	movaps	%xmm1,16(%rsp)
	movaps	%xmm2,32(%rsp)
	movaps	%xmm3,48(%rsp)
.byte	0xf3,0x0f,0xa6,0xe0	
	movaps	(%rsp),%xmm0
	movaps	16(%rsp),%xmm1
	movaps	32(%rsp),%xmm2
	movaps	48(%rsp),%xmm3
	addq	$128+8,%rsp
	movups	%xmm0,(%rdx)
	movups	%xmm1,16(%rdx)
	movups	%xmm2,32(%rdx)
	movups	%xmm3,48(%rdx)
	movq	8(%rsp),%rdi
	movq	16(%rsp),%rsi
	.byte	0xf3,0xc3
.LSEH_end_padlock_sha512_blocks:
.globl	padlock_ecb_encrypt
.def	padlock_ecb_encrypt;	.scl 2;	.type 32;	.endef
.p2align	4
padlock_ecb_encrypt:
	movq	%rdi,8(%rsp)
	movq	%rsi,16(%rsp)
	movq	%rsp,%rax
.LSEH_begin_padlock_ecb_encrypt:
	movq	%rcx,%rdi
	movq	%rdx,%rsi
	movq	%r8,%rdx
	movq	%r9,%rcx

	pushq	%rbp
	pushq	%rbx

	xorl	%eax,%eax
	testq	$15,%rdx
	jnz	.Lecb_abort
	testq	$15,%rcx
	jnz	.Lecb_abort
	leaq	.Lpadlock_saved_context(%rip),%rax
	pushf
	cld
	call	_padlock_verify_ctx
	leaq	16(%rdx),%rdx
	xorl	%eax,%eax
	xorl	%ebx,%ebx
	testl	$32,(%rdx)
	jnz	.Lecb_aligned
	testq	$15,%rdi
	setz	%al
	testq	$15,%rsi
	setz	%bl
	testl	%ebx,%eax
	jnz	.Lecb_aligned
	negq	%rax
	movq	$512,%rbx
	notq	%rax
	leaq	(%rsp),%rbp
	cmpq	%rbx,%rcx
	cmovcq	%rcx,%rbx
	andq	%rbx,%rax
	movq	%rcx,%rbx
	negq	%rax
	andq	$512-1,%rbx
	leaq	(%rax,%rbp,1),%rsp
	movq	$512,%rax
	cmovzq	%rax,%rbx
	cmpq	%rbx,%rcx
	ja	.Lecb_loop
	movq	%rsi,%rax
	cmpq	%rsp,%rbp
	cmoveq	%rdi,%rax
	addq	%rcx,%rax
	negq	%rax
	andq	$4095,%rax
	cmpq	$128,%rax
	movq	$-128,%rax
	cmovaeq	%rbx,%rax
	andq	%rax,%rbx
	jz	.Lecb_unaligned_tail
	jmp	.Lecb_loop
.p2align	4
.Lecb_loop:
	cmpq	%rcx,%rbx
	cmovaq	%rcx,%rbx
	movq	%rdi,%r8
	movq	%rsi,%r9
	movq	%rcx,%r10
	movq	%rbx,%rcx
	movq	%rbx,%r11
	testq	$15,%rdi
	cmovnzq	%rsp,%rdi
	testq	$15,%rsi
	jz	.Lecb_inp_aligned
	shrq	$3,%rcx
.byte	0xf3,0x48,0xa5		
	subq	%rbx,%rdi
	movq	%rbx,%rcx
	movq	%rdi,%rsi
.Lecb_inp_aligned:
	leaq	-16(%rdx),%rax
	leaq	16(%rdx),%rbx
	shrq	$4,%rcx
.byte	0xf3,0x0f,0xa7,200	
	movq	%r8,%rdi
	movq	%r11,%rbx
	testq	$15,%rdi
	jz	.Lecb_out_aligned
	movq	%rbx,%rcx
	leaq	(%rsp),%rsi
	shrq	$3,%rcx
.byte	0xf3,0x48,0xa5		
	subq	%rbx,%rdi
.Lecb_out_aligned:
	movq	%r9,%rsi
	movq	%r10,%rcx
	addq	%rbx,%rdi
	addq	%rbx,%rsi
	subq	%rbx,%rcx
	movq	$512,%rbx
	jz	.Lecb_break
	cmpq	%rbx,%rcx
	jae	.Lecb_loop
.Lecb_unaligned_tail:
	xorl	%eax,%eax
	cmpq	%rsp,%rbp
	cmoveq	%rcx,%rax
	movq	%rdi,%r8
	movq	%rcx,%rbx
	subq	%rax,%rsp
	shrq	$3,%rcx
	leaq	(%rsp),%rdi
.byte	0xf3,0x48,0xa5		
	movq	%rsp,%rsi
	movq	%r8,%rdi
	movq	%rbx,%rcx
	jmp	.Lecb_loop
.p2align	4
.Lecb_break:
	cmpq	%rbp,%rsp
	je	.Lecb_done

	pxor	%xmm0,%xmm0
	leaq	(%rsp),%rax
.Lecb_bzero:
	movaps	%xmm0,(%rax)
	leaq	16(%rax),%rax
	cmpq	%rax,%rbp
	ja	.Lecb_bzero

.Lecb_done:
	leaq	(%rbp),%rsp
	jmp	.Lecb_exit

.p2align	4
.Lecb_aligned:
	leaq	(%rsi,%rcx,1),%rbp
	negq	%rbp
	andq	$4095,%rbp
	xorl	%eax,%eax
	cmpq	$128,%rbp
	movq	$128-1,%rbp
	cmovaeq	%rax,%rbp
	andq	%rcx,%rbp
	subq	%rbp,%rcx
	jz	.Lecb_aligned_tail
	leaq	-16(%rdx),%rax
	leaq	16(%rdx),%rbx
	shrq	$4,%rcx
.byte	0xf3,0x0f,0xa7,200	
	testq	%rbp,%rbp
	jz	.Lecb_exit

.Lecb_aligned_tail:
	movq	%rdi,%r8
	movq	%rbp,%rbx
	movq	%rbp,%rcx
	leaq	(%rsp),%rbp
	subq	%rcx,%rsp
	shrq	$3,%rcx
	leaq	(%rsp),%rdi
.byte	0xf3,0x48,0xa5		
	leaq	(%r8),%rdi
	leaq	(%rsp),%rsi
	movq	%rbx,%rcx
	jmp	.Lecb_loop
.Lecb_exit:
	movl	$1,%eax
	leaq	8(%rsp),%rsp
.Lecb_abort:
	popq	%rbx
	popq	%rbp
	movq	8(%rsp),%rdi
	movq	16(%rsp),%rsi
	.byte	0xf3,0xc3
.LSEH_end_padlock_ecb_encrypt:
.globl	padlock_cbc_encrypt
.def	padlock_cbc_encrypt;	.scl 2;	.type 32;	.endef
.p2align	4
padlock_cbc_encrypt:
	movq	%rdi,8(%rsp)
	movq	%rsi,16(%rsp)
	movq	%rsp,%rax
.LSEH_begin_padlock_cbc_encrypt:
	movq	%rcx,%rdi
	movq	%rdx,%rsi
	movq	%r8,%rdx
	movq	%r9,%rcx

	pushq	%rbp
	pushq	%rbx

	xorl	%eax,%eax
	testq	$15,%rdx
	jnz	.Lcbc_abort
	testq	$15,%rcx
	jnz	.Lcbc_abort
	leaq	.Lpadlock_saved_context(%rip),%rax
	pushf
	cld
	call	_padlock_verify_ctx
	leaq	16(%rdx),%rdx
	xorl	%eax,%eax
	xorl	%ebx,%ebx
	testl	$32,(%rdx)
	jnz	.Lcbc_aligned
	testq	$15,%rdi
	setz	%al
	testq	$15,%rsi
	setz	%bl
	testl	%ebx,%eax
	jnz	.Lcbc_aligned
	negq	%rax
	movq	$512,%rbx
	notq	%rax
	leaq	(%rsp),%rbp
	cmpq	%rbx,%rcx
	cmovcq	%rcx,%rbx
	andq	%rbx,%rax
	movq	%rcx,%rbx
	negq	%rax
	andq	$512-1,%rbx
	leaq	(%rax,%rbp,1),%rsp
	movq	$512,%rax
	cmovzq	%rax,%rbx
	cmpq	%rbx,%rcx
	ja	.Lcbc_loop
	movq	%rsi,%rax
	cmpq	%rsp,%rbp
	cmoveq	%rdi,%rax
	addq	%rcx,%rax
	negq	%rax
	andq	$4095,%rax
	cmpq	$64,%rax
	movq	$-64,%rax
	cmovaeq	%rbx,%rax
	andq	%rax,%rbx
	jz	.Lcbc_unaligned_tail
	jmp	.Lcbc_loop
.p2align	4
.Lcbc_loop:
	cmpq	%rcx,%rbx
	cmovaq	%rcx,%rbx
	movq	%rdi,%r8
	movq	%rsi,%r9
	movq	%rcx,%r10
	movq	%rbx,%rcx
	movq	%rbx,%r11
	testq	$15,%rdi
	cmovnzq	%rsp,%rdi
	testq	$15,%rsi
	jz	.Lcbc_inp_aligned
	shrq	$3,%rcx
.byte	0xf3,0x48,0xa5		
	subq	%rbx,%rdi
	movq	%rbx,%rcx
	movq	%rdi,%rsi
.Lcbc_inp_aligned:
	leaq	-16(%rdx),%rax
	leaq	16(%rdx),%rbx
	shrq	$4,%rcx
.byte	0xf3,0x0f,0xa7,208	
	movdqa	(%rax),%xmm0
	movdqa	%xmm0,-16(%rdx)
	movq	%r8,%rdi
	movq	%r11,%rbx
	testq	$15,%rdi
	jz	.Lcbc_out_aligned
	movq	%rbx,%rcx
	leaq	(%rsp),%rsi
	shrq	$3,%rcx
.byte	0xf3,0x48,0xa5		
	subq	%rbx,%rdi
.Lcbc_out_aligned:
	movq	%r9,%rsi
	movq	%r10,%rcx
	addq	%rbx,%rdi
	addq	%rbx,%rsi
	subq	%rbx,%rcx
	movq	$512,%rbx
	jz	.Lcbc_break
	cmpq	%rbx,%rcx
	jae	.Lcbc_loop
.Lcbc_unaligned_tail:
	xorl	%eax,%eax
	cmpq	%rsp,%rbp
	cmoveq	%rcx,%rax
	movq	%rdi,%r8
	movq	%rcx,%rbx
	subq	%rax,%rsp
	shrq	$3,%rcx
	leaq	(%rsp),%rdi
.byte	0xf3,0x48,0xa5		
	movq	%rsp,%rsi
	movq	%r8,%rdi
	movq	%rbx,%rcx
	jmp	.Lcbc_loop
.p2align	4
.Lcbc_break:
	cmpq	%rbp,%rsp
	je	.Lcbc_done

	pxor	%xmm0,%xmm0
	leaq	(%rsp),%rax
.Lcbc_bzero:
	movaps	%xmm0,(%rax)
	leaq	16(%rax),%rax
	cmpq	%rax,%rbp
	ja	.Lcbc_bzero

.Lcbc_done:
	leaq	(%rbp),%rsp
	jmp	.Lcbc_exit

.p2align	4
.Lcbc_aligned:
	leaq	(%rsi,%rcx,1),%rbp
	negq	%rbp
	andq	$4095,%rbp
	xorl	%eax,%eax
	cmpq	$64,%rbp
	movq	$64-1,%rbp
	cmovaeq	%rax,%rbp
	andq	%rcx,%rbp
	subq	%rbp,%rcx
	jz	.Lcbc_aligned_tail
	leaq	-16(%rdx),%rax
	leaq	16(%rdx),%rbx
	shrq	$4,%rcx
.byte	0xf3,0x0f,0xa7,208	
	movdqa	(%rax),%xmm0
	movdqa	%xmm0,-16(%rdx)
	testq	%rbp,%rbp
	jz	.Lcbc_exit

.Lcbc_aligned_tail:
	movq	%rdi,%r8
	movq	%rbp,%rbx
	movq	%rbp,%rcx
	leaq	(%rsp),%rbp
	subq	%rcx,%rsp
	shrq	$3,%rcx
	leaq	(%rsp),%rdi
.byte	0xf3,0x48,0xa5		
	leaq	(%r8),%rdi
	leaq	(%rsp),%rsi
	movq	%rbx,%rcx
	jmp	.Lcbc_loop
.Lcbc_exit:
	movl	$1,%eax
	leaq	8(%rsp),%rsp
.Lcbc_abort:
	popq	%rbx
	popq	%rbp
	movq	8(%rsp),%rdi
	movq	16(%rsp),%rsi
	.byte	0xf3,0xc3
.LSEH_end_padlock_cbc_encrypt:
.globl	padlock_cfb_encrypt
.def	padlock_cfb_encrypt;	.scl 2;	.type 32;	.endef
.p2align	4
padlock_cfb_encrypt:
	movq	%rdi,8(%rsp)
	movq	%rsi,16(%rsp)
	movq	%rsp,%rax
.LSEH_begin_padlock_cfb_encrypt:
	movq	%rcx,%rdi
	movq	%rdx,%rsi
	movq	%r8,%rdx
	movq	%r9,%rcx

	pushq	%rbp
	pushq	%rbx

	xorl	%eax,%eax
	testq	$15,%rdx
	jnz	.Lcfb_abort
	testq	$15,%rcx
	jnz	.Lcfb_abort
	leaq	.Lpadlock_saved_context(%rip),%rax
	pushf
	cld
	call	_padlock_verify_ctx
	leaq	16(%rdx),%rdx
	xorl	%eax,%eax
	xorl	%ebx,%ebx
	testl	$32,(%rdx)
	jnz	.Lcfb_aligned
	testq	$15,%rdi
	setz	%al
	testq	$15,%rsi
	setz	%bl
	testl	%ebx,%eax
	jnz	.Lcfb_aligned
	negq	%rax
	movq	$512,%rbx
	notq	%rax
	leaq	(%rsp),%rbp
	cmpq	%rbx,%rcx
	cmovcq	%rcx,%rbx
	andq	%rbx,%rax
	movq	%rcx,%rbx
	negq	%rax
	andq	$512-1,%rbx
	leaq	(%rax,%rbp,1),%rsp
	movq	$512,%rax
	cmovzq	%rax,%rbx
	jmp	.Lcfb_loop
.p2align	4
.Lcfb_loop:
	cmpq	%rcx,%rbx
	cmovaq	%rcx,%rbx
	movq	%rdi,%r8
	movq	%rsi,%r9
	movq	%rcx,%r10
	movq	%rbx,%rcx
	movq	%rbx,%r11
	testq	$15,%rdi
	cmovnzq	%rsp,%rdi
	testq	$15,%rsi
	jz	.Lcfb_inp_aligned
	shrq	$3,%rcx
.byte	0xf3,0x48,0xa5		
	subq	%rbx,%rdi
	movq	%rbx,%rcx
	movq	%rdi,%rsi
.Lcfb_inp_aligned:
	leaq	-16(%rdx),%rax
	leaq	16(%rdx),%rbx
	shrq	$4,%rcx
.byte	0xf3,0x0f,0xa7,224	
	movdqa	(%rax),%xmm0
	movdqa	%xmm0,-16(%rdx)
	movq	%r8,%rdi
	movq	%r11,%rbx
	testq	$15,%rdi
	jz	.Lcfb_out_aligned
	movq	%rbx,%rcx
	leaq	(%rsp),%rsi
	shrq	$3,%rcx
.byte	0xf3,0x48,0xa5		
	subq	%rbx,%rdi
.Lcfb_out_aligned:
	movq	%r9,%rsi
	movq	%r10,%rcx
	addq	%rbx,%rdi
	addq	%rbx,%rsi
	subq	%rbx,%rcx
	movq	$512,%rbx
	jnz	.Lcfb_loop
	cmpq	%rbp,%rsp
	je	.Lcfb_done

	pxor	%xmm0,%xmm0
	leaq	(%rsp),%rax
.Lcfb_bzero:
	movaps	%xmm0,(%rax)
	leaq	16(%rax),%rax
	cmpq	%rax,%rbp
	ja	.Lcfb_bzero

.Lcfb_done:
	leaq	(%rbp),%rsp
	jmp	.Lcfb_exit

.p2align	4
.Lcfb_aligned:
	leaq	-16(%rdx),%rax
	leaq	16(%rdx),%rbx
	shrq	$4,%rcx
.byte	0xf3,0x0f,0xa7,224	
	movdqa	(%rax),%xmm0
	movdqa	%xmm0,-16(%rdx)
.Lcfb_exit:
	movl	$1,%eax
	leaq	8(%rsp),%rsp
.Lcfb_abort:
	popq	%rbx
	popq	%rbp
	movq	8(%rsp),%rdi
	movq	16(%rsp),%rsi
	.byte	0xf3,0xc3
.LSEH_end_padlock_cfb_encrypt:
.globl	padlock_ofb_encrypt
.def	padlock_ofb_encrypt;	.scl 2;	.type 32;	.endef
.p2align	4
padlock_ofb_encrypt:
	movq	%rdi,8(%rsp)
	movq	%rsi,16(%rsp)
	movq	%rsp,%rax
.LSEH_begin_padlock_ofb_encrypt:
	movq	%rcx,%rdi
	movq	%rdx,%rsi
	movq	%r8,%rdx
	movq	%r9,%rcx

	pushq	%rbp
	pushq	%rbx

	xorl	%eax,%eax
	testq	$15,%rdx
	jnz	.Lofb_abort
	testq	$15,%rcx
	jnz	.Lofb_abort
	leaq	.Lpadlock_saved_context(%rip),%rax
	pushf
	cld
	call	_padlock_verify_ctx
	leaq	16(%rdx),%rdx
	xorl	%eax,%eax
	xorl	%ebx,%ebx
	testl	$32,(%rdx)
	jnz	.Lofb_aligned
	testq	$15,%rdi
	setz	%al
	testq	$15,%rsi
	setz	%bl
	testl	%ebx,%eax
	jnz	.Lofb_aligned
	negq	%rax
	movq	$512,%rbx
	notq	%rax
	leaq	(%rsp),%rbp
	cmpq	%rbx,%rcx
	cmovcq	%rcx,%rbx
	andq	%rbx,%rax
	movq	%rcx,%rbx
	negq	%rax
	andq	$512-1,%rbx
	leaq	(%rax,%rbp,1),%rsp
	movq	$512,%rax
	cmovzq	%rax,%rbx
	jmp	.Lofb_loop
.p2align	4
.Lofb_loop:
	cmpq	%rcx,%rbx
	cmovaq	%rcx,%rbx
	movq	%rdi,%r8
	movq	%rsi,%r9
	movq	%rcx,%r10
	movq	%rbx,%rcx
	movq	%rbx,%r11
	testq	$15,%rdi
	cmovnzq	%rsp,%rdi
	testq	$15,%rsi
	jz	.Lofb_inp_aligned
	shrq	$3,%rcx
.byte	0xf3,0x48,0xa5		
	subq	%rbx,%rdi
	movq	%rbx,%rcx
	movq	%rdi,%rsi
.Lofb_inp_aligned:
	leaq	-16(%rdx),%rax
	leaq	16(%rdx),%rbx
	shrq	$4,%rcx
.byte	0xf3,0x0f,0xa7,232	
	movdqa	(%rax),%xmm0
	movdqa	%xmm0,-16(%rdx)
	movq	%r8,%rdi
	movq	%r11,%rbx
	testq	$15,%rdi
	jz	.Lofb_out_aligned
	movq	%rbx,%rcx
	leaq	(%rsp),%rsi
	shrq	$3,%rcx
.byte	0xf3,0x48,0xa5		
	subq	%rbx,%rdi
.Lofb_out_aligned:
	movq	%r9,%rsi
	movq	%r10,%rcx
	addq	%rbx,%rdi
	addq	%rbx,%rsi
	subq	%rbx,%rcx
	movq	$512,%rbx
	jnz	.Lofb_loop
	cmpq	%rbp,%rsp
	je	.Lofb_done

	pxor	%xmm0,%xmm0
	leaq	(%rsp),%rax
.Lofb_bzero:
	movaps	%xmm0,(%rax)
	leaq	16(%rax),%rax
	cmpq	%rax,%rbp
	ja	.Lofb_bzero

.Lofb_done:
	leaq	(%rbp),%rsp
	jmp	.Lofb_exit

.p2align	4
.Lofb_aligned:
	leaq	-16(%rdx),%rax
	leaq	16(%rdx),%rbx
	shrq	$4,%rcx
.byte	0xf3,0x0f,0xa7,232	
	movdqa	(%rax),%xmm0
	movdqa	%xmm0,-16(%rdx)
.Lofb_exit:
	movl	$1,%eax
	leaq	8(%rsp),%rsp
.Lofb_abort:
	popq	%rbx
	popq	%rbp
	movq	8(%rsp),%rdi
	movq	16(%rsp),%rsi
	.byte	0xf3,0xc3
.LSEH_end_padlock_ofb_encrypt:
.globl	padlock_ctr32_encrypt
.def	padlock_ctr32_encrypt;	.scl 2;	.type 32;	.endef
.p2align	4
padlock_ctr32_encrypt:
	movq	%rdi,8(%rsp)
	movq	%rsi,16(%rsp)
	movq	%rsp,%rax
.LSEH_begin_padlock_ctr32_encrypt:
	movq	%rcx,%rdi
	movq	%rdx,%rsi
	movq	%r8,%rdx
	movq	%r9,%rcx

	pushq	%rbp
	pushq	%rbx

	xorl	%eax,%eax
	testq	$15,%rdx
	jnz	.Lctr32_abort
	testq	$15,%rcx
	jnz	.Lctr32_abort
	leaq	.Lpadlock_saved_context(%rip),%rax
	pushf
	cld
	call	_padlock_verify_ctx
	leaq	16(%rdx),%rdx
	xorl	%eax,%eax
	xorl	%ebx,%ebx
	testl	$32,(%rdx)
	jnz	.Lctr32_aligned
	testq	$15,%rdi
	setz	%al
	testq	$15,%rsi
	setz	%bl
	testl	%ebx,%eax
	jnz	.Lctr32_aligned
	negq	%rax
	movq	$512,%rbx
	notq	%rax
	leaq	(%rsp),%rbp
	cmpq	%rbx,%rcx
	cmovcq	%rcx,%rbx
	andq	%rbx,%rax
	movq	%rcx,%rbx
	negq	%rax
	andq	$512-1,%rbx
	leaq	(%rax,%rbp,1),%rsp
	movq	$512,%rax
	cmovzq	%rax,%rbx
.Lctr32_reenter:
	movl	-4(%rdx),%eax
	bswapl	%eax
	negl	%eax
	andl	$31,%eax
	movq	$512,%rbx
	shll	$4,%eax
	cmovzq	%rbx,%rax
	cmpq	%rax,%rcx
	cmovaq	%rax,%rbx
	cmovbeq	%rcx,%rbx
	cmpq	%rbx,%rcx
	ja	.Lctr32_loop
	movq	%rsi,%rax
	cmpq	%rsp,%rbp
	cmoveq	%rdi,%rax
	addq	%rcx,%rax
	negq	%rax
	andq	$4095,%rax
	cmpq	$32,%rax
	movq	$-32,%rax
	cmovaeq	%rbx,%rax
	andq	%rax,%rbx
	jz	.Lctr32_unaligned_tail
	jmp	.Lctr32_loop
.p2align	4
.Lctr32_loop:
	cmpq	%rcx,%rbx
	cmovaq	%rcx,%rbx
	movq	%rdi,%r8
	movq	%rsi,%r9
	movq	%rcx,%r10
	movq	%rbx,%rcx
	movq	%rbx,%r11
	testq	$15,%rdi
	cmovnzq	%rsp,%rdi
	testq	$15,%rsi
	jz	.Lctr32_inp_aligned
	shrq	$3,%rcx
.byte	0xf3,0x48,0xa5		
	subq	%rbx,%rdi
	movq	%rbx,%rcx
	movq	%rdi,%rsi
.Lctr32_inp_aligned:
	leaq	-16(%rdx),%rax
	leaq	16(%rdx),%rbx
	shrq	$4,%rcx
.byte	0xf3,0x0f,0xa7,216	
	movl	-4(%rdx),%eax
	testl	$4294901760,%eax
	jnz	.Lctr32_no_carry
	bswapl	%eax
	addl	$65536,%eax
	bswapl	%eax
	movl	%eax,-4(%rdx)
.Lctr32_no_carry:
	movq	%r8,%rdi
	movq	%r11,%rbx
	testq	$15,%rdi
	jz	.Lctr32_out_aligned
	movq	%rbx,%rcx
	leaq	(%rsp),%rsi
	shrq	$3,%rcx
.byte	0xf3,0x48,0xa5		
	subq	%rbx,%rdi
.Lctr32_out_aligned:
	movq	%r9,%rsi
	movq	%r10,%rcx
	addq	%rbx,%rdi
	addq	%rbx,%rsi
	subq	%rbx,%rcx
	movq	$512,%rbx
	jz	.Lctr32_break
	cmpq	%rbx,%rcx
	jae	.Lctr32_loop
	movq	%rcx,%rbx
	movq	%rsi,%rax
	cmpq	%rsp,%rbp
	cmoveq	%rdi,%rax
	addq	%rcx,%rax
	negq	%rax
	andq	$4095,%rax
	cmpq	$32,%rax
	movq	$-32,%rax
	cmovaeq	%rbx,%rax
	andq	%rax,%rbx
	jnz	.Lctr32_loop
.Lctr32_unaligned_tail:
	xorl	%eax,%eax
	cmpq	%rsp,%rbp
	cmoveq	%rcx,%rax
	movq	%rdi,%r8
	movq	%rcx,%rbx
	subq	%rax,%rsp
	shrq	$3,%rcx
	leaq	(%rsp),%rdi
.byte	0xf3,0x48,0xa5		
	movq	%rsp,%rsi
	movq	%r8,%rdi
	movq	%rbx,%rcx
	jmp	.Lctr32_loop
.p2align	4
.Lctr32_break:
	cmpq	%rbp,%rsp
	je	.Lctr32_done

	pxor	%xmm0,%xmm0
	leaq	(%rsp),%rax
.Lctr32_bzero:
	movaps	%xmm0,(%rax)
	leaq	16(%rax),%rax
	cmpq	%rax,%rbp
	ja	.Lctr32_bzero

.Lctr32_done:
	leaq	(%rbp),%rsp
	jmp	.Lctr32_exit

.p2align	4
.Lctr32_aligned:
	movl	-4(%rdx),%eax
	bswapl	%eax
	negl	%eax
	andl	$65535,%eax
	movq	$1048576,%rbx
	shll	$4,%eax
	cmovzq	%rbx,%rax
	cmpq	%rax,%rcx
	cmovaq	%rax,%rbx
	cmovbeq	%rcx,%rbx
	jbe	.Lctr32_aligned_skip

.Lctr32_aligned_loop:
	movq	%rcx,%r10
	movq	%rbx,%rcx
	movq	%rbx,%r11

	leaq	-16(%rdx),%rax
	leaq	16(%rdx),%rbx
	shrq	$4,%rcx
.byte	0xf3,0x0f,0xa7,216	

	movl	-4(%rdx),%eax
	bswapl	%eax
	addl	$65536,%eax
	bswapl	%eax
	movl	%eax,-4(%rdx)

	movq	%r10,%rcx
	subq	%r11,%rcx
	movq	$1048576,%rbx
	jz	.Lctr32_exit
	cmpq	%rbx,%rcx
	jae	.Lctr32_aligned_loop

.Lctr32_aligned_skip:
	leaq	(%rsi,%rcx,1),%rbp
	negq	%rbp
	andq	$4095,%rbp
	xorl	%eax,%eax
	cmpq	$32,%rbp
	movq	$32-1,%rbp
	cmovaeq	%rax,%rbp
	andq	%rcx,%rbp
	subq	%rbp,%rcx
	jz	.Lctr32_aligned_tail
	leaq	-16(%rdx),%rax
	leaq	16(%rdx),%rbx
	shrq	$4,%rcx
.byte	0xf3,0x0f,0xa7,216	
	testq	%rbp,%rbp
	jz	.Lctr32_exit

.Lctr32_aligned_tail:
	movq	%rdi,%r8
	movq	%rbp,%rbx
	movq	%rbp,%rcx
	leaq	(%rsp),%rbp
	subq	%rcx,%rsp
	shrq	$3,%rcx
	leaq	(%rsp),%rdi
.byte	0xf3,0x48,0xa5		
	leaq	(%r8),%rdi
	leaq	(%rsp),%rsi
	movq	%rbx,%rcx
	jmp	.Lctr32_loop
.Lctr32_exit:
	movl	$1,%eax
	leaq	8(%rsp),%rsp
.Lctr32_abort:
	popq	%rbx
	popq	%rbp
	movq	8(%rsp),%rdi
	movq	16(%rsp),%rsi
	.byte	0xf3,0xc3
.LSEH_end_padlock_ctr32_encrypt:
.byte	86,73,65,32,80,97,100,108,111,99,107,32,120,56,54,95,54,52,32,109,111,100,117,108,101,44,32,67,82,89,80,84,79,71,65,77,83,32,98,121,32,60,97,112,112,114,111,64,111,112,101,110,115,115,108,46,111,114,103,62,0
.p2align	4
.data	
.p2align	3
.Lpadlock_saved_context:
.quad	0

