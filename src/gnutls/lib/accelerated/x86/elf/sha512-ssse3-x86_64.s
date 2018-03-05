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


.globl	sha256_block_data_order
.type	sha256_block_data_order,@function
.align	16
sha256_block_data_order:
	leaq	_gnutls_x86_cpuid_s(%rip),%r11
	movl	0(%r11),%r9d
	movl	4(%r11),%r10d
	movl	8(%r11),%r11d
	testl	$512,%r10d
	jnz	.Lssse3_shortcut
	pushq	%rbx
	pushq	%rbp
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15
	movq	%rsp,%r11
	shlq	$4,%rdx
	subq	$64+32,%rsp
	leaq	(%rsi,%rdx,4),%rdx
	andq	$-64,%rsp
	movq	%rdi,64+0(%rsp)
	movq	%rsi,64+8(%rsp)
	movq	%rdx,64+16(%rsp)
	movq	%r11,64+24(%rsp)
.Lprologue:

	movl	0(%rdi),%eax
	movl	4(%rdi),%ebx
	movl	8(%rdi),%ecx
	movl	12(%rdi),%edx
	movl	16(%rdi),%r8d
	movl	20(%rdi),%r9d
	movl	24(%rdi),%r10d
	movl	28(%rdi),%r11d
	jmp	.Lloop

.align	16
.Lloop:
	movl	%ebx,%edi
	leaq	K256(%rip),%rbp
	xorl	%ecx,%edi
	movl	0(%rsi),%r12d
	movl	%r8d,%r13d
	movl	%eax,%r14d
	bswapl	%r12d
	rorl	$14,%r13d
	movl	%r9d,%r15d

	rorl	$9,%r14d
	xorl	%r8d,%r13d
	xorl	%r10d,%r15d

	movl	%r12d,0(%rsp)
	xorl	%eax,%r14d
	andl	%r8d,%r15d

	rorl	$5,%r13d
	addl	%r11d,%r12d
	xorl	%r10d,%r15d

	rorl	$11,%r14d
	xorl	%r8d,%r13d
	addl	%r15d,%r12d

	movl	%eax,%r15d
	addl	(%rbp),%r12d
	xorl	%eax,%r14d

	rorl	$6,%r13d
	xorl	%ebx,%r15d
	movl	%ebx,%r11d

	rorl	$2,%r14d
	andl	%r15d,%edi
	addl	%r13d,%r12d

	xorl	%edi,%r11d
	addl	%r12d,%edx
	addl	%r12d,%r11d
	leaq	4(%rbp),%rbp
	addl	%r14d,%r11d

	movl	4(%rsi),%r12d
	movl	%edx,%r13d
	movl	%r11d,%r14d
	bswapl	%r12d
	rorl	$14,%r13d
	movl	%r8d,%edi

	rorl	$9,%r14d
	xorl	%edx,%r13d
	xorl	%r9d,%edi

	movl	%r12d,4(%rsp)
	xorl	%r11d,%r14d
	andl	%edx,%edi

	rorl	$5,%r13d
	addl	%r10d,%r12d
	xorl	%r9d,%edi

	rorl	$11,%r14d
	xorl	%edx,%r13d
	addl	%edi,%r12d

	movl	%r11d,%edi
	addl	(%rbp),%r12d
	xorl	%r11d,%r14d

	rorl	$6,%r13d
	xorl	%eax,%edi
	movl	%eax,%r10d

	rorl	$2,%r14d
	andl	%edi,%r15d
	addl	%r13d,%r12d

	xorl	%r15d,%r10d
	addl	%r12d,%ecx
	addl	%r12d,%r10d
	leaq	4(%rbp),%rbp
	addl	%r14d,%r10d

	movl	8(%rsi),%r12d
	movl	%ecx,%r13d
	movl	%r10d,%r14d
	bswapl	%r12d
	rorl	$14,%r13d
	movl	%edx,%r15d

	rorl	$9,%r14d
	xorl	%ecx,%r13d
	xorl	%r8d,%r15d

	movl	%r12d,8(%rsp)
	xorl	%r10d,%r14d
	andl	%ecx,%r15d

	rorl	$5,%r13d
	addl	%r9d,%r12d
	xorl	%r8d,%r15d

	rorl	$11,%r14d
	xorl	%ecx,%r13d
	addl	%r15d,%r12d

	movl	%r10d,%r15d
	addl	(%rbp),%r12d
	xorl	%r10d,%r14d

	rorl	$6,%r13d
	xorl	%r11d,%r15d
	movl	%r11d,%r9d

	rorl	$2,%r14d
	andl	%r15d,%edi
	addl	%r13d,%r12d

	xorl	%edi,%r9d
	addl	%r12d,%ebx
	addl	%r12d,%r9d
	leaq	4(%rbp),%rbp
	addl	%r14d,%r9d

	movl	12(%rsi),%r12d
	movl	%ebx,%r13d
	movl	%r9d,%r14d
	bswapl	%r12d
	rorl	$14,%r13d
	movl	%ecx,%edi

	rorl	$9,%r14d
	xorl	%ebx,%r13d
	xorl	%edx,%edi

	movl	%r12d,12(%rsp)
	xorl	%r9d,%r14d
	andl	%ebx,%edi

	rorl	$5,%r13d
	addl	%r8d,%r12d
	xorl	%edx,%edi

	rorl	$11,%r14d
	xorl	%ebx,%r13d
	addl	%edi,%r12d

	movl	%r9d,%edi
	addl	(%rbp),%r12d
	xorl	%r9d,%r14d

	rorl	$6,%r13d
	xorl	%r10d,%edi
	movl	%r10d,%r8d

	rorl	$2,%r14d
	andl	%edi,%r15d
	addl	%r13d,%r12d

	xorl	%r15d,%r8d
	addl	%r12d,%eax
	addl	%r12d,%r8d
	leaq	20(%rbp),%rbp
	addl	%r14d,%r8d

	movl	16(%rsi),%r12d
	movl	%eax,%r13d
	movl	%r8d,%r14d
	bswapl	%r12d
	rorl	$14,%r13d
	movl	%ebx,%r15d

	rorl	$9,%r14d
	xorl	%eax,%r13d
	xorl	%ecx,%r15d

	movl	%r12d,16(%rsp)
	xorl	%r8d,%r14d
	andl	%eax,%r15d

	rorl	$5,%r13d
	addl	%edx,%r12d
	xorl	%ecx,%r15d

	rorl	$11,%r14d
	xorl	%eax,%r13d
	addl	%r15d,%r12d

	movl	%r8d,%r15d
	addl	(%rbp),%r12d
	xorl	%r8d,%r14d

	rorl	$6,%r13d
	xorl	%r9d,%r15d
	movl	%r9d,%edx

	rorl	$2,%r14d
	andl	%r15d,%edi
	addl	%r13d,%r12d

	xorl	%edi,%edx
	addl	%r12d,%r11d
	addl	%r12d,%edx
	leaq	4(%rbp),%rbp
	addl	%r14d,%edx

	movl	20(%rsi),%r12d
	movl	%r11d,%r13d
	movl	%edx,%r14d
	bswapl	%r12d
	rorl	$14,%r13d
	movl	%eax,%edi

	rorl	$9,%r14d
	xorl	%r11d,%r13d
	xorl	%ebx,%edi

	movl	%r12d,20(%rsp)
	xorl	%edx,%r14d
	andl	%r11d,%edi

	rorl	$5,%r13d
	addl	%ecx,%r12d
	xorl	%ebx,%edi

	rorl	$11,%r14d
	xorl	%r11d,%r13d
	addl	%edi,%r12d

	movl	%edx,%edi
	addl	(%rbp),%r12d
	xorl	%edx,%r14d

	rorl	$6,%r13d
	xorl	%r8d,%edi
	movl	%r8d,%ecx

	rorl	$2,%r14d
	andl	%edi,%r15d
	addl	%r13d,%r12d

	xorl	%r15d,%ecx
	addl	%r12d,%r10d
	addl	%r12d,%ecx
	leaq	4(%rbp),%rbp
	addl	%r14d,%ecx

	movl	24(%rsi),%r12d
	movl	%r10d,%r13d
	movl	%ecx,%r14d
	bswapl	%r12d
	rorl	$14,%r13d
	movl	%r11d,%r15d

	rorl	$9,%r14d
	xorl	%r10d,%r13d
	xorl	%eax,%r15d

	movl	%r12d,24(%rsp)
	xorl	%ecx,%r14d
	andl	%r10d,%r15d

	rorl	$5,%r13d
	addl	%ebx,%r12d
	xorl	%eax,%r15d

	rorl	$11,%r14d
	xorl	%r10d,%r13d
	addl	%r15d,%r12d

	movl	%ecx,%r15d
	addl	(%rbp),%r12d
	xorl	%ecx,%r14d

	rorl	$6,%r13d
	xorl	%edx,%r15d
	movl	%edx,%ebx

	rorl	$2,%r14d
	andl	%r15d,%edi
	addl	%r13d,%r12d

	xorl	%edi,%ebx
	addl	%r12d,%r9d
	addl	%r12d,%ebx
	leaq	4(%rbp),%rbp
	addl	%r14d,%ebx

	movl	28(%rsi),%r12d
	movl	%r9d,%r13d
	movl	%ebx,%r14d
	bswapl	%r12d
	rorl	$14,%r13d
	movl	%r10d,%edi

	rorl	$9,%r14d
	xorl	%r9d,%r13d
	xorl	%r11d,%edi

	movl	%r12d,28(%rsp)
	xorl	%ebx,%r14d
	andl	%r9d,%edi

	rorl	$5,%r13d
	addl	%eax,%r12d
	xorl	%r11d,%edi

	rorl	$11,%r14d
	xorl	%r9d,%r13d
	addl	%edi,%r12d

	movl	%ebx,%edi
	addl	(%rbp),%r12d
	xorl	%ebx,%r14d

	rorl	$6,%r13d
	xorl	%ecx,%edi
	movl	%ecx,%eax

	rorl	$2,%r14d
	andl	%edi,%r15d
	addl	%r13d,%r12d

	xorl	%r15d,%eax
	addl	%r12d,%r8d
	addl	%r12d,%eax
	leaq	20(%rbp),%rbp
	addl	%r14d,%eax

	movl	32(%rsi),%r12d
	movl	%r8d,%r13d
	movl	%eax,%r14d
	bswapl	%r12d
	rorl	$14,%r13d
	movl	%r9d,%r15d

	rorl	$9,%r14d
	xorl	%r8d,%r13d
	xorl	%r10d,%r15d

	movl	%r12d,32(%rsp)
	xorl	%eax,%r14d
	andl	%r8d,%r15d

	rorl	$5,%r13d
	addl	%r11d,%r12d
	xorl	%r10d,%r15d

	rorl	$11,%r14d
	xorl	%r8d,%r13d
	addl	%r15d,%r12d

	movl	%eax,%r15d
	addl	(%rbp),%r12d
	xorl	%eax,%r14d

	rorl	$6,%r13d
	xorl	%ebx,%r15d
	movl	%ebx,%r11d

	rorl	$2,%r14d
	andl	%r15d,%edi
	addl	%r13d,%r12d

	xorl	%edi,%r11d
	addl	%r12d,%edx
	addl	%r12d,%r11d
	leaq	4(%rbp),%rbp
	addl	%r14d,%r11d

	movl	36(%rsi),%r12d
	movl	%edx,%r13d
	movl	%r11d,%r14d
	bswapl	%r12d
	rorl	$14,%r13d
	movl	%r8d,%edi

	rorl	$9,%r14d
	xorl	%edx,%r13d
	xorl	%r9d,%edi

	movl	%r12d,36(%rsp)
	xorl	%r11d,%r14d
	andl	%edx,%edi

	rorl	$5,%r13d
	addl	%r10d,%r12d
	xorl	%r9d,%edi

	rorl	$11,%r14d
	xorl	%edx,%r13d
	addl	%edi,%r12d

	movl	%r11d,%edi
	addl	(%rbp),%r12d
	xorl	%r11d,%r14d

	rorl	$6,%r13d
	xorl	%eax,%edi
	movl	%eax,%r10d

	rorl	$2,%r14d
	andl	%edi,%r15d
	addl	%r13d,%r12d

	xorl	%r15d,%r10d
	addl	%r12d,%ecx
	addl	%r12d,%r10d
	leaq	4(%rbp),%rbp
	addl	%r14d,%r10d

	movl	40(%rsi),%r12d
	movl	%ecx,%r13d
	movl	%r10d,%r14d
	bswapl	%r12d
	rorl	$14,%r13d
	movl	%edx,%r15d

	rorl	$9,%r14d
	xorl	%ecx,%r13d
	xorl	%r8d,%r15d

	movl	%r12d,40(%rsp)
	xorl	%r10d,%r14d
	andl	%ecx,%r15d

	rorl	$5,%r13d
	addl	%r9d,%r12d
	xorl	%r8d,%r15d

	rorl	$11,%r14d
	xorl	%ecx,%r13d
	addl	%r15d,%r12d

	movl	%r10d,%r15d
	addl	(%rbp),%r12d
	xorl	%r10d,%r14d

	rorl	$6,%r13d
	xorl	%r11d,%r15d
	movl	%r11d,%r9d

	rorl	$2,%r14d
	andl	%r15d,%edi
	addl	%r13d,%r12d

	xorl	%edi,%r9d
	addl	%r12d,%ebx
	addl	%r12d,%r9d
	leaq	4(%rbp),%rbp
	addl	%r14d,%r9d

	movl	44(%rsi),%r12d
	movl	%ebx,%r13d
	movl	%r9d,%r14d
	bswapl	%r12d
	rorl	$14,%r13d
	movl	%ecx,%edi

	rorl	$9,%r14d
	xorl	%ebx,%r13d
	xorl	%edx,%edi

	movl	%r12d,44(%rsp)
	xorl	%r9d,%r14d
	andl	%ebx,%edi

	rorl	$5,%r13d
	addl	%r8d,%r12d
	xorl	%edx,%edi

	rorl	$11,%r14d
	xorl	%ebx,%r13d
	addl	%edi,%r12d

	movl	%r9d,%edi
	addl	(%rbp),%r12d
	xorl	%r9d,%r14d

	rorl	$6,%r13d
	xorl	%r10d,%edi
	movl	%r10d,%r8d

	rorl	$2,%r14d
	andl	%edi,%r15d
	addl	%r13d,%r12d

	xorl	%r15d,%r8d
	addl	%r12d,%eax
	addl	%r12d,%r8d
	leaq	20(%rbp),%rbp
	addl	%r14d,%r8d

	movl	48(%rsi),%r12d
	movl	%eax,%r13d
	movl	%r8d,%r14d
	bswapl	%r12d
	rorl	$14,%r13d
	movl	%ebx,%r15d

	rorl	$9,%r14d
	xorl	%eax,%r13d
	xorl	%ecx,%r15d

	movl	%r12d,48(%rsp)
	xorl	%r8d,%r14d
	andl	%eax,%r15d

	rorl	$5,%r13d
	addl	%edx,%r12d
	xorl	%ecx,%r15d

	rorl	$11,%r14d
	xorl	%eax,%r13d
	addl	%r15d,%r12d

	movl	%r8d,%r15d
	addl	(%rbp),%r12d
	xorl	%r8d,%r14d

	rorl	$6,%r13d
	xorl	%r9d,%r15d
	movl	%r9d,%edx

	rorl	$2,%r14d
	andl	%r15d,%edi
	addl	%r13d,%r12d

	xorl	%edi,%edx
	addl	%r12d,%r11d
	addl	%r12d,%edx
	leaq	4(%rbp),%rbp
	addl	%r14d,%edx

	movl	52(%rsi),%r12d
	movl	%r11d,%r13d
	movl	%edx,%r14d
	bswapl	%r12d
	rorl	$14,%r13d
	movl	%eax,%edi

	rorl	$9,%r14d
	xorl	%r11d,%r13d
	xorl	%ebx,%edi

	movl	%r12d,52(%rsp)
	xorl	%edx,%r14d
	andl	%r11d,%edi

	rorl	$5,%r13d
	addl	%ecx,%r12d
	xorl	%ebx,%edi

	rorl	$11,%r14d
	xorl	%r11d,%r13d
	addl	%edi,%r12d

	movl	%edx,%edi
	addl	(%rbp),%r12d
	xorl	%edx,%r14d

	rorl	$6,%r13d
	xorl	%r8d,%edi
	movl	%r8d,%ecx

	rorl	$2,%r14d
	andl	%edi,%r15d
	addl	%r13d,%r12d

	xorl	%r15d,%ecx
	addl	%r12d,%r10d
	addl	%r12d,%ecx
	leaq	4(%rbp),%rbp
	addl	%r14d,%ecx

	movl	56(%rsi),%r12d
	movl	%r10d,%r13d
	movl	%ecx,%r14d
	bswapl	%r12d
	rorl	$14,%r13d
	movl	%r11d,%r15d

	rorl	$9,%r14d
	xorl	%r10d,%r13d
	xorl	%eax,%r15d

	movl	%r12d,56(%rsp)
	xorl	%ecx,%r14d
	andl	%r10d,%r15d

	rorl	$5,%r13d
	addl	%ebx,%r12d
	xorl	%eax,%r15d

	rorl	$11,%r14d
	xorl	%r10d,%r13d
	addl	%r15d,%r12d

	movl	%ecx,%r15d
	addl	(%rbp),%r12d
	xorl	%ecx,%r14d

	rorl	$6,%r13d
	xorl	%edx,%r15d
	movl	%edx,%ebx

	rorl	$2,%r14d
	andl	%r15d,%edi
	addl	%r13d,%r12d

	xorl	%edi,%ebx
	addl	%r12d,%r9d
	addl	%r12d,%ebx
	leaq	4(%rbp),%rbp
	addl	%r14d,%ebx

	movl	60(%rsi),%r12d
	movl	%r9d,%r13d
	movl	%ebx,%r14d
	bswapl	%r12d
	rorl	$14,%r13d
	movl	%r10d,%edi

	rorl	$9,%r14d
	xorl	%r9d,%r13d
	xorl	%r11d,%edi

	movl	%r12d,60(%rsp)
	xorl	%ebx,%r14d
	andl	%r9d,%edi

	rorl	$5,%r13d
	addl	%eax,%r12d
	xorl	%r11d,%edi

	rorl	$11,%r14d
	xorl	%r9d,%r13d
	addl	%edi,%r12d

	movl	%ebx,%edi
	addl	(%rbp),%r12d
	xorl	%ebx,%r14d

	rorl	$6,%r13d
	xorl	%ecx,%edi
	movl	%ecx,%eax

	rorl	$2,%r14d
	andl	%edi,%r15d
	addl	%r13d,%r12d

	xorl	%r15d,%eax
	addl	%r12d,%r8d
	addl	%r12d,%eax
	movl	4(%rsp),%r13d
	leaq	20(%rbp),%rbp
	addl	%r14d,%eax

	jmp	.Lrounds_16_xx
