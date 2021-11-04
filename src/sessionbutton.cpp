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

#include "sessionbutton.h"
#include "x2goclientconfig.h"
#include "x2goutils.h"

#include <QFont>
#include <QPixmap>
#include <QLabel>
#include "x2gosettings.h"
#include <QDir>
#include <QLayout>
#include <QComboBox>
#include <QMouseEvent>
#include <QMenu>
#include <QPushButton>
#include "onmainwindow.h"
#include "x2gologdebug.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QSvgRenderer>
#include <QPainter>
#include <QImage>
#include <QPixmap>
#include <QPolygon>
#include "sessionexplorer.h"


SessionButton::SessionButton ( ONMainWindow* mw,QWidget *parent, QString id )
    : SVGFrame ( mw->images_resource_path("/svg/sessionbut.svg"),false,parent )
{
    editable=mw->sessionEditEnabled();

    running=suspended=0;

    QPalette pal=palette();
    pal.setColor ( QPalette::Active, QPalette::WindowText, QPalette::Mid );
    pal.setColor ( QPalette::Active, QPalette::ButtonText, QPalette::Mid );
    pal.setColor ( QPalette::Active, QPalette::Text, QPalette::Mid );
    pal.setColor ( QPalette::Inactive, QPalette::WindowText, QPalette::Mid );
    pal.setColor ( QPalette::Inactive, QPalette::ButtonText, QPalette::Mid );
    pal.setColor ( QPalette::Inactive, QPalette::Text, QPalette::Mid );

    setPalette(pal);

    updated=false;
    fav=false;

    QFont fnt=font();
    if ( mw->retMiniMode() )
#ifdef Q_WS_HILDON
        fnt.setPointSize ( 10 );
#else
        fnt.setPointSize ( 9 );
#endif
    setFont ( fnt );
    setFocusPolicy ( Qt::NoFocus );
    bool miniMode=mw->retMiniMode();
    if ( !miniMode )
        setFixedSize ( 340,190 );
    else
        setFixedSize ( 250,145 );


    par= mw;
    connect ( this,SIGNAL ( clicked() ),this,SLOT ( slotClicked() ) );

    sid=id;

    cmdBox=new QComboBox ( this );
    cmdBox->setMouseTracking ( true );
    cmdBox->setFrame ( false );
    QPalette cpal=cmdBox->palette();

    cpal.setColor ( QPalette::Button,QColor ( 255,255,255 ) );
    cpal.setColor ( QPalette::Base,QColor ( 255,255,255 ) );
    cpal.setColor ( QPalette::Window,QColor ( 255,255,255 ) );
    cmdBox->setPalette ( cpal );

    geomBox=new QComboBox ( this );
    geomBox->setMouseTracking ( true );
    geomBox->setFrame ( false );
    geomBox->setEditable ( true );
    geomBox->setEditable ( false );
    geomBox->update();
    geomBox->setPalette ( cpal );

    pathLabel=new QLabel ( this );
    QFont lfnt=fnt;
    lfnt.setWeight(QFont::Thin);
    lfnt.setPointSize(lfnt.pointSize()-2);
    pathLabel->setFont(lfnt);

    sessName=new QLabel ( this );
    sessStatus=new QLabel ( this );
    fnt=sessName->font();
    fnt.setBold ( true );
    sessName->setFont ( fnt );
    icon=new QLabel ( this );
    cmd=new QLabel ( this );
    cmd->setMouseTracking ( true );
    serverIcon=new QLabel ( this );
    geomIcon=new QLabel ( this );
    geomIcon->setMouseTracking ( true );
    cmdIcon=new QLabel ( this );
    cmdIcon->setMouseTracking ( true );
    server=new QLabel ( this );
    geom=new QLabel ( this );
    geom->setMouseTracking ( true );

    sound=new QPushButton ( this );
    soundIcon=new QLabel ( this );
    sound->setPalette ( cpal );
    sound->setFlat ( true );
    sound->setMouseTracking ( true );
    connect ( sound,SIGNAL ( clicked() ),this,
              SLOT ( slot_soundClicked() ) );

    editBut=new QPushButton ( this );
    editBut->setMouseTracking ( true );
    connect ( editBut,SIGNAL ( pressed() ),this,SLOT ( slotShowMenu() ) );

    /* Load our edit button SVG file. */
    QSvgRenderer svg_renderer (par->images_resource_path ("/svg/hamburger.svg"));

    /* Prepare image to render to with full transparency. */
    QImage tmp_image (16, 16, QImage::Format_ARGB32);
    tmp_image.fill (0x00000000);

    /* Paint icon to the image. */
    QPainter tmp_painter (&tmp_image);
    svg_renderer.render (&tmp_painter);

    editBut->setIcon ( QIcon ( QPixmap::fromImage (tmp_image) ) );
    editBut->setIconSize ( QSize ( 16,16 ) );
    editBut->setFixedSize ( 24,24 );
    editBut->setFlat ( true );
    editBut->setPalette ( cpal );
    sessMenu=new QMenu ( this );


    favButton=new QPushButton (this);
    favButton->setIcon(QIcon(mw->iconsPath ( "/16x16/fav.png" )));
    favButton->setFlat(true);
    favButton->setFixedSize(24,24);
    favButton->setMouseTracking(true);
    favButton->setPalette ( cpal );
    connect( favButton, SIGNAL(clicked()), this, SLOT(slotFavClick()));

    connect ( sessMenu,SIGNAL ( aboutToHide() ),this,
              SLOT ( slotMenuHide() ) );

    act_edit=sessMenu->addAction (
                 QIcon (
                     mw->iconsPath ( "/16x16/edit.png" ) ),
                 tr ( "Session preferences ..." ) );
#if (!defined Q_WS_HILDON) && (!defined Q_OS_DARWIN)
    act_createIcon=sessMenu->addAction (
                       QIcon ( mw->iconsPath ( "/16x16/create_file.png" ) ),
                       tr (
                           "Create session icon on desktop ..." ) );
#endif
    act_remove=sessMenu->addAction (
                   QIcon ( mw->iconsPath ( "/16x16/delete.png" ) ),
                   tr ( "Delete session" ) );


    connect ( act_edit,SIGNAL ( triggered ( bool ) ),this,
              SLOT ( slotEdit() ) );

    connect ( act_remove,SIGNAL ( triggered ( bool ) ),this,
              SLOT ( slotRemove() ) );
#if (!defined Q_WS_HILDON) && (!defined Q_OS_DARWIN)
    connect ( act_createIcon,SIGNAL ( triggered ( bool ) ),this,
              SLOT ( slotCreateSessionIcon() ) );
#endif

    editBut->setToolTip ( tr ( "Session actions" ) );
    favButton->setToolTip(tr("Add to favorites"));
    cmdBox->setToolTip ( tr ( "Select type" ) );

    geomBox->setToolTip ( tr ( "Select resolution" ) );
    sound->setToolTip ( tr ( "Toggle sound support" ) );
    icon->move ( 10,10 );

    if ( !miniMode )
    {
        pathLabel->move(80,10);
        pathLabel->setMaximumWidth(224);
        sessName->move ( 80,34 );
        sessStatus->move(80,50);
        editBut->move ( 307,156 );
        favButton->move ( 307,10 );
        serverIcon->move ( 58,84 );
        server->move ( 80,84 );
        cmdIcon->move ( 58,108 );
        cmd->move ( 80,108 );
        cmdBox->move ( 80,108 );
        geomIcon->move ( 58,132 );
        geom->move ( 80,132 );
        geomBox->move ( 80,132 );
        soundIcon->move ( 58,156 );
        sound->move ( 76,156 );
    }
    else
    {
        pathLabel->move(64,6);
        pathLabel->setMaximumWidth(154);
        editBut->move ( 218,113 );
        favButton->move ( 218,10 );
        sessName->move ( 64,22 );
        serverIcon->move ( 66,44 );
        server->move ( 88,44 );
        cmdIcon->move ( 66,68 );
        cmd->move ( 88,68 );
        cmdBox->move ( 88,68 );
        geomIcon->move ( 66,92 );
        geom->move ( 88,92 );
        geomBox->move ( 88,92 );
        soundIcon->move ( 66,116 );
        sound->move ( 86,116 );
    }

    if (mw->brokerMode)
    {
        pathLabel->move(10,10);
        if(miniMode)
            pathLabel->setMaximumWidth(204);
        else
            pathLabel->setMaximumWidth(294);
        icon->move(10,30);
        sessName->move(90,50);
        sessStatus->move(90,70);
        sessStatus->setMinimumWidth(220);
        setFixedHeight(120);
    }


    cmdBox->hide();
    geomBox->hide();
    QPixmap spix;
    spix.load ( par->iconsPath ( "/16x16/session.png" ) );
    serverIcon->setPixmap ( spix );
    serverIcon->setFixedSize ( 16,16 );

    QPixmap rpix;
    rpix.load ( par->iconsPath ( "/16x16/resolution.png" ) );
    geomIcon->setPixmap ( rpix );
    geomIcon->setFixedSize ( 16,16 );

    QPixmap apix;
    apix.load ( par->iconsPath ( "/16x16/audio.png" ) );
    soundIcon->setPixmap ( apix );
    soundIcon->setFixedSize ( 16,16 );
    redraw();

    connect ( cmdBox,SIGNAL ( activated ( const QString& ) ),this,
              SLOT ( slot_cmd_change ( const QString& ) ) );
    connect ( geomBox,SIGNAL ( activated ( const QString& ) ),this,
              SLOT ( slot_geom_change ( const QString& ) ) );

    editBut->setFocusPolicy ( Qt::NoFocus );
    favButton->setFocusPolicy ( Qt::NoFocus );
    sound->setFocusPolicy ( Qt::NoFocus );
    cmdBox->setFocusPolicy ( Qt::NoFocus );
    geomBox->setFocusPolicy ( Qt::NoFocus );
    setMouseTracking(true);

    if (!editable)
    {
        editBut->hide();
        cmdBox->hide();
        geomBox->hide();
        sessMenu->hide();
        sound->setEnabled(false);
    }
    if (mw->brokerMode)
    {
        cmd->hide();
        cmdIcon->hide();
        server->hide();
        serverIcon->hide();
        geom->hide();
        geomIcon->hide();
        sound->hide();
        soundIcon->hide();
    }
}

