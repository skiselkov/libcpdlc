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

ifeq ($(shell uname),Linux)
	PLATFORM_DEFS=-DLIN=1 -DIBM=0 -DAPL=0
	PLATFORM_NAME=linux-64
	PLATFORM_LIBNAME=lin64
else
	PLATFORM_DEFS=-DLIN=0 -DIBM=0 -DAPL=1
	PLATFORM_NAME=mac-64
	PLATFORM_LIBNAME=mac64
endif

SRCPREFIX=../src
FMSBOX=../fmsbox

CORE_SRC_OBJS=\
	$(SRCPREFIX)/cpdlc_assert.o \
	$(SRCPREFIX)/cpdlc_client.o \
	$(SRCPREFIX)/cpdlc_infos.o \
	$(SRCPREFIX)/cpdlc_msg.o \
	$(SRCPREFIX)/cpdlc_msglist.o \
	$(SRCPREFIX)/minilist.o \

FMSBOX_OBJS=\
	$(FMSBOX)/fmsbox.o \
	$(FMSBOX)/fmsbox_emer.o \
	$(FMSBOX)/fmsbox_freetext.o \
	$(FMSBOX)/fmsbox_main_menu.o \
	$(FMSBOX)/fmsbox_msg.o \
	$(FMSBOX)/fmsbox_logon_status.o \
	$(FMSBOX)/fmsbox_parsing.o \
	$(FMSBOX)/fmsbox_pos_pick.o \
	$(FMSBOX)/fmsbox_pos_rep.o \
	$(FMSBOX)/fmsbox_rej.o \
	$(FMSBOX)/fmsbox_req.o \
	$(FMSBOX)/fmsbox_req_alt.o \
	$(FMSBOX)/fmsbox_req_clx.o \
	$(FMSBOX)/fmsbox_req_off.o \
	$(FMSBOX)/fmsbox_req_rte.o \
	$(FMSBOX)/fmsbox_req_spd.o \
	$(FMSBOX)/fmsbox_req_vmc.o \
	$(FMSBOX)/fmsbox_req_wcw.o \
	$(FMSBOX)/fmsbox_req_voice.o \
	$(FMSBOX)/fmsbox_scratchpad.o \
	$(FMSBOX)/fmsbox_vrfy.o
