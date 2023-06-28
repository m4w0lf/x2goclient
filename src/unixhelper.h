/***************************************************************************
 *  Copyright (C) 2015-2023 by Mihai Moldovan <ionic@ionic.de>             *
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program; if not, write to the                          *
 *  Free Software Foundation, Inc.,                                        *
 *  59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.              *
 ***************************************************************************/


#ifndef UNIXHELPER_H
#define UNIXHELPER_H

#include <QtCore/qglobal.h>
#include <unistd.h>

#ifdef Q_OS_UNIX

namespace unixhelper {
  /*
   * Unblocks all signals and installs a signal handler for SIGHUP,
   * which calls kill_pgroup ().
   *
   * Should signal unblocking or installing the signal handler fail,
   * an emergency exit is performed and
   * the whole process group killed.
   *
   * The signals SIGINT, SIGTERM, SIGPIPE, SIGQUIT, SIGUSR1 and
   * SIGUSR2 are ignored.
   *
   * Loops indefinitely afterwards.
   *
   * In this loop, the current parent PID is polled and compared against
   * the original value passed via parameter parent.
   * Should they mismatch, the parent died and kill_pgroup () is called.
   */
  int unix_cleanup (const pid_t parent);

  /*
   * Wrapper for killing a whole process group.
   * The "real" killing work is done by real_kill_pgroup ().
   * This function tries to fork off another process and change
   * the new function's process group ID.
   * If any of these operations fail, killing the original process
   * group ID will still continue, albeit with warning messages.
   *
   * signal may be any of:
   *   * -1       to indicate an error leading to emergency termination
   *   * SIGHUP   as the standard signal that is sent when the
   *              group leader dies under specific circumstances
   *              (we cannot rely that this always happens, though,
   *               so a polling solution is needed, see unix_cleanup().)
   * Other values are not handled.
   */
  void kill_pgroup (const int signal);

  /*
   * Kills the whole process group.
   * First, SIGTERM is sent to the group.
   * A 10 seconds grace period is granted to make sure
   * processes exit cleanly on their own.
   * Lastly, SIGKILL is sent to the group -- which also
   * implies the demise of this program.
   *
   * pgid is the process group ID to be killed.
   */
  void real_kill_pgroup (const pid_t pgid);
}

#endif /* defined (Q_OS_UNIX) */

#endif /* !defined (UNIXHELPER_H) */
