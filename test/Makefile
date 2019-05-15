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

include ../Makefile.com

CFLAGS += -W -Wall -Wextra -Werror -O0 -g -DDEBUG $(PLATFORM_DEFS) \
    $(shell pkg-config gnutls --cflags)
LIBS += $(shell pkg-config gnutls --libs) -lpthread -lm
CLIENT_TEST_LIBS += -lncursesw

SRCPREFIX=../src
FMSBOX=../fmsbox

MSGTEST_OBJS = \
	msgtest.o \
	$(SRCPREFIX)/cpdlc_assert.o \
	$(SRCPREFIX)/cpdlc_infos.o \
	$(SRCPREFIX)/cpdlc_msg.o \
	$(SRCPREFIX)/minilist.o

CLIENT_TEST_OBJS = \
	client_test.o \
	$(SRCPREFIX)/cpdlc_assert.o \
	$(SRCPREFIX)/cpdlc_client.o \
	$(SRCPREFIX)/cpdlc_infos.o \
	$(SRCPREFIX)/cpdlc_msg.o \
	$(SRCPREFIX)/cpdlc_msglist.o \
	$(SRCPREFIX)/minilist.o \
	$(FMSBOX)/fmsbox.o \
	$(FMSBOX)/fmsbox_freetext.o \
	$(FMSBOX)/fmsbox_main_menu.o \
	$(FMSBOX)/fmsbox_msg.o \
	$(FMSBOX)/fmsbox_logon_status.o \
	$(FMSBOX)/fmsbox_parsing.o \
	$(FMSBOX)/fmsbox_pos_pick.o \
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

all : msgtest client_test

msgtest : $(MSGTEST_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

client_test : $(CLIENT_TEST_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(CLIENT_TEST_LIBS) $(LIBS)

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean :
	rm -f msgtest $(MSGTEST_OBJS) client_test $(CLIENT_TEST_OBJS)
