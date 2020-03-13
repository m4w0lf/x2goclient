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

#include "SVGFrame.h"
#include "x2goclientconfig.h"

#include <QPainter>
#include <QTimer>
#include "x2gologdebug.h"
#include <QResizeEvent>



SVGFrame::SVGFrame ( QString fname,bool st,QWidget* parent,
                     Qt::WindowFlags f ) :QFrame ( parent,f )
{
	empty=false;
    renderer=0;
#ifdef Q_OS_WIN
	parentWidget=0;
#endif
	if ( fname==QString::null )
		empty=true;
	if ( !empty )
	{
		repaint=true;
		drawImg=st;
        if(fname.indexOf("png")==-1)
        {
		   renderer=new QSvgRenderer ( this );
		   renderer->load ( fname );
        }
        else
        {
            pix.load(fname);
        }

		if ( drawImg )
		{
			setAutoFillBackground ( true );
		}
		else
		{
			QTimer *timer = new QTimer ( this );
			connect ( timer, SIGNAL ( timeout() ), this, SLOT ( update() ) );
			if ( renderer->animated() )
			{
				timer->start ( 1000/renderer->framesPerSecond() );
				x2goDebug<<"Animated, fps:"<<renderer->framesPerSecond() <<endl;
			}
		}
	}
}

SVGFrame::SVGFrame ( QWidget* parent,
                     Qt::WindowFlags f ) :QFrame ( parent,f )
{
	repaint=false;
	empty=true;
}

void SVGFrame::paintEvent ( QPaintEvent* event )
{
	if ( repaint && !empty )
	{
		QPainter p ( this );
        if(renderer)
            renderer->render ( &p );
        else
            p.drawPixmap(0,0,width(), height(),pix);
	}
	QFrame::paintEvent ( event );
}


void SVGFrame::resizeEvent ( QResizeEvent* event )
{
	QFrame::resizeEvent ( event );
	emit resized ( event->size() );
}


QSize SVGFrame::sizeHint() const
{
	if ( !empty)
    {
        if(renderer)
		  return renderer->defaultSize();
        return pix.size();
    }
	return QFrame::sizeHint();
}

void SVGFrame::loadBg ( QString fl )
{
    if(renderer)
	   renderer->load ( fl );
	update();
}

#ifdef Q_OS_WIN
#include "wapi.h"
void SVGFrame::mousePressEvent ( QMouseEvent * event )
{
/*	if ( isVisible() )
	{
		int vBorder;
		int hBorder;
		int barHeight;
		if ( parentWidget )
			wapiGetBorders ( parentWidget->winId(), vBorder, hBorder, barHeight );
		x2goDebug<<"svg frame: "<<event->pos() <<
		":"<<mapFromGlobal ( event->pos() ) <<":"<<barHeight<<":"<<vBorder<<":"
		<<hBorder<<":"<<pos() <<endl;
		QMouseEvent * nevent=new QMouseEvent(event->type(), QPoint(0,0),
				event->button(), event-> buttons(), event->modifiers());
		QFrame::mousePressEvent ( nevent );
		return;
	}*/
	QFrame::mousePressEvent ( event );
}
#endif
