#!/bin/bash
#
# prepare or build x2goclient and x2gohelper for Windows using MXE
# cross build enviroment for Linux (https://mxe.cc/).
#
# Usage: $0 [prepare] [<config>]
#
# prepare means only prepare the build dirs (create them, clean
# them). If ommitted, it will build the binaries instead.  <config>
# can be "debug" or "release". Default is "release"
#
#
# Adjust these values to match your mxe installation:
MXE_BASE=/usr/lib/mxe/usr
#MXE_TARGET=i686-w64-mingw32.shared
MXE_TARGET=i686-w64-mingw32.static
#MXE_TARGET=x86-64-w64-mingw32.shared
#MXE_TARGET=x86-64-w64-mingw32.static
MXE_PATH=${MXE_BASE}/${MXE_TARGET}

test -d "${MXE_PATH}" || { echo >&2 "Cannot find mxe installation at '$MXE_PATH'"; exit 1; } 

BUILD_DIR="client_build_mxe"
BUILD_CONFIG=release

MODE=build
[ "$1" == "prepare" ] && MODE=prepare && shift

if [ "$1" == "debug" ]; then
    BUILD_CONFIG=debug
    shift
fi

export MXE_BASE MXE_TARGET MXE_PATH BUILD_DIR BUILD_CONFIG

export X2GO_CLIENT_TARGET=

[ "$MODE" == "build" ] && [ ! -d ${BUILD_DIR} ] && { echo >&2 "Please run '$0 prepare' first"; exit 1; }

if [ "$MODE" == "prepare" ]; then
    test -e "${BUILD_DIR}" && rm -rf "${BUILD_DIR}"
    make clean

    mkdir -p "${BUILD_DIR}/${BUILD_CONFIG}"
    pushd "${BUILD_DIR}"

    ${MXE_PATH}/qt5/bin/lrelease ../x2goclient.pro

    # no special Makefile required as qmake will create that
    ${MXE_PATH}/qt5/bin/qmake ../x2goclient.pro -config "${BUILD_CONFIG}"

    popd
else
    pushd "${BUILD_DIR}"
    make
    popd
fi

pushd x2gohelper
# here we do not have qmake but an own Makefile for mxe
if [ "$MODE" == "prepare" ]; then
    make -f Makefile.mxe clean
else
    make -f Makefile.mxe
fi
popd
