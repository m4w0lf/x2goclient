/**************************************************************************
*   Copyright (C) 2005-2023 by Oleksandr Shneyder                         *
*                              <o.shneyder@phoca-gmbh.de>                 *
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

#ifndef SESSIONMANAGEDIALOG_H
#define SESSIONMANAGEDIALOG_H
#include "x2goclientconfig.h"

#include <QDialog>
class QTreeWidget;
class QTreeWidgetItem;
class QPushButton;
class QModelIndex;
class ONMainWindow;
/**
	@author Oleksandr Shneyder <oleksandr.shneyder@obviously-nice.de>
*/

class SessionManageDialog : public QDialog
{
    Q_OBJECT
public:
    SessionManageDialog ( QWidget * parent,
                          bool onlyCreateIcon=false,
                          Qt::WindowFlags f=0 );
    ~SessionManageDialog();
    void loadSessions();
private:
    void initFolders(QTreeWidgetItem* parent, QString path);
private:
    QTreeWidget* sessions;
    QPushButton* editSession;
    QPushButton* removeSession;
    QPushButton* createSessionIcon;
    ONMainWindow* par;
    QString currentPath;
private slots:
    void slot_endisable (QTreeWidgetItem *item, int col = -1);
    void slot_endisable_ItemChanged_wrapper (QTreeWidgetItem *item, QTreeWidgetItem *);
    void slotNew();
    void slot_edit();
    void slot_createSessionIcon();
    void slot_delete();
    void slot_dclicked ( QTreeWidgetItem * item, int );
};

#endif
