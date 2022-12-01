# This file was created by KDevelop.
# -------------------------------------------

# Override settings in this file.
include (x2goclientconfig.pri)

CONFIG += $$(X2GO_CLIENT_TARGET)
CONFIG += $$(X2GO_LINUX_STATIC)
CONFIG += link_pkgconfig
#CONFIG += console

VERSION = "$$cat($${PWD}/VERSION)"

FORMS += src/ui/cupsprintsettingsdialog.ui \
         src/ui/cupsprintwidget.ui \
         src/ui/printdialog.ui \
         src/ui/printercmddialog.ui \
         src/ui/printwidget.ui \
         src/ui/xsettingsui.ui \
         src/ui/brokerpassdialog.ui \
         src/ui/contest.ui \
         src/ui/appdialog.ui \
         src/ui/helpdialog.ui \
         src/ui/folderexplorer.ui

TRANSLATIONS += res/i18n/x2goclient_cs.ts \
                res/i18n/x2goclient_de.ts \
                res/i18n/x2goclient_da.ts \
                res/i18n/x2goclient_es.ts \
                res/i18n/x2goclient_et.ts \
                res/i18n/x2goclient_fi.ts \
                res/i18n/x2goclient_fr.ts \
                res/i18n/x2goclient_nb_no.ts \
                res/i18n/x2goclient_nl.ts \
                res/i18n/x2goclient_pt.ts \
                res/i18n/x2goclient_ru.ts \
                res/i18n/x2goclient_sv.ts \
                res/i18n/x2goclient_tr.ts \
                res/i18n/x2goclient_zh_tw.ts

HEADERS += src/configdialog.h \
           src/editconnectiondialog.h \
           src/exportdialog.h \
           src/imgframe.h \
           src/LDAPSession.h \
           src/onmainwindow.h \
           src/sessionbutton.h \
           src/sessionmanagedialog.h \
           src/sshmasterconnection.h \
           src/sshprocess.h \
           src/SVGFrame.h \
           src/InteractionDialog.h \
           src/userbutton.h \
           src/x2goclientconfig.h \
           src/x2gologdebug.h \
           src/printprocess.h \
           src/cupsprint.h \
           src/cupsprintwidget.h \
           src/cupsprintersettingsdialog.h \
           src/printwidget.h \
           src/printercmddialog.h \
           src/printdialog.h \
           src/wapi.h \
           src/sessionwidget.h \
           src/configwidget.h \
           src/connectionwidget.h \
           src/settingswidget.h \
           src/mediawidget.h \
           src/sharewidget.h \
           src/clicklineedit.h \
           src/httpbrokerclient.h \
           src/ongetpass.h \
           src/onmainwindow_privat.h \
           src/x2gosettings.h \
           src/brokerpassdlg.h \
           src/contest.h \
           src/xsettingswidget.h \
           src/appdialog.h \
           src/x2goutils.h \
           src/helpdialog.h \
           src/sessionexplorer.h \
           src/folderbutton.h \
           src/folderexplorer.h \
           src/non_modal_messagebox.h \
           src/help.h \
           src/unixhelper.h \
           src/compat.h \
           src/pulsemanager.h

SOURCES += src/sharewidget.cpp \
           src/settingswidget.cpp \
           src/mediawidget.cpp \
           src/configwidget.cpp \
           src/sessionwidget.cpp \
           src/connectionwidget.cpp \
           src/configdialog.cpp \
           src/editconnectiondialog.cpp \
           src/exportdialog.cpp \
           src/imgframe.cpp \
           src/LDAPSession.cpp \
           src/onmainwindow.cpp \
           src/sessionbutton.cpp \
           src/sessionmanagedialog.cpp \
           src/sshmasterconnection.cpp \
           src/sshprocess.cpp \
           src/SVGFrame.cpp \
           src/InteractionDialog.cpp \
           src/userbutton.cpp \
           src/x2gologdebug.cpp \
           src/printprocess.cpp \
           src/cupsprint.cpp \
           src/cupsprintwidget.cpp \
           src/cupsprintersettingsdialog.cpp \
           src/printwidget.cpp \
           src/printercmddialog.cpp \
           src/printdialog.cpp \
           src/wapi.cpp \
           src/clicklineedit.cpp \
           src/httpbrokerclient.cpp \
           src/ongetpass.cpp \
           src/x2gosettings.cpp \
           src/brokerpassdlg.cpp \
           src/contest.cpp \
           src/xsettingswidget.cpp \
           src/appdialog.cpp \
           src/x2goutils.cpp \
           src/helpdialog.cpp \
           src/sessionexplorer.cpp \
           src/folderbutton.cpp \
           src/folderexplorer.cpp \
           src/non_modal_messagebox.cpp \
           src/help.cpp \
           src/unixhelper.cpp \
           src/compat.cpp \
           src/pulsemanager.cpp