.align	16
.Lrounds_16_xx:

	movl	56(%rsp),%r14d

	movl	%r13d,%r12d
	rorl	$11,%r13d
	movl	%r14d,%r15d
	rorl	$2,%r14d

	xorl	%r12d,%r13d
	shrl	$3,%r12d
	rorl	$7,%r13d
	xorl	%r15d,%r14d
	shrl	$10,%r15d

	xorl	%r13d,%r12d
	rorl	$17,%r14d
	addl	36(%rsp),%r12d
	xorl	%r15d,%r14d

	addl	0(%rsp),%r12d
	movl	%r8d,%r13d
	addl	%r14d,%r12d
	movl	%eax,%r14d
	rorl	$14,%r13d
	movl	%r9d,%r15d

	rorl	$9,%r14d
	xorl	%r8d,%r13d
	xorl	%r10d,%r15d

	movl	%r12d,0(%rsp)
	xorl	%eax,%r14d
	andl	%r8d,%r15d

	rorl	$5,%r13d
	addl	%r11d,%r12d
	xorl	%r10d,%r15d

	rorl	$11,%r14d
	xorl	%r8d,%r13d
	addl	%r15d,%r12d

	movl	%eax,%r15d
	addl	(%rbp),%r12d
	xorl	%eax,%r14d

	rorl	$6,%r13d
	xorl	%ebx,%r15d
	movl	%ebx,%r11d

	rorl	$2,%r14d
	andl	%r15d,%edi
	addl	%r13d,%r12d

	xorl	%edi,%r11d
	addl	%r12d,%edx
	addl	%r12d,%r11d
	movl	8(%rsp),%r13d
	leaq	4(%rbp),%rbp
	addl	%r14d,%r11d


	movl	60(%rsp),%r14d

	movl	%r13d,%r12d
	rorl	$11,%r13d
	movl	%r14d,%edi
	rorl	$2,%r14d

	xorl	%r12d,%r13d
	shrl	$3,%r12d
	rorl	$7,%r13d
	xorl	%edi,%r14d
	shrl	$10,%edi

	xorl	%r13d,%r12d
	rorl	$17,%r14d
	addl	40(%rsp),%r12d
	xorl	%edi,%r14d

	addl	4(%rsp),%r12d
	movl	%edx,%r13d
	addl	%r14d,%r12d
	movl	%r11d,%r14d
	rorl	$14,%r13d
	movl	%r8d,%edi

	rorl	$9,%r14d
	xorl	%edx,%r13d
	xorl	%r9d,%edi

	movl	%r12d,4(%rsp)
	xorl	%r11d,%r14d
	andl	%edx,%edi

	rorl	$5,%r13d
	addl	%r10d,%r12d
	xorl	%r9d,%edi

	rorl	$11,%r14d
	xorl	%edx,%r13d
	addl	%edi,%r12d

	movl	%r11d,%edi
	addl	(%rbp),%r12d
	xorl	%r11d,%r14d

	rorl	$6,%r13d
	xorl	%eax,%edi
	movl	%eax,%r10d

	rorl	$2,%r14d
	andl	%edi,%r15d
	addl	%r13d,%r12d

	xorl	%r15d,%r10d
	addl	%r12d,%ecx
	addl	%r12d,%r10d
	movl	12(%rsp),%r13d
	leaq	4(%rbp),%rbp
	addl	%r14d,%r10d


	movl	0(%rsp),%r14d

	movl	%r13d,%r12d
	rorl	$11,%r13d
	movl	%r14d,%r15d
	rorl	$2,%r14d

	xorl	%r12d,%r13d
	shrl	$3,%r12d
	rorl	$7,%r13d
	xorl	%r15d,%r14d
	shrl	$10,%r15d

	xorl	%r13d,%r12d
	rorl	$17,%r14d
	addl	44(%rsp),%r12d
	xorl	%r15d,%r14d

	addl	8(%rsp),%r12d
	movl	%ecx,%r13d
	addl	%r14d,%r12d
	movl	%r10d,%r14d
	rorl	$14,%r13d
	movl	%edx,%r15d

	rorl	$9,%r14d
	xorl	%ecx,%r13d
	xorl	%r8d,%r15d

	movl	%r12d,8(%rsp)
	xorl	%r10d,%r14d
	andl	%ecx,%r15d

	rorl	$5,%r13d
	addl	%r9d,%r12d
	xorl	%r8d,%r15d

	rorl	$11,%r14d
	xorl	%ecx,%r13d
	addl	%r15d,%r12d

	movl	%r10d,%r15d
	addl	(%rbp),%r12d
	xorl	%r10d,%r14d

	rorl	$6,%r13d
	xorl	%r11d,%r15d
	movl	%r11d,%r9d

	rorl	$2,%r14d
	andl	%r15d,%edi
	addl	%r13d,%r12d

	xorl	%edi,%r9d
	addl	%r12d,%ebx
	addl	%r12d,%r9d
	movl	16(%rsp),%r13d
	leaq	4(%rbp),%rbp
	addl	%r14d,%r9d


	movl	4(%rsp),%r14d

	movl	%r13d,%r12d
	rorl	$11,%r13d
	movl	%r14d,%edi
	rorl	$2,%r14d

	xorl	%r12d,%r13d
	shrl	$3,%r12d
	rorl	$7,%r13d
	xorl	%edi,%r14d
	shrl	$10,%edi

	xorl	%r13d,%r12d
	rorl	$17,%r14d
	addl	48(%rsp),%r12d
	xorl	%edi,%r14d

	addl	12(%rsp),%r12d
	movl	%ebx,%r13d
	addl	%r14d,%r12d
	movl	%r9d,%r14d
	rorl	$14,%r13d
	movl	%ecx,%edi

	rorl	$9,%r14d
	xorl	%ebx,%r13d
	xorl	%edx,%edi

	movl	%r12d,12(%rsp)
	xorl	%r9d,%r14d
	andl	%ebx,%edi

	rorl	$5,%r13d
	addl	%r8d,%r12d
	xorl	%edx,%edi

	rorl	$11,%r14d
	xorl	%ebx,%r13d
	addl	%edi,%r12d

	movl	%r9d,%edi
	addl	(%rbp),%r12d
	xorl	%r9d,%r14d

	rorl	$6,%r13d
	xorl	%r10d,%edi
	movl	%r10d,%r8d

	rorl	$2,%r14d
	andl	%edi,%r15d
	addl	%r13d,%r12d

	xorl	%r15d,%r8d
	addl	%r12d,%eax
	addl	%r12d,%r8d
	movl	20(%rsp),%r13d
	leaq	20(%rbp),%rbp
	addl	%r14d,%r8d


	movl	8(%rsp),%r14d

	movl	%r13d,%r12d
	rorl	$11,%r13d
	movl	%r14d,%r15d
	rorl	$2,%r14d

	xorl	%r12d,%r13d
	shrl	$3,%r12d
	rorl	$7,%r13d
	xorl	%r15d,%r14d
	shrl	$10,%r15d

	xorl	%r13d,%r12d
	rorl	$17,%r14d
	addl	52(%rsp),%r12d
	xorl	%r15d,%r14d

	addl	16(%rsp),%r12d
	movl	%eax,%r13d
	addl	%r14d,%r12d
	movl	%r8d,%r14d
	rorl	$14,%r13d
	movl	%ebx,%r15d

	rorl	$9,%r14d
	xorl	%eax,%r13d
	xorl	%ecx,%r15d

	movl	%r12d,16(%rsp)
	xorl	%r8d,%r14d
	andl	%eax,%r15d

	rorl	$5,%r13d
	addl	%edx,%r12d
	xorl	%ecx,%r15d

	rorl	$11,%r14d
	xorl	%eax,%r13d
	addl	%r15d,%r12d

	movl	%r8d,%r15d
	addl	(%rbp),%r12d
	xorl	%r8d,%r14d

	rorl	$6,%r13d
	xorl	%r9d,%r15d
	movl	%r9d,%edx

	rorl	$2,%r14d
	andl	%r15d,%edi
	addl	%r13d,%r12d

	xorl	%edi,%edx
	addl	%r12d,%r11d
	addl	%r12d,%edx
	movl	24(%rsp),%r13d
	leaq	4(%rbp),%rbp
	addl	%r14d,%edx


	movl	12(%rsp),%r14d

	movl	%r13d,%r12d
	rorl	$11,%r13d
	movl	%r14d,%edi
	rorl	$2,%r14d

	xorl	%r12d,%r13d
	shrl	$3,%r12d
	rorl	$7,%r13d
	xorl	%edi,%r14d
	shrl	$10,%edi

	xorl	%r13d,%r12d
	rorl	$17,%r14d
	addl	56(%rsp),%r12d
	xorl	%edi,%r14d

	addl	20(%rsp),%r12d
	movl	%r11d,%r13d
	addl	%r14d,%r12d
	movl	%edx,%r14d
	rorl	$14,%r13d
	movl	%eax,%edi

	rorl	$9,%r14d
	xorl	%r11d,%r13d
	xorl	%ebx,%edi

	movl	%r12d,20(%rsp)
	xorl	%edx,%r14d
	andl	%r11d,%edi

	rorl	$5,%r13d
	addl	%ecx,%r12d
	xorl	%ebx,%edi

	rorl	$11,%r14d
	xorl	%r11d,%r13d
	addl	%edi,%r12d

	movl	%edx,%edi
	addl	(%rbp),%r12d
	xorl	%edx,%r14d

	rorl	$6,%r13d
	xorl	%r8d,%edi
	movl	%r8d,%ecx

	rorl	$2,%r14d
	andl	%edi,%r15d
	addl	%r13d,%r12d

	xorl	%r15d,%ecx
	addl	%r12d,%r10d
	addl	%r12d,%ecx
	movl	28(%rsp),%r13d
	leaq	4(%rbp),%rbp
	addl	%r14d,%ecx


	movl	16(%rsp),%r14d

	movl	%r13d,%r12d
	rorl	$11,%r13d
	movl	%r14d,%r15d
	rorl	$2,%r14d

	xorl	%r12d,%r13d
	shrl	$3,%r12d
	rorl	$7,%r13d
	xorl	%r15d,%r14d
	shrl	$10,%r15d

	xorl	%r13d,%r12d
	rorl	$17,%r14d
	addl	60(%rsp),%r12d
	xorl	%r15d,%r14d

	addl	24(%rsp),%r12d
	movl	%r10d,%r13d
	addl	%r14d,%r12d
	movl	%ecx,%r14d
	rorl	$14,%r13d
	movl	%r11d,%r15d

	rorl	$9,%r14d
	xorl	%r10d,%r13d
	xorl	%eax,%r15d

	movl	%r12d,24(%rsp)
	xorl	%ecx,%r14d
	andl	%r10d,%r15d

	rorl	$5,%r13d
	addl	%ebx,%r12d
	xorl	%eax,%r15d

	rorl	$11,%r14d
	xorl	%r10d,%r13d
	addl	%r15d,%r12d

	movl	%ecx,%r15d
	addl	(%rbp),%r12d
	xorl	%ecx,%r14d

	rorl	$6,%r13d
	xorl	%edx,%r15d
	movl	%edx,%ebx

	rorl	$2,%r14d
	andl	%r15d,%edi
	addl	%r13d,%r12d

	xorl	%edi,%ebx
	addl	%r12d,%r9d
	addl	%r12d,%ebx
	movl	32(%rsp),%r13d
	leaq	4(%rbp),%rbp
	addl	%r14d,%ebx


	movl	20(%rsp),%r14d

	movl	%r13d,%r12d
	rorl	$11,%r13d
	movl	%r14d,%edi
	rorl	$2,%r14d

	xorl	%r12d,%r13d
	shrl	$3,%r12d
	rorl	$7,%r13d
	xorl	%edi,%r14d
	shrl	$10,%edi

	xorl	%r13d,%r12d
	rorl	$17,%r14d
	addl	0(%rsp),%r12d
	xorl	%edi,%r14d

	addl	28(%rsp),%r12d
	movl	%r9d,%r13d
	addl	%r14d,%r12d
	movl	%ebx,%r14d
	rorl	$14,%r13d
	movl	%r10d,%edi

	rorl	$9,%r14d
	xorl	%r9d,%r13d
	xorl	%r11d,%edi

	movl	%r12d,28(%rsp)
	xorl	%ebx,%r14d
	andl	%r9d,%edi

	rorl	$5,%r13d
	addl	%eax,%r12d
	xorl	%r11d,%edi

	rorl	$11,%r14d
	xorl	%r9d,%r13d
	addl	%edi,%r12d

	movl	%ebx,%edi
	addl	(%rbp),%r12d
	xorl	%ebx,%r14d

	rorl	$6,%r13d
	xorl	%ecx,%edi
	movl	%ecx,%eax

	rorl	$2,%r14d
	andl	%edi,%r15d
	addl	%r13d,%r12d

	xorl	%r15d,%eax
	addl	%r12d,%r8d
	addl	%r12d,%eax
	movl	36(%rsp),%r13d
	leaq	20(%rbp),%rbp
	addl	%r14d,%eax


	movl	24(%rsp),%r14d

	movl	%r13d,%r12d
	rorl	$11,%r13d
	movl	%r14d,%r15d
	rorl	$2,%r14d

	xorl	%r12d,%r13d
	shrl	$3,%r12d
	rorl	$7,%r13d
	xorl	%r15d,%r14d
	shrl	$10,%r15d

	xorl	%r13d,%r12d
	rorl	$17,%r14d
	addl	4(%rsp),%r12d
	xorl	%r15d,%r14d

	addl	32(%rsp),%r12d
	movl	%r8d,%r13d
	addl	%r14d,%r12d
	movl	%eax,%r14d
	rorl	$14,%r13d
	movl	%r9d,%r15d

	rorl	$9,%r14d
	xorl	%r8d,%r13d
	xorl	%r10d,%r15d

	movl	%r12d,32(%rsp)
	xorl	%eax,%r14d
	andl	%r8d,%r15d

	rorl	$5,%r13d
	addl	%r11d,%r12d
	xorl	%r10d,%r15d

	rorl	$11,%r14d
	xorl	%r8d,%r13d
	addl	%r15d,%r12d

	movl	%eax,%r15d
	addl	(%rbp),%r12d
	xorl	%eax,%r14d

	rorl	$6,%r13d
	xorl	%ebx,%r15d
	movl	%ebx,%r11d

	rorl	$2,%r14d
	andl	%r15d,%edi
	addl	%r13d,%r12d

	xorl	%edi,%r11d
	addl	%r12d,%edx
	addl	%r12d,%r11d
	movl	40(%rsp),%r13d
	leaq	4(%rbp),%rbp
	addl	%r14d,%r11d


	movl	28(%rsp),%r14d

	movl	%r13d,%r12d
	rorl	$11,%r13d
	movl	%r14d,%edi
	rorl	$2,%r14d

	xorl	%r12d,%r13d
	shrl	$3,%r12d
	rorl	$7,%r13d
	xorl	%edi,%r14d
	shrl	$10,%edi

	xorl	%r13d,%r12d
	rorl	$17,%r14d
	addl	8(%rsp),%r12d
	xorl	%edi,%r14d

	addl	36(%rsp),%r12d
	movl	%edx,%r13d
	addl	%r14d,%r12d
	movl	%r11d,%r14d
	rorl	$14,%r13d
	movl	%r8d,%edi

	rorl	$9,%r14d
	xorl	%edx,%r13d
	xorl	%r9d,%edi

	movl	%r12d,36(%rsp)
	xorl	%r11d,%r14d
	andl	%edx,%edi

	rorl	$5,%r13d
	addl	%r10d,%r12d
	xorl	%r9d,%edi

	rorl	$11,%r14d
	xorl	%edx,%r13d
	addl	%edi,%r12d

	movl	%r11d,%edi
	addl	(%rbp),%r12d
	xorl	%r11d,%r14d

	rorl	$6,%r13d
	xorl	%eax,%edi
	movl	%eax,%r10d

	rorl	$2,%r14d
	andl	%edi,%r15d
	addl	%r13d,%r12d

	xorl	%r15d,%r10d
	addl	%r12d,%ecx
	addl	%r12d,%r10d
	movl	44(%rsp),%r13d
	leaq	4(%rbp),%rbp
	addl	%r14d,%r10d


	movl	32(%rsp),%r14d

	movl	%r13d,%r12d
	rorl	$11,%r13d
	movl	%r14d,%r15d
	rorl	$2,%r14d

	xorl	%r12d,%r13d
	shrl	$3,%r12d
	rorl	$7,%r13d
	xorl	%r15d,%r14d
	shrl	$10,%r15d

	xorl	%r13d,%r12d
	rorl	$17,%r14d
	addl	12(%rsp),%r12d
	xorl	%r15d,%r14d

	addl	40(%rsp),%r12d
	movl	%ecx,%r13d
	addl	%r14d,%r12d
	movl	%r10d,%r14d
	rorl	$14,%r13d
	movl	%edx,%r15d

	rorl	$9,%r14d
	xorl	%ecx,%r13d
	xorl	%r8d,%r15d

	movl	%r12d,40(%rsp)
	xorl	%r10d,%r14d
	andl	%ecx,%r15d

	rorl	$5,%r13d
	addl	%r9d,%r12d
	xorl	%r8d,%r15d

	rorl	$11,%r14d
	xorl	%ecx,%r13d
	addl	%r15d,%r12d

	movl	%r10d,%r15d
	addl	(%rbp),%r12d
	xorl	%r10d,%r14d

	rorl	$6,%r13d
	xorl	%r11d,%r15d
	movl	%r11d,%r9d

	rorl	$2,%r14d
	andl	%r15d,%edi
	addl	%r13d,%r12d

	xorl	%edi,%r9d
	addl	%r12d,%ebx
	addl	%r12d,%r9d
	movl	48(%rsp),%r13d
	leaq	4(%rbp),%rbp
	addl	%r14d,%r9d


	movl	36(%rsp),%r14d

	movl	%r13d,%r12d
	rorl	$11,%r13d
	movl	%r14d,%edi
	rorl	$2,%r14d

	xorl	%r12d,%r13d
	shrl	$3,%r12d
	rorl	$7,%r13d
	xorl	%edi,%r14d
	shrl	$10,%edi

	xorl	%r13d,%r12d
	rorl	$17,%r14d
	addl	16(%rsp),%r12d
	xorl	%edi,%r14d

	addl	44(%rsp),%r12d
	movl	%ebx,%r13d
	addl	%r14d,%r12d
	movl	%r9d,%r14d
	rorl	$14,%r13d
	movl	%ecx,%edi

	rorl	$9,%r14d
	xorl	%ebx,%r13d
	xorl	%edx,%edi

	movl	%r12d,44(%rsp)
	xorl	%r9d,%r14d
	andl	%ebx,%edi

	rorl	$5,%r13d
	addl	%r8d,%r12d
	xorl	%edx,%edi

	rorl	$11,%r14d
	xorl	%ebx,%r13d
	addl	%edi,%r12d

	movl	%r9d,%edi
	addl	(%rbp),%r12d
	xorl	%r9d,%r14d

	rorl	$6,%r13d
	xorl	%r10d,%edi
	movl	%r10d,%r8d

	rorl	$2,%r14d
	andl	%edi,%r15d
	addl	%r13d,%r12d

	xorl	%r15d,%r8d
	addl	%r12d,%eax
	addl	%r12d,%r8d
	movl	52(%rsp),%r13d
	leaq	20(%rbp),%rbp
	addl	%r14d,%r8d


	movl	40(%rsp),%r14d

	movl	%r13d,%r12d
	rorl	$11,%r13d
	movl	%r14d,%r15d
	rorl	$2,%r14d

	xorl	%r12d,%r13d
	shrl	$3,%r12d
	rorl	$7,%r13d
	xorl	%r15d,%r14d
	shrl	$10,%r15d

	xorl	%r13d,%r12d
	rorl	$17,%r14d
	addl	20(%rsp),%r12d
	xorl	%r15d,%r14d

	addl	48(%rsp),%r12d
	movl	%eax,%r13d
	addl	%r14d,%r12d
	movl	%r8d,%r14d
	rorl	$14,%r13d
	movl	%ebx,%r15d

	rorl	$9,%r14d
	xorl	%eax,%r13d
	xorl	%ecx,%r15d

	movl	%r12d,48(%rsp)
	xorl	%r8d,%r14d
	andl	%eax,%r15d

	rorl	$5,%r13d
	addl	%edx,%r12d
	xorl	%ecx,%r15d

	rorl	$11,%r14d
	xorl	%eax,%r13d
	addl	%r15d,%r12d

	movl	%r8d,%r15d
	addl	(%rbp),%r12d
	xorl	%r8d,%r14d

	rorl	$6,%r13d
	xorl	%r9d,%r15d
	movl	%r9d,%edx

	rorl	$2,%r14d
	andl	%r15d,%edi
	addl	%r13d,%r12d

	xorl	%edi,%edx
	addl	%r12d,%r11d
	addl	%r12d,%edx
	movl	56(%rsp),%r13d
	leaq	4(%rbp),%rbp
	addl	%r14d,%edx


	movl	44(%rsp),%r14d

	movl	%r13d,%r12d
	rorl	$11,%r13d
	movl	%r14d,%edi
	rorl	$2,%r14d

	xorl	%r12d,%r13d
	shrl	$3,%r12d
	rorl	$7,%r13d
	xorl	%edi,%r14d
	shrl	$10,%edi

	xorl	%r13d,%r12d
	rorl	$17,%r14d
	addl	24(%rsp),%r12d
	xorl	%edi,%r14d

	addl	52(%rsp),%r12d
	movl	%r11d,%r13d
	addl	%r14d,%r12d
	movl	%edx,%r14d
	rorl	$14,%r13d
	movl	%eax,%edi

	rorl	$9,%r14d
	xorl	%r11d,%r13d
	xorl	%ebx,%edi

	movl	%r12d,52(%rsp)
	xorl	%edx,%r14d
	andl	%r11d,%edi

	rorl	$5,%r13d
	addl	%ecx,%r12d
	xorl	%ebx,%edi

	rorl	$11,%r14d
	xorl	%r11d,%r13d
	addl	%edi,%r12d

	movl	%edx,%edi
	addl	(%rbp),%r12d
	xorl	%edx,%r14d

	rorl	$6,%r13d
	xorl	%r8d,%edi
	movl	%r8d,%ecx

	rorl	$2,%r14d
	andl	%edi,%r15d
	addl	%r13d,%r12d

	xorl	%r15d,%ecx
	addl	%r12d,%r10d
	addl	%r12d,%ecx
	movl	60(%rsp),%r13d
	leaq	4(%rbp),%rbp
	addl	%r14d,%ecx


	movl	48(%rsp),%r14d

	movl	%r13d,%r12d
	rorl	$11,%r13d
	movl	%r14d,%r15d
	rorl	$2,%r14d

	xorl	%r12d,%r13d
	shrl	$3,%r12d
	rorl	$7,%r13d
	xorl	%r15d,%r14d
	shrl	$10,%r15d

	xorl	%r13d,%r12d
	rorl	$17,%r14d
	addl	28(%rsp),%r12d
	xorl	%r15d,%r14d

	addl	56(%rsp),%r12d
	movl	%r10d,%r13d
	addl	%r14d,%r12d
	movl	%ecx,%r14d
	rorl	$14,%r13d
	movl	%r11d,%r15d

	rorl	$9,%r14d
	xorl	%r10d,%r13d
	xorl	%eax,%r15d

	movl	%r12d,56(%rsp)
	xorl	%ecx,%r14d
	andl	%r10d,%r15d

	rorl	$5,%r13d
	addl	%ebx,%r12d
	xorl	%eax,%r15d

	rorl	$11,%r14d
	xorl	%r10d,%r13d
	addl	%r15d,%r12d

	movl	%ecx,%r15d
	addl	(%rbp),%r12d
	xorl	%ecx,%r14d

	rorl	$6,%r13d
	xorl	%edx,%r15d
	movl	%edx,%ebx

	rorl	$2,%r14d
	andl	%r15d,%edi
	addl	%r13d,%r12d

	xorl	%edi,%ebx
	addl	%r12d,%r9d
	addl	%r12d,%ebx
	movl	0(%rsp),%r13d
	leaq	4(%rbp),%rbp
	addl	%r14d,%ebx


	movl	52(%rsp),%r14d

	movl	%r13d,%r12d
	rorl	$11,%r13d
	movl	%r14d,%edi
	rorl	$2,%r14d

	xorl	%r12d,%r13d
	shrl	$3,%r12d
	rorl	$7,%r13d
	xorl	%edi,%r14d
	shrl	$10,%edi

	xorl	%r13d,%r12d
	rorl	$17,%r14d
	addl	32(%rsp),%r12d
	xorl	%edi,%r14d

	addl	60(%rsp),%r12d
	movl	%r9d,%r13d
	addl	%r14d,%r12d
	movl	%ebx,%r14d
	rorl	$14,%r13d
	movl	%r10d,%edi

	rorl	$9,%r14d
	xorl	%r9d,%r13d
	xorl	%r11d,%edi

	movl	%r12d,60(%rsp)
	xorl	%ebx,%r14d
	andl	%r9d,%edi

	rorl	$5,%r13d
	addl	%eax,%r12d
	xorl	%r11d,%edi

	rorl	$11,%r14d
	xorl	%r9d,%r13d
	addl	%edi,%r12d

	movl	%ebx,%edi
	addl	(%rbp),%r12d
	xorl	%ebx,%r14d

	rorl	$6,%r13d
	xorl	%ecx,%edi
	movl	%ecx,%eax

	rorl	$2,%r14d
	andl	%edi,%r15d
	addl	%r13d,%r12d

	xorl	%r15d,%eax
	addl	%r12d,%r8d
	addl	%r12d,%eax
	movl	4(%rsp),%r13d
	leaq	20(%rbp),%rbp
	addl	%r14d,%eax

	cmpb	$0,3(%rbp)
	jnz	.Lrounds_16_xx

	movq	64+0(%rsp),%rdi
	leaq	64(%rsi),%rsi

	addl	0(%rdi),%eax
	addl	4(%rdi),%ebx
	addl	8(%rdi),%ecx
	addl	12(%rdi),%edx
	addl	16(%rdi),%r8d
	addl	20(%rdi),%r9d
	addl	24(%rdi),%r10d
	addl	28(%rdi),%r11d

	cmpq	64+16(%rsp),%rsi

	movl	%eax,0(%rdi)
	movl	%ebx,4(%rdi)
	movl	%ecx,8(%rdi)
	movl	%edx,12(%rdi)
	movl	%r8d,16(%rdi)
	movl	%r9d,20(%rdi)
	movl	%r10d,24(%rdi)
	movl	%r11d,28(%rdi)
	jb	.Lloop

	movq	64+24(%rsp),%rsi
	movq	(%rsi),%r15
	movq	8(%rsi),%r14
	movq	16(%rsi),%r13
	movq	24(%rsi),%r12
	movq	32(%rsi),%rbp
	movq	40(%rsi),%rbx
	leaq	48(%rsi),%rsp
