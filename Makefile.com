#
# Copyright 2019 Saso Kiselkov
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use,
# copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following
# conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.

ifeq ($(CROSSCOMPILE),win)
	PLATFORM_DEFS=-DLIN=0 -DIBM=1 -DAPL=0 -DSUN=0 -D_WIN32_WINNT=0x0600
	PLATFORM_NAME=win-64
	PLATFORM_LIBNAME=win64
	HOSTCC=x86_64-w64-mingw32-gcc
else
	UNAME=$(shell uname)
	ifeq ($(UNAME),Linux)
		PLATFORM_DEFS=-DLIN=1 -DIBM=0 -DAPL=0 -DSUN=0
		PLATFORM_NAME=linux-64
		PLATFORM_LIBNAME=lin64
		HOSTCC=$(CC)
	endif
	ifeq ($(UNAME),Darwin)
		PLATFORM_DEFS=-DLIN=0 -DIBM=0 -DAPL=1 -DSUN=0
		PLATFORM_NAME=mac-64
		PLATFORM_LIBNAME=mac64
		HOSTCC=$(CC)
	endif
	ifeq ($(UNAME),SunOS)
		PLATFORM_DEFS=-DLIN=0 -DIBM=0 -DAPL=0 -DSUN=1
		PLATFORM_NAME=mac-64
		PLATFORM_LIBNAME=mac64
		HOSTCC=$(CC)
	endif
endif

SRCPREFIX=../src
FANS=../fans

CORE_SRC_OBJS=\
	$(SRCPREFIX)/cpdlc_assert.o \
	$(SRCPREFIX)/cpdlc_client.o \
	$(SRCPREFIX)/cpdlc_infos.o \
	$(SRCPREFIX)/cpdlc_msg.o \
	$(SRCPREFIX)/cpdlc_msglist.o \
	$(SRCPREFIX)/minilist.o \

FANS_OBJS=\
	$(FANS)/fans.o \
	$(FANS)/fans_emer.o \
	$(FANS)/fans_freetext.o \
	$(FANS)/fans_main_menu.o \
	$(FANS)/fans_msg.o \
	$(FANS)/fans_logon_status.o \
	$(FANS)/fans_parsing.o \
	$(FANS)/fans_pos_pick.o \
	$(FANS)/fans_pos_rep.o \
	$(FANS)/fans_rej.o \
	$(FANS)/fans_req.o \
	$(FANS)/fans_req_alt.o \
	$(FANS)/fans_req_clx.o \
	$(FANS)/fans_req_off.o \
	$(FANS)/fans_req_rte.o \
	$(FANS)/fans_req_spd.o \
	$(FANS)/fans_req_vmc.o \
	$(FANS)/fans_req_wcw.o \
	$(FANS)/fans_req_voice.o \
	$(FANS)/fans_scratchpad.o \
	$(FANS)/fans_vrfy.o

LWS_CFLAGS=-I../libwebsockets-3.1.0/include
LWS_LIBS=../libwebsockets-3.1.0/lib/libwebsockets.a
LWS_OBJS=../libwebsockets-3.1.0/lib/libwebsockets.a

ACFUTILS=../libacfutils
GNUTLS=../gnutls-3.6.11.1
NETTLE=../nettle-3.5
PKG_CONFIG_PATH=$(GNUTLS)/install/lib/pkgconfig:$(NETTLE)/install/lib/pkgconfig
