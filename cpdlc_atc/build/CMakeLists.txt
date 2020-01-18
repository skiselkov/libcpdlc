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

cmake_minimum_required(VERSION 2.8)
project(cpdlc_atc)

include("../../common.cmake")

SET(SRC
    ../main.c
    ../imgui/imgui.cpp
    ../imgui/imgui_draw.cpp
    ../imgui/imgui_widgets.cpp
    ../imgui/examples/imgui_impl_opengl3.cpp
    ../gl3w/src/gl3w.c
    ${LIBCPDLC_SOURCES}
    )
SET(HDR
    ../main.h
    ../imgui/imconfig.h
    ../imgui/imgui_internal.h
    ../imgui/imstb_textedit.h
    ../imgui/imgui.h
    ../imgui/imstb_rectpack.h
    ../imgui/imstb_truetype.h
    ../imgui/examples/imgui_impl_opengl3.h
    ../gl3w/include/GL/gl3w.h
    ${LIBCPDLC_HEADERS}
    )

SET(ALL_SRC ${SRC} ${HDR})
LIST(SORT ALL_SRC)
add_executable(cpdlc_atc ${ALL_SRC})

target_include_directories(cpdlc_atc PUBLIC
    ${LIBCPDLC_INCLUDES}
    "../../libacfutils/src"
    "../imgui"
    "../imgui/examples"
    "../glfw/include"
    "../gl3w/include"
    )

FIND_LIBRARY(GLFW_LIBRARY "glfw3" "../glfw/src")
if(WIN32)
	FIND_LIBRARY(OPENGL_LIBRARY "opengl32")
	FIND_LIBRARY(ACFUTILS_LIBRARY "acfutils"
	    "../../libacfutils/qmake/win64")
elseif(APPLE)
	FIND_LIBRARY(OPENGL_LIBRARY "OpenGL")
	FIND_LIBRARY(ACFUTILS_LIBRARY "acfutils"
	    "../../libacfutils/qmake/mac64")
else()
	FIND_LIBRARY(OPENGL_LIBRARY "GL")
	FIND_LIBRARY(ACFUTILS_LIBRARY "acfutils"
	    "../../libacfutils/qmake/lin64")
endif()

target_link_libraries(cpdlc_atc
    ${ACFUTILS_LIBRARY}
    ${GLFW_LIBRARY}
    ${OPENGL_LIBRARY}
    ${OPENGL_PLATFORM_LIBRARY}
    ${GNUTLS_LIBRARY}
    ${PTHREAD_LIBRARY}
    ${MATH_LIBRARY}
    )

if(UNIX)
	FIND_LIBRARY(X11_LIBRARY "X11")
	FIND_LIBRARY(DL_LIBRARY "dl")
	target_link_libraries(cpdlc_atc
	    ${X11_LIBRARY}
	    ${DL_LIBRARY}
	    )
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W -Wall -Wextra -Werror \
    -fvisibility=hidden \
    -Wno-unused-local-typedefs -Wno-missing-field-initializers")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