.Lepilogue:
	.byte	0xf3,0xc3
.size	sha256_block_data_order,.-sha256_block_data_order
.align	64
.type	K256,@object
K256:
.long	0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5
.long	0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5
.long	0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5
.long	0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5
.long	0xd807aa98,0x12835b01,0x243185be,0x550c7dc3
.long	0xd807aa98,0x12835b01,0x243185be,0x550c7dc3
.long	0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174
.long	0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174
.long	0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc
.long	0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc
.long	0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da
.long	0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da
.long	0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7
.long	0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7
.long	0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967
.long	0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967
.long	0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13
.long	0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13
.long	0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85
.long	0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85
.long	0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3
.long	0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3
.long	0xd192e819,0xd6990624,0xf40e3585,0x106aa070
.long	0xd192e819,0xd6990624,0xf40e3585,0x106aa070
.long	0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5
.long	0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5
.long	0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3
.long	0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3
.long	0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208
.long	0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208
.long	0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
.long	0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2

.long	0x00010203,0x04050607,0x08090a0b,0x0c0d0e0f
.long	0x00010203,0x04050607,0x08090a0b,0x0c0d0e0f
.long	0x03020100,0x0b0a0908,0xffffffff,0xffffffff
.long	0x03020100,0x0b0a0908,0xffffffff,0xffffffff
.long	0xffffffff,0xffffffff,0x03020100,0x0b0a0908
.long	0xffffffff,0xffffffff,0x03020100,0x0b0a0908
.byte	83,72,65,50,53,54,32,98,108,111,99,107,32,116,114,97,110,115,102,111,114,109,32,102,111,114,32,120,56,54,95,54,52,44,32,67,82,89,80,84,79,71,65,77,83,32,98,121,32,60,97,112,112,114,111,64,111,112,101,110,115,115,108,46,111,114,103,62,0
.type	sha256_block_data_order_ssse3,@function
.align	64
sha256_block_data_order_ssse3:
.Lssse3_shortcut:
	pushq	%rbx
	pushq	%rbp
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15
	movq	%rsp,%r11
	shlq	$4,%rdx
	subq	$96,%rsp
	leaq	(%rsi,%rdx,4),%rdx
	andq	$-64,%rsp
	movq	%rdi,64+0(%rsp)
	movq	%rsi,64+8(%rsp)
	movq	%rdx,64+16(%rsp)
	movq	%r11,64+24(%rsp)
