#
# Copyright (c) 2010, Atheros Communications Inc.
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#
# $Id: //depot/sw/releases/Aquila_9.2.0_U10/drivers/wlan/hal/linux/public/powerpc-be-eabi.inc#1 $
#

#
# Compilation configuration for building big-endian PowerPC/powerpc-eabi.
#
# Known to work on:
#	IBM 450EP

#
# Built with GNU cross-devel tools:
#
# PREFIX=/pub/gnu
# BINUTILS=binutils-2.14
# GCC=gcc-3.3.2
# target=powerpc-eabi
#
# ${BINUTILS}/configure --target=$target --prefix=${PREFIX}
# ${GCC}/configure --target=$target   --prefix=${PREFIX} \
#	--enable-languages=c   --with-gnu-as --with-gnu-ld \
#	--with-newlib   --with-gxx-include-dir=${PREFIX}/$target/include
#
ifndef TOOLPREFIX
TOOLPREFIX=	/pub/gnu/bin/powerpc-eabi-
endif
#
CC=	${TOOLPREFIX}gcc
LD=	${TOOLPREFIX}ld
STRIP=	${TOOLPREFIX}strip
OBJCOPY=${TOOLPREFIX}objcopy
NM=	${TOOLPREFIX}nm

LDOPTS=	-EB
# zhaoyang1 transplant from 717
ifeq ($(BOARD_TYPE),amcc)
COPTS+=	-DAH_BYTE_ORDER=AH_BIG_ENDIAN -DAH_REGOPS_FUNC -D__LINUX_POWERPC_ARCH__
COPTS+= -ffixed-r2
else
COPTS+=	-DAH_BYTE_ORDER=AH_BIG_ENDIAN -DAH_REGOPS_FUNC
COPTS+= -msoft-float -ffixed-r2
endif
COPTS+= -mbig-endian
# zhaoyang1 transplant end

