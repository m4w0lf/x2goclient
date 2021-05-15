#!/usr/bin/make -f

TOP_DIR=$(CURDIR)
CLIENT_DIR=$(TOP_DIR)/client_build
CLIENT_BINARY=$(CLIENT_DIR)/x2goclient

SHELL=/bin/bash

# Default to Qt 4.
# We'll change this to Qt 5 as soon as Qt-4-based platforms go EOL.
QT_VERSION ?= 4

INSTALL_DIR=install -d -o root -g root -m 755
INSTALL_FILE=install -o root -g root -m 644
INSTALL_SYMLINK=ln -s -f
INSTALL_PROGRAM=install -o root -g root -m 755

RM_FILE=rm -f
RM_DIR=rmdir -p --ignore-fail-on-non-empty

DESTDIR=
PREFIX=/usr/local
ETCDIR=/etc/x2go
BINDIR=$(PREFIX)/bin
SHAREDIR=$(PREFIX)/share
MANDIR=$(SHAREDIR)/man
ifeq ($(QT_VERSION),4)
  QMAKE_BINARY := qmake-qt4
  LRELEASE_BINARY := lrelease-qt4
else
  ifeq ($(QT_VERSION),5)
    QMAKE_BINARY := qmake
    LRELEASE_BINARY := lrelease
  else
    $(error Unsupported Qt version "$(QT_VERSION)" passed.)
  endif
endif
QMAKE_OPTS=

LDFLAGS=
LIBS=


#####################################################################
# Make sure that variables passed via the command line are not
# automatically exported.
#
# By default, such variables are exported and passed on to child
# (make) processes, which means that we will run into trouble if we
# pass CXXFLAGS="some value" as an argument to the top-level
# make call. Whatever qmake generates will be overridden by this
# definition, leading to build failures (since the code expects
# macro values to be set in some cases - which won't be the case
# for "generic" CXXFLAGS values.)
#
# Doing that turns out to be somewhat difficult, though.
#
# While preventing make from passing down *options* is possible via
# giving the new make call an empty MAKEFLAGS value and even though
# variables defined on the command line are part of MAKEFLAGS, this
# doesn't affect implicit exporting of variables (for most
# implementations.)
#
# Even worse, the correct way to this stuff differs from make
# implementation to make implementation.

# GNU make way.
MAKEOVERRIDES = SHELL QT_VERSION INSTALL_DIR INSTALL_FILE INSTALL_SYMLINK INSTALL_PROGRAM RM_FILE RM_DIR DESTDIR PREFIX ETCDIR BINDIR SHAREDIR MANDIR QMAKE_BINARY LRELEASE_BINARY QMAKE_OPTS LDFLAGS LIBS

# FreeBSD and NetBSD way.
.MAKEOVERRIDES = SHELL QT_VERSION INSTALL_DIR INSTALL_FILE INSTALL_SYMLINK INSTALL_PROGRAM RM_FILE RM_DIR DESTDIR PREFIX ETCDIR BINDIR SHAREDIR MANDIR QMAKE_BINARY LRELEASE_BINARY QMAKE_OPTS LDFLAGS LIBS

# OpenBSD way.
.MAKEFLAGS = SHELL QT_VERSION INSTALL_DIR INSTALL_FILE INSTALL_SYMLINK INSTALL_PROGRAM RM_FILE RM_DIR DESTDIR PREFIX ETCDIR BINDIR SHAREDIR MANDIR QMAKE_BINARY LRELEASE_BINARY QMAKE_OPTS LDFLAGS LIBS


all: build

build: build_man
	$(MAKE) build_client

build_client:
	$(LRELEASE_BINARY) x2goclient.pro
	mkdir -p $(CLIENT_DIR) && cd $(CLIENT_DIR) && $(QMAKE_BINARY) QMAKE_CFLAGS="${CPPFLAGS} ${CFLAGS}" QMAKE_CXXFLAGS="${CPPFLAGS} ${CXXFLAGS}" QMAKE_LFLAGS="${LDFLAGS}" QMAKE_LIBS="${LIBS}" $(QMAKE_OPTS) ../x2goclient.pro
	cd $(CLIENT_DIR) && $(MAKE)

build_man:
	${MAKE} -f Makefile.man2html build

clean: clean_client clean_man
	find . -maxdepth 3 -name '*.o' -exec rm -vf {} + -type f
	find . -maxdepth 3 -name 'moc_*.cpp' -exec rm -vf {} + -type f
	find . -maxdepth 3 -name 'ui_*.h' -exec rm -vf {} + -type f
	find . -maxdepth 3 -name 'qrc_*.cpp' -exec rm -vf {} + -type f
	find . -maxdepth 3 -name 'x2goclient_*.qm' -exec rm -vf {} + -type f
	rm -f x2goclient
	rm -f x2goclient.tag
	rm -f res/txt/changelog
	rm -f res/txt/git-info