.Lprologue_ssse3:

	movl	0(%rdi),%eax
	movl	4(%rdi),%ebx
	movl	8(%rdi),%ecx
	movl	12(%rdi),%edx
	movl	16(%rdi),%r8d
	movl	20(%rdi),%r9d
	movl	24(%rdi),%r10d
	movl	28(%rdi),%r11d
	movdqa	K256+512+32(%rip),%xmm8
	movdqa	K256+512+64(%rip),%xmm9
	jmp	.Lloop_ssse3
.align	16
.Lloop_ssse3:
	movdqa	K256+512(%rip),%xmm7
	movdqu	0(%rsi),%xmm0
	movdqu	16(%rsi),%xmm1
	movdqu	32(%rsi),%xmm2
	movdqu	48(%rsi),%xmm3
.byte	102,15,56,0,199
	leaq	K256(%rip),%rbp
.byte	102,15,56,0,207
	movdqa	0(%rbp),%xmm4
.byte	102,15,56,0,215
	movdqa	32(%rbp),%xmm5
	paddd	%xmm0,%xmm4
	movdqa	64(%rbp),%xmm6
.byte	102,15,56,0,223
	movdqa	96(%rbp),%xmm7
	paddd	%xmm1,%xmm5
	paddd	%xmm2,%xmm6
	paddd	%xmm3,%xmm7
	movdqa	%xmm4,0(%rsp)
	movl	%eax,%r14d
	movdqa	%xmm5,16(%rsp)
	movl	%ebx,%edi
	movdqa	%xmm6,32(%rsp)
	xorl	%ecx,%edi
	movdqa	%xmm7,48(%rsp)
	movl	%r8d,%r13d
	jmp	.Lssse3_00_47

