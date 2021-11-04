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

#ifndef SESSIONBUTTON_H
#define SESSIONBUTTON_H

#include "SVGFrame.h"
#include <QPushButton>
#include <QLabel>
class ONMainWindow;
class QComboBox;
class QPushButton;

/**
	@author Oleksandr Shneyder <oleksandr.shneyder@obviously-nice.de>
*/
class SessionButton : public SVGFrame
{
    Q_OBJECT
public:
    enum {KDE,GNOME,LXDE, LXQt, XFCE,MATE,UNITY,CINNAMON,TRINITY,OPENBOX,ICEWM,RDP,XDMCP,SHADOW,PUBLISHED,OTHER,APPLICATION};
    SessionButton ( ONMainWindow* mw, QWidget* parent,QString id );
    ~SessionButton();
    QString id() {
        return sid;
    }
    void redraw();
    const QPixmap* sessIcon() {
        return icon->pixmap();
    }
    static bool lessThen ( const SessionButton* b1, const SessionButton* b2 );
    QString name();
    QString getPath()
    {
        return path;
    }
    void setPath(QString path)
    {
        this->path=path;
    }
    void setNotUpdated()
    {
        updated=false;
    }
    bool isUpdated()
    {
        return updated;
    }
    bool isFav()
    {
        return fav;
    }
    int getRunning()
    {
        return running;
    }
    int getSuspended()
    {
        return suspended;
    }

private:
    void setFav(bool fav);
    bool getFavFromConfig();
    QString nameofSession;
    QString sid;
    QString path;
    QLabel* sessName;
    QLabel* sessStatus;
    QLabel* icon;
    QComboBox* cmdBox;
    QLabel* cmd;
    QLabel* serverIcon;
    QLabel* geomIcon;
    QLabel* cmdIcon;
    QLabel* server;
    QPushButton* editBut;
    QPushButton* favButton;
    QLabel* geom;
    QMenu* sessMenu;
    QComboBox* geomBox;
    QPushButton* sound;
    QLabel* soundIcon;
    ONMainWindow* par;
    QAction* act_edit;
    QAction* act_createIcon;
    QAction* act_remove;
    bool rootless;
    bool published;
    bool editable;
    bool updated;
    bool fav;
    int running;
    int suspended;

private slots:
    void slotClicked();
    void slotEdit();
    void slot_soundClicked();
    void slot_cmd_change ( const QString& command );
    void slot_geom_change ( const QString& new_g );
    void slotRemove();
    void slotMenuHide();
    void slotShowMenu();
    void slotCreateSessionIcon();
    void slotFavClick();
signals:
    void sessionSelected ( SessionButton* );
    void signal_edit ( SessionButton* );
    void signal_remove ( SessionButton* );
    void clicked();
    void favClicked();
protected:
    virtual void mouseMoveEvent ( QMouseEvent * event );
    virtual void mousePressEvent ( QMouseEvent * event );
    virtual void mouseReleaseEvent ( QMouseEvent * event );
};

#endif
