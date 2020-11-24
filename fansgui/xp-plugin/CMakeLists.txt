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

cmake_minimum_required(VERSION 3.10)

project(fans_plugin)

set(FANS_PLUGIN_SOURCES "plugin.c")
add_library(fans_plugin SHARED ${FANS_PLUGIN_SOURCES})

if(WIN32)
	add_definitions(-DIBM=1 -DAPL=0 -DLIN=0)
	add_definitions(-D_WIN32_WINNT=0x0600 -DUNICODE)
	set(PLAT_NAME "win-64")
elseif(APPLE)
	add_definitions(-DIBM=0 -DAPL=1 -DLIN=0)
	set(PLAT_NAME "mac-64")
elseif(UNIX)
	add_definitions(-DIBM=0 -DAPL=0 -DLIN=1)
	add_definitions(-D_POSIX_C_SOURCE=200809L)
	set(PLAT_NAME "linux-64")
endif()

set(LIBACFUTILS "${CMAKE_SOURCE_DIR}/../../libacfutils")
set(ZEROMQ "${CMAKE_SOURCE_DIR}/../zmq/zeromq-${PLAT_NAME}/install")
list(APPEND PKG_CONFIG_DEPS_OPTS
    --static-openal
    --no-tesseract
    --no-cairo
    --no-shapelib
    --no-glewmx)
execute_process(COMMAND ${LIBACFUTILS}/pkg-config-deps ${PLAT_NAME} ${PKG_CONFIG_DEPS_OPTS} --cflags
    OUTPUT_VARIABLE ACFUTILS_CFLAGS)
string(REGEX REPLACE "\n$" "" ACFUTILS_CFLAGS "${ACFUTILS_CFLAGS}")

execute_process(COMMAND ${LIBACFUTILS}/pkg-config-deps ${PLAT_NAME} ${PKG_CONFIG_DEPS_OPTS} --libs
    OUTPUT_VARIABLE ACFUTILS_LIBS)
string(REGEX REPLACE "\n$" "" ACFUTILS_LIBS "${ACFUTILS_LIBS}")

execute_process(COMMAND git describe --abbrev=0 --tags
    OUTPUT_VARIABLE PLUGIN_VERSION)
string(REGEX REPLACE "\n$" "" PLUGIN_VERSION "${PLUGIN_VERSION}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W -Wall -Wextra -Werror --std=c99 \
    -Wno-missing-field-initializers ${ACFUTILS_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -Wextra -Werror --std=c++11 \
    -Wno-missing-field-initializers ${ACFUTILS_CFLAGS}")
add_definitions(-DDEBUG)
add_definitions(-DPLUGIN_VERSION="${PLUGIN_VERSION}")
add_definitions(-DXPLM200=1 -DXPLM210=1 -DXPLM300=1 -DXPLM301=1 -DXPLM302=1)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
set(ACFUTILS_INCLUDES "${LIBACFUTILS}/src")
list(APPEND XP_SDK_INCLUDES
    "${LIBACFUTILS}/SDK/CHeaders/XPLM"
    "${LIBACFUTILS}/SDK/CHeaders/Widgets"
    "${LIBACFUTILS}/SDK/CHeaders/Wrappers"
)

find_library(PTHREAD_LIBRARY "pthread")
find_library(MATH_LIBRARY "m")
set(ZEROMQ_LIBRARY "${ZEROMQ}/lib/libzmq.a")

if(WIN32)
	find_library(ACFUTILS_LIBRARY "acfutils" "${LIBACFUTILS}/qmake/win64")
	find_library(XPLM_LIBRARY "XPLM_64" "${LIBACFUTILS}/SDK/Libraries/Win")
	set(PLATFORM_LIBS "-lws2_32 -lgdi32 -ldbghelp -lpsapi")
elseif(APPLE)
	find_library(ACFUTILS_LIBRARY "acfutils" "${LIBACFUTILS}/qmake/mac64")
	find_library(XPLM_LIBRARY "XPLM" "${LIBACFUTILS}/SDK/Libraries/Mac")
elseif(UNIX)
	find_library(ACFUTILS_LIBRARY "acfutils" "${LIBACFUTILS}/qmake/lin64")
	set(XPLM_LIBRARY "")
endif()
find_library(LZMA_LIBRARY "lzma" "${LIBACFUTILS}/lzma/qmake/${PLAT_NAME}")
find_library(ZLIB_LIBRARY "z" "${LIBACFUTILS}/zlib/zlib-${PLAT_NAME}/lib")

target_include_directories(fans_plugin PUBLIC
    "${ZEROMQ}/include"
    ${ACFUTILS_INCLUDES}
    ${XP_SDK_INCLUDES}
)

target_link_libraries(fans_plugin
    ${ACFUTILS_LIBRARY}
    ${ACFUTILS_LIBS}
    ${XPLM_LIBRARY}
    ${ZEROMQ_LIBRARY}
    ${PTHREAD_LIBRARY}
    ${MATH_LIBRARY}
    ${PLATFORM_LIBS}
    "-static-libgcc"
    "-static-libstdc++"
)

set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_SUFFIX "")
set_target_properties(fans_plugin PROPERTIES OUTPUT_NAME "fans.plugin.xpl")
if(WIN32)
	set_target_properties(fans_plugin PROPERTIES LINK_FLAGS
	    "${CMAKE_SHARED_LINKER_FLAGS} -fvisibility=hidden")
	set_target_properties(fans_plugin PROPERTIES
	    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/fans.plugin/win_x64"
	    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/fans.plugin/win_x64"
	)
elseif(APPLE)
	set_target_properties(fans_plugin PROPERTIES LINK_FLAGS
	    "${CMAKE_SHARED_LINKER_FLAGS} -fvisibility=hidden")
	set_target_properties(fans_plugin PROPERTIES
	    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/fans.plugin/mac_x64"
	    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/fans.plugin/mac_x64"
	)
elseif(UNIX)
	set_target_properties(fans_plugin PROPERTIES LINK_FLAGS
	    "${CMAKE_SHARED_LINKER_FLAGS} -rdynamic -nodefaultlibs \
	    -undefined_warning -fPIC -fvisibility=hidden")
	set_target_properties(fans_plugin PROPERTIES
	    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/fans.plugin/lin_x64"
	    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/fans.plugin/lin_x64"
	)
endif()