.align	16
.Lssse3_00_47:
	subq	$-32*4,%rbp
	rorl	$14,%r13d
	movl	%r14d,%eax
	movdqa	%xmm1,%xmm4
	movl	%r9d,%r12d
	movdqa	%xmm3,%xmm7
	xorl	%r8d,%r13d
	rorl	$9,%r14d
	xorl	%r10d,%r12d
.byte	102,15,58,15,224,4
	rorl	$5,%r13d
	xorl	%eax,%r14d
.byte	102,15,58,15,250,4
	andl	%r8d,%r12d
	xorl	%r8d,%r13d
	addl	0(%rsp),%r11d
	movl	%eax,%r15d
	rorl	$11,%r14d
	xorl	%r10d,%r12d
	movdqa	%xmm4,%xmm5
	xorl	%ebx,%r15d
	movdqa	%xmm4,%xmm6
	rorl	$6,%r13d
	addl	%r12d,%r11d
	andl	%r15d,%edi
	psrld	$3,%xmm4
	xorl	%eax,%r14d
	addl	%r13d,%r11d
	xorl	%ebx,%edi
	paddd	%xmm7,%xmm0
	addl	%r11d,%edx
	rorl	$2,%r14d
	addl	%edi,%r11d
	psrld	$7,%xmm6
	movl	%edx,%r13d
	addl	%r11d,%r14d
	rorl	$14,%r13d
	movl	%r14d,%r11d
	pshufd	$250,%xmm3,%xmm7
	movl	%r8d,%r12d
	pslld	$14,%xmm5
	xorl	%edx,%r13d
	pxor	%xmm6,%xmm4
	rorl	$9,%r14d
	xorl	%r9d,%r12d
	psrld	$11,%xmm6
	rorl	$5,%r13d
	xorl	%r11d,%r14d
	pxor	%xmm5,%xmm4
	andl	%edx,%r12d
	xorl	%edx,%r13d
	pslld	$11,%xmm5
	addl	4(%rsp),%r10d
	pxor	%xmm6,%xmm4
	movl	%r11d,%edi
	rorl	$11,%r14d
	xorl	%r9d,%r12d
	movdqa	%xmm7,%xmm6
	xorl	%eax,%edi
	rorl	$6,%r13d
	addl	%r12d,%r10d
	pxor	%xmm5,%xmm4
	andl	%edi,%r15d
	xorl	%r11d,%r14d
	psrld	$10,%xmm7
	addl	%r13d,%r10d
	xorl	%eax,%r15d
	paddd	%xmm4,%xmm0
	addl	%r10d,%ecx
	rorl	$2,%r14d
	addl	%r15d,%r10d
	movl	%ecx,%r13d
	psrlq	$17,%xmm6
	addl	%r10d,%r14d
	rorl	$14,%r13d
	movl	%r14d,%r10d
	movl	%edx,%r12d
	pxor	%xmm6,%xmm7
	xorl	%ecx,%r13d
	rorl	$9,%r14d
	psrlq	$2,%xmm6
	xorl	%r8d,%r12d
	rorl	$5,%r13d
	xorl	%r10d,%r14d
	pxor	%xmm6,%xmm7
	andl	%ecx,%r12d
	xorl	%ecx,%r13d
	addl	8(%rsp),%r9d
