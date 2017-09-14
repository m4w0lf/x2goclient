/********************************************************************************
 *  Copyright (C) 2015-2017 by Mihai Moldovan <ionic@ionic.de> +49 721 14595728 *
 *                                                                              *
 *  This program is free software; you can redistribute it and/or modify        *
 *  it under the terms of the GNU General Public License as published by        *
 *  the Free Software Foundation; either version 2 of the License, or           *
 *  (at your option) any later version.                                         *
 *                                                                              *
 *  This program is distributed in the hope that it will be useful,             *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
 *  GNU General Public License for more details.                                *
 *                                                                              *
 *  You should have received a copy of the GNU General Public License           *
 *  along with this program; if not, write to the                               *
 *  Free Software Foundation, Inc.,                                             *
 *  59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.                   *
 ********************************************************************************/

#include <QCoreApplication>
#include <QtDebug>
#include <QTextStream>
#include <QString>
#include <QFile>
#include <QObject>
#include <cstddef>
#include <algorithm>

/* For terminal size. */
#ifdef Q_OS_WIN
/* winsock2.h is not really needed, but including it silences a compile warning... */
#include <winsock2.h>
#include <windows.h>
#elif defined (Q_OS_UNIX)
#include <stdio.h>
#include <sys/ioctl.h>
#endif

#include "help.h"
#include "version.h"
#include "x2gologdebug.h"
#include "onmainwindow.h"
#include "x2goutils.h"

help::prelude_t help::cleanup_prelude (help::prelude_t prelude) {
  for (help::prelude_t::iterator it = prelude.begin (); it != prelude.end (); ++it) {
    *it = (*it).trimmed ();
  }

  return (prelude);
}

help::params_t help::cleanup_params (help::params_t params) {
  for (help::params_t::iterator it = params.begin (); it != params.end (); ++it) {
    (*it).first = (*it).first.trimmed ();
    (*it).second = (*it).second.trimmed ();
  }

  return (params);
}

help::prelude_t help::build_prelude () {
  help::prelude_t ret;

  QStringList args = QCoreApplication::arguments ();

  QString ver ("X2Go Client " + QString (VERSION));

  if (QFile::exists (":/txt/git-info")) {
    QFile file (":/txt/git-info");

    if (file.open (QIODevice::ReadOnly | QIODevice::Text)) {
      QTextStream stream (&file);

      QString git_info (stream.readAll ().trimmed ());
      git_info = git_changelog_extract_commit_sha (git_info);

      if (!(git_info.isEmpty ())) {
        ver.append (" (Git information: " + git_info + ")");
      }
    }
  }

  ret.append (ver);
  ret.append ("Usage: " + QString (args.at (0)) + " [OPTION]...");
  ret.append ("Options:");
  ret.append ("");

  return (ret);
}

