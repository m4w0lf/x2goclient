#!/bin/bash
#
# prepare or build x2goclient and x2gohelper for Windows using MXE
# cross build environment for Linux (https://mxe.cc/).
#
# Usage: $0 [prepare] [<config>]
#
# prepare means only prepare the build dirs (create them, clean
# them). If omitted, it will build the binaries instead.  <config>
# can be "debug" or "release". Default is "release".
#
#
# Adjust these values to match your MXE installation:
MXE_BASE='/usr/lib/mxe/usr'
#MXE_TARGET='i686-w64-mingw32.shared
MXE_TARGET='i686-w64-mingw32.static'
#MXE_TARGET='x86-64-w64-mingw32.shared'
#MXE_TARGET='x86-64-w64-mingw32.static'
mxe_path="${MXE_BASE}/${MXE_TARGET}"

if ! test -d "${mxe_path}"; then
  printf 'Cannot find MXE installation at "%s".' "${mxe_path}" >&2
  exit '1'
fi

BUILD_DIR='client_build_mxe'
BUILD_CONFIG='release'

mode='build'
if [ 'prepare' = "${1}" ]; then
  mode='prepare'
  shift
fi

if [ 'debug' = "${1}" ]; then
  BUILD_CONFIG='debug'
  shift
fi

export 'MXE_BASE' 'MXE_TARGETS' 'BUILD_DIR' 'BUILD_CONFIG'

X2GO_CLIENT_TARGET=''
export 'X2GO_CLIENT_TARGET'

if [ 'build' = "${mode}" ] && [ ! -d "${BUILD_DIR}" ]; then
  printf 'Please run "%s prepare" first.' "${0}" >&2
  exit '2'
fi

if [ 'prepare' = "${mode}" ]; then
  if test -e "${BUILD_DIR}"; then
    rm -rf "${BUILD_DIR}"
  fi
  make clean

  mkdir -p "${BUILD_DIR}/${BUILD_CONFIG}"
  pushd "${BUILD_DIR}"

  "${mxe_path}/qt5/bin/lrelease" '../x2goclient.pro'

  # no special Makefile required as qmake will create that
  "${mxe_path}/qt5/bin/qmake" '../x2goclient.pro' -config "${BUILD_CONFIG}"

  popd
else
  pushd "${BUILD_DIR}"
  make
  popd
fi

pushd 'x2gohelper'
# Here we do not have qmake but a unique Makefile for MXE.
if [ 'prepare' = "${mode}" ]; then
  make -f 'Makefile.mxe' 'clean'
else
  make -f 'Makefile.mxe'
fi
popd