.byte	102,65,15,56,0,248
	movl	%r10d,%r15d
	rorl	$11,%r14d
	xorl	%r8d,%r12d
	xorl	%r11d,%r15d
	rorl	$6,%r13d
	paddd	%xmm7,%xmm0
	addl	%r12d,%r9d
	pshufd	$80,%xmm0,%xmm7
	andl	%r15d,%edi
	xorl	%r10d,%r14d
	addl	%r13d,%r9d
	xorl	%r11d,%edi
	movdqa	%xmm7,%xmm6
	addl	%r9d,%ebx
	rorl	$2,%r14d
	addl	%edi,%r9d
	psrld	$10,%xmm7
	movl	%ebx,%r13d
	psrlq	$17,%xmm6
	addl	%r9d,%r14d
	rorl	$14,%r13d
	movl	%r14d,%r9d
	movl	%ecx,%r12d
	pxor	%xmm6,%xmm7
	xorl	%ebx,%r13d
	rorl	$9,%r14d
	xorl	%edx,%r12d
	psrlq	$2,%xmm6
	rorl	$5,%r13d
	xorl	%r9d,%r14d
	andl	%ebx,%r12d
	xorl	%ebx,%r13d
	pxor	%xmm6,%xmm7
	addl	12(%rsp),%r8d
	movl	%r9d,%edi
	movdqa	0(%rbp),%xmm6
	rorl	$11,%r14d
	xorl	%edx,%r12d
.byte	102,65,15,56,0,249
	xorl	%r10d,%edi
	rorl	$6,%r13d
	addl	%r12d,%r8d
	andl	%edi,%r15d
	xorl	%r9d,%r14d
	paddd	%xmm7,%xmm0
	addl	%r13d,%r8d
	xorl	%r10d,%r15d
	addl	%r8d,%eax
	paddd	%xmm0,%xmm6
	rorl	$2,%r14d
	addl	%r15d,%r8d
	movl	%eax,%r13d
	addl	%r8d,%r14d
	movdqa	%xmm6,0(%rsp)
	rorl	$14,%r13d
	movl	%r14d,%r8d
	movdqa	%xmm2,%xmm4
	movl	%ebx,%r12d
	movdqa	%xmm0,%xmm7
	xorl	%eax,%r13d
	rorl	$9,%r14d
	xorl	%ecx,%r12d
.byte	102,15,58,15,225,4
	rorl	$5,%r13d
	xorl	%r8d,%r14d
.byte	102,15,58,15,251,4
	andl	%eax,%r12d
	xorl	%eax,%r13d
	addl	16(%rsp),%edx
	movl	%r8d,%r15d
	rorl	$11,%r14d
	xorl	%ecx,%r12d
	movdqa	%xmm4,%xmm5
	xorl	%r9d,%r15d
	movdqa	%xmm4,%xmm6
	rorl	$6,%r13d
	addl	%r12d,%edx
	andl	%r15d,%edi
	psrld	$3,%xmm4
	xorl	%r8d,%r14d
	addl	%r13d,%edx
	xorl	%r9d,%edi
	paddd	%xmm7,%xmm1
	addl	%edx,%r11d
	rorl	$2,%r14d
	addl	%edi,%edx
	psrld	$7,%xmm6
	movl	%r11d,%r13d
	addl	%edx,%r14d
	rorl	$14,%r13d
	movl	%r14d,%edx
	pshufd	$250,%xmm0,%xmm7
	movl	%eax,%r12d
	pslld	$14,%xmm5
	xorl	%r11d,%r13d
	pxor	%xmm6,%xmm4
	rorl	$9,%r14d
	xorl	%ebx,%r12d
	psrld	$11,%xmm6
	rorl	$5,%r13d
	xorl	%edx,%r14d
	pxor	%xmm5,%xmm4
	andl	%r11d,%r12d
	xorl	%r11d,%r13d
	pslld	$11,%xmm5
	addl	20(%rsp),%ecx
	pxor	%xmm6,%xmm4
	movl	%edx,%edi
	rorl	$11,%r14d
	xorl	%ebx,%r12d
	movdqa	%xmm7,%xmm6
	xorl	%r8d,%edi
	rorl	$6,%r13d
	addl	%r12d,%ecx
	pxor	%xmm5,%xmm4
	andl	%edi,%r15d
	xorl	%edx,%r14d
	psrld	$10,%xmm7
	addl	%r13d,%ecx
	xorl	%r8d,%r15d
	paddd	%xmm4,%xmm1
	addl	%ecx,%r10d
	rorl	$2,%r14d
	addl	%r15d,%ecx
	movl	%r10d,%r13d
	psrlq	$17,%xmm6
	addl	%ecx,%r14d
	rorl	$14,%r13d
	movl	%r14d,%ecx
	movl	%r11d,%r12d
	pxor	%xmm6,%xmm7
	xorl	%r10d,%r13d
	rorl	$9,%r14d
	psrlq	$2,%xmm6
	xorl	%eax,%r12d
	rorl	$5,%r13d
	xorl	%ecx,%r14d
	pxor	%xmm6,%xmm7
	andl	%r10d,%r12d
	xorl	%r10d,%r13d
	addl	24(%rsp),%ebx
.byte	102,65,15,56,0,248
	movl	%ecx,%r15d
	rorl	$11,%r14d
	xorl	%eax,%r12d
	xorl	%edx,%r15d
	rorl	$6,%r13d
	paddd	%xmm7,%xmm1
	addl	%r12d,%ebx
	pshufd	$80,%xmm1,%xmm7
	andl	%r15d,%edi
	xorl	%ecx,%r14d
	addl	%r13d,%ebx
	xorl	%edx,%edi
	movdqa	%xmm7,%xmm6
	addl	%ebx,%r9d
	rorl	$2,%r14d
	addl	%edi,%ebx
	psrld	$10,%xmm7
	movl	%r9d,%r13d
	psrlq	$17,%xmm6
	addl	%ebx,%r14d
	rorl	$14,%r13d
	movl	%r14d,%ebx
	movl	%r10d,%r12d
	pxor	%xmm6,%xmm7
	xorl	%r9d,%r13d
	rorl	$9,%r14d
	xorl	%r11d,%r12d
	psrlq	$2,%xmm6
	rorl	$5,%r13d
	xorl	%ebx,%r14d
	andl	%r9d,%r12d
	xorl	%r9d,%r13d
	pxor	%xmm6,%xmm7
	addl	28(%rsp),%eax
	movl	%ebx,%edi
	movdqa	32(%rbp),%xmm6
	rorl	$11,%r14d
	xorl	%r11d,%r12d
.byte	102,65,15,56,0,249
	xorl	%ecx,%edi
	rorl	$6,%r13d
	addl	%r12d,%eax
	andl	%edi,%r15d
	xorl	%ebx,%r14d
	paddd	%xmm7,%xmm1
	addl	%r13d,%eax
	xorl	%ecx,%r15d
	addl	%eax,%r8d
	paddd	%xmm1,%xmm6
	rorl	$2,%r14d
	addl	%r15d,%eax
	movl	%r8d,%r13d
	addl	%eax,%r14d
	movdqa	%xmm6,16(%rsp)
	rorl	$14,%r13d
	movl	%r14d,%eax
	movdqa	%xmm3,%xmm4
	movl	%r9d,%r12d
	movdqa	%xmm1,%xmm7
	xorl	%r8d,%r13d
	rorl	$9,%r14d
	xorl	%r10d,%r12d
.byte	102,15,58,15,226,4
	rorl	$5,%r13d
	xorl	%eax,%r14d
.byte	102,15,58,15,248,4
	andl	%r8d,%r12d
	xorl	%r8d,%r13d
	addl	32(%rsp),%r11d
	movl	%eax,%r15d
	rorl	$11,%r14d
	xorl	%r10d,%r12d
	movdqa	%xmm4,%xmm5
	xorl	%ebx,%r15d
	movdqa	%xmm4,%xmm6
	rorl	$6,%r13d
	addl	%r12d,%r11d
	andl	%r15d,%edi
	psrld	$3,%xmm4
	xorl	%eax,%r14d
	addl	%r13d,%r11d
	xorl	%ebx,%edi
	paddd	%xmm7,%xmm2
	addl	%r11d,%edx
	rorl	$2,%r14d
	addl	%edi,%r11d
	psrld	$7,%xmm6
	movl	%edx,%r13d
	addl	%r11d,%r14d
	rorl	$14,%r13d
	movl	%r14d,%r11d
	pshufd	$250,%xmm1,%xmm7
	movl	%r8d,%r12d
	pslld	$14,%xmm5
	xorl	%edx,%r13d
	pxor	%xmm6,%xmm4
	rorl	$9,%r14d
	xorl	%r9d,%r12d
	psrld	$11,%xmm6
	rorl	$5,%r13d
	xorl	%r11d,%r14d
	pxor	%xmm5,%xmm4
	andl	%edx,%r12d
	xorl	%edx,%r13d
	pslld	$11,%xmm5
	addl	36(%rsp),%r10d
	pxor	%xmm6,%xmm4
	movl	%r11d,%edi
	rorl	$11,%r14d
	xorl	%r9d,%r12d
	movdqa	%xmm7,%xmm6
	xorl	%eax,%edi
	rorl	$6,%r13d
	addl	%r12d,%r10d
	pxor	%xmm5,%xmm4
	andl	%edi,%r15d
	xorl	%r11d,%r14d
	psrld	$10,%xmm7
	addl	%r13d,%r10d
	xorl	%eax,%r15d
	paddd	%xmm4,%xmm2
	addl	%r10d,%ecx
	rorl	$2,%r14d
	addl	%r15d,%r10d
	movl	%ecx,%r13d
	psrlq	$17,%xmm6
	addl	%r10d,%r14d
	rorl	$14,%r13d
	movl	%r14d,%r10d
	movl	%edx,%r12d
	pxor	%xmm6,%xmm7
	xorl	%ecx,%r13d
	rorl	$9,%r14d
	psrlq	$2,%xmm6
	xorl	%r8d,%r12d
	rorl	$5,%r13d
	xorl	%r10d,%r14d
	pxor	%xmm6,%xmm7
	andl	%ecx,%r12d
	xorl	%ecx,%r13d
	addl	40(%rsp),%r9d
.byte	102,65,15,56,0,248
	movl	%r10d,%r15d
	rorl	$11,%r14d
	xorl	%r8d,%r12d
	xorl	%r11d,%r15d
	rorl	$6,%r13d
	paddd	%xmm7,%xmm2
	addl	%r12d,%r9d
	pshufd	$80,%xmm2,%xmm7
	andl	%r15d,%edi
	xorl	%r10d,%r14d
	addl	%r13d,%r9d
	xorl	%r11d,%edi
	movdqa	%xmm7,%xmm6
	addl	%r9d,%ebx
	rorl	$2,%r14d
	addl	%edi,%r9d
	psrld	$10,%xmm7
	movl	%ebx,%r13d
	psrlq	$17,%xmm6
	addl	%r9d,%r14d
	rorl	$14,%r13d
	movl	%r14d,%r9d
	movl	%ecx,%r12d
	pxor	%xmm6,%xmm7
	xorl	%ebx,%r13d
	rorl	$9,%r14d
	xorl	%edx,%r12d
	psrlq	$2,%xmm6
	rorl	$5,%r13d
	xorl	%r9d,%r14d
	andl	%ebx,%r12d
	xorl	%ebx,%r13d
	pxor	%xmm6,%xmm7
	addl	44(%rsp),%r8d
	movl	%r9d,%edi
	movdqa	64(%rbp),%xmm6
	rorl	$11,%r14d
	xorl	%edx,%r12d
