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

set(SRCPREFIX "../../src")
set(FANSPREFIX "../../fans")

set(LIBCPDLC_SOURCES
    ${SRCPREFIX}/cpdlc_assert.c
    ${SRCPREFIX}/cpdlc_client.c
    ${SRCPREFIX}/cpdlc_infos.c
    ${SRCPREFIX}/cpdlc_msg.c
    ${SRCPREFIX}/cpdlc_msglist.c
    ${SRCPREFIX}/minilist.c
    )

set(LIBCPDLC_HEADERS
    ${SRCPREFIX}/cpdlc_alloc.h
    ${SRCPREFIX}/cpdlc_assert.h
    ${SRCPREFIX}/cpdlc_client.h
    ${SRCPREFIX}/cpdlc_core.h
    ${SRCPREFIX}/cpdlc_msg.h
    ${SRCPREFIX}/cpdlc_msglist.h
    ${SRCPREFIX}/cpdlc_string.h
    ${SRCPREFIX}/cpdlc_thread.h
    ${SRCPREFIX}/minilist.h
    )

set(FANS_SOURCES
    ${FANSPREFIX}/fans.c
    ${FANSPREFIX}/fans_emer.c
    ${FANSPREFIX}/fans_freetext.c
    ${FANSPREFIX}/fans_main_menu.c
    ${FANSPREFIX}/fans_msg.c
    ${FANSPREFIX}/fans_logon_status.c
    ${FANSPREFIX}/fans_parsing.c
    ${FANSPREFIX}/fans_pos_pick.c
    ${FANSPREFIX}/fans_pos_rep.c
    ${FANSPREFIX}/fans_rej.c
    ${FANSPREFIX}/fans_req.c
    ${FANSPREFIX}/fans_req_alt.c
    ${FANSPREFIX}/fans_req_clx.c
    ${FANSPREFIX}/fans_req_off.c
    ${FANSPREFIX}/fans_req_rte.c
    ${FANSPREFIX}/fans_req_spd.c
    ${FANSPREFIX}/fans_req_vmc.c
    ${FANSPREFIX}/fans_req_wcw.c
    ${FANSPREFIX}/fans_req_voice.c
    ${FANSPREFIX}/fans_scratchpad.c
    ${FANSPREFIX}/fans_vrfy.c
    )

set(FANS_HEADERS
    ${FANSPREFIX}/fans_emer.h
    ${FANSPREFIX}/fans_freetext.h
    ${FANSPREFIX}/fans.h
    ${FANSPREFIX}/fans_impl.h
    ${FANSPREFIX}/fans_logon_status.h
    ${FANSPREFIX}/fans_main_menu.h
    ${FANSPREFIX}/fans_msg.h
    ${FANSPREFIX}/fans_parsing.h
    ${FANSPREFIX}/fans_pos_pick.h
    ${FANSPREFIX}/fans_pos_rep.h
    ${FANSPREFIX}/fans_rej.h
    ${FANSPREFIX}/fans_req_alt.h
    ${FANSPREFIX}/fans_req_clx.h
    ${FANSPREFIX}/fans_req.h
    ${FANSPREFIX}/fans_req_off.h
    ${FANSPREFIX}/fans_req_rte.h
    ${FANSPREFIX}/fans_req_spd.h
    ${FANSPREFIX}/fans_req_vmc.h
    ${FANSPREFIX}/fans_req_voice.h
    ${FANSPREFIX}/fans_req_wcw.h
    ${FANSPREFIX}/fans_scratchpad.h
    ${FANSPREFIX}/fans_vrfy.h
    )

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DAPL=0 -DIBM=0 -DLIN=1")

set(LIBCPDLC_INCLUDES "../../src")
set(FANS_INCLUDES "../../fans")

FIND_LIBRARY(GNUTLS_LIBRARY "gnutls")
FIND_LIBRARY(PTHREAD_LIBRARY "pthread")
FIND_LIBRARY(MATH_LIBRARY "m")
FIND_LIBRARY(NCURSESW_LIBRARY "ncursesw")

#LWS_CFLAGS=$(shell pkg-config libwebsockets --cflags)
#LWS_LIBS=$(shell pkg-config libwebsockets --libs)
