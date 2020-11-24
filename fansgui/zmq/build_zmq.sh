#!/bin/bash
#
# Copyright 2020 Saso Kiselkov
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

source build_dep.common

# libzmq
ZEROMQ="zeromq-4.3.1"
ZEROMQ_CONFOPTS_COMMON="--enable-static --disable-shared"
LIBZMQ_PRODUCT="install/lib/libzmq.a"
[ -d "$ZEROMQ" ] || tar xJf "$ZEROMQ.tar.xz" || exit 1
case $(uname) in
    Linux)
	CFLAGS="-g\\ -m64" CXXLAGS="-g\\ -m64" LDFLAGS="-m64" \
	    build_dep "linux-64" \
	    "$ZEROMQ_CONFOPTS_COMMON" \
	    "$ZEROMQ.tar.xz" "zeromq" "$LIBZMQ_PRODUCT" && \
	CFLAGS="-g\\ -m64" CXXLAGS="-g\\ -m64" LDFLAGS="-m64" \
	    build_dep "win-64" \
	    "$ZEROMQ_CONFOPTS_COMMON --host=x86_64-w64-mingw32" \
	    "$ZEROMQ.tar.xz" "zeromq" "$LIBZMQ_PRODUCT"
	;;
    Darwin)
	CFLAGS="-g\\ -m64" LDFLAGS="-m64" build_dep "mac-64" \
	    "$ZEROMQ_CONFOPTS_COMMON" \
	    "$ZEROMQ.tar.xz" "zeromq" "$LIBZMQ_PRODUCT"
	;;
esac