.byte	102,65,15,56,0,249
	xorl	%r10d,%edi
	rorl	$6,%r13d
	addl	%r12d,%r8d
	andl	%edi,%r15d
	xorl	%r9d,%r14d
	paddd	%xmm7,%xmm2
	addl	%r13d,%r8d
	xorl	%r10d,%r15d
	addl	%r8d,%eax
	paddd	%xmm2,%xmm6
	rorl	$2,%r14d
	addl	%r15d,%r8d
	movl	%eax,%r13d
	addl	%r8d,%r14d
	movdqa	%xmm6,32(%rsp)
	rorl	$14,%r13d
	movl	%r14d,%r8d
	movdqa	%xmm0,%xmm4
	movl	%ebx,%r12d
	movdqa	%xmm2,%xmm7
	xorl	%eax,%r13d
	rorl	$9,%r14d
	xorl	%ecx,%r12d
.byte	102,15,58,15,227,4
	rorl	$5,%r13d
	xorl	%r8d,%r14d
.byte	102,15,58,15,249,4
	andl	%eax,%r12d
	xorl	%eax,%r13d
	addl	48(%rsp),%edx
	movl	%r8d,%r15d
	rorl	$11,%r14d
	xorl	%ecx,%r12d
	movdqa	%xmm4,%xmm5
	xorl	%r9d,%r15d
	movdqa	%xmm4,%xmm6
	rorl	$6,%r13d
	addl	%r12d,%edx
	andl	%r15d,%edi
	psrld	$3,%xmm4
	xorl	%r8d,%r14d
	addl	%r13d,%edx
	xorl	%r9d,%edi
	paddd	%xmm7,%xmm3
	addl	%edx,%r11d
	rorl	$2,%r14d
	addl	%edi,%edx
	psrld	$7,%xmm6
	movl	%r11d,%r13d
	addl	%edx,%r14d
	rorl	$14,%r13d
	movl	%r14d,%edx
	pshufd	$250,%xmm2,%xmm7
	movl	%eax,%r12d
	pslld	$14,%xmm5
	xorl	%r11d,%r13d
	pxor	%xmm6,%xmm4
	rorl	$9,%r14d
	xorl	%ebx,%r12d
	psrld	$11,%xmm6
	rorl	$5,%r13d
	xorl	%edx,%r14d
	pxor	%xmm5,%xmm4
	andl	%r11d,%r12d
	xorl	%r11d,%r13d
	pslld	$11,%xmm5
	addl	52(%rsp),%ecx
	pxor	%xmm6,%xmm4
	movl	%edx,%edi
	rorl	$11,%r14d
	xorl	%ebx,%r12d
	movdqa	%xmm7,%xmm6
	xorl	%r8d,%edi
	rorl	$6,%r13d
	addl	%r12d,%ecx
	pxor	%xmm5,%xmm4
	andl	%edi,%r15d
	xorl	%edx,%r14d
	psrld	$10,%xmm7
	addl	%r13d,%ecx
	xorl	%r8d,%r15d
	paddd	%xmm4,%xmm3
	addl	%ecx,%r10d
	rorl	$2,%r14d
	addl	%r15d,%ecx
	movl	%r10d,%r13d
	psrlq	$17,%xmm6
	addl	%ecx,%r14d
	rorl	$14,%r13d
	movl	%r14d,%ecx
	movl	%r11d,%r12d
	pxor	%xmm6,%xmm7
	xorl	%r10d,%r13d
	rorl	$9,%r14d
	psrlq	$2,%xmm6
	xorl	%eax,%r12d
	rorl	$5,%r13d
	xorl	%ecx,%r14d
	pxor	%xmm6,%xmm7
	andl	%r10d,%r12d
	xorl	%r10d,%r13d
	addl	56(%rsp),%ebx
.byte	102,65,15,56,0,248
	movl	%ecx,%r15d
	rorl	$11,%r14d
	xorl	%eax,%r12d
	xorl	%edx,%r15d
	rorl	$6,%r13d
	paddd	%xmm7,%xmm3
	addl	%r12d,%ebx
	pshufd	$80,%xmm3,%xmm7
	andl	%r15d,%edi
	xorl	%ecx,%r14d
	addl	%r13d,%ebx
	xorl	%edx,%edi
	movdqa	%xmm7,%xmm6
	addl	%ebx,%r9d
	rorl	$2,%r14d
	addl	%edi,%ebx
	psrld	$10,%xmm7
	movl	%r9d,%r13d
	psrlq	$17,%xmm6
	addl	%ebx,%r14d
	rorl	$14,%r13d
	movl	%r14d,%ebx
	movl	%r10d,%r12d
	pxor	%xmm6,%xmm7
	xorl	%r9d,%r13d
	rorl	$9,%r14d
	xorl	%r11d,%r12d
	psrlq	$2,%xmm6
	rorl	$5,%r13d
	xorl	%ebx,%r14d
	andl	%r9d,%r12d
	xorl	%r9d,%r13d
	pxor	%xmm6,%xmm7
	addl	60(%rsp),%eax
	movl	%ebx,%edi
	movdqa	96(%rbp),%xmm6
	rorl	$11,%r14d
	xorl	%r11d,%r12d