SessionButton::~SessionButton()
{}

bool SessionButton::getFavFromConfig()
{
    X2goSettings st( "favorits" );
    return st.setting()->value("favorites").toStringList().contains(sid);
}

void SessionButton::slotFavClick()
{
    setFav(!fav);
    X2goSettings st( "favorits" );
    QStringList favs=st.setting()->value("favorites").toStringList();
    if(fav)
    {
        if(! favs.contains(sid))
        {
            favs.append(sid);
            st.setting()->setValue("favorites",favs);
        }
    }
    else
    {
        if(favs.contains(sid))
        {
            favs.removeAll(sid);
            st.setting()->setValue("favorites",favs);
        }
    }
    st.setting()->sync();
    emit favClicked();
}

void SessionButton::setFav(bool fav)
{
    this->fav=fav;
    if(fav)
    {
        favButton->setIcon(QIcon(par->iconsPath ( "/16x16/fav1.png" )));
        favButton->setToolTip(tr("Remove from favorites"));
    }
    else
    {
        favButton->setIcon(QIcon(par->iconsPath ( "/16x16/fav.png" )));
        favButton->setToolTip(tr("Add to favorites"));
    }
}

void SessionButton::slotClicked()
{
    emit sessionSelected ( this );
}

void SessionButton::slotEdit()
{
// 	editBut->setFlat ( true );
    emit signal_edit ( this );
}

