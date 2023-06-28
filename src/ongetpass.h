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

#ifndef ONGETPASS_H
#define ONGETPASS_H
#include <QString>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef Q_OS_WIN
__declspec(dllexport)	
#endif	
int x2goMain ( int argc, char *argv[] );

void askpass ( const QString& param, const QString& accept,
               const QString& cookie, const QString& socketName );


#ifdef __cplusplus
}
#endif


#endif
