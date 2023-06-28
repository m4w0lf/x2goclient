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

#include "connectionwidget.h"

#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QStringList>
#include <QGroupBox>
#include <QBoxLayout>
#include <QSpinBox>
#include <QComboBox>
#include "x2gosettings.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include "onmainwindow.h"
ConnectionWidget::ConnectionWidget ( QString id, ONMainWindow * mw,
                                     QWidget * parent, Qt::WindowFlags f )
		: ConfigWidget ( id,mw,parent,f )
{
	QVBoxLayout *connLay=new QVBoxLayout ( this );
	QGroupBox* netSpd=new QGroupBox ( tr ( "&Connection speed" ),this );
	QVBoxLayout *spdLay=new QVBoxLayout ( netSpd );
	spd=new QSlider ( Qt::Horizontal,netSpd );
	spd->setMinimum ( 0 );
	spd->setMaximum ( 4 );
	spd->setTickPosition ( QSlider::TicksBelow );
	spd->setTickInterval ( 1 );
	spd->setSingleStep ( 1 );
	spd->setPageStep ( 1 );

	QHBoxLayout *tickLay=new QHBoxLayout();
	QHBoxLayout *slideLay=new QHBoxLayout();
	slideLay->addWidget ( spd );
	QLabel* mlab= new QLabel ( "MODEM",netSpd );
	tickLay->addWidget ( mlab );
	tickLay->addStretch();
	tickLay->addWidget ( new QLabel ( "ISDN",netSpd ) );
	tickLay->addStretch();
	tickLay->addWidget ( new QLabel ( "ADSL",netSpd ) );
	tickLay->addStretch();
	tickLay->addWidget ( new QLabel ( "WAN",netSpd ) );
	tickLay->addStretch();
	tickLay->addWidget ( new QLabel ( "LAN",netSpd ) );
	spdLay->addLayout ( slideLay );
	spdLay->addLayout ( tickLay );
	QFontMetrics fm ( mlab->font() );
	slideLay->insertSpacing ( 0,fm.width ( "MODEM" ) /2 );
	slideLay->addSpacing ( fm.width ( "LAN" ) /2 );

	QGroupBox* compr=new QGroupBox ( tr ( "C&ompression" ),this );
	QHBoxLayout* comprLay=new QHBoxLayout ( compr );
	packMethode = new QComboBox ( this );
	quali= new QSpinBox ( this );
	quali->setRange ( 0,9 );

	QVBoxLayout* colLay=new QVBoxLayout();
	QVBoxLayout* cowLay=new QVBoxLayout();
	QHBoxLayout* spbl=new QHBoxLayout();
	colLay->addWidget ( new QLabel ( tr ( "Method:" ),compr ) );
	colLay->addWidget ( qualiLabel=new QLabel ( tr ( "Image quality:" ),
	        compr ) );
	cowLay->addWidget ( packMethode );
	spbl->addWidget ( quali );
	spbl->addStretch();
	cowLay->addLayout ( spbl );
	comprLay->addLayout ( colLay );
	comprLay->addLayout ( cowLay );
	connLay->addWidget ( netSpd );
	connLay->addWidget ( compr );
	connLay->addStretch();

	connect ( packMethode,SIGNAL ( activated ( const QString& ) ),this,
	          SLOT ( slot_changePack ( const QString& ) ) );
	readConfig();
}


ConnectionWidget::~ConnectionWidget()
{
}

void ConnectionWidget::loadPackMethods()
{
	QFile file ( ":/txt/packs" );
	if ( !file.open ( QIODevice::ReadOnly | QIODevice::Text ) )
		return;
	QTextStream in ( &file );
	while ( !in.atEnd() )
	{
		QString pc=in.readLine();
		if ( pc.indexOf ( "-%" ) !=-1 )
		{
			pc=pc.left ( pc.indexOf ( "-%" ) );
			qualiList<<pc;
		}
		packMethode->addItem ( pc );
	}
	file.close();
}


void ConnectionWidget::slot_changePack ( const QString& pc )
{
	bool ct=qualiList.contains ( pc );
	quali->setEnabled ( ct );
	qualiLabel->setEnabled ( ct );
}

void ConnectionWidget::readConfig()
{

	loadPackMethods();
	X2goSettings st ( "sessions" );

	spd->setValue ( st.setting()->value (
	                    sessionId+"/speed",
	                    ( QVariant ) mainWindow->getDefaultLink()
	                ).toInt() );
	QString mt=st.setting()->value (
	               sessionId+"/pack",
	               ( QVariant ) mainWindow->getDefaultPack() ).toString();


	packMethode->setCurrentIndex ( packMethode->findText ( mt ) );
	quali->setValue ( st.setting()->value ( sessionId+"/quality",
	                             mainWindow->getDefaultQuality() ).toInt() );
	slot_changePack ( mt );
}


void ConnectionWidget::setDefaults()
{
	spd->setValue ( 2 );
	packMethode->setCurrentIndex (
	    packMethode->findText ( "16m-jpeg" ) );
	quali->setValue ( 9 );
	slot_changePack ( "16m-jpeg" );
}

void ConnectionWidget::saveSettings()
{

	X2goSettings st ( "sessions" );
	st.setting()->setValue ( sessionId+"/speed", ( QVariant ) spd->value() );
	st.setting()->setValue ( sessionId+"/pack",
	              ( QVariant ) packMethode->currentText() );
	st.setting()->setValue ( sessionId+"/quality", ( QVariant ) quali->value() );
	st.setting()->sync();
}
