/**************************************************************************
*   Copyright (C) 2005-2023 by Oleksandr Shneyder                         *
*                              <o.shneyder@phoca-gmbh.de>                 *
*   Copyright (C) 2015-2023 by Mihai Moldovan <ionic@ionic.de>            *
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

#ifndef HELPDIALOG_H
#define HELPDIALOG_H

#include "ui_helpdialog.h"

class HelpDialog : public QDialog, public Ui_HelpDialog {
  public:
    HelpDialog (QWidget *parent = 0);
    void setText (QString text);
};

#endif /* !defined (HELPDIALOG_H) */