.byte	102,65,15,56,0,249
	xorl	%ecx,%edi
	rorl	$6,%r13d
	addl	%r12d,%eax
	andl	%edi,%r15d
	xorl	%ebx,%r14d
	paddd	%xmm7,%xmm3
	addl	%r13d,%eax
	xorl	%ecx,%r15d
	addl	%eax,%r8d
	paddd	%xmm3,%xmm6
	rorl	$2,%r14d
	addl	%r15d,%eax
	movl	%r8d,%r13d
	addl	%eax,%r14d
	movdqa	%xmm6,48(%rsp)
	cmpb	$0,131(%rbp)
	jne	.Lssse3_00_47
	rorl	$14,%r13d
	movl	%r14d,%eax
	movl	%r9d,%r12d
	xorl	%r8d,%r13d
	rorl	$9,%r14d
	xorl	%r10d,%r12d
	rorl	$5,%r13d
	xorl	%eax,%r14d
	andl	%r8d,%r12d
	xorl	%r8d,%r13d
	addl	0(%rsp),%r11d
	movl	%eax,%r15d
	rorl	$11,%r14d
	xorl	%r10d,%r12d
	xorl	%ebx,%r15d
	rorl	$6,%r13d
	addl	%r12d,%r11d
	andl	%r15d,%edi
	xorl	%eax,%r14d
	addl	%r13d,%r11d
	xorl	%ebx,%edi
	addl	%r11d,%edx
	rorl	$2,%r14d
	addl	%edi,%r11d
	movl	%edx,%r13d
	addl	%r11d,%r14d
	rorl	$14,%r13d
	movl	%r14d,%r11d
	movl	%r8d,%r12d
	xorl	%edx,%r13d
	rorl	$9,%r14d
	xorl	%r9d,%r12d
	rorl	$5,%r13d
	xorl	%r11d,%r14d
	andl	%edx,%r12d
	xorl	%edx,%r13d
	addl	4(%rsp),%r10d
	movl	%r11d,%edi
	rorl	$11,%r14d
	xorl	%r9d,%r12d
	xorl	%eax,%edi
	rorl	$6,%r13d
	addl	%r12d,%r10d
	andl	%edi,%r15d
	xorl	%r11d,%r14d
	addl	%r13d,%r10d
	xorl	%eax,%r15d
	addl	%r10d,%ecx
	rorl	$2,%r14d
	addl	%r15d,%r10d
	movl	%ecx,%r13d
	addl	%r10d,%r14d
	rorl	$14,%r13d
	movl	%r14d,%r10d
	movl	%edx,%r12d
	xorl	%ecx,%r13d
	rorl	$9,%r14d
	xorl	%r8d,%r12d
	rorl	$5,%r13d
	xorl	%r10d,%r14d
	andl	%ecx,%r12d
	xorl	%ecx,%r13d
	addl	8(%rsp),%r9d
	movl	%r10d,%r15d
	rorl	$11,%r14d
	xorl	%r8d,%r12d
	xorl	%r11d,%r15d
	rorl	$6,%r13d
	addl	%r12d,%r9d
	andl	%r15d,%edi
	xorl	%r10d,%r14d
	addl	%r13d,%r9d
	xorl	%r11d,%edi
	addl	%r9d,%ebx
	rorl	$2,%r14d
	addl	%edi,%r9d
	movl	%ebx,%r13d
	addl	%r9d,%r14d
	rorl	$14,%r13d
	movl	%r14d,%r9d
	movl	%ecx,%r12d
	xorl	%ebx,%r13d
	rorl	$9,%r14d
	xorl	%edx,%r12d
	rorl	$5,%r13d
	xorl	%r9d,%r14d
	andl	%ebx,%r12d
	xorl	%ebx,%r13d
	addl	12(%rsp),%r8d
	movl	%r9d,%edi
	rorl	$11,%r14d
	xorl	%edx,%r12d
	xorl	%r10d,%edi
	rorl	$6,%r13d
	addl	%r12d,%r8d
	andl	%edi,%r15d
	xorl	%r9d,%r14d
	addl	%r13d,%r8d
	xorl	%r10d,%r15d
	addl	%r8d,%eax
	rorl	$2,%r14d
	addl	%r15d,%r8d
	movl	%eax,%r13d
	addl	%r8d,%r14d
	rorl	$14,%r13d
	movl	%r14d,%r8d
	movl	%ebx,%r12d
	xorl	%eax,%r13d
	rorl	$9,%r14d
	xorl	%ecx,%r12d
	rorl	$5,%r13d
	xorl	%r8d,%r14d
	andl	%eax,%r12d
	xorl	%eax,%r13d
	addl	16(%rsp),%edx
	movl	%r8d,%r15d
	rorl	$11,%r14d
	xorl	%ecx,%r12d
	xorl	%r9d,%r15d
	rorl	$6,%r13d
	addl	%r12d,%edx
	andl	%r15d,%edi
	xorl	%r8d,%r14d
	addl	%r13d,%edx
	xorl	%r9d,%edi
	addl	%edx,%r11d
	rorl	$2,%r14d
	addl	%edi,%edx
	movl	%r11d,%r13d
	addl	%edx,%r14d
	rorl	$14,%r13d
	movl	%r14d,%edx
	movl	%eax,%r12d
	xorl	%r11d,%r13d
	rorl	$9,%r14d
	xorl	%ebx,%r12d
	rorl	$5,%r13d
	xorl	%edx,%r14d
	andl	%r11d,%r12d
	xorl	%r11d,%r13d
	addl	20(%rsp),%ecx
	movl	%edx,%edi
	rorl	$11,%r14d
	xorl	%ebx,%r12d
	xorl	%r8d,%edi
	rorl	$6,%r13d
	addl	%r12d,%ecx
	andl	%edi,%r15d
	xorl	%edx,%r14d
	addl	%r13d,%ecx
	xorl	%r8d,%r15d
	addl	%ecx,%r10d
	rorl	$2,%r14d
	addl	%r15d,%ecx
	movl	%r10d,%r13d
	addl	%ecx,%r14d
	rorl	$14,%r13d
	movl	%r14d,%ecx
	movl	%r11d,%r12d
	xorl	%r10d,%r13d
	rorl	$9,%r14d
	xorl	%eax,%r12d
	rorl	$5,%r13d
	xorl	%ecx,%r14d
	andl	%r10d,%r12d
	xorl	%r10d,%r13d
	addl	24(%rsp),%ebx
	movl	%ecx,%r15d
	rorl	$11,%r14d
	xorl	%eax,%r12d
	xorl	%edx,%r15d
	rorl	$6,%r13d
	addl	%r12d,%ebx
	andl	%r15d,%edi
	xorl	%ecx,%r14d
	addl	%r13d,%ebx
	xorl	%edx,%edi
	addl	%ebx,%r9d
	rorl	$2,%r14d
	addl	%edi,%ebx
	movl	%r9d,%r13d
	addl	%ebx,%r14d
	rorl	$14,%r13d
	movl	%r14d,%ebx
	movl	%r10d,%r12d
	xorl	%r9d,%r13d
	rorl	$9,%r14d
	xorl	%r11d,%r12d
	rorl	$5,%r13d
	xorl	%ebx,%r14d
	andl	%r9d,%r12d
	xorl	%r9d,%r13d
	addl	28(%rsp),%eax
	movl	%ebx,%edi
	rorl	$11,%r14d
	xorl	%r11d,%r12d
	xorl	%ecx,%edi
	rorl	$6,%r13d
	addl	%r12d,%eax
	andl	%edi,%r15d
	xorl	%ebx,%r14d
	addl	%r13d,%eax
	xorl	%ecx,%r15d
	addl	%eax,%r8d
	rorl	$2,%r14d
	addl	%r15d,%eax
	movl	%r8d,%r13d
	addl	%eax,%r14d
	rorl	$14,%r13d
	movl	%r14d,%eax
	movl	%r9d,%r12d
	xorl	%r8d,%r13d
	rorl	$9,%r14d
	xorl	%r10d,%r12d
	rorl	$5,%r13d
	xorl	%eax,%r14d
	andl	%r8d,%r12d
	xorl	%r8d,%r13d
	addl	32(%rsp),%r11d
	movl	%eax,%r15d
	rorl	$11,%r14d
	xorl	%r10d,%r12d
	xorl	%ebx,%r15d
	rorl	$6,%r13d
	addl	%r12d,%r11d
	andl	%r15d,%edi
	xorl	%eax,%r14d
	addl	%r13d,%r11d
	xorl	%ebx,%edi
	addl	%r11d,%edx
	rorl	$2,%r14d
	addl	%edi,%r11d
	movl	%edx,%r13d
	addl	%r11d,%r14d
	rorl	$14,%r13d
	movl	%r14d,%r11d
	movl	%r8d,%r12d
	xorl	%edx,%r13d
	rorl	$9,%r14d
	xorl	%r9d,%r12d
	rorl	$5,%r13d
	xorl	%r11d,%r14d
	andl	%edx,%r12d
	xorl	%edx,%r13d
	addl	36(%rsp),%r10d
	movl	%r11d,%edi
	rorl	$11,%r14d
	xorl	%r9d,%r12d
	xorl	%eax,%edi
	rorl	$6,%r13d
	addl	%r12d,%r10d
	andl	%edi,%r15d
	xorl	%r11d,%r14d
	addl	%r13d,%r10d
	xorl	%eax,%r15d
	addl	%r10d,%ecx
	rorl	$2,%r14d
	addl	%r15d,%r10d
	movl	%ecx,%r13d
	addl	%r10d,%r14d
	rorl	$14,%r13d
	movl	%r14d,%r10d
	movl	%edx,%r12d
	xorl	%ecx,%r13d
	rorl	$9,%r14d
	xorl	%r8d,%r12d
	rorl	$5,%r13d
	xorl	%r10d,%r14d
	andl	%ecx,%r12d
	xorl	%ecx,%r13d
	addl	40(%rsp),%r9d
	movl	%r10d,%r15d
	rorl	$11,%r14d
	xorl	%r8d,%r12d
	xorl	%r11d,%r15d
	rorl	$6,%r13d
	addl	%r12d,%r9d
	andl	%r15d,%edi
	xorl	%r10d,%r14d
	addl	%r13d,%r9d
	xorl	%r11d,%edi
	addl	%r9d,%ebx
	rorl	$2,%r14d
	addl	%edi,%r9d
	movl	%ebx,%r13d
	addl	%r9d,%r14d
	rorl	$14,%r13d
	movl	%r14d,%r9d
	movl	%ecx,%r12d
	xorl	%ebx,%r13d
	rorl	$9,%r14d
	xorl	%edx,%r12d
	rorl	$5,%r13d
	xorl	%r9d,%r14d
	andl	%ebx,%r12d
	xorl	%ebx,%r13d
	addl	44(%rsp),%r8d
	movl	%r9d,%edi
	rorl	$11,%r14d
	xorl	%edx,%r12d
	xorl	%r10d,%edi
	rorl	$6,%r13d
	addl	%r12d,%r8d
	andl	%edi,%r15d
	xorl	%r9d,%r14d
	addl	%r13d,%r8d
	xorl	%r10d,%r15d
	addl	%r8d,%eax
	rorl	$2,%r14d
	addl	%r15d,%r8d
	movl	%eax,%r13d
	addl	%r8d,%r14d
	rorl	$14,%r13d
	movl	%r14d,%r8d
	movl	%ebx,%r12d
	xorl	%eax,%r13d
	rorl	$9,%r14d
	xorl	%ecx,%r12d
	rorl	$5,%r13d
	xorl	%r8d,%r14d
	andl	%eax,%r12d
	xorl	%eax,%r13d
	addl	48(%rsp),%edx
	movl	%r8d,%r15d
	rorl	$11,%r14d
	xorl	%ecx,%r12d
	xorl	%r9d,%r15d
	rorl	$6,%r13d
	addl	%r12d,%edx
	andl	%r15d,%edi
	xorl	%r8d,%r14d
	addl	%r13d,%edx
	xorl	%r9d,%edi
	addl	%edx,%r11d
	rorl	$2,%r14d
	addl	%edi,%edx
	movl	%r11d,%r13d
	addl	%edx,%r14d
	rorl	$14,%r13d
	movl	%r14d,%edx
	movl	%eax,%r12d
	xorl	%r11d,%r13d
	rorl	$9,%r14d
	xorl	%ebx,%r12d
	rorl	$5,%r13d
	xorl	%edx,%r14d
	andl	%r11d,%r12d
	xorl	%r11d,%r13d
	addl	52(%rsp),%ecx
	movl	%edx,%edi
	rorl	$11,%r14d
	xorl	%ebx,%r12d
	xorl	%r8d,%edi
	rorl	$6,%r13d
	addl	%r12d,%ecx
	andl	%edi,%r15d
	xorl	%edx,%r14d
	addl	%r13d,%ecx
	xorl	%r8d,%r15d
	addl	%ecx,%r10d
	rorl	$2,%r14d
	addl	%r15d,%ecx
	movl	%r10d,%r13d
	addl	%ecx,%r14d
	rorl	$14,%r13d
	movl	%r14d,%ecx
	movl	%r11d,%r12d
	xorl	%r10d,%r13d
	rorl	$9,%r14d
	xorl	%eax,%r12d
	rorl	$5,%r13d
	xorl	%ecx,%r14d
	andl	%r10d,%r12d
	xorl	%r10d,%r13d
	addl	56(%rsp),%ebx
	movl	%ecx,%r15d
	rorl	$11,%r14d
	xorl	%eax,%r12d
	xorl	%edx,%r15d
	rorl	$6,%r13d
	addl	%r12d,%ebx
	andl	%r15d,%edi
	xorl	%ecx,%r14d
	addl	%r13d,%ebx
	xorl	%edx,%edi
	addl	%ebx,%r9d
	rorl	$2,%r14d
	addl	%edi,%ebx
	movl	%r9d,%r13d
	addl	%ebx,%r14d
	rorl	$14,%r13d
	movl	%r14d,%ebx
	movl	%r10d,%r12d
	xorl	%r9d,%r13d
	rorl	$9,%r14d
	xorl	%r11d,%r12d
	rorl	$5,%r13d
	xorl	%ebx,%r14d
	andl	%r9d,%r12d
	xorl	%r9d,%r13d
	addl	60(%rsp),%eax
	movl	%ebx,%edi
	rorl	$11,%r14d
	xorl	%r11d,%r12d
	xorl	%ecx,%edi
	rorl	$6,%r13d
	addl	%r12d,%eax
	andl	%edi,%r15d
	xorl	%ebx,%r14d
	addl	%r13d,%eax
	xorl	%ecx,%r15d
	addl	%eax,%r8d
	rorl	$2,%r14d
	addl	%r15d,%eax
	movl	%r8d,%r13d
	addl	%eax,%r14d
	movq	64+0(%rsp),%rdi
	movl	%r14d,%eax

	addl	0(%rdi),%eax
	leaq	64(%rsi),%rsi
	addl	4(%rdi),%ebx
	addl	8(%rdi),%ecx
	addl	12(%rdi),%edx
	addl	16(%rdi),%r8d
	addl	20(%rdi),%r9d
	addl	24(%rdi),%r10d
	addl	28(%rdi),%r11d

	cmpq	64+16(%rsp),%rsi

	movl	%eax,0(%rdi)
	movl	%ebx,4(%rdi)
	movl	%ecx,8(%rdi)
	movl	%edx,12(%rdi)
	movl	%r8d,16(%rdi)
	movl	%r9d,20(%rdi)
	movl	%r10d,24(%rdi)
	movl	%r11d,28(%rdi)
	jb	.Lloop_ssse3

	movq	64+24(%rsp),%rsi
	movq	(%rsi),%r15
	movq	8(%rsi),%r14
	movq	16(%rsi),%r13
	movq	24(%rsi),%r12
	movq	32(%rsi),%rbp
	movq	40(%rsi),%rbx
	leaq	48(%rsi),%rsp
.Lepilogue_ssse3:
	.byte	0xf3,0xc3
.size	sha256_block_data_order_ssse3,.-sha256_block_data_order_ssse3


.section .note.GNU-stack,"",%progbits