help::params_t help::build_params () {
  params_t ret;

# define ADD_OPT(param, desc) do { ret.append (params_elem_t (param, qApp->translate ("Help", desc))); } while (0)
# define NEWLINE "\n"

  ADD_OPT ("--help", QT_TRANSLATE_NOOP ("Help", "Shows this message."));
  ADD_OPT ("--version", QT_TRANSLATE_NOOP ("Help", "Prints version information."));

  if (QFile::exists (":/txt/changelog")) {
    ADD_OPT ("--changelog", QT_TRANSLATE_NOOP ("Help", "Shows the changelog."));
  }

  if (QFile::exists (":/txt/git-info")) {
    ADD_OPT ("--git-info", QT_TRANSLATE_NOOP ("Help", "Shows git information as used at compile time. [Deprecated: please use --version.]"));
  }

  ADD_OPT ("--help-pack", QT_TRANSLATE_NOOP ("Help", "Shows available pack methods."));
  ADD_OPT ("--debug", QT_TRANSLATE_NOOP ("Help", "Enables extensive debug output to the console." NEWLINE
                                         "On Windows, also enables PulseAudio logging to a file under \".x2go/pulse\" & cygwin sshd logging to a file under \".x2go/sshLogs\" directory, both under the USERPROFILE directory." NEWLINE
                                         "The logs are not deleted when X2Go Client terminates."));
  ADD_OPT ("--no-autoresume", QT_TRANSLATE_NOOP ("Help", "Do not resume sessions automatically."));
  ADD_OPT ("--no-menu", QT_TRANSLATE_NOOP ("Help", "Hides menu bar."));
  ADD_OPT ("--no-session-edit", QT_TRANSLATE_NOOP ("Help", "Disables session editing."));
  ADD_OPT ("--maximize", QT_TRANSLATE_NOOP ("Help", "Starts maximized."));
  ADD_OPT ("--hide", QT_TRANSLATE_NOOP ("Help", "Starts hidden (minimized to system tray where available.)"));
  ADD_OPT ("--portable", QT_TRANSLATE_NOOP ("Help", "Starts in \"portable\" mode."));
  ADD_OPT ("--pgp-card", QT_TRANSLATE_NOOP ("Help", "Forces OpenPGP smart card authentication."));
  ADD_OPT ("--xinerama", QT_TRANSLATE_NOOP ("Help", "Enables Xinerama by default."));
  ADD_OPT ("--ldap-printing", QT_TRANSLATE_NOOP ("Help", "Allows client side printing in LDAP mode."));
  ADD_OPT ("--thinclient", QT_TRANSLATE_NOOP ("Help", "Enables thinclient mode. Starts without a window manager."));
  ADD_OPT ("--haltbt", QT_TRANSLATE_NOOP ("Help", "Enables shutdown button."));
  ADD_OPT ("--add-to-known-hosts", QT_TRANSLATE_NOOP ("Help", "Adds RSA key fingerprint to \".ssh/known_hosts\" if authenticity of the server can't be determined."));
  ADD_OPT ("--ldap=<host:port:dn>", QT_TRANSLATE_NOOP ("Help", "Starts with LDAP support. Example: --ldap=ldapserver:389:o=organization,c=de"));
  ADD_OPT ("--ldap1=<host:port>", QT_TRANSLATE_NOOP ("Help", "Defines the first LDAP failover server."));
  ADD_OPT ("--ldap2=<host:port>", QT_TRANSLATE_NOOP ("Help", "Defines the second LDAP failover server."));
  ADD_OPT ("--ssh-port=<port>", QT_TRANSLATE_NOOP ("Help", "Defines the remote SSH server port. Default: 22."));
  ADD_OPT ("--client-ssh-port=<port>", QT_TRANSLATE_NOOP ("Help", "Defines the local machine's SSH server port. Needed for Client-Side Printing and File Sharing support. Default: 22."));
  ADD_OPT ("--command=<cmd>", QT_TRANSLATE_NOOP ("Help", "Sets the default command. Default: 'KDE' (Desktop Session)"));
  ADD_OPT ("--session=<session>", QT_TRANSLATE_NOOP ("Help", "Starts the session named \"session\"."));
  ADD_OPT ("--user=<username>", QT_TRANSLATE_NOOP ("Help", "Sets the user name for connecting to the remote SSH server to \"username\"."));
  ADD_OPT ("--geometry=<<W>x<H>|fullscreen>", QT_TRANSLATE_NOOP ("Help", "Sets the default window geometry. Default: 800x600."));
  ADD_OPT ("--dpi=<dpi>", QT_TRANSLATE_NOOP ("Help", "Sets the remote X2Go Agent's DPI value to \"dpi\". Default: same as local display."));
  ADD_OPT ("--link=<modem|isdn|adsl|wan|lan>", QT_TRANSLATE_NOOP ("Help", "Sets the default link type. Default: \"adsl\"."));
  ADD_OPT ("--pack=<packmethod>", QT_TRANSLATE_NOOP ("Help", "Sets default pack method. Default: \"16m-jpeg-9\"."));
  ADD_OPT ("--clipboard=<both|client|server|none>", QT_TRANSLATE_NOOP ("Help", "Sets the default clipboard mode. Default: \"both\"."));
  ADD_OPT ("--kbd-layout=<layout>", QT_TRANSLATE_NOOP ("Help", "Sets the default keyboard layout to \"layout\". \"layout\" may be a comma-separated list."));
  ADD_OPT ("--kbd-type=<type>", QT_TRANSLATE_NOOP ("Help", "Sets the default keyboard type."));
  ADD_OPT ("--home=<dir>", QT_TRANSLATE_NOOP ("Help", "Sets the user's home directory."));
  ADD_OPT ("--set-kbd=<0|1>", QT_TRANSLATE_NOOP ("Help", "Enables or disables overwriting the current keyboard settings."));
  ADD_OPT ("--autostart=<app>,[<app2>,...]", QT_TRANSLATE_NOOP ("Help", "Automatically launches the application(s) \"app\", \"app2\", ... on session start in Published Applications mode."));
  ADD_OPT ("--session-conf=<file>", QT_TRANSLATE_NOOP ("Help", "Defines an alternative session config file path."));
  ADD_OPT ("--tray-icon", QT_TRANSLATE_NOOP ("Help", "Force-enables session system tray icon."));
  ADD_OPT ("--close-disconnect", QT_TRANSLATE_NOOP ("Help", "Automatically closes X2Go Client after a disconnect."));
  ADD_OPT ("--hide-foldersharing", QT_TRANSLATE_NOOP ("Help", "Hides all Folder-Sharing-related options."));
  ADD_OPT ("--broker-name=<name>", QT_TRANSLATE_NOOP ("Help", "Sets the broker name to display in X2Go Client. This parameter is optional."));
  ADD_OPT ("--broker-url=<protocol>://[username@]<host>[:port]/path", QT_TRANSLATE_NOOP ("Help", "Sets the URL of the session broker." NEWLINE
                                                                                         "\"protocol\" must be one of \"http\", \"https\" or \"ssh\"." NEWLINE
                                                                                         "If \"username@\" is provided, it will be pasted into the authorization dialog of X2Go Client." NEWLINE
                                                                                         "URL examples are:" NEWLINE
                                                                                         "    https://x2gobroker.org/cgi-bin/x2gobroker.cgi" NEWLINE
                                                                                         "    ssh://user@x2gobroker.org:22/usr/lib/x2go/x2gobroker.pl"));
  ADD_OPT ("--broker-ssh-key=<path to key>", QT_TRANSLATE_NOOP ("Help", "Sets the path to an SSH key to use for authentication against an SSH session broker. The client's behavior is undefined if this flag is used for non-SSH session brokers."));
  ADD_OPT ("--broker-autologin", QT_TRANSLATE_NOOP ("Help", "Enables the use of the default SSH key or SSH agent for authentication against an SSH session broker. The client's behavior is undefined if this flag is used for non-SSH session brokers."));
  ADD_OPT ("--broker-noauth", QT_TRANSLATE_NOOP ("Help", "Does not ask for user credentials during session broker authentication. This can be useful if you are using an HTTP(S) session broker without authentication. If you run an HTTP(S) server without authentication, but with user-specific profiles, then put the user name into the broker URL (refer to --broker-url.) The user name then will be extracted from the broker URL and be sent to the session broker. The client's behavior is undefined if this flag is used for non-HTTP(S) session brokers."));
  ADD_OPT ("--background=<svg-file>", QT_TRANSLATE_NOOP ("Help", "Use a custom/branded background image (SVG format) for X2Go Client's main window."));
  ADD_OPT ("--branding=<svg-file>", QT_TRANSLATE_NOOP ("Help", "Use a custom icon (SVG format) for additional branding to replace the default in the lower left corner of X2Go Client's main window."));

# undef NEWLINE
# undef ADD_OPT

  return (ret);
}

