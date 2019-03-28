/**************************************************************************
*   Copyright (C) 2005-2017 by Oleksandr Shneyder                         *
*   o.shneyder@phoca-gmbh.de                                              *
*   Copyright (C) 2015-2017 by Mihai Moldovan <ionic@ionic.de>            *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program.  If not, see <https://www.gnu.org/licenses/>. *
***************************************************************************/

#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <vector>
#include <csignal>
#include <array>

#include "unixhelper.h"
#include "ongetpass.h"
#include "compat.h"

int wrap_x2go_main (int argc, char **argv) {
  return (x2goMain (argc, argv));
}

#ifdef Q_OS_UNIX
int fork_helper (int argc, char **argv) {
  /* Fork off to start helper process. */
  pid_t tmp_pid = fork ();

  /* Child. */
  if (0 == tmp_pid) {
    /* Starting unixhelper. */
    std::vector<std::string> new_argv;
    new_argv.push_back (std::string (argv[0]));
    new_argv.push_back ("--unixhelper");

    std::vector<char *> new_argv_c_str;
    for (std::vector<std::string>::iterator it = new_argv.begin (); it != new_argv.end (); ++it) {
      const char *elem = (*it).c_str ();
      new_argv_c_str.push_back (strndup (elem, std::strlen (elem)));
    }

    /* Add null pointer as last element. */
    new_argv_c_str.push_back (0);

    if (0 != execvp (new_argv_c_str.front (), &(new_argv_c_str.front ()))) {
      const int saved_errno = errno;
      std::cerr << "Failed to re-execute process as UNIX cleanup helper tool: " << std::strerror (saved_errno) << "\n"
                << "Terminating and killing parent." << "\n"
                << "Please report a bug, refer to this documentation: https://wiki.x2go.org/doku.php/wiki:bugs" << std::endl;

      pid_t parent_pid = getppid ();
      if (0 != kill (parent_pid, SIGTERM)) {
        const int saved_errno = errno;
        std::cerr << "Failed to kill parent process: " << std::strerror (saved_errno) << std::endl;
      }

      std::exit (EXIT_FAILURE);
    }

    /* Anything here shall be unreachable. */
    return (0);
  }
  /* Error. */
  else if (-1 == tmp_pid) {
    const int saved_errno = errno;
    std::cerr << "Unable to create a new process for the UNIX cleanup watchdog: " << std::strerror (saved_errno) << "\n";
    std::cerr << "Terminating. Please report a bug, refer to this documentation: https://wiki.x2go.org/doku.php/wiki:bugs" << std::endl;

    std::exit (EXIT_FAILURE);
  }
  /* Parent. */
  else {
    /* Start real X2Go Client. */
    return (wrap_x2go_main (argc, argv));
  }
}
#endif /* defined (Q_OS_UNIX) */

int main (int argc, char **argv) {
#ifdef Q_OS_UNIX
  /*
   * Flags we don't need a cleanup helper for, since we know that X2Go Client
   * will never spawn other processes.
   *
   * FIXME: What we'd actually want to have at this point (instead of a
   *        hardcoded list of parameters, anyway) is to use the argument parser
   *        from ONMainWindow (parseParameter). If this function returns false
   *        for any parameter, we know that we won't ever need the UNIX cleanup
   *        helper tool. Sadly, ONMainWindow is only started/available later,
   *        so we can't use any of its functionality here. We'd also need to
   *        make the function side-effect free. It should probably be
   *        refactored into a special options parser class.
   */
  const std::array<const std::string, 6> bypass_flags = {
    "--help",
    "--help-pack",
    "--version",
    "-v",
    "--changelog",
    "--git-info"
  };

  bool bypass_unix_helper = 0;
  bool unix_helper_request = 0;
  for (int i = 0; i < argc; ++i) {
    /* No need to continue scanning if we got the information we were looking for. */
    if ((bypass_unix_helper) && (unix_helper_request)) {
      break;
    }

    std::string cur_arg (argv[i]);

    /* Make the current argument lowercase. */
    std::transform (cur_arg.begin (), cur_arg.end (), cur_arg.begin (), ::tolower);

    if (!(cur_arg.empty ())) {
      /* Scan program arguments for --unixhelper flag. */
      if ((!(unix_helper_request)) && (0 == cur_arg.compare ("--unixhelper"))) {
        unix_helper_request = 1;
      }

      /* Scan for flags bypassing the unix helper. */
      if (!(bypass_unix_helper)) {
        for (std::array<const std::string, 6>::const_iterator cit = bypass_flags.cbegin (); cit != bypass_flags.cend (); ++cit) {
          if (0 == cur_arg.compare (*cit)) {
            bypass_unix_helper = 1;
          }
        }
      }
    }
  }

  /* Sanity checks! */
  if ((unix_helper_request) && (bypass_unix_helper)) {
    std::cerr << "Re-execution in UNIX cleanup helper mode was requested, but a command line parameter that is supposed to "
              << "disable the UNIX cleanup helper was found.\n"
              << "Terminating. Please report a bug, refer to this documentation: https://wiki.x2go.org/doku.php/wiki:bugs" << std::endl;

    return (EXIT_FAILURE);
  }

  if (bypass_unix_helper) {
    return (wrap_x2go_main (argc, argv));
  }
  else {
    if (unix_helper_request) {
      /* We were instructed to start as the UNIX cleanup helper tool. */
      return (unixhelper::unix_cleanup (getppid ()));
    }
    else {
      /*
       * setsid() may succeed and we become a session and process
       * group leader, or it may fail indicating that we already
       * are a process group leader. Either way is fine.
       */
      setsid ();

      /* We should be process group leader by now. */
      return (fork_helper (argc, argv));
    }
  }
#else /* defined (Q_OS_UNIX) */
  return (wrap_x2go_main (argc, argv));
#endif /* defined (Q_OS_UNIX) */
}