void SessionButton::slotRemove()
{
    emit ( signal_remove ( this ) );
}

void SessionButton::redraw()
{

    updated=true;
    bool snd;

    QStringList toolTip;

    X2goSettings *st;

    if (par->brokerMode)
        st=new X2goSettings(par->config.iniFile,QSettings::IniFormat);
    else
        st= new X2goSettings( "sessions" );

    QString name=st->setting()->value ( sid+"/name",
                                        ( QVariant ) tr ( "New Session" ) ).toString();


    QStringList tails=name.split("/",QString::SkipEmptyParts);
    if(tails.count()>0)
    {
        name=tails.last();
        tails.pop_back();
        path=tails.join("/");
        if(path.length())
        {
            toolTip<<path;
        }
    }

    QFontMetrics metr(sessName->font());
    nameofSession=name;

    QString elName=metr.elidedText(name, Qt::ElideRight, 250);
    sessName->setText (elName);
    sessName->setToolTip(nameofSession);

    sessStatus->setText("");
    QString status=st->setting()->value ( sid+"/status",
                                          ( QVariant ) QString::null ).toString();
    if (status == "R")
    {
        sessStatus->setText("("+tr("running")+")");
    }
    if (status == "S")
    {
        sessStatus->setText("("+tr("suspended")+")");
    }

    suspended=st->setting()->value ( sid+"/suspended",
                                         ( QVariant ) QString::null ).toUInt();

    running=st->setting()->value ( sid+"/running",
                                         ( QVariant ) QString::null ).toUInt();
    if(suspended || running)
    {
        QStringList parts;
        if(suspended)
        {
            parts<<tr("suspended")+" - "+QString::number(suspended);
        }
        if(running)
        {
            parts<<tr("running")+" - "+QString::number(running);
        }
        sessStatus->setText(parts.join(", "));
    }

    QString sessIcon = wrap_legacy_resource_URIs (st->setting()->value (sid+"/icon",
                       (QVariant) par->iconsPath("/128x128/x2gosession.png")
                                                                       ).toString ());
    sessIcon = expandHome(sessIcon);
    QPixmap* pix;

    if(sessIcon == ":/img/icons/128x128/x2gosession.png")
    {
        sessIcon=par->iconsPath("/128x128/x2gosession.png");
    }
//     x2goDebug << "Creating QPixmap with session icon: " << sessIcon.toLatin1 () << ".";
    if (!par->brokerMode || sessIcon == par->iconsPath("/128x128/x2gosession.png"))
        pix=new QPixmap( sessIcon );
    else
    {
        pix=new QPixmap;
        if(sessIcon.indexOf("file://")!=-1)
        {
            //load icon from file URL
            pix->load(sessIcon.replace("file://",""));
        }
        else
            pix->loadFromData(QByteArray::fromBase64(sessIcon.toLatin1()));
        if(pix->isNull())
        {
            //loading icon has failed, load default icon
            pix->load(par->iconsPath("/128x128/x2gosession.png"));
        }
    }
    if ( !par->retMiniMode() )
        icon->setPixmap ( pix->scaled ( 64,64,Qt::IgnoreAspectRatio,
                                        Qt::SmoothTransformation ) );
    else
        icon->setPixmap ( pix->scaled ( 48,48,Qt::IgnoreAspectRatio,
                                        Qt::SmoothTransformation ) );

    delete pix;
    QString sv=st->setting()->value ( sid+"/host", ( QVariant )
                                      QString::null ).toString();
    QString uname=st->setting()->value ( sid+"/user", ( QVariant )
                                         QString::null ).toString();
    server->setText ( uname+"@"+sv );

    QString command=st->setting()->value ( sid+"/command",
                                           ( QVariant )
                                           tr (
                                                   "KDE" ) ).
                    toString();
    rootless=st->setting()->value ( sid+"/rootless",
                                    false ).toBool();
    published=st->setting()->value ( sid+"/published",
                                     false ).toBool();

    toolTip<<uname+"@"+sv;

    cmdBox->clear();
    cmdBox->addItem ( "KDE" );
    cmdBox->addItem ( "GNOME" );
    cmdBox->addItem ( "LXDE" );
    cmdBox->addItem ("LXQt");
    cmdBox->addItem ( "XFCE" );
    cmdBox->addItem ( "MATE" );
    cmdBox->addItem ( "UNITY" );
    cmdBox->addItem ( "CINNAMON" );
    cmdBox->addItem ( "TRINITY" );
    cmdBox->addItem ( "OPENBOX" );
    cmdBox->addItem ( "ICEWM" );
    cmdBox->addItem ( tr ( "RDP connection" ) );
    cmdBox->addItem ( tr ( "XDMCP" ) );
    cmdBox->addItem ( tr ( "Connection to local desktop" ) );
    cmdBox->addItem ( tr ( "Published applications" ) );

    cmdBox->addItems ( par->transApplicationsNames() );

    bool directRDP=false;
    QPixmap cmdpix;
    if ( command=="KDE" )
    {
        cmdpix.load ( par->iconsPath ( "/16x16/kde.png" ) );
        cmdBox->setCurrentIndex ( KDE );
    }
    else if ( command =="GNOME" )
    {
        cmdpix.load ( par->iconsPath ( "/16x16/gnome.png" ) );
        cmdBox->setCurrentIndex ( GNOME );
    }
    else if ( command =="UNITY" )
    {
        cmdpix.load ( par->iconsPath ( "/16x16/unity.png" ) );
        cmdBox->setCurrentIndex ( UNITY );
    }
    else if ( command == "XFCE" )
    {
        cmdpix.load ( par->iconsPath ( "/16x16/xfce.png" ) );
        cmdBox->setCurrentIndex ( XFCE );
    }
    else if ( command == "MATE" )
    {
        cmdpix.load ( par->iconsPath ( "/16x16/mate.png" ) );
        cmdBox->setCurrentIndex ( MATE );
    }
    else if ( command =="LXDE" )
    {
        cmdpix.load ( par->iconsPath ( "/16x16/lxde.png" ) );
        cmdBox->setCurrentIndex ( LXDE );
    }
    else if (command == "LXQt")
    {
        cmdpix.load (par->iconsPath ("/16x16/lxqt.png"));
        cmdBox->setCurrentIndex (LXQt);
    }
    else if ( command == "CINNAMON" )
    {
        cmdpix.load ( par->iconsPath ( "/16x16/cinnamon.png" ) );
        cmdBox->setCurrentIndex ( CINNAMON );
    }
    else if ( command == "TRINITY" )
    {
        cmdpix.load ( par->iconsPath ( "/16x16/trinity.png" ) );
        cmdBox->setCurrentIndex ( TRINITY );
    }
    else if ( command == "OPENBOX" )
    {
        cmdpix.load ( par->iconsPath ( "/16x16/openbox.png" ) );
        cmdBox->setCurrentIndex ( OPENBOX );
    }
    else if ( command == "ICEWM" )
    {
        cmdpix.load ( par->iconsPath ( "/16x16/icewm.png" ) );
        cmdBox->setCurrentIndex ( ICEWM );
    }
    else if ( command =="SHADOW" )
    {
        cmdpix.load ( par->iconsPath ( "/16x16/X.png" ) );
        cmdBox->setCurrentIndex ( SHADOW );
        command=tr ( "Connection to local desktop" );
    }
    else if ( command =="RDP" )
    {
#ifdef Q_OS_LINUX
        if (st->setting()->value ( sid+"/directrdp",
                                   ( QVariant ) false ).toBool())
            directRDP=true;
#endif
        cmdpix.load ( par->iconsPath ( "/16x16/rdp.png" ) );
        cmdBox->setCurrentIndex ( RDP );
        command=tr ( "RDP connection" );
    }
    else if ( command =="XDMCP" )
    {
#ifdef Q_OS_LINUX
        if (st->setting()->value ( sid+"/directxdmcp",
                                   ( QVariant ) false ).toBool()) {
            directRDP=true;
            server->setText ( "XDM@"+sv );
        }
#endif
        cmdpix.load ( par->iconsPath ( "/16x16/X.png" ) );
        cmdBox->setCurrentIndex ( XDMCP );
        command=tr ( "XDMCP" );
    }
    else if (published)
    {
        cmdpix.load ( par->iconsPath ( "/16x16/X.png" ) );
        command=tr ("Published applications");
        cmdBox->setCurrentIndex (PUBLISHED);
    }
    else
    {
        cmdpix.load ( par->iconsPath ( "/16x16/X.png" ) );
        command=par->transAppName ( command );
        int id= cmdBox->findText ( command );
        if ( id ==-1 )
        {
            cmdBox->addItem ( command );
            cmdBox->setCurrentIndex ( cmdBox->count()-1 );
        }
        else
            cmdBox->setCurrentIndex ( id );
    }



    cmdIcon->setPixmap ( cmdpix );
    cmd->setText ( command );

    toolTip<<command;

    geomBox->clear();
    geomBox->addItem ( tr ( "fullscreen" ) );
    uint displays=QApplication::desktop()->numScreens();
    if (!directRDP)
        for (uint i=0; i<displays; ++i)
        {
            geomBox->addItem ( tr( "Display " )+QString::number(i+1));

            //add maximun available area
            geomBox->addItem( QString::number(QApplication::desktop()->availableGeometry(i).width()) + "x" + QString::number(QApplication::desktop()->availableGeometry(i).height()));


        }
#ifndef Q_WS_HILDON


    geomBox->addItem ( "1440x900" );
    geomBox->addItem ( "1280x1024" );
    geomBox->addItem ( "1024x768" );
    geomBox->addItem ( "800x600" );
#else
    geomBox->addItem ( tr ( "window" ) );
#endif
    if ( st->setting()->value ( sid+"/fullscreen",
                                ( QVariant ) false ).toBool() )
    {
        geom->setText ( tr ( "fullscreen" ) );
    }
    else if (st->setting()->value ( sid+"/multidisp",
                                    ( QVariant ) false ).toBool() && !directRDP)
    {
        uint disp=st->setting()->value ( sid+"/display",
                                         ( QVariant ) 1 ).toUInt();
        if (disp<=displays)
        {
            geom->setText( tr( "Display " )+QString::number(disp));
        }
        else
        {
            geom->setText( tr( "Display " )+QString::number(1));
        }
        for (int i=0; i<geomBox->count(); ++i)
            if (geomBox->itemText(i)==geom->text())
            {
                geomBox->setCurrentIndex(i);
                break;
            }
    }
    else
    {
#ifndef	Q_WS_HILDON
        QString g=QString::number ( st->setting()->value (
                                        sid+"/width" ).toInt() );
        g+="x"+QString::number ( st->setting()->value (
                                     sid+"/height" ).toInt() );
        geom->setText ( g );
        if ( geomBox->findText ( g ) ==-1 )
            geomBox->addItem ( g );
        geomBox->setCurrentIndex ( geomBox->findText ( g ) );
#else
        geom->setText ( tr ( "window" ) );
        geomBox->setCurrentIndex ( 1 );
#endif
    }

    if (directRDP)
    {
        geomBox->addItem ( tr("Maximum") );
        if (st->setting()->value ( sid+"/maxdim",
                                   ( QVariant ) false ).toBool())
        {
            geom->setText ( tr("Maximum") );
            geomBox->setCurrentIndex ( geomBox->findText ( tr("Maximum") ));
        }
    }


    snd=st->setting()->value ( sid+"/sound", ( QVariant ) true ).toBool();
    if(directRDP)
        snd=false;
    if ( snd )
        sound->setText ( tr ( "Enabled" ) );
    else
        sound->setText ( tr ( "Disabled" ) );
    soundIcon->setEnabled ( snd );
    QFontMetrics fm ( sound->font() );
    sound->setFixedSize ( fm.size ( Qt::TextSingleLine,sound->text() ) +
                          QSize ( 8,4 ) );

    sessName->setMinimumSize ( sessName->sizeHint() );
    geom->setMinimumSize ( geom->sizeHint() );
    cmd->setMinimumSize ( cmd->sizeHint() );
    server->setMinimumSize ( server->sizeHint() );
    delete st;
    setFav(getFavFromConfig());
    setToolTip(toolTip.join("\n"));
}

