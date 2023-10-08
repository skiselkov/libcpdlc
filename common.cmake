#
# Copyright 2023 Saso Kiselkov
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

set(SRCPREFIX "../src")
set(FANSPREFIX "../fans")

file(GLOB LIBCPDLC_SOURCES ${SRCPREFIX}/*.c ${SRCPREFIX}/asn1/*.c)
file(GLOB LIBCPDLC_HEADERS ${SRCPREFIX}/*.h ${SRCPREFIX}/asn1/*.h)

file(GLOB FANS_SOURCES ${FANSPREFIX}/*.c)
file(GLOB FANS_HEADERS ${FANSPREFIX}/*.h)

if(WIN32)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DAPL=0 -DIBM=1 -DLIN=0")
elseif(APPLE)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DAPL=1 -DIBM=0 -DLIN=0")
else()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DAPL=0 -DIBM=0 -DLIN=1")
endif()

set(LIBCPDLC_INCLUDES "../src" "../src/asn1")
set(FANS_INCLUDES "../fans")

find_library(GNUTLS_LIBRARY "gnutls")
find_library(PTHREAD_LIBRARY "pthread")
find_library(MATH_LIBRARY "m")
find_library(NCURSESW_LIBRARY "ncursesw")
