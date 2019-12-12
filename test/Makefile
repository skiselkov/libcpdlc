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

CFLAGS += -W -Wall -Wextra -Werror -O0 -g -DDEBUG -I$(SRCPREFIX) \
    $(PLATFORM_DEFS) $(shell pkg-config gnutls --cflags)
LIBS += $(shell pkg-config gnutls --libs) -lpthread -lm
CLIENT_TEST_LIBS += -lncursesw

MSGTEST_OBJS = \
	msgtest.o \
	$(CORE_SRC_OBJS)

CLIENT_TEST_OBJS = \
	client_test.o \
	$(CORE_SRC_OBJS) \
	$(FANS_OBJS)

all : msgtest client_test

clean :
	rm -f msgtest $(MSGTEST_OBJS) client_test $(CLIENT_TEST_OBJS)

msgtest : $(MSGTEST_OBJS)
	$(HOSTCC) $(LDFLAGS) -o $@ $^ $(LIBS)

client_test : $(CLIENT_TEST_OBJS)
	$(HOSTCC) $(LDFLAGS) -o $@ $^ $(CLIENT_TEST_LIBS) $(LIBS)

include ../Makefile.rules