void SessionButton::mousePressEvent ( QMouseEvent * event )
{
    SVGFrame::mousePressEvent ( event );
    loadBg ( par->images_resource_path("/svg/sessionbut_grey.svg" ));
}

void SessionButton::mouseReleaseEvent ( QMouseEvent * event )
{
    SVGFrame::mouseReleaseEvent ( event );
    loadBg ( par->images_resource_path("/svg/sessionbut.svg" ));

    QPoint click_pos (event->x (), event->y ());

    QRect sensitive_area (0, 0, width (), height ());

    bool recognized_click = false;

    /*
     * If button is editable, let only the top "half"
     * of the button be clickable to trigger an event.
     *
     * Actually, the area will look something like that:
     * ----------------------------
     * |                          |
     * |    -----------------------
     * |    |
     * |    |
     * |    |
     * ------
     *
     */
    if (editable) {
        bool mini_mode = par->retMiniMode ();

        QRect sensitive_area2;

        if (!mini_mode) {
          sensitive_area.setHeight (84);
          sensitive_area2 = QRect (0, 83, 58, height () - 84);
        }
        else {
          sensitive_area.setHeight (44);
          sensitive_area2 = QRect (0, 43, 66, height () - 44);
        }

        QPolygon real_sensitive_area;
        real_sensitive_area << sensitive_area.topLeft () << sensitive_area.topRight ()
                            << sensitive_area.bottomRight () << sensitive_area2.topRight ()
                            << sensitive_area2.bottomRight () << sensitive_area2.bottomLeft ()
                            << sensitive_area.topLeft ();

        recognized_click = real_sensitive_area.containsPoint (click_pos, Qt::WindingFill);
    }
    else {
        recognized_click = sensitive_area.contains (click_pos);
    }

    if (recognized_click) {
        emit clicked ();
    }
}

