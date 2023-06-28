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

#ifndef INTERACTIONDIALOG_H
#define INTERACTIONDIALOG_H
#include "x2goclientconfig.h"


#include "SVGFrame.h"

class ONMainWindow;
class QTextEdit;
class QLineEdit;
class QPushButton;
class InteractionDialog: public SVGFrame
{

    Q_OBJECT
public:
    enum IMode {SESSION,BROKER};
    InteractionDialog (ONMainWindow* mainw, QWidget* parent);
    virtual ~InteractionDialog();
    void reset();
    void appendText(QString txt);
    bool isInterrupted() {
        return interrupted;
    }
    void setDisplayMode();
    void setInteractionMode(IMode value);
    IMode getInteractionMode()
    {
        return interactionMode;
    }
private:
    ONMainWindow* mw;
    QTextEdit* textEdit;
    QPushButton* cancelButton;
    QLineEdit* textEntry;
    bool interrupted;
    bool display;
    IMode interactionMode;
private slots:
    void slotTextEntered();
    void slotButtonPressed();
signals:
    void textEntered(QString text);
    void interrupt();
    void closeInterractionDialog();
};

#endif