clean_client:
	rm -fr $(CLIENT_DIR)

clean_man:
	make -f Makefile.man2html clean

install: install_client install_man

install_client:
	$(INSTALL_DIR) $(DESTDIR)$(BINDIR)/
	$(INSTALL_DIR) $(DESTDIR)$(SHAREDIR)/applications
	$(INSTALL_DIR) $(DESTDIR)$(SHAREDIR)/mime/packages
	$(INSTALL_DIR) $(DESTDIR)$(SHAREDIR)/x2goclient/icons
	$(INSTALL_DIR) $(DESTDIR)$(SHAREDIR)/icons/hicolor/128x128/apps
	$(INSTALL_DIR) $(DESTDIR)$(SHAREDIR)/icons/hicolor/16x16/apps
	$(INSTALL_DIR) $(DESTDIR)$(SHAREDIR)/icons/hicolor/64x64/apps
	$(INSTALL_DIR) $(DESTDIR)$(SHAREDIR)/icons/hicolor/32x32/apps
	$(INSTALL_PROGRAM) $(CLIENT_DIR)/x2goclient $(DESTDIR)$(BINDIR)/x2goclient
	$(INSTALL_FILE) desktop/x2goclient.desktop    $(DESTDIR)$(SHAREDIR)/applications/x2goclient.desktop
	$(INSTALL_FILE) mime/x-x2go.xml                       $(DESTDIR)$(SHAREDIR)/mime/packages/x-x2go.xml
	$(INSTALL_FILE) res/img/icons/x2goclient.xpm          $(DESTDIR)$(SHAREDIR)/x2goclient/icons/x2goclient.xpm
	$(INSTALL_FILE) res/img/icons/128x128/x2goclient.png  $(DESTDIR)$(SHAREDIR)/x2goclient/icons/x2goclient.png
	$(INSTALL_FILE) res/img/icons/128x128/x2gosession.png $(DESTDIR)$(SHAREDIR)/x2goclient/icons/x2gosession.png
	$(INSTALL_FILE) res/img/icons/128x128/x2goclient.png  $(DESTDIR)$(SHAREDIR)/icons/hicolor/128x128/apps/x2goclient.png
	$(INSTALL_FILE) res/img/icons/16x16/x2goclient.png    $(DESTDIR)$(SHAREDIR)/icons/hicolor/16x16/apps/x2goclient.png
	$(INSTALL_FILE) res/img/icons/64x64/x2goclient.png    $(DESTDIR)$(SHAREDIR)/icons/hicolor/64x64/apps/x2goclient.png
	$(INSTALL_FILE) res/img/icons/32x32/x2goclient.png    $(DESTDIR)$(SHAREDIR)/icons/hicolor/32x32/apps/x2goclient.png

install_man:
	$(INSTALL_DIR) $(DESTDIR)$(MANDIR)/
	$(INSTALL_DIR) $(DESTDIR)$(MANDIR)/man1
	$(INSTALL_FILE) man/man1/x2goclient.1    $(DESTDIR)$(MANDIR)/man1/x2goclient.1
	gzip -f $(DESTDIR)$(MANDIR)/man1/x2goclient.1

uninstall: uninstall_client uninstall_man

uninstall_client:
	$(RM_FILE) $(BINDIR)/x2goclient
	$(RM_FILE) $(SHAREDIR)/applications/x2goclient.desktop
	$(RM_FILE) $(SHAREDIR)/x2goclient/icons/x2goclient.png
	$(RM_FILE) $(SHAREDIR)/x2goclient/icons/x2goclient.xpm
	$(RM_FILE) $(SHAREDIR)/x2goclient/icons/x2gosession.png
	$(RM_FILE) $(SHAREDIR)/icons/hicolor/128x128/apps/x2goclient.png
	$(RM_FILE) $(SHAREDIR)/icons/hicolor/16x16/apps/x2goclient.png
	$(RM_FILE) $(SHAREDIR)/icons/hicolor/64x64/apps/x2goclient.png
	$(RM_FILE) $(SHAREDIR)/icons/hicolor/32x32/apps/x2goclient.png
	$(RM_DIR) $(SHAREDIR)/applications
	$(RM_DIR) $(SHAREDIR)/x2goclient/icons
	$(RM_DIR) $(SHAREDIR)/icons/hicolor/128x128/apps
	$(RM_DIR) $(SHAREDIR)/icons/hicolor/16x16/apps
	$(RM_DIR) $(SHAREDIR)/icons/hicolor/64x64/apps
	$(RM_DIR) $(SHAREDIR)/icons/hicolor/32x32/apps

uninstall_man:
	$(RM_FILE) $(MANDIR)/man1/x2goclient.1.gz
	$(RM_DIR) $(MANDIR)/man1
