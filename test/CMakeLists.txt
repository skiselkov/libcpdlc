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

cmake_minimum_required(VERSION 3.18)

project(client_test)

include("../common.cmake")

set(CLIENT_TEST_SOURCES
    ../client_test.c
    ${FANS_SOURCES}
    ${FANS_HEADERS}
    ${LIBCPDLC_SOURCES}
    ${LIBCPDLC_HEADERS}
    )

add_executable(client_test ${CLIENT_TEST_SOURCES})
add_definitions(-DCPDLC_WITH_OPENSSL)
find_package(OpenSSL)
target_include_directories(client_test PUBLIC
    ${LIBCPDLC_INCLUDES}
    ${FANS_INCLUDES}
    ${OPENSSL_INCLUDE_DIR}
    )
target_link_libraries(client_test
    ${OPENSSL_LIBRARIES}
    ${NCURSESW_LIBRARY}
    ${PTHREAD_LIBRARY}
    ${MATH_LIBRARY}
    )
set_property(TARGET client_test PROPERTY C_STANDARD 99)