unix {
  isEmpty(PKG_CONFIG):PKG_CONFIG = pkg-config

  !x2go_linux_static {
    PKGCONFIG += libssh

    # Failure to find libssh_threads is non-fatal, since newer libssh versions
    # don't ship a separate library any longer.
    # We can't just check for version 0.8.0, though, since some distros
    # (looking at you, Ubuntu Bionic/18.04) ship a pre-release version of 0.8.0
    # which is actually still based upon 0.7.x and still has libssh_threads...
    system("$$PKG_CONFIG --exists 'libssh_threads'"):PKGCONFIG += libssh_threads
  }
  else {
    # No pkgconfig stuff, because... for some reason we wanted to call the
    # library libssh_static. This doesn't sound like a standard name a Linux
    # distro would ship, but at the same time I doubt anyone is building this
    # package statically anyway. And even if they do, they shouldn't.
    # It's also unclear how libssh_threads is called in this scheme.
    LIBS += -lssh_static
  }
}
else:win32 {
  mxe {
    QMAKE_CXXFLAGS += -DLIBSSH_STATIC=1
    LIBS += -lssh -lgcrypt -lgpg-error
  }
  else {
    # pkgconfig is... tricky on Windows. Additionally, libssh 0.7.x stopped
    # distributing pkgconfig files on non-UNIX platforms, including Windows.
    # We'll hardcode stuff here. Make sure that it's consistent with the
    # libraries we use on Windows.
    LIBS += -lssh
  }
}
else {
  # For backwards-compatibility.
  LIBS += -lssh -lssh_threads
}

win32:!mxe {
  LIBS += -lAdvAPI32 -lshell32 -lUser32
}

RC_FILE = res/x2goclient.rc
SOURCES += src/x2goclient.cpp
TARGET = x2goclient
DEFINES += CFGCLIENT

!isEmpty(TRANSLATIONS) {
  isEmpty(QMAKE_LRELEASE) {
    # Some Qt versions (looking at you, EPEL 6) are broken
    # and do not provide the qtPrepareTool test function.
    # This section is only for working around bugs...
    !defined(qtPrepareTool) {
      win32:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\\lrelease.exe
      else:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
    }
    else {
      qtPrepareTool(QMAKE_LRELEASE, lrelease)
    }
  }

  isEmpty(TS_DIR):TS_DIR = .

  TSQM.name = lrelease ${QMAKE_FILE_IN}
  TSQM.input = TRANSLATIONS
  TSQM.output = $$TS_DIR/${QMAKE_FILE_BASE}.qm
  TSQM.commands = $$QMAKE_LRELEASE ${QMAKE_FILE_IN}
  TSQM.CONFIG = no_link
  QMAKE_EXTRA_COMPILERS += TSQM
  PRE_TARGETDEPS += compiler_TSQM_make_all
}
else:message("No translation files in project")

TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += src
RESOURCES += res/resources.qrc

exists(res/txt/git-info) {
  message("Configuring with --git-info")
  RESOURCES += res/git.qrc
}

exists(res/txt/changelog) {
  message("Configuring with --changelog")
  RESOURCES += res/changelog.qrc
}

linux|linux-* {
  message("building $$TARGET via $$QMAKESPEC with ldap and cups")
  LIBS += -lldap -lcups -lX11 -lXpm
  DEFINES += __linux__
}
x2go_linux_static {
  message("linking all libs statically")
  DEFINES += __linux__
  LIBS += -lssl -lXpm
  QMAKE_LFLAGS = -Bstatic $$QMAKE_LFLAGS
}

ICON = $${PWD}/res/img/icons/x2go-mac.icns

macx {
  message("building $$TARGET with ldap and cups")
  LIBS += -framework LDAP -lcups -lcrypto -lssl -lz

  !isEmpty(OSX_STDLIB) {
    QMAKE_CXXFLAGS += -stdlib=$${OSX_STDLIB}
    QMAKE_LFLAGS += -stdlib=$${OSX_STDLIB}
  }

  !isEmpty(MACPORTS_INCLUDE_PATH) {
    INCLUDEPATH += $${MACPORTS_INCLUDE_PATH}
  }

  !isEmpty(MACPORTS_LIBRARY_PATH) {
    LIBS = -L$${MACPORTS_LIBRARY_PATH} $${LIBS}
  }

  !isEmpty(MACPORTS_PREFIX) {
    DEFINES += MACPORTS_PREFIX=\"\\\"$${MACPORTS_PREFIX}\\\"\"
  }

  # Strictly speaking, this is a bug in qmake and we should neither need $${PWD}
  # nor QMAKE_INFO_PLIST_OUT nor PRE_TARGETDEPS nor QMAKE_POST_LINK.
  # Not defining the latter two will however lead to it being empty and no
  # Info.plist file being created in the first place.
  # The last command takes care of actually putting the icon in place - yet
  # another bug in qmake. Bummer.
  # Qt 4.8 is currently missing patches for substituting FULL_VERSION. Work
  # around by using the postbuild.sh script.
  QMAKE_INFO_PLIST = $${PWD}/res/osxbundle/Info.plist
  QMAKE_INFO_PLIST_OUT = $${TARGET}.app/Contents/Info.plist
  PRE_TARGETDEPS += $${TARGET}.app/Contents/Info.plist
  QMAKE_POST_LINK += $${PWD}/res/osxbundle/postbuild.sh \"$${TARGET}\" \"$${VERSION}\" \"$${QMAKE_INFO_PLIST_OUT}\" \"$${QMAKE_COPY}\" \"$${ICON}\" \"$${OUT_PWD}/$${TARGET}.app/Contents/Resources/\"
}
win32|win32-* {
  message("building $$TARGET for windows without ldap and cups")
  LIBS += -lwinspool -lws2_32
  CONFIG += static release
}
QT += svg network
linux|linux-* {
  greaterThan(QT_MAJOR_VERSION, 4): QT += x11extras
}
win32|win32-* {
  greaterThan(QT_MAJOR_VERSION, 4): QT += winextras
}

mxe:QMAKE_CXXFLAGS += -std=c++11 -Wno-deprecated-declarations
QMAKE_CXXFLAGS_DEBUG -= -g
QMAKE_CXXFLAGS_DEBUG += -O2 -g3 -ggdb3 -gdwarf-4
