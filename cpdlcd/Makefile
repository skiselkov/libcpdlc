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

CFLAGS += -W -Wall -Wextra -Werror -Wno-unused-result -fPIC -O0 -g -DDEBUG \
	-std=c99 $(PLATFORM_DEFS) -D_GNU_SOURCE \
	-I$(ACFUTILS)/src $(LWS_CFLAGS) \
	$(shell $(ACFUTILS)/pkg-config-deps $(PLATFORM_NAME) --cflags) \
	$(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config gnutls --cflags)
LIBS += -fPIC -L$(ACFUTILS)/qmake/$(PLATFORM_LIBNAME) -lacfutils $(LWS_LIBS) \
	$(shell $(ACFUTILS)/pkg-config-deps $(PLATFORM_NAME) --libs) \
	$(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config \
	--static --libs gnutls) -lcrypto $(LIBCRYPT) -lpthread -lm

DAEMON_OBJS=\
	auth.o \
	blocklist.o \
	cpdlcd.o \
	msgquota.o \
	$(COMPREFIX)/cpdlc_config_common.o \
	$(SRCPREFIX)/cpdlc_assert.o \
	$(SRCPREFIX)/cpdlc_infos.o \
	$(SRCPREFIX)/cpdlc_msg.o

all : cpdlcd

redist : cpdlcd.tar.gz

cpdlcd.tar.gz : cpdlcd
	mkdir cpdlcd-redist
	cp cpdlcd cpdlcd-redist/

clean :
	rm -f cpdlcd $(DAEMON_OBJS)

cpdlcd : $(DAEMON_OBJS) $(LWS_OBJS)
	$(HOSTCC) $(LDFLAGS) -o $@ $^ $(LIBS)

include ../Makefile.rules
