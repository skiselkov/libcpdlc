#
# Copyright 2021 Saso Kiselkov
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

.PHONY : all clean xp-plugin

DIST_RESOURCES = \
    $(wildcard client.conf) \
    sample-client.conf \
    fans_alert.wav \
    fansgui.png \
    MCDU.png \
    MCDU_small.png \
    $(wildcard ../ca_cert.pem)

ifeq ($(PLATFORM_NAME),linux-64)
	PLATFORM_LIBS = -lGL -lX11 -ldl
else
	ifeq ($(PLATFORM_NAME),mac-64)
		PLATFORM_LIBS = \
		    -framework OpenGL \
		    -framework IOKit \
		    -framework CoreVideo \
		    -framework Cocoa \
		    -framework AudioToolbox \
		    -framework CoreAudio \
		    -framework AudioUnit \
		    -framework Security
	else
		# iphlpapi is needed for ZeroMQ
		PLATFORM_LIBS = -liphlpapi -lopengl32 -mwindows \
		    -fstack-protector
		EXE_RESOURCES = fansgui.res
	endif
endif

BOGUS_WARNINGS = -Wno-unused-result -Wno-missing-field-initializers
ifneq ($(PLATFORM_NAME),mac-64)
	BOGUS_WARNINGS += -Wno-format-truncation -Wno-stringop-truncation
endif
PKG_CONFIG_DEPS_FLAGS = --glfw --no-tesseract --no-shapelib --static-openal
ZEROMQ=zmq/zeromq-$(PLATFORM_NAME)/install

CFLAGS += $(shell PKG_CONFIG_PATH=$(ZEROMQ)/lib/pkgconfig pkg-config \
    --cflags libzmq) -DZMQ_STATIC
LIBS += $(shell PKG_CONFIG_PATH=$(ZEROMQ)/lib/pkgconfig pkg-config \
    --libs libzmq)

CFLAGS += -std=c99 -W -Wall -Wextra $(BOGUS_WARNINGS) -Werror -O2 -g -DDEBUG \
    -DLIBCPDLC_VERSION='"$(LIBCPDLC_VERSION)"' $(PLATFORM_DEFS) \
    -D_GNU_SOURCE -I$(SRCPREFIX) -I$(SRCPREFIX)/asn1 -I$(ACFUTILS)/src \
    $(shell $(ACFUTILS)/pkg-config-deps $(PLATFORM_NAME) \
    $(PKG_CONFIG_DEPS_FLAGS) --cflags)

LIBS += -L$(ACFUTILS)/qmake/$(PLATFORM_LIBNAME) -lacfutils \
    $(shell $(ACFUTILS)/pkg-config-deps $(PLATFORM_NAME) \
    $(PKG_CONFIG_DEPS_FLAGS) --libs) \

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
		ifeq ($(PLATFORM_NAME),linux-64)
			LIBS += -Wl,-Bstatic \
			    $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) \
			    pkg-config --libs --static gnutls libidn2 gmp) \
			    -Wl,-Bdynamic
			LIBS += -static-libgcc
		else
			LIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) \
			    pkg-config --libs --static gnutls gmp)
		endif
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
	xpintf.o \
	$(CORE_SRC_OBJS) \
	$(ASN_SRC_OBJS) \
	$(FANS_OBJS)
ifeq ($(PLATFORM_NAME),win-64)
	OUTBIN=fansgui.exe
else
	OUTBIN=fansgui
endif

all : xp-plugin $(OUTBIN)

xp-plugin :
	[ -f xp-plugin/CMakeCache.txt ] || ( cd xp-plugin && cmake . )
	( cd xp-plugin && make )

OUTDIR=fansgui-$(PLATFORM_NAME)

$(OUTBIN) : $(OBJS) $(EXE_RESOURCES)
	$(HOSTCXX) $(LDFLAGS) -o $@ $^ $(LIBS)
	rm -rf $(OUTDIR)
	mkdir $(OUTDIR)
	cp -r xp-plugin/fans-xplane.plugin $(OUTDIR)
ifeq ($(PLATFORM_NAME),mac-64)
	mkdir -p $(OUTDIR)/fansgui.app/Contents/Resources \
	    $(OUTDIR)/fansgui.app/Contents/MacOS
	cp Info.plist $(OUTDIR)/fansgui.app/Contents
	cp $(DIST_RESOURCES) fansgui-icon.icns \
	    $(OUTDIR)/fansgui.app/Contents/Resources
	cp $(OUTBIN) $(OUTDIR)/fansgui.app/Contents/MacOS
else
	cp $(DIST_RESOURCES) $(OUTDIR)
	cp $(OUTBIN) $(OUTDIR)
	strip $(OUTDIR)/$(OUTBIN)
endif
ifeq ($(PLATFORM_NAME),win-64)
	cp $(wildcard ../gnutls-win64/bin/*.dll) $(OUTDIR)
	rm -f $(OUTDIR).zip
	zip -r $(OUTDIR).zip $(OUTDIR)
endif
ifeq ($(PLATFORM_NAME),linux-64)
	rm -f $(OUTDIR).tar.gz
	tar czf $(OUTDIR).tar.gz $(OUTDIR)
endif

clean :
	rm -f fansgui fansgui.exe $(OBJS)
	[ -f xp-plugin/Makefile ] && ( cd xp-plugin && make clean )
	rm -rf fansgui-mac-64

# Apple-specific stuff for code signing, DMG creation and notarization
ifeq ($(PLATFORM_NAME),mac-64)
include Makefile.mac
endif

include ../Makefile.rules
