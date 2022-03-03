/**************************************************************************
*   Copyright (C) 2005-2020 by Oleksandr Shneyder                         *
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

#ifndef SESSIONEXPLORER_H
#define SESSIONEXPLORER_H

#include <QObject>
#include <QList>
#include <QStringList>

class SessionButton;
class FolderButton;
class ONMainWindow;
class QToolButton;
class QPushButton;
class QLabel;
class QHBoxLayout;

class SessionExplorer: public QObject
{
    Q_OBJECT
public:
    SessionExplorer(ONMainWindow *p);
    ~SessionExplorer();
    QList<SessionButton*> * getSessionsList()
    {
        return &sessions;
    }
    QList<FolderButton*> * getFoldersList()
    {
        return &folders;
    }
    SessionButton* getLastSession() {
        return lastSession;
    }
    void setLastSession(SessionButton* s)
    {
        lastSession=s;
    }
    void cleanSessions();
    SessionButton* createBut ( const QString& id );
    void placeButtons();
    QHBoxLayout* getNavigationLayout()
    {
        return navigationLayout;
    }
    QHBoxLayout* getSelectLayout()
    {
        return selectLayout;
    }
    bool isFolderEmpty(QString path);
    void resize();
    void setFolderIcon(QString path, QString icon);
    void createNewFolder(QString path);
    void renameFolder(QString oldPath, QString currentPath);
    void deleteFolder(QString path);
    void updateSessions(QStringList slst);

    QString getCurrentPath()
    {
        return currentPath;
    }

    void setCurrrentPath(QString path)
    {
        currentPath=path;
    }

    void setEnable(bool enable);


//vars
private:
    QList<SessionButton*> sessions;
    QList<FolderButton*> folders;
    SessionButton* lastSession;
    ONMainWindow* parent;
    QToolButton* backButton;
    QPushButton* favButton;
    QPushButton* runButton;
    QPushButton* allButton;
    QLabel* pathLabel;
    QHBoxLayout* navigationLayout;
    QHBoxLayout* selectLayout;
    QString currentPath;
    enum {ALL, FAV, RUN} viewMode;
    bool initUpdate;

//functions
private:
    void setNavigationVisible(bool value);
    int findFolder(QString path);
    void createFolder(QString path);
    void getFoldersFromConfig();
    SessionButton* findSession( const QString& id);
    void checkPath(SessionButton* s);
    bool isFolderEmpty(FolderButton* folder);

public slots:
    void slotDeleteButton ( SessionButton * bt );
    void slotEdit ( SessionButton* );
    void slotCreateDesktopIcon ( SessionButton* bt );
    void exportsEdit ( SessionButton* bt );
private slots:
    void slotFolderSelected(FolderButton* bt);
    void slotLevelUp();
    QStringList getFolderChildren(FolderButton* folder);
    void slotShowAll();
    void slotShowRun();
    void slotShowFav();
    void updateView();
};

#endif
