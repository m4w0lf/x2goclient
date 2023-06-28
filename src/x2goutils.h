/**************************************************************************
*   Copyright (C) 2005-2023 by Oleksandr Shneyder                         *
*                              <o.shneyder@phoca-gmbh.de>                 *
*   Copyright (C) 2016-2023 by Mihai Moldovan <ionic@ionic.de>            *
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

#ifndef X2GOUTILS_H
#define X2GOUTILS_H

#include <QString>
#include <QFont>
#include <QMessageBox>

#ifndef UNUSED
#define UNUSED(x) do { (void) x; } while (0)
#endif

#ifndef MACPORTS_PREFIX
#define MACPORTS_PREFIX "/opt/local"
#endif

QString expandHome (QString path);

QString fixup_resource_URIs (const QString &res_path);

QString wrap_legacy_resource_URIs (const QString &res_path);

QString convert_to_rich_text (const QString &text, bool force = false);

void show_RichText_Generic_MsgBox (QMessageBox::Icon icon, const QString &main_text, const QString &informative_text, bool app_modal = false);
void show_RichText_WarningMsgBox (const QString &main_text, const QString &informative_text = "", bool app_modal = false);
void show_RichText_ErrorMsgBox (const QString &main_text, const QString &informative_text = "", bool app_modal = false);

QString git_changelog_extract_commit_sha (const QString &gitlog);

bool font_is_monospaced (const QFont &font);

#ifdef Q_OS_DARWIN
void show_XQuartz_not_found_error ();
void show_XQuartz_start_error ();
void show_XQuartz_generic_error (const QString &main_error, const QString &additional_info);
#endif /* defined (Q_OS_DARWIN) */

#ifdef Q_OS_UNIX
/*
 * Add a list of strings to a PATH value.
 *
 * If back is true (the default), each entry is (potentially) appended to the original
 * PATH value, if not already existing within the original PATH value.
 * Ex.: <orig_path>:<add_entry1>:<add_entry2>:...
 *
 * Otherwise, each entry is prepended to the original PATH value, if not already existing.
 * Ex.: <add_entry1>:<add_entry2>:...:<orig_path>
 */
QString add_to_path (const QString &orig_path, const QStringList &add, const bool back = true);

/*
 * Returns the first existing path that contains binary_name.
 * Iff no component contains a binary called binary_name, an empty path is returned.
 *
 * Iff path is empty, (only) the current working dir is searched.
 */
QString find_binary (const QString &path, const QString &binary_name);
#endif /* defined (Q_OS_UNIX) */

#endif /* !defined (X2GOUTILS_H) */