void SessionButton::mouseMoveEvent ( QMouseEvent * event )
{

    SVGFrame::mouseMoveEvent ( event );
    if ( favButton->isFlat() )
    {
        if ( event->x() > favButton->x() && event->x() < favButton->x() +
            favButton->width() &&
            event->y() >favButton->y() && event->y() <favButton->y() +
            favButton->height() )
            favButton->setFlat ( false );
    }
    else
    {
        if ( event->x() < favButton->x() || event->x() > favButton->x() +
            favButton->width() ||
            event->y() <favButton->y() || event->y() >favButton->y() +
            favButton->height() )
            favButton->setFlat ( true );
    }

    if (!editable)
        return;
    if ( cmd->isVisible() )
        if ( event->x() > cmd->x() && event->x() < cmd->x() +
                cmd->width() &&
                event->y() >cmd->y() && event->y() <cmd->y() +
                cmd->height() )
        {
            if ( cmdBox->width() <cmd->width() )
                cmdBox->setFixedWidth ( cmd->width() +20 );
            if ( cmdBox->height() !=cmd->height() )
                cmdBox->setFixedHeight ( cmd->height() );
            cmd->hide();
            cmdBox->show();
        }
    if ( cmdBox->isVisible() )
        if ( event->x() < cmdBox->x() || event->x() > cmdBox->x() +
                cmdBox->width() ||
                event->y() <cmdBox->y() || event->y() >cmdBox->y() +
                cmdBox->height() )
        {
            cmdBox->hide();
            cmd->show();
        }


    if ( sound->isFlat() )
    {
        if ( event->x() > sound->x() && event->x() < sound->x() +
                sound->width() &&
                event->y() >sound->y() && event->y() <sound->y() +
                sound->height() )
        {
            sound->setFlat ( false );
        }
    }
    else
    {
        if ( event->x() < sound->x() || event->x() > sound->x() +
                sound->width() ||
                event->y() <sound->y() || event->y() >sound->y() +
                sound->height() )
        {
            sound->setFlat ( true );
        }
    }


    if ( editBut->isFlat() )
    {
        if ( event->x() > editBut->x() && event->x() < editBut->x() +
                editBut->width() &&
                event->y() >editBut->y() && event->y() <editBut->y() +
                editBut->height() )
            editBut->setFlat ( false );
    }
    else
    {
        if ( event->x() < editBut->x() || event->x() > editBut->x() +
                editBut->width() ||
                event->y() <editBut->y() || event->y() >editBut->y() +
                editBut->height() )
            editBut->setFlat ( true );
    }

    if ( geom->isVisible() )
        if ( event->x() > geom->x() && event->x() < geom->x() +
                geom->width() &&
                event->y() >geom->y() && event->y() <geom->y() +
                geom->height() )
        {
            if ( geomBox->width() <geom->width() )
                geomBox->setFixedWidth ( geom->width() +20 );
            if ( geomBox->height() !=geom->height() )
                geomBox->setFixedHeight ( geom->height() );
            geom->hide();
            geomBox->show();
        }
    if ( geomBox->isVisible() )
        if ( event->x() < geomBox->x() || event->x() > geomBox->x() +
                geomBox->width() ||
                event->y() <geomBox->y() || event->y() >geomBox->y() +
                geomBox->height() )
        {
            geomBox->hide();
            geom->show();
        }
}


