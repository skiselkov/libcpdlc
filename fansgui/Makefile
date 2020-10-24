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

ifeq ($(PLATFORM_NAME),linux-64)
	PLATFORM_LIBS = -lGL -lX11 -ldl
else
	ifeq ($(PLATFORM_NAME),mac-64)
		PLATFORM_LIBS = \
		    -framework OpenGL \
		    -framework IOKit \
		    -framework CoreVideo \
		    -framework Cocoa
	else
		PLATFORM_LIBS += -lopengl32 -mwindows
	endif
endif

BOGUS_WARNINGS=\
    -Wno-unused-result \
    -Wno-missing-field-initializers

CFLAGS += -std=c99 -W -Wall -Wextra -Werror $(BOGUS_WARNINGS) -O0 -g -DDEBUG \
	$(PLATFORM_DEFS) -D_GNU_SOURCE -I$(SRCPREFIX) -I$(ACFUTILS)/src \
	$(shell $(ACFUTILS)/pkg-config-deps $(PLATFORM_NAME) --glfw --cflags) \

LIBS += -L$(ACFUTILS)/qmake/$(PLATFORM_LIBNAME) -lacfutils \
	$(shell $(ACFUTILS)/pkg-config-deps $(PLATFORM_NAME) --glfw --libs) \

ifeq ($(PLATFORM_NAME),win-64)
	CFLAGS += -I../gnutls-win64/lib $(MATH_DEFINES)
	LIBS += ../gnutls-win64/lib/libgnutls.dll.a
else
	ifeq ($(LWS),yes)
		CFLAGS += -DCPDLC_CLIENT_LWS $(LWS_CFLAGS)
		LIBS += $(LWS_LIBS)
	else
		CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) \
		    pkg-config --cflags gnutls nettle hogweed)
		LIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config \
		    --libs gnutls nettle hogweed) -lcrypto
	endif
endif
LIBS += $(PLATFORM_LIBS) -lpthread -lm

# clang doesn't properly support optimized debug code.
ifeq ($(PLATFORM_NAME),mac-64)
	CFLAGS += -O0
endif

OBJS = \
	mtcr_mini.o \
	fansgui.o \
	$(CORE_SRC_OBJS) \
	$(FANS_OBJS)
ifeq ($(PLATFORM_NAME),win-64)
	OUTBIN=fansgui.exe
else
	OUTBIN=fansgui
endif

all : $(OUTBIN)

$(OUTBIN) : $(OBJS)
	$(HOSTCC) $(LDFLAGS) -o $@ $^ $(LIBS)
ifeq ($(PLATFORM_NAME),win-64)
	rm -rf fansgui-app
	mkdir fansgui-app
	cp $(OUTBIN) \
	    sample-client.conf \
	    $(wildcard ../gnutls-win64/bin/*.dll) \
	    $(wildcard *.png) \
	    $(wildcard ../ca_cert.pem) \
	    fansgui-app
	zip -r fansgui-app.zip fansgui-app
	rm -rf fansgui-app
endif

clean :
	rm -f fansgui fansgui.exe $(OBJS)

include ../Makefile.rules