help::data_t help::build_data () {
  return (help::data_t (help::cleanup_prelude (help::build_prelude ()), help::cleanup_params (help::build_params ())));
}

help::string_split_t help::split_long_line (QString &line, std::ptrdiff_t max_length) {
  string_split_t ret (line, "");

  if (line.size () > max_length) {
    /* Try to find the next split point. */
    std::ptrdiff_t split_point = line.lastIndexOf (" ", max_length - 1);

    /* Only care for valid split points. */
    if (0 <= split_point) {
      x2goDebug << "Split onto " << line.left (split_point) << " and new part " << line.mid (split_point + 1);
      ret.first = line.left (split_point);
      ret.second = line.mid (split_point + 1);
    }
  }

  return (ret);
}

QString help::pretty_print (bool terminal_output) {
  return (help::pretty_print (help::build_data (), terminal_output));
}

QString help::pretty_print (help::data_t data, bool terminal_output) {
  QString ret = "";
  QTextStream out (&ret);
  out << data.first.join ("\n") << "\n";

  std::size_t max_len = 0;

  /* Iterate over all parameter options and get max width. */
  for (help::params_t::const_iterator it = data.second.constBegin (); it != data.second.constEnd (); ++it) {
    max_len = std::max (max_len, static_cast<std::size_t> ((*it).first.size ()));
  }

  std::size_t terminal_cols = 0;

  if (terminal_output) {
#ifdef Q_OS_WIN
    CONSOLE_SCREEN_BUFFER_INFO terminal_internal;
    HANDLE stderr_handle = GetStdHandle (STD_ERROR_HANDLE);
    if (stderr_handle && (stderr_handle != INVALID_HANDLE_VALUE)) {
      if (GetConsoleScreenBufferInfo (stderr_handle, &terminal_internal)) {
        terminal_cols = (terminal_internal.srWindow.Right - terminal_internal.srWindow.Left) + 1;
      }
    }
#elif defined (Q_OS_UNIX)
    struct winsize terminal_internal;
    ioctl (0, TIOCGWINSZ, &terminal_internal);
    terminal_cols = terminal_internal.ws_col;
#endif
  }

  x2goDebug << "Terminal cols: " << terminal_cols << endl;

  for (help::params_t::const_iterator it = data.second.constBegin (); it != data.second.constEnd (); ++it) {
    std::size_t indent = (max_len - (*it).first.size ()) + 4;
    x2goDebug << "Indent: " << indent << "; max_len: " << max_len << "; param size: " << (*it).first.size () << endl;
    out << "  ";
    out << (*it).first;
    out << QString (" ").repeated (indent);

    /* Append first two spaces to the general indent level for upcoming lines. */
    indent += 2;
    std::ptrdiff_t remaining = 0;

    /* Split up description on newlines. */
    QStringList desc_split = (*it).second.split ("\n");

    for (QStringList::const_iterator desc_split_it = desc_split.constBegin (); desc_split_it != desc_split.constEnd (); ++desc_split_it) {
      std::size_t cur_len = (*desc_split_it).size ();
      x2goDebug << "Going to output a description " << (*desc_split_it).size () << " chars wide." << endl;
      if (0 != terminal_cols) {
        /*
         * Only set this the first time right after having written the parameter and indent spaces.
         * Don't change it after that.
         */
        if (desc_split_it == desc_split.constBegin ()) {
          remaining = terminal_cols - (indent + (*it).first.size ());
        }
        x2goDebug << "Still have " << remaining << " characters left on this line." << endl;

        /* Ran out of space? That's bad... print a newline and don't use any indentation level. */
        if (0 > remaining) {
          x2goDebug << "Ran out of space! Will break line and start the description on a new one." << endl;
          out << "\n";
          remaining = terminal_cols;
          indent = 0;
        }
      }

      QString working_copy (*desc_split_it);

      while (!working_copy.isEmpty ()) {
        cur_len = working_copy.size ();
        x2goDebug << "Trying to fit a (remaining) description " << cur_len << " characters wide." << endl;

        string_split_t string_split;

        if (0 != terminal_cols) {
          string_split = split_long_line (working_copy, remaining);
        }
        else {
          /* For non-terminal printing (or if the width is unknown), use the default splitting length. */
          string_split = split_long_line (working_copy);
        }

        /* Print potentially splitted line. */
        working_copy = string_split.first;
        out << working_copy;

        /* Continue with next chunk. */
        working_copy = string_split.second;;

        out << "\n";

        /*
         * Print whitespace if the remainder string is non-empty
         * or printing shall continue on next line.
         */
        if ((!working_copy.isEmpty ()) || ((desc_split_it + 1) != desc_split.constEnd ())) {
          indent = 2 + max_len + 4;
          out << QString (" ").repeated (indent);
        }
      }
    }
  }

  if (terminal_output) {
    qCritical ().nospace () << qPrintable (ret);
  }

  return (ret);
}