void SessionButton::slot_soundClicked()
{
    bool snd=!soundIcon->isEnabled();
    soundIcon->setEnabled ( snd );
    if ( snd )
        sound->setText ( tr ( "Enabled" ) );
    else
        sound->setText ( tr ( "Disabled" ) );
    QFontMetrics fm ( sound->font() );
    sound->setFixedSize ( fm.size ( Qt::TextSingleLine,sound->text() ) +
                          QSize ( 8,4 ) );

    X2goSettings st ( "sessions" );
    st.setting()->setValue ( sid+"/sound", ( QVariant ) snd );
    st.setting()->sync();


}

void SessionButton::slot_cmd_change ( const QString& command )
{
    cmd->setText ( command );
    QPixmap pix;
    bool newRootless=rootless;
    published=false;
    QString cmd=command;
    if ( command=="KDE" )
    {
        newRootless=false;
        pix.load ( par->iconsPath ( "/16x16/kde.png" ) );
    }
    else if ( command =="GNOME" )
    {
        newRootless=false;
        pix.load ( par->iconsPath ( "/16x16/gnome.png" ) );
    }
    else if ( command =="LXDE" )
    {
        newRootless=false;
        pix.load ( par->iconsPath ( "/16x16/lxde.png" ) );
    }
    else if (command == "LXQt")
    {
        newRootless = false;
        pix.load (par->iconsPath ("/16x16/lxqt.png"));
    }
    else if ( command =="UNITY" )
    {
        newRootless=false;
        pix.load ( par->iconsPath ( "/16x16/unity.png" ) );
    }
    else if ( command == "XFCE" )
    {
        newRootless=false;
        pix.load ( par->iconsPath ( "/16x16/xfce.png" ) );
    }
    else if ( command == "MATE" )
    {
        newRootless=false;
        pix.load ( par->iconsPath ( "/16x16/mate.png" ) );
    }
    else if ( command == "CINNAMON" )
    {
        newRootless=false;
        // As of 2014-10-01, Cinnamon does not have a logo. This icon is the
        // gear, which is the default start menu icon as of 2.2.
        pix.load ( par->iconsPath ( "/16x16/cinnamon.png" ) );
    }
    else if ( command == "TRINITY" )
    {
        newRootless=false;
        pix.load ( par->iconsPath ( "/16x16/trinity.png" ) );
    }
    else if ( command == "OPENBOX" )
    {
        newRootless=false;
        pix.load ( par->iconsPath ( "/16x16/openbox.png" ) );
    }
    else if ( command == "ICEWM" )
    {
        newRootless=false;
        pix.load ( par->iconsPath ( "/16x16/icewm.png" ) );
    }
    else if ( command ==tr ( "Connection to local desktop" ) )
    {
        newRootless=false;
        pix.load ( par->iconsPath ( "/16x16/X.png" ) );
        cmd="SHADOW";
    }
    else if ( command == tr ( "RDP connection" ) )
    {
        newRootless=false;
        pix.load ( par->iconsPath ( "/16x16/rdp.png" ) );
        cmd="RDP";
    }
    else if ( command == tr ( "XDMCP" ) )
    {
        newRootless=false;
        pix.load ( par->iconsPath ( "/16x16/X.png" ) );
        cmd="XDMCP";
    }
    else
        pix.load ( par->iconsPath ( "/16x16/X.png" ) );
    cmdIcon->setPixmap ( pix );

    X2goSettings st ( "sessions" );
    if ( command=="startkde" )
    {
        cmd="KDE";
        newRootless=false;
    }
    if ( command=="gnome-session" )
    {
        cmd="GNOME";
        newRootless=false;
    }
    if ( command=="LXDE" )
    {
        cmd="LXDE";
        newRootless=false;
    }
    if (command == "LXQt")
    {
        cmd = "LXQt";
        newRootless = false;
    }
    if ( command=="unity" )
    {
        cmd="UNITY";
        newRootless=false;
    }
    if ( command=="xfce4-session" )
    {
        cmd="XFCE";
        newRootless=false;
    }
    if ( command=="mate-session" )
    {
        cmd="MATE";
        newRootless=false;
    }
    if ( command=="cinnamon-session" )
    {
        cmd="CINNAMON";
        newRootless=false;
    }
    if (command== tr("Published applications"))
    {
        published=true;
        cmd="PUBLISHED";
    }
    bool found=false;
    cmd=par->internAppName ( cmd,&found );
    if ( found )
        newRootless=true;
    st.setting()->setValue ( sid+"/command", ( QVariant ) cmd );
    st.setting()->setValue ( sid+"/rootless", ( QVariant ) newRootless );
    st.setting()->setValue ( sid+"/published", ( QVariant ) published );
    st.setting()->sync();

}


void SessionButton::slot_geom_change ( const QString& new_g )
{
    geom->setText ( new_g );
    X2goSettings st ( "sessions" );
    if ( new_g==tr ( "fullscreen" ) )
    {
        st.setting()->setValue ( sid+"/fullscreen", ( QVariant ) true );
        st.setting()->setValue ( sid+"/multidisp", ( QVariant ) false );
        st.setting()->setValue ( sid+"/maxdim", ( QVariant ) false );
    }
    else if ( new_g==tr ( "Maximum" ))
    {
        st.setting()->setValue ( sid+"/fullscreen", ( QVariant ) false );
        st.setting()->setValue ( sid+"/multidisp", ( QVariant ) false );
        st.setting()->setValue ( sid+"/maxdim", ( QVariant ) true );
    }
    else if (new_g.indexOf(tr("Display "))==0)
    {
        QString g= new_g;
        g.replace(tr("Display "),"");
        st.setting()->setValue ( sid+"/multidisp", ( QVariant ) true );
        st.setting()->setValue ( sid+"/display", ( QVariant ) g.toUInt());
        st.setting()->setValue ( sid+"/fullscreen", ( QVariant ) false );
        st.setting()->setValue ( sid+"/maxdim", ( QVariant ) false );
    }
    else
    {
        QString new_geom=new_g;
#ifdef Q_WS_HILDON
        new_geom="800x600";
#endif
        st.setting()->setValue ( sid+"/fullscreen", ( QVariant ) false );
        st.setting()->setValue ( sid+"/multidisp", ( QVariant ) false );
        st.setting()->setValue ( sid+"/maxdim", ( QVariant ) false );
        QStringList lst=new_geom.split ( 'x' );
        st.setting()->setValue ( sid+"/width", ( QVariant ) lst[0] );
        st.setting()->setValue ( sid+"/height", ( QVariant ) lst[1] );
    }
    st.setting()->sync();
}

bool SessionButton::lessThen ( const SessionButton* b1,
                               const SessionButton* b2 )
{
    return (b1->path+"/"+b1->sessName->text()).toLower().localeAwareCompare (
               (b2->path+"/"+b2->sessName->text()).toLower() ) <0;
}

QString SessionButton::name()
{
    return nameofSession;
}

void SessionButton::slotMenuHide()
{
    editBut->setDown ( false );
    editBut->setFlat ( true );
}


void SessionButton::slotShowMenu()
{
    sessMenu->popup ( mapToGlobal ( QPoint ( editBut->x() +editBut->width(),
                                    editBut->y() +editBut->height() ) ) );
}

void SessionButton::showAdvInfo(bool show)
{
    pathLabel->setText(path);
    pathLabel->setVisible((show && path.length()));
}


void SessionButton::slotCreateSessionIcon()
{
    par->getSessionExplorer()->slotCreateDesktopIcon ( this );
}
