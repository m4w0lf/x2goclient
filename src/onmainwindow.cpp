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

#include "onmainwindow_privat.h"
#include "help.h"
#include "compat.h"

#include <QStyleFactory>

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
#include <QLoggingCategory>
#endif


void x2goSession::operator = ( const x2goSession& s )
{
    agentPid=s.agentPid;
    clientIp=s.clientIp;
    cookie=s.cookie;
    crTime=s.crTime;
    display=s.display;
    grPort=s.grPort;
    server=s.server;
    sessionId=s.sessionId;
    sndPort=s.sndPort;
    fsPort=s.fsPort;
    status=s.status;
    sessionType=s.sessionType;
    command=s.command;
}

bool	ONMainWindow::portable=false;
QString ONMainWindow::homeDir;
QString ONMainWindow::sessionCfg;

#ifdef Q_OS_WIN
QString ONMainWindow::u3Device;
#endif

bool ONMainWindow::debugging=false;
bool ONMainWindow::libssh_debugging = false;
bool ONMainWindow::libssh_packetlog = false;

ONMainWindow::ONMainWindow ( QWidget *parent ) :QMainWindow ( parent )
{
    haveTerminal=false;
#ifndef Q_OS_WIN
    QFile fl("/dev/tty");
    if(fl.open( QIODevice::ReadOnly))
    {
        haveTerminal=true;
        fl.close();
    }
#endif

#ifdef Q_OS_LINUX
    image=shape=0;
#endif
    x2goInfof(1) << tr("Starting X2Go Client %1...").arg (QString (VERSION));
    debugging = false;
    libssh_debugging = false;
    libssh_packetlog = false;

    resourceTmpDir=0;

    setFocusPolicy ( Qt::NoFocus );
    installTranslator();
    ignoreBrokerSessions=false;
    autoresume=true;
    cleanAllFiles=false;
    drawMenu=true;
    usePGPCard=false;
    PGPInited=false;
    extLogin=false;
    startMaximized=false;
    startHidden=false;
    keepTrayIcon=false;
    hideFolderSharing=false;
    brokerNoauthWithSessionUsername=false;
    brokerCredsForSession=false;
    thinMode=false;
    closeDisconnect=false;
    showHaltBtn=false;
    showBrokerLogoutBtn=false;
    defaultUseSound=true;
    defaultSetKbd=true;
    extStarted=false;
    cmdAutologin=false;
    defaultLink=2;
    defaultFullscreen=false;
    defaultXinerama=false;
    acceptRsa=false;
    cardStarted=false;
    cardReady=false;
    shadowSession=false;
    proxyRunning=false;
// 	useSshAgent=false;
    closeEventSent=false;
    miniMode=false;
    embedMode=false;
    proxyWinEmbedded=false;
    proxyWinId=0;
    embedParent=embedChild=0l;
    defaultSession=false;
    connTest=false;
    defaultUser=false;
    defaultWidth=800;
    defaultHeight=600;
    defaultPack="16m-jpeg";
    defaultQuality=9;
    defaultLayout<<tr ( "us" );
    defaultClipboardMode="both";
    defaultKbdType="auto";
    defaultCmd="KDE";
    defaultSshPort=sshPort=clientSshPort="22";
    LDAPPrintSupport=false;
    managedMode=false;
    brokerMode=false;
    startEmbedded=false;
    sshConnection=0;
    sessionStatusDlg=0;
    noSessionEdit=false;
    changeBrokerPass=false;
    resumeAfterSuspending=false;
    forceToShowTrayicon=false;
    splash=0;

    /* Initialize at least these variables before they get filled via loadSettings()
     * They have to be initialized as they are used in closeEvent() and closeClient()...
     */
    trayIcon=NULL;
    useLdap=false;
    trayNoclose=false;

    appSeparator=0;
    config.brokerNoAuth=false;
    config.brokerAutologin=false;
    config.brokerKrbLogin=false;
    config.brokerAutologoff=false;
    config.brokerAuthenticated = false;
    config.published=false;
    cmdAutologin=false;
#if defined (Q_OS_DARWIN) || defined (Q_OS_WIN)
    systemDisablePARecord=false;
    systemDisablePA=false;
#endif /* defined (Q_OS_DARWIN) || defined (Q_OS_WIN) */

#ifdef Q_OS_LINUX
    directRDP = false;
#endif /* defined (Q_OS_LINUX) */

// Try to determine the native DPI and use it for the default
    int dpix = QApplication::desktop()->physicalDpiX();
    int dpiy = QApplication::desktop()->physicalDpiY();
    if ( dpix >0 && dpiy >0) {
        defaultSetDPI=true;
        defaultDPI=(dpix+dpiy)/2;
    } else {
        defaultSetDPI=false;
        defaultDPI=96;
    }

#ifdef Q_OS_WIN
    clientSshPort="7022";
    X2goSettings st ( "settings" );
    winSshdStarted=false;
#else
    sshd=0l;
#endif

    appDir=QApplication::applicationDirPath();

    homeDir=QDir::homePath();

#ifdef Q_OS_WIN
    xorg=0l;
    xDisplay=0;
    x_start_tries_ = 0;
    x_start_limit_ = 3;
#endif

    if(X2goSettings::centralSettings())
    {
        x2goInfof(0)<<"using central configuration";
        noSessionEdit=true;
    }

    cleanAskPass();
    setWindowTitle ( tr ( "X2Go Client" ) );
    ld=0;
    tunnel=0l;
    sndTunnel=0l;
    fsTunnel=0l;
    nxproxy=0l;
    soundServer=0l;
    scDaemon=0l;
    statusLabel=0;
    gpg=0l;
    bBrokerLogout = NULL;
    restartResume=false;
    isPassShown=true;
    readExportsFrom=QString();
    spoolTimer=0l;
    brokerAliveTimer=0l;
    brokerSyncTimer=0l;
#ifdef Q_OS_DARWIN
    modMapTimer = NULL;
    kbMap = QString ();
#endif /* defined(Q_OS_DARWIN) */
    ldapOnly=false;
    embedControlChanged=false;
    resourcesLoaded=true;
    statusString=tr ( "connecting" );

#if defined (Q_OS_DARWIN) || defined (Q_OS_WIN)
    pulseManager = NULL;
    pulseManagerThread = NULL;
#endif /* defined (Q_OS_DARWIN) || defined (Q_OS_WIN) */


    connectionType=DEFAULT;

    hide();
    kdeIconsPath=getKdeIconsPath();

    addToAppNames ( "WWWBROWSER",tr ( "Internet browser" ) );
    addToAppNames ( "MAILCLIENT",tr ( "Email client" ) );
    addToAppNames ( "OFFICE",tr ( "OpenOffice.org" ) );
    addToAppNames ( "TERMINAL",tr ( "Terminal" ) );

#ifndef Q_OS_LINUX
#if QT_VERSION < 0x050000
    widgetExtraStyle = new QPlastiqueStyle ();
#else
    widgetExtraStyle = QStyleFactory::create ("fusion");
#endif
#endif

#ifdef CFGCLIENT
    QStringList args;
    if(!X2goSettings::centralSettings())
        args=QCoreApplication::arguments();
    else
    {
        X2goSettings st ( "settings" );
        QString cmdLine=st.setting()->value ( "commandline", "" ).toString();
//         x2goErrorf(100)<<"cmd line:"<<cmdLine;
        args=cmdLine.split(";",Qt::SkipEmptyParts);
        args.push_front(QCoreApplication::arguments()[0]);
    }

    for ( int i=1; i<args.size(); ++i )
    {
        if ( !parseParameter ( args[i] ) )
        {
            close();
            exit ( -1 );
        }
    }
#endif

    if(resourcesLoaded)
        initUI();
}


ONMainWindow::~ONMainWindow()
{
    x2goDebug<<"Destroying X2Go Client's main window.";
    if ( !closeEventSent )
        closeClient();
    x2goDebug<<"Finished destructor hooks for X2Go Client's main window.";
}

void ONMainWindow::initSplash()
{
    if(!startHidden)
        return;
    if(splashPix.length()<1)
        return;
    QPixmap px(splashPix);
    if(px.isNull())
    {
        x2goDebug<<"Can't load "<<splashPix<<" not showing splash screen";
        return;
    }
    splash=new QSplashScreen(px,  Qt::WindowStaysOnTopHint);
    splash->show();
}

void ONMainWindow::destroySplash()
{
    if(splash)
    {
        splash->close();
        delete splash;
        splash=0;
    }
}


void ONMainWindow::initUI()
{
//set homedir as portable,etc

#ifdef Q_OS_WIN
    QString u3Path=u3DataPath();
//we have U3 System
    if ( u3Path.length() >0 )
    {
        portableDataPath=u3Path;
        ONMainWindow::portable=true;
        setWindowTitle ( "X2Go Client - U3" );
    }
#endif

    initSplash();

    if ( ONMainWindow::portable )
    {
        if ( portableDataPath.length() <=0 )
            portableDataPath=QDir::currentPath();

        homeDir=portableDataPath;

        x2goInfof(2)<<tr("Starting X2Go Client in portable mode. Data directory is: ")<<portableDataPath;
        QTimer *timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(slotCheckPortableDir()));
        timer->start(1000);
    }

    loadSettings();
    trayIconActiveConnectionMenu = NULL;
    trayIconMenu=NULL;
    trayAutoHidden=false;

    trayEnabled=trayMinToTray=trayMinCon=trayMaxDiscon=false;

    trayIconInit();

    if(brokerMode)
        statusBar()->show();


    if ( embedMode )
    {
        miniMode=false;
        useLdap=false;
    }

    if ( readExportsFrom!=QString() )
    {
        exportTimer=new QTimer ( this );
        connect ( exportTimer,SIGNAL ( timeout() ),this,
                  SLOT ( slotExportTimer() ) );
    }
    if ( extLogin )
    {
        extTimer=new QTimer ( this );
        extTimer->start ( 2000 );
        connect ( extTimer,SIGNAL ( timeout() ),this,
                  SLOT ( slotExtTimer() ) );
    }

    if ( startMaximized )
    {
        QTimer::singleShot ( 10, this, SLOT ( slotResize() ) );
    }


    QDesktopWidget wd;

    if ( wd.screenGeometry(wd.screenNumber(this)).width() <1024 ||
            wd.screenGeometry(wd.screenNumber(this)).height() <768 )
    {
        miniMode=true;
        x2goDebug<<"Switching to \"mini\" mode...";
    }


    if ( usePGPCard  && !useLdap)
    {
        QTimer::singleShot ( 10, this, SLOT ( slotStartPGPAuth() ) );
    }


    //fr=new SVGFrame(QString(),true,this);
    fr=new IMGFrame ( ( QImage* ) 0l,this );
    setCentralWidget ( fr );



    // See if BGFile is a directory and retrieve an SVG file randomly from within
    QFileInfo bg_dir(BGFile);
    if (bg_dir.isDir())
    {
        QDirIterator it(BGFile, QStringList() << "*.svg", QDir::Files, QDirIterator::NoIteratorFlags);
        QStringList fileList;
        while (it.hasNext())
            fileList << it.next();

        qsrand(QTime::currentTime().msec());
        int max=fileList.size();
        int random_selector=qrand() % max;
        BGFile=fileList.at(random_selector);
    }

    if (BGFile.size())
        bgFrame=new SVGFrame ( ( QString ) BGFile,true,fr );
    else
    {
        if(QFile::exists(images_resource_path("/svg/bg.png")))
        {
            bgFrame=new SVGFrame ( ( QString ) images_resource_path("/svg/bg.png"),true,fr );
        }
        else
        {
            bgFrame=new SVGFrame ( ( QString ) images_resource_path("/svg/bg.svg"),true,fr );
        }
    }
    //bgFrame=new SVGFrame((QString)"/home/admin/test.svg",false,fr);

    QString x2gologopath=images_resource_path("/svg/x2gologo.svg");

    SVGFrame* x2g=new SVGFrame ( ( QString ) x2gologopath,
                                 false,fr );

    QPalette pl=x2g->palette();
    pl.setColor ( QPalette::Base, QColor ( 255,255,255,0 ) );
    pl.setColor ( QPalette::Window, QColor ( 255,255,255,0 ) );
    x2g->setPalette ( pl );

    QString onlogopath;

    if (OnFile.size())
    {
        onlogopath=OnFile;
    }
    else
    {
        onlogopath=images_resource_path ("/svg/onlogo.svg");
    }


    on=new SVGFrame ( onlogopath,false,fr );
    on->setPalette ( pl );

    if ( !miniMode )
    {
        QImage im(onlogopath);
        if(im.width()&&im.height())
        {
            on->setFixedSize ( 100*im.width()/im.height(),100 );
        }

        QImage im1(x2gologopath);
        if(im1.width()&&im1.height())
        {
            x2g->setFixedSize ( 100*im1.width()/im1.height(),100 );
        }
    }
    else
    {
        x2g->setFixedSize ( 50,50 );
        on->setFixedSize ( 50,50 );
    }

    mainL=new QHBoxLayout ( fr );
    QVBoxLayout* onlay=new QVBoxLayout();
    onlay->addStretch();
    onlay->addWidget ( on );

    QVBoxLayout* x2golay=new QVBoxLayout();
    x2golay->addStretch();
    x2golay->addWidget ( x2g );


    bgLay=new QHBoxLayout ( bgFrame );
    bgLay->setSpacing ( 0 );
    bgLay->setMargin ( 0 );
    bgLay->addLayout ( onlay );
    bgLay->addStretch();
    username=new QHBoxLayout();
    bgLay->addLayout ( username );
    if ( embedMode )
        bgLay->addStretch();
    bgLay->addLayout ( x2golay );



    act_set=new QAction (
        QIcon ( iconsPath ( "/32x32/edit_settings.png" ) ),
        tr ( "&Settings ..." ),this );

    if (supportMenuFile!=QString())
    {
        act_support=new QAction ( tr ( "Support ..." ),this );
        connect ( act_support,SIGNAL ( triggered ( bool ) ),this,
                  SLOT ( slotSupport() ) );

    }

    act_abclient=new QAction ( QIcon ( iconsPath("/32x32/x2goclient.png" )),
                               tr ( "About X2Go Client" ),this );





    connect ( act_set,SIGNAL ( triggered ( bool ) ),this,
              SLOT ( slotConfig() ) );
    connect ( act_abclient,SIGNAL ( triggered ( bool ) ),this,
              SLOT ( slotAbout() ) );


    sessionExplorer=new SessionExplorer(this);


#ifdef Q_OS_DARWIN
    embedMode=false;
#endif


#if defined (Q_OS_WIN) //&& defined (CFGCLIENT )
    xorgSettings();
#endif


#ifdef Q_OS_WIN
    winServersReady=false;
    saveCygnusSettings();
#endif
    initPassDlg();
    initStatusDlg();
    interDlg=new InteractionDialog(this, bgFrame);
    connect(interDlg, SIGNAL(closeInterractionDialog()), this, SLOT(slotCloseInteractionDialog()));
    username->addWidget ( interDlg );

    if ( !embedMode )
    {
        initWidgetsNormal();
    }

#ifdef Q_OS_WIN
    QProcess::startDetached("x2gohelper.exe "+QString::number(GetCurrentProcessId()));
    QTimer::singleShot ( 500, this, SLOT ( startWinServers() ) );
#endif

    /* FIXME: add Linux. */
#if defined (Q_OS_DARWIN) || defined (Q_OS_WIN)
    QTimer::singleShot (500, this, SLOT (pulseManagerWrapper ()));
#endif /* defined (Q_OS_DARWIN) || defined (Q_OS_WIN) */

    mainL->setSpacing ( 0 );
    mainL->setMargin ( 0 );
    mainL->insertWidget ( 0, bgFrame );
    hide();
    QTimer::singleShot ( 1, this, SLOT ( slotResize() ) );
    connect ( fr,SIGNAL ( resized ( const QSize ) ),this,
              SLOT ( slotResize ( const QSize ) ) );
    slotResize ( fr->size() );



#ifdef Q_OS_LINUX
    if (thinMode)
    {
        QTimer *timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(slotSyncX()));
        timer->start(200);
    }
#endif
    if (showHaltBtn)
    {
        QPushButton* bHalt=new QPushButton(bgFrame);
        QPixmap p(images_resource_path("/png/power-button.png"));
        bHalt->setIcon(p);
        bHalt->setFocusPolicy(Qt::NoFocus);
        bHalt->setFixedSize(32,32);
        bHalt->move(10,10);
        bHalt->show();
        connect(bHalt,SIGNAL(clicked()),this, SLOT(slotShutdownThinClient()));
    }


    bBrokerLogout = new QPushButton (bgFrame);
    QPixmap p (images_resource_path("/png/broker-logout-button.png"));
    bBrokerLogout->setIcon (p);
    bBrokerLogout->setFocusPolicy (Qt::NoFocus);
    bBrokerLogout->setFixedSize (32,32);

    if (showHaltBtn)
    {
        bBrokerLogout->move (40,10);
    }
    else
    {
        bBrokerLogout->move (10,10);
    }

    bBrokerLogout->setEnabled (false);

    if (showBrokerLogoutBtn)
    {
        bBrokerLogout->show ();
    }
    else
    {
        bBrokerLogout->hide ();
    }

    connect (bBrokerLogout, SIGNAL (clicked ()), this, SLOT (slotBrokerLogoutButton ()));


    if (brokerMode)
    {
        broker=new HttpBrokerClient ( this, &config );
        connect ( broker,SIGNAL ( fatalHttpError() ),this,
                  SLOT ( close() ) );
        connect ( broker, SIGNAL ( authFailed()), this ,SLOT ( slotGetBrokerAuth()));
        connect ( broker, SIGNAL( sessionsLoaded()), this, SLOT (slotReadSessions()));
        connect ( broker, SIGNAL ( sessionSelected()), this, SLOT (slotGetBrokerSession()));
        connect ( broker, SIGNAL ( passwordChanged(QString)), this, SLOT ( slotPassChanged(QString)));
        connect (broker, SIGNAL (enableBrokerLogoutButton ()), this, SLOT (slotEnableBrokerLogoutButton ()));
        brokerAliveTimer=new QTimer(this);
        connect ( brokerAliveTimer, SIGNAL ( timeout() ), this,
                  SLOT ( slotSendBrokerAlive() ) );
        brokerSyncTimer=new QTimer(this);
        connect ( brokerSyncTimer, SIGNAL( timeout()), broker , SLOT(getUserSessions()));
    }

    initSelectSessDlg();

    proxyWinTimer=new QTimer ( this );
    connect ( proxyWinTimer, SIGNAL ( timeout() ), this,
              SLOT ( slotFindProxyWin() ) );

    xineramaTimer=new QTimer (this);

    connect( xineramaTimer, SIGNAL(timeout()), this, SLOT(slotConfigXinerama()));

    x2goInfof(3)<<tr("Started X2Go Client.");
    x2goDebug<<"$HOME=" + homeDir.toLatin1();
    if (thinMode)
    {
        x2goDebug<<"Thin Mode is active.";
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        resize(QGuiApplication::primaryScreen()->size());
#else
        showMaximized();
#endif
        activateWindow();
        raise();
    }

    QTimer::singleShot (200, this, SLOT (slotInitLibssh ()));

}


void ONMainWindow::slotShutdownThinClient()
{
    QFile file(QDir::homePath()+"/.halt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);
    out << "\n";
    file.close();
}

void ONMainWindow::slotEnableBrokerLogoutButton () {
  bBrokerLogout->setEnabled (true);
}

void ONMainWindow::slotBrokerLogoutButton () {
  /*
   * Log out of the broker if currently logged in, otherwise
   * does nothing.
   */
  if (config.brokerAuthenticated) {
    x2goDebug << "Logging off from broker via logout button.";
    QTimer::singleShot (1, this, SLOT (slotGetBrokerAuth ()));
  }
}


void ONMainWindow::slotSendBrokerAlive()
{
    sendEventToBroker(ALIVE);
}


void ONMainWindow::slotSyncX()
{
    if (proxyRunning)
    {
        if (!isHidden())
            hide();
#ifdef Q_OS_LINUX
        XSync(QX11Info::display(),false);
#endif
    }
    else
    {
        if (isHidden())
        {
            showMaximized();
            activateWindow();
            raise();
        }
    }
}


bool ONMainWindow::get_translator (QString file_name_start, QTranslator **translator) {
    QTranslator *tmp_translator = new QTranslator ();

    /* Qt 4.8.0 introduced a new overload for QTranslator::load(), taking a QLocale
     * object and loading up the UI language.
     * Additionally, a lower-cased version is automatically added to the search
     * list on case-sensitive file systems.
     * We still need to iterate over the UI languages list in case an English
     * locale is in there. As we do not ship a nop-English translation, loading
     * an English translation will always fail and the next language in the list
     * be preferred.
     * We also still need the original "compat" version for Qt < 4.8.0.
     */

    QString filename = file_name_start;
    QStringList ui_languages;
#if QT_VERSION < 0x040800
    filename = QString (filename + "_%1" ).arg (QLocale::system ().name ());
    filename = filename.toLower ();
#else /* QT_VERSION < 0x040800 */
    ui_languages = QLocale::system ().uiLanguages ();
#endif /* QT_VERSION < 0x040800 */

#if QT_VERSION < 0x040800
    if (tmp_translator->load (filename)) {
        *translator = tmp_translator;
        x2goInfof (4) << tr ("Translator: ") + filename.toLatin1 () + tr (" found.");
        return (true);
    }
    else {
        x2goWarningf (1) << tr ("Can't load translator: ") + filename.toLatin1 ();
        return (false);
    }
#else /* QT_VERSION < 0x040800 */
    QString load_filename = "";
    bool translator_found = false;
    for (QStringList::const_iterator it = ui_languages.constBegin (); it != ui_languages.constEnd (); ++it) {
        /* Respect English locales. Don't try to load any translation, because we do not ship nop-English translations. */
        if ((*it).startsWith ("en")) {
            x2goWarningf (1) << "English language requested, not loading translator.";
            break;
        }
        else {
            /*
             * QLocale::uiLanguages() may return an unexpected format.
             * See: https://bugreports.qt.io/browse/QTBUG-25973
             */
            QString tmp_locale = (*it);
            tmp_locale.replace ("-", "_");
            load_filename = filename;
            load_filename.append ("_").append (tmp_locale.toLower ());

            if (tmp_translator->load (load_filename)) {
                /* Some translation successfully loaded. That's good enough. */
                x2goInfof (4) << tr ("Translator: ") + load_filename.toLatin1 () + tr (" found.");
                translator_found = true;
                *translator = tmp_translator;
                break;
            }
            else {
                x2goWarningf (1) << tr ("Non-fatal: can't load translator: ") + load_filename.toLatin1 ();
                x2goWarningf (1) << tr ("Trying to load language with lower preference, if existent.");
            }
        }
    }

    return (translator_found);
#endif /* QT_VERSION < 0x040800 */
}

void ONMainWindow::installTranslator () {
    QTranslator *x2goclientTranslator = new QTranslator ();

    bool translator_found = get_translator (QString (":/i18n/x2goclient"), &x2goclientTranslator);

    if (translator_found) {
        QCoreApplication::installTranslator (x2goclientTranslator);
    }

    QTranslator *qtTranslator = new QTranslator ();

    translator_found = get_translator (QString (":/i18n/qt"), &qtTranslator);

    if (translator_found) {
        QCoreApplication::installTranslator (qtTranslator);
    }
}


void ONMainWindow::initWidgetsEmbed()
{
}

void ONMainWindow::initWidgetsNormal()
{
    username->setSpacing ( 10 );
    username->addStretch();
    username->addStretch();
    ln=new SVGFrame ( ( QString ) images_resource_path("/svg/line.svg"),true,fr );
    ln->setFixedWidth ( ln->sizeHint().width() );
    uname=new QLineEdit ( bgFrame );
    setWidgetStyle ( uname );

    uname->hide();
    uname->setFrame ( false );
    u=new QLabel ( tr ( "Session:" ),bgFrame );
    u->hide();
    QFont fnt=u->font();
    fnt.setPointSize ( 16 );
    if ( miniMode )
    {
        fnt.setPointSize ( 12 );
    }

    u->setFont ( fnt );

    connect ( uname,SIGNAL ( returnPressed() ),this,
              SLOT ( slotUnameEntered() ) );

    QPalette pal=u->palette();
    pal.setColor ( QPalette::WindowText,
                   QColor ( 200,200,200,255 ) );
    u->setPalette ( pal );
    uname->setFont ( fnt );
    pal=uname->palette();
    pal.setColor ( QPalette::Base, QColor ( 255,255,255,0 ) );
    pal.setColor ( QPalette::Text, QColor ( 200,200,200,255 ) );
    uname->setPalette ( pal );

    u->show();
    uname->show();


    QVBoxLayout* vblay=new QVBoxLayout();


    users=new QScrollArea ( fr );
    vblay->addLayout(sessionExplorer->getSelectLayout());
    vblay->addLayout(sessionExplorer->getNavigationLayout());
    vblay->addWidget(users);


    pal=users->verticalScrollBar()->palette();
    pal.setBrush ( QPalette::Window, QColor ( 110,112,127,255 ) );
    pal.setBrush ( QPalette::Base, QColor ( 110,112,127,255 ) );
    pal.setBrush ( QPalette::Button, QColor ( 110,112,127,255 ) );
    users->verticalScrollBar()->setPalette ( pal );
    users->setFrameStyle ( QFrame::Plain );
    users->setFocusPolicy ( Qt::NoFocus );


    pal=users->palette();
    pal.setBrush ( QPalette::Window, QColor ( 110,112,127,255 ) );
    users->setPalette ( pal );
    users->setWidgetResizable ( true );

    uframe=new QFrame();
    users->setWidget ( uframe );

    mainL->insertWidget ( 1, ln );
//     mainL->addWidget ( users );
    mainL->addLayout(vblay);

    QAction *act_exit=new QAction (
        QIcon ( iconsPath ( "/32x32/exit.png" ) ),
        tr ( "&Quit" ),this );
    act_exit->setShortcut ( tr ( "Ctrl+Q" ) );
    act_exit->setStatusTip ( tr ( "Quit" ) );

    act_new=new QAction ( QIcon ( iconsPath ( "/32x32/new_file.png" ) ),
                          tr ( "&New session ..." ),this );
    act_new->setShortcut ( tr ( "Ctrl+N" ) );


    setWindowIcon ( QIcon ( iconsPath("/128x128/x2go.png" )) );
    act_edit=new QAction ( QIcon ( iconsPath ( "/32x32/edit.png" ) ),
                           tr ( "Session management ..." ),this );
    act_edit->setShortcut ( tr ( "Ctrl+E" ) );

    if (noSessionEdit)
    {
        act_edit->setEnabled(false);
        act_new->setEnabled(false);
    }

    act_sessicon=new QAction (
        QIcon ( iconsPath ( "/32x32/create_file.png" ) ),
        tr ( "&Create session icon on desktop ..." ),
        this );
    if (brokerMode)
        act_sessicon->setEnabled(false);

    if (changeBrokerPass)
    {
        act_changeBrokerPass=new QAction (
            QIcon ( iconsPath ( "/32x32/auth.png" ) ),
            tr ( "&Set broker password ..." ),
            this );
        connect ( act_changeBrokerPass,SIGNAL ( triggered(bool)),this,
                  SLOT ( slotChangeBrokerPass()) );
        act_changeBrokerPass->setEnabled(false);
    }

    if (connTest)
    {
        act_testCon=new QAction (
            QIcon ( iconsPath ( "/32x32/contest.png" ) ),
            tr ( "&Connectivity test ..." ),
            this );
        connect ( act_testCon,SIGNAL ( triggered(bool)),this,
                  SLOT ( slotTestConnection()) );
    }


    QAction *act_tb=new QAction ( tr ( "Show toolbar" ),this );
    act_tb->setCheckable ( true );
    act_tb->setChecked ( showToolBar );



    QAction *act_abqt=new QAction ( tr ( "About Qt" ),this );

    connect ( act_abqt,SIGNAL ( triggered ( bool ) ),this,
              SLOT ( slotAboutQt() ) );
    connect ( act_new,SIGNAL ( triggered ( bool ) ),this,
              SLOT ( slotNewSession() ) );
    connect ( act_sessicon,SIGNAL ( triggered ( bool ) ),this,
              SLOT ( slotCreateSessionIcon() ) );
    connect ( act_edit,SIGNAL ( triggered ( bool ) ),this,
              SLOT ( slotManage() ) );
    connect ( act_exit,SIGNAL ( triggered ( bool ) ),this,
              SLOT ( trayQuit()) ) ;
    connect ( act_tb,SIGNAL ( toggled ( bool ) ),this,
              SLOT ( displayToolBar ( bool ) ) );

    stb=addToolBar ( tr ( "Show toolbar" ) );

    if ( drawMenu )
    {
        QMenu* menu_sess=menuBar()->addMenu ( tr ( "&Session" ) );
        QMenu* menu_opts=0;
        if(!X2goSettings::centralSettings())
            menu_opts=menuBar()->addMenu ( tr ( "&Options" ) );
        if (!brokerMode && !X2goSettings::centralSettings())
        {
            menu_sess->addAction ( act_new );
            menu_sess->addAction ( act_edit );
#if (!defined Q_OS_DARWIN)
            if ( !portable )
                menu_sess->addAction ( act_sessicon );
#endif
            menu_sess->addSeparator();
        }
        menu_sess->addAction ( act_exit );
        if(!X2goSettings::centralSettings())
        {
            menu_opts->addAction ( act_set );
            menu_opts->addAction ( act_tb );
            if (changeBrokerPass)
                menu_opts->addAction(act_changeBrokerPass);
            if (connTest)
                menu_opts->addAction(act_testCon);
        }

        QMenu* menu_help=menuBar()->addMenu ( tr ( "&Help" ) );
        if (supportMenuFile!=QString())
            menu_help->addAction ( act_support );
        menu_help->addAction ( act_abclient );
        menu_help->addAction ( act_abqt );

        if (!brokerMode)
        {
            stb->addAction ( act_new );
            stb->addAction ( act_edit );
#if (!defined Q_OS_DARWIN)
            if ( !portable )
                stb->addAction ( act_sessicon );
#endif
            stb->addSeparator();
        }
        stb->addAction ( act_set );
        if (changeBrokerPass)
            stb->addAction(act_changeBrokerPass);
        if (connTest)
            stb->addAction(act_testCon);

        if ( !showToolBar  || X2goSettings::centralSettings())
            stb->hide();
        connect ( act_tb,SIGNAL ( toggled ( bool ) ),stb,
                  SLOT ( setVisible ( bool ) ) );
    }
    else
        stb->hide();

#if !defined USELDAP

    useLdap=false;
#endif

    if ( useLdap )
    {
        act_new->setEnabled ( false );
        act_edit->setEnabled ( false );
        u->setText ( tr ( "Login:" ) );
        QTimer::singleShot ( 1500, this, SLOT ( readUsers() ) );
    }
    else
    {
        if (!brokerMode)
            QTimer::singleShot ( 1, this, SLOT ( slotReadSessions() ) );
        else
        {
            bgLay->insertStretch(3);
            QTimer::singleShot(1, this,SLOT(slotGetBrokerAuth()));
        }
    }
}



void ONMainWindow::slotPassChanged(const QString& result)
{

    if (result==QString())
    {
        QMessageBox::critical(this, tr("Error"),tr("Operation failed"));
    }
    else
    {
        QMessageBox::information(this, tr("Password changed"),tr("Password changed"));
        config.brokerPass=result;
    }
    setEnabled(true);

    slotClosePass();
    sessionStatusDlg->hide();

}

void ONMainWindow::slotTestConnection()
{
    ConTest test( broker, config.brokerurl, this);
    test.exec();
}

void ONMainWindow::slotChangeBrokerPass()
{
    x2goDebug<<"Changing broker password.";

    BrokerPassDlg passDlg;
    if (passDlg.exec()!=QDialog::Accepted)
        return;
    if (passDlg.oldPass()!=config.brokerPass)
    {
        QMessageBox::critical(this,tr("Error"),tr("Wrong password!"));
        return;
    }
    broker->changePassword(passDlg.newPass());
    setStatStatus ( tr ( "Connecting to broker" ) );
    stInfo->insertPlainText ( "broker url: "+config.brokerurl );
    setEnabled ( false );
    uname->hide();
    u->hide();
    return;
}


void ONMainWindow::slotCheckPortableDir()
{
    if (!QFile::exists(homeDir))
    {
        x2goDebug<<"Portable directory does not exist, closing.";
        close();
    }
}

void ONMainWindow::slotGetBrokerAuth()
{
    pass->clear();
    login->clear();
    QString pixFile=iconsPath("/128x128/x2gosession.png");
    if (SPixFile!=QString())
        pixFile=SPixFile;
    QPixmap pix(pixFile);
    if ( !miniMode )
    {
        fotoLabel->setPixmap (
            pix.scaled ( 64,64,
                         Qt::IgnoreAspectRatio,
                         Qt::SmoothTransformation ) );
        fotoLabel->setFixedSize ( 64,64 );
    }
    else
    {
        fotoLabel->setPixmap (
            pix.scaled ( 48,48,
                         Qt::IgnoreAspectRatio,
                         Qt::SmoothTransformation ) );
        fotoLabel->setFixedSize ( 48,48 );
    }

    users->hide();
    ln->hide();

    QString text=tr("<b>Authentication</b>");
    /* if(config.brokerName.length()>0)
       text+=config.brokerName;
     else
       text+=config.brokerurl;*/
    nameLabel->setText ( text );
    slotShowPassForm();
    config.brokerAuthenticated = false;
    bBrokerLogout->setEnabled (false);

    statusBar()->showMessage(config.brokerurl);
    if(config.brokerUser.length()>0)
    {
        login->setText(config.brokerUser);
        pass->setFocus();
    }
    if(config.brokerNoAuth)
        slotSessEnter();
    else if(config.brokerurl.indexOf("ssh://")==0 && (config.brokerAutologin || config.brokerKrbLogin|| config.brokerSshKey.length()>0))
        slotSessEnter();
}

void ONMainWindow::setBrokerStatus(const QString& text, bool error)
{
    QPalette p=statusBar()->palette();
    if(error)
        p.setColor(QPalette::WindowText, QColor(204,54,54));
    else
        p.setColor(QPalette::WindowText, QColor(22,103,39));
    statusBar()->setPalette(p);
    statusBar()->showMessage(text);
    if(splash)
    {
        splash->showMessage(text, Qt::AlignLeft|Qt::AlignBottom, Qt::white);
    }
}


void ONMainWindow::trayIconInit()
{
    X2goSettings st ( "settings" );
    trayEnabled=st.setting()->value ( "trayicon/enabled", false ).toBool();
    trayMinCon=st.setting()->value ( "trayicon/mincon", false ).toBool();
    trayMaxDiscon=st.setting()->value ( "trayicon/maxdiscon", false ).toBool();
    trayNoclose=st.setting()->value ( "trayicon/noclose", false ).toBool();
    trayMinToTray=st.setting()->value ( "trayicon/mintotray", false ).toBool();



    if (!trayEnabled)
    {
        trayMinCon=trayMaxDiscon=trayNoclose=trayMinToTray=false;
        if (trayIcon)
        {
            delete trayIcon;
            delete trayIconMenu;
            trayIcon=0l;
            trayIconMenu=0l;
        }
    }
    else
    {
        if (!trayIcon)
        {
            trayIconMenu = new QMenu(this);
            trayIconMenu->addAction(tr("Restore"),this, SLOT(showNormal()));

            trayIconActiveConnectionMenu = trayIconMenu->addMenu(tr("Not connected"));

            appMenu[Application::MULTIMEDIA]=initTrayAppMenu(tr("Multimedia"),
                                             QPixmap(iconsPath("/22x22/applications-multimedia.png")));
            appMenu[Application::DEVELOPMENT]=initTrayAppMenu(tr("Development"),
                                                              QPixmap(iconsPath("/22x22/applications-development.png")));
            appMenu[Application::EDUCATION]=initTrayAppMenu(tr("Education"),
                                                            QPixmap(iconsPath("/22x22/applications-education.png")));
            appMenu[Application::GAME]=initTrayAppMenu(tr("Game"),
                                                       QPixmap(iconsPath("/22x22/applications-games.png")));
            appMenu[Application::GRAPHICS]=initTrayAppMenu(tr("Graphics"),
                                                           QPixmap(iconsPath("/22x22/applications-graphics.png")));
            appMenu[Application::NETWORK]=initTrayAppMenu(tr("Network"),
                                                          QPixmap(iconsPath("/22x22/applications-internet.png")));
            appMenu[Application::OFFICE]=initTrayAppMenu(tr("Office"),
                                                         QPixmap(iconsPath("/22x22/applications-office.png")));
            appMenu[Application::SETTINGS]=initTrayAppMenu(tr("Settings"),
                                                           QPixmap(iconsPath("/22x22/preferences-system.png")));
            appMenu[Application::SYSTEM]=initTrayAppMenu(tr("System"),
                                                         QPixmap(iconsPath("/22x22/applications-system.png")));
            appMenu[Application::UTILITY]=initTrayAppMenu(tr("Utility"),
                                                          QPixmap(iconsPath("/22x22/applications-utilities.png")));
            appMenu[Application::OTHER]=initTrayAppMenu(tr("Other"),
                                                        QPixmap(iconsPath("/22x22/applications-other.png")));
            appSeparator=trayIconActiveConnectionMenu->addSeparator();


            if (!hideFolderSharing)
                trayIconActiveConnectionMenu->addAction(tr ("Share folder ..." ),this, SLOT(slotExportDirectory()));
            trayIconActiveConnectionMenu->addAction(tr("Suspend"),this, SLOT(slotSuspendSessFromSt()));
            trayIconActiveConnectionMenu->addAction(tr("Terminate"),this, SLOT(slotTermSessFromSt()));
            connect (trayIconActiveConnectionMenu, SIGNAL(triggered(QAction*)), this,
                     SLOT(slotAppMenuTriggered(QAction*)));


            if (sessionStatusDlg && sessionStatusDlg->isVisible())
            {
                if (!useLdap) {
                    if (!(sessionExplorer->getLastSession())) {
                        x2goDebug << "No session selected, not setting tray icon title.";
                    }
                    else {
                        trayIconActiveConnectionMenu->setTitle(sessionExplorer->getLastSession()->name());
                    }
                }
                else
                    trayIconActiveConnectionMenu->setTitle(lastUser->username());
            }
            else
            {
                trayIconActiveConnectionMenu->setEnabled(false);
            }
            trayIconMenu->addSeparator();
            trayIconMenu->addAction(tr("Quit"),this, SLOT(trayQuit()));

            // setup the tray icon itself
            trayIcon = new QSystemTrayIcon(this);

#ifndef Q_OS_DARWIN
            connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                    this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
#endif
            connect(trayIcon, SIGNAL(messageClicked()), this, SLOT(trayMessageClicked()));

            trayIcon->setContextMenu(trayIconMenu);
            trayIcon->setIcon(QIcon ( iconsPath("/128x128/x2go.png")) );
#ifdef Q_OS_DARWIN
            trayIcon->setToolTip(tr("A click displays the context menu."));
#else
            trayIcon->setToolTip(tr("A left click hides or restores the window. A right click displays the context menu."));
#endif
        }

        if (!startHidden | forceToShowTrayicon)
        {
            trayIcon->show();
            plugAppsInTray();
        }
    }
}

QMenu* ONMainWindow::initTrayAppMenu(QString text, QPixmap icon)
{
    QMenu* menu=trayIconActiveConnectionMenu->addMenu(text);
    if (!keepTrayIcon) {
        menu->setIcon(icon);
    }
    return menu;
}


void ONMainWindow::slotAppMenuTriggered(QAction* action)
{
    x2goDebug<<"slotAppMenuTriggered: "<<action->data().toString();

    if (action->data().toString() != "")
        runApplication(action->data().toString());
}

void ONMainWindow::plugAppsInTray()
{
    if (!trayIcon)
        return;
    removeAppsFromTray();

    x2goDebug<<"Plugging apps in tray.";

    bool empty=true;
    topActions.clear();
    foreach(Application app, applications)
    {
        QAction* act;
        if (app.category==Application::TOP)
        {
            act=new QAction(app.icon,app.name,trayIconActiveConnectionMenu);
            trayIconActiveConnectionMenu->insertAction(appSeparator, act);
            topActions.append(act);
        }
        else
        {
            act=appMenu[app.category]->addAction(app.icon,app.name);
            appMenu[app.category]->menuAction()->setVisible(true);
        }
        act->setToolTip(app.comment);
        act->setData(app.exec);
        empty=false;
    }
    if (!empty)
        appSeparator->setVisible(true);
}


void ONMainWindow::removeAppsFromTray()
{
    if (!trayIcon)
        return;

    x2goDebug<<"Removing apps from tray";

    for (int i=0; i<=Application::OTHER; ++i)
    {
        appMenu[i]->clear();
        appMenu[i]->menuAction()->setVisible(false);
    }
    foreach (QAction* act, topActions)
    {
        trayIconActiveConnectionMenu->removeAction(act);
        delete act;
    }
    topActions.clear();
    appSeparator->setVisible(false);
}


QString ONMainWindow::findTheme ( QString /*theme*/ )
{
    return QString();
}

QString ONMainWindow::getKdeIconsPath()
{
    return ":/img/icons";
}

void ONMainWindow::slotResize ( const QSize sz )
{
    if ( startHidden )
    {
        return;
    }
    int height;
    int usize;
    height=sz.height();
    if ( !embedMode )
    {
        if ( !miniMode )
        {
            usize=sz.width()-800;
            if ( usize<360 )
                usize=360;
            if ( usize>500 )
                usize=500;
        }
        else
        {
            usize=285;
        }

        if ( users->width() !=usize )
        {
            users->setFixedWidth ( usize );
            if ( useLdap )
            {
                QList<UserButton*>::iterator it;
                QList<UserButton*>::iterator end=names.end();
                for ( it=names.begin(); it!=end; it++ )
                {
                    if ( !miniMode )
                        ( *it )->move (
                            ( usize-360 ) /2,
                            ( *it )->pos().y() );
                    else
                        ( *it )->move (
                            ( usize-250 ) /2,
                            ( *it )->pos().y() );
                }
            }
            else
            {
                QList<FolderButton*>::iterator fit;
                QList<FolderButton*>::iterator fend=sessionExplorer->getFoldersList()->end();
                for ( fit=sessionExplorer->getFoldersList()->begin(); fit!=fend; fit++ )
                {
                    if ( !miniMode )
                        ( *fit )->move (
                            ( usize-360 ) /2,
                            ( *fit )->pos().y() );
                    else
                        ( *fit )->move (
                            ( usize-250 ) /2,
                            ( *fit )->pos().y() );
                }
                QList<SessionButton*>::iterator it;
                QList<SessionButton*>::iterator end=sessionExplorer->getSessionsList()->end();
                for ( it=sessionExplorer->getSessionsList()->begin(); it!=end; it++ )
                {
                    if ( !miniMode )
                        ( *it )->move (
                            ( usize-360 ) /2,
                            ( *it )->pos().y() );
                    else
                        ( *it )->move (
                            ( usize-250 ) /2,
                            ( *it )->pos().y() );
                }
            }
        }
        u->setFixedWidth ( u->sizeHint().width() );

        int bwidth=bgFrame->width();
        int upos= ( bwidth-u->width() ) /2;
        if ( upos<0 )
            upos=0;
        int rwidth=bwidth- ( upos+u->width() +5 );
        if ( rwidth<0 )
            rwidth=1;
        uname->setMinimumWidth ( rwidth );
        u->move ( upos,height/2 );
        uname->move ( u->pos().x() +u->width() +5,u->pos().y() );
        sessionExplorer->resize();
    }
}

void ONMainWindow::closeClient()
{
    destroySplash();
    x2goInfof(6)<<tr("Closing X2Go Client ...");
    if(trayIcon)
        trayIcon->hide();
    closeEventSent=true;
    if ( !startMaximized && !startHidden && !embedMode )
    {
        x2goDebug<<"Saving settings ...";
        X2goSettings st ( "sizes" );

        st.setting()->setValue ( "mainwindow/size",
                                 QVariant ( size() ) );
        st.setting()->setValue ( "mainwindow/pos",QVariant ( pos() ) );
        st.setting()->setValue ( "mainwindow/maximized",
                                 QVariant ( isMaximized() ) );
        st.setting()->sync();
        x2goDebug<<"Saved settings.";
#ifdef Q_OS_LINUX
        if (image)
            XFreePixmap(QX11Info::display(),image);
        if (shape)
            XFreePixmap(QX11Info::display(),shape);
#endif
    }
    if ( nxproxy!=0l )
    {
        if ( nxproxy->state() ==QProcess::Running )
        {
            x2goDebug<<"Terminating proxy ...";
            nxproxy->terminate();
            x2goDebug<<"Terminated proxy.";
        }
        x2goDebug<<"Deleting proxy ...";
        delete nxproxy;
        x2goDebug<<"Deleted proxy.";
    }
    if ( sshConnection && !useLdap)
    {
        x2goDebug<<"Waiting for the SSH connection to finish ...";
        delete sshConnection;
        x2goDebug<<"SSH connection finished.";
        sshConnection=0;
    }
    if (useLdap)
    {
        for (int i=0; i<serverSshConnections.count(); ++i)
        {
            if (serverSshConnections[i])
            {
                x2goDebug<<"Waiting for the SSH connection to finish ...";
                delete serverSshConnections[i];
                x2goDebug<<"SSH connection finished.";
            }
        }
        serverSshConnections.clear();
    }

    if ( soundServer )
    {
        x2goDebug<<"Deleting the sound server ...";
        delete soundServer;
        x2goDebug<<"Deleted the sound server.";
    }
#ifdef Q_OS_WIN
    if ( xorg )
    {
        x2goDebug<<"Terminating the X.Org Server ...";
        xorg->terminate();
        x2goDebug<<"Terminated the X.Org Server.";
        x2goDebug<<"Deleting the X.Org Server ...";
        delete xorg;
        x2goDebug<<"Deleted the X.Org Server.";
    }

    if ( winSshdStarted )
    {
        TerminateProcess ( sshd.hProcess,0 );
        CloseHandle ( sshd.hProcess );
        CloseHandle ( sshd.hThread );
    }
#endif /* defined (Q_OS_WIN) */

#if defined (Q_OS_DARWIN) || defined (Q_OS_WIN)
    if (pulseManager) {
        disconnect(pulseManager, SIGNAL(sig_pulse_user_warning(bool, const QString&, const QString&, bool)),
                   this, SLOT(slotShowPAMSGDialog(bool, const QString&, const QString&, bool)));
        delete (pulseManager);

        if (pulseManagerThread) {

            pulseManagerThread->quit ();
            pulseManagerThread->wait ();
        }

        delete (pulseManagerThread);
    }
#endif /* defined (Q_OS_DARWIN) || defined (Q_OS_WIN) */

#ifndef Q_OS_WIN
    if (sshd) {
        x2goDebug << "Terminating the OpenSSH server ...";
        sshd->terminate ();

        /* Allow sshd a grace time of 5 seconds to terminate. */
        QTime sleep_time = QTime::currentTime ().addSecs (5);
        bool killed = false;
        while (QTime::currentTime () < sleep_time) {
            if (QProcess::NotRunning == sshd->state ()) {
                killed = true;
                break;
            }

            QCoreApplication::processEvents (QEventLoop::AllEvents, 100);
        }

        if (!killed) {
            /* Grace period over, force termination. */
            sshd->kill ();

            if (!(sshd->waitForFinished (500))) {
                x2goWarningf (8) << "OpenSSH Server failed to terminate in time "
                                    " and the kill command timed out.";
            }
        }

        x2goDebug << "Terminated the OpenSSH server.";
        delete sshd;
    }
#endif /* !defined (Q_OS_WIN) */

    if ( embedMode )
    {
        passForm->close();
        selectSessionDlg->close();
#ifndef Q_OS_DARWIN
// 		closeEmbedWidget();
#endif
    }

    if ( ONMainWindow::portable )
    {
#ifdef Q_OS_WIN
        if ( !cyEntry )
        {
            removeCygwinEntry();
        }
#endif
        cleanPortable();
    }

    if(resourceTmpDir)
    {
        removeDir(resourceTmpDir->path());
        delete resourceTmpDir;
    }

    if (ssh_finalize ()) {
        x2goDebug << "Unable to finalize libssh. Something may be wrong!";
    }
    else {
        x2goDebug << "libssh finalized.";
    }

    delete (bBrokerLogout);

    x2goInfof(7)<<tr("Finished X2Go Client closing hooks.");
}


void ONMainWindow::closeEvent ( QCloseEvent* event )
{
    x2goDebug<<"Close event received."<<Qt::endl;

    if (trayNoclose && !brokerMode)
    {
        hide();
        event->ignore();
    }
    else
    {
        trayQuit();
    }
}


void ONMainWindow::hideEvent(QHideEvent* event)
{
    QMainWindow::hideEvent(event);
    if (event->spontaneous() && trayMinToTray)
        hide();

}

void ONMainWindow::trayQuit()
{
    x2goDebug<<"Quitting from tray icon and closing application."<<Qt::endl;

    closeClient();
    qApp->quit();

}

void ONMainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason )
{
    switch (reason) {
// use single left click on unix
// and double click on windows (Is it standard behaviour conform?)
#ifdef Q_OS_UNIX
    case QSystemTrayIcon::Trigger:
#else
    case QSystemTrayIcon::DoubleClick:
#endif
        x2goDebug << "tray icon clicked with Trigger (left click)";
        if (isVisible())
            hide();
        else
        {
            showNormal();
        }
        break;
    default:
        break;
    }
}



void ONMainWindow::trayMessageClicked()
{

}

void ONMainWindow::loadSettings()
{

    X2goSettings st ( "sizes" );
    mwSize=st.setting()->value ( "mainwindow/size",
                                 ( QVariant ) QSize ( 800,600 ) ).toSize();
    mwPos=st.setting()->value ( "mainwindow/pos",
                                ( QVariant ) QPoint ( 20,20 ) ).toPoint();
    mwMax=st.setting()->value ( "mainwindow/maximized",
                                ( QVariant ) false ).toBool();

    /*
     * Sanity check for X2Go Client main window position.
     * Check whether the given position is within bounds of any connected display.
     * Otherwise use the default.
     */
    bool placement_found = false;
    QDesktopWidget *desktop_widget = QApplication::desktop ();
    QRect main_window_geom (mwPos, mwSize);
    for (size_t i = 0; i < static_cast<size_t> (desktop_widget->screenCount ()); ++i) {
        QRect tmp_geom = desktop_widget->availableGeometry (i);

        if (tmp_geom.intersects (main_window_geom)) {
            placement_found = true;
            break;
        }
    }

    if (!placement_found) {
        mwPos = QPoint (20, 20);
    }

    X2goSettings st1 ( "settings" );

    if ( !ldapOnly )
    {
        useLdap=st1.setting()->value ( "LDAP/useldap",
                                       ( QVariant ) false ).toBool();
        ldapServer=st1.setting()->value ( "LDAP/server",
                                          ( QVariant ) "localhost" ).toString();
        ldapPort=st1.setting()->value ( "LDAP/port",
                                        ( QVariant ) 389 ).toInt();
        ldapDn=st1.setting()->value ( "LDAP/basedn",
                                      ( QVariant ) QString() ).toString();
        ldapServer1=st1.setting()->value ( "LDAP/server1",
                                           ( QVariant ) QString() ).toString();
        ldapPort1=st1.setting()->value ( "LDAP/port1",
                                         ( QVariant ) 0 ).toInt();
        ldapServer2=st1.setting()->value ( "LDAP/server2",
                                           ( QVariant ) QString() ).toString();
        ldapPort2=st1.setting()->value ( "LDAP/port2",
                                         ( QVariant ) 0 ).toInt();
    }
    showToolBar=st1.setting()->value ( "toolbar/show",
                                       ( QVariant ) true ).toBool();

}

QString ONMainWindow::iconsPath ( const QString &fname ) const
{
    /*    QFile fl(this->kdeIconsPath+fname);
    	if(fl.exists())
    	return kdeIconsPath+fname;*/
    return (images_resource_path (fname, "/icons/"));
}

QString ONMainWindow::images_resource_path (const QString &filename, const QString &base) const {

    if(resourceDir.length()>0)
    {
        if(QFile::exists(resourceDir+"/img/"+base+"/"+filename))
            return resourceDir+"/img/"+base+"/"+filename;
//         x2goDebug<<"not exists: "<<resourceDir+"/img/"+base+"/"+filename;
    }

    QString ret = ":/img";

  /*
   * The base parameter is optional and might be empty.
   * In this case, it's completely skipped.
   * Otherwise, we want to make sure that the base parameter
   * is appended to the fixed start with a slash, but doesn't
   * end in a slash to avoid double-slashes.
   * We add a trailing slash if the file name doesn't start
   * with one.
   */
  if (!(base.isEmpty ())) {
    if (!(base.startsWith ('/'))) {
      ret.append ('/');
    }

    ret.append (base);

    if (ret.endsWith ('/')) {
      ret.chop (1);
    }
  }

  if (!(filename.startsWith ('/'))) {
    ret.append ('/');
  }

  ret.append (filename);

  return (ret);
}

void ONMainWindow::displayUsers()
{

    QPixmap pix;
    if ( !miniMode )
        pix=QPixmap ( images_resource_path("/png/ico.png" ));
    else
        pix=QPixmap ( images_resource_path("/png/ico_mini.png" ));
    QPixmap foto=QPixmap ( iconsPath ( "/64x64/personal.png" ) );

    QPalette pal=palette();
    pal.setBrush ( QPalette::Window, QBrush ( pix ) );
    pal.setBrush ( QPalette::Base, QBrush ( pix ) );
    pal.setBrush ( QPalette::Button, QBrush ( pix ) );
    QFont fnt=font();
    fnt.setPointSize ( 12 );
    uframe->setFont ( fnt );
    QList<user>::iterator it;
    QList<user>::iterator end=userList.end();
    int i=0;
    for ( it=userList.begin(); it!=end; it++ )
    {
        int val=i+1;
        UserButton* l;
        if ( ( *it ).foto.isNull() )
            l=new UserButton ( this, uframe, ( *it ).uid,
                               ( *it ).name,foto,pal );
        else
            l=new UserButton ( this, uframe, ( *it ).uid,
                               ( *it ).name, ( *it ).foto,pal );
        connect ( l,SIGNAL ( userSelected ( UserButton* ) ),this,
                  SLOT ( slotSelectedFromList ( UserButton* ) ) );
        if ( !miniMode )
            l->move ( ( users->width()-360 ) /2,
                      i*120+ ( val-1 ) *25+5 );
        else
            l->move ( ( users->width()-260 ) /2,
                      i*120+ ( val-1 ) *25+5 );
        l->show();
        names.append ( l );
        i++;
    }
    int val=i;
    uframe->setFixedHeight ( val*120+ ( val-1 ) *25 );
    uname->setText ( "" );
    disconnect ( uname,SIGNAL ( textEdited ( const QString& ) ),this,
                 SLOT ( slotSnameChanged ( const QString& ) ) );
    connect ( uname,SIGNAL ( textEdited ( const QString& ) ),this,
              SLOT ( slotUnameChanged ( const QString& ) ) );
    if ( usePGPCard  && !PGPInited)
    {
        PGPInited=true;

        x2goDebug<<"Users loaded, starting smart card daemon.";

        QTimer::singleShot ( 10, this, SLOT ( slotStartPGPAuth() ) );
    }
}

void ONMainWindow::showPass ( UserButton* user )
{
    QPalette pal=users->palette();
    setUsersEnabled ( false );
    QString fullName;
    QPixmap foto;
    if ( user )
    {
        foto=user->foto();
        nick=user->username();
        fullName=user->fullName();
        user->hide();
        lastUser=user;
    }
    else
    {
        lastUser=0;
        foto.load ( iconsPath ( "/64x64/personal.png" ) );
        foto=foto.scaled ( 100,100 );
        nick=uname->text();
        fullName="User Unknown";
    }

    fotoLabel->setPixmap ( foto );

    QString text="<b>"+nick+"</b><br>("+fullName+")";
    nameLabel->setText ( text );
    login->setText ( nick );
    login->hide();

    pass->setEchoMode ( QLineEdit::Password );
    pass->setFocus();
    slotShowPassForm();
}



void ONMainWindow::slotSelectedFromList ( UserButton* user )
{
    pass->setText ( "" );
    showPass ( user );
}


bool ONMainWindow::isPassFormHidden()
{
    return passForm->isHidden();
}

bool ONMainWindow::isSelectFormHidden()
{
    return selectSessionDlg->isHidden();
}

void ONMainWindow::slotClosePass()
{
    if (brokerMode)
    {
        if (!config.brokerAuthenticated)
        {
            x2goErrorf(15)<<tr("Broker authentication failed!");
            close();
        }
    }
    passForm->hide();
    if ( !embedMode )
    {
        u->show();
        uname->show();
        if ( useLdap )
        {
            if ( lastUser )
            {
                lastUser->show();
                uname->setText ( lastUser->username() );
            }
        }
        else
        {
            if (sessionExplorer->getLastSession())
            {
                sessionExplorer->getLastSession()->show();
                uname->setText ( sessionExplorer->getLastSession()->name() );
            }
            sessionExplorer->setLastSession(0);
        }
        uname->setEnabled ( true );
        u->setEnabled ( true );
        setUsersEnabled ( true );
        uname->selectAll();
        uname->setFocus();
    }
}


void ONMainWindow::slotPassEnter()
{

    if(!embedMode)
        shadowSession=false;
#if defined ( Q_OS_WIN ) || defined (Q_OS_DARWIN )
    QString disp=getXDisplay();
    if ( disp==QString() )
        return;
#endif
#ifdef USELDAP

    if ( ! initLdapSession() )
    {
        QMessageBox::critical ( 0l,tr ( "Error" ),
                                tr ( "Please check LDAP settings" ),
                                QMessageBox::Ok,QMessageBox::NoButton );

        slotConfig();
        return;
    }

    passForm->setEnabled ( false );

    x2goServers.clear();

    list<string> attr;
    attr.push_back ( "cn" );
    attr.push_back ( "serialNumber" );
    attr.push_back ( "l" );
    list<LDAPStringEntry> res;
    QString searchBase="ou=Servers,ou=ON,"+ldapDn;

    try
    {
        ld->stringSearch ( searchBase.toStdString(),attr,
                           "objectClass=ipHost",res );
    }
    catch ( LDAPExeption e )
    {
        QString message="Exception in: ";
        message=message+e.err_type.c_str();
        message=message+" : "+e.err_str.c_str();
        QMessageBox::critical ( 0l,tr ( "Error" ),message,
                                QMessageBox::Ok,QMessageBox::NoButton );
        slotConfig();
        return;
    }
    if ( res.size() ==0 )
    {
        QString message=tr ( "no X2Go Server found in LDAP " );
        QMessageBox::critical ( 0l,tr ( "Error" ),message,
                                QMessageBox::Ok,QMessageBox::NoButton );
        slotConfig();
        return;
    }


    list<LDAPStringEntry>::iterator it=res.begin();
    list<LDAPStringEntry>::iterator end=res.end();
    QString freeServer;
    QString firstServer;
    bool isFirstServerSet=false;
    for ( ; it!=end; ++it )
    {
        serv server;
        server.name=LDAPSession::getStringAttrValues (
                        *it,"cn" ).front().c_str();

        QString sPort="22";
        list<string> sL=LDAPSession::getStringAttrValues (
                            *it,"l" );
        if ( sL.size() >0 )
        {
            sPort=sL.front().c_str();
        }

        x2goDebug<<"SSH server: "<<server.name<<"; port: "<<sPort;

        QString sFactor="1";
        list<string> serialNumber=LDAPSession::getStringAttrValues (
                                      *it,"serialNumber" );
        if ( serialNumber.size() >0 )
        {
            sFactor=serialNumber.front().c_str();
        }

        x2goDebug<<"SSH server: "<<server.name<<"; factor: "<<sFactor;

        server.factor=sFactor.toFloat();
        server.sess=0;
        server.sshPort=sPort;
        server.connOk=true;
        if ( !isFirstServerSet )
        {
            isFirstServerSet=true;
            firstServer=server.name;
            sshPort=server.sshPort;
        }
        x2goServers.append ( server );
    }
    if ( ld )
    {
        delete ld;
        ld=0;
    }
    setEnabled ( false );
    passForm->setEnabled ( false );

    QString passwd;
    if ( !extLogin )
        currentKey=QString();
    QString user=getCurrentUname();
//      get x2gogetservers not from ldap server, but from first x2goserver
// 	QString host=ldapServer;
    QString host=firstServer;
    passwd=getCurrentPass();
    if (sshConnection)
        delete sshConnection;
    sshConnection=startSshConnection ( host,sshPort,acceptRsa,user,passwd,true, false );

#endif
}


void ONMainWindow::slotUnameChanged ( const QString& text )
{
    if ( prevText==text )
        return;
    if ( text=="" )
        return;
    QList<UserButton*>::iterator it;
    QList<UserButton*>::iterator endit=names.end();
    for ( it=names.begin(); it!=endit; it++ )
    {
        QString username= ( *it )->username();
        if ( username.indexOf ( text,0,Qt::CaseInsensitive ) ==0 )
        {
            QPoint pos= ( *it )->pos();
            uname->setText ( username );
            QScrollBar* bar=users->verticalScrollBar();
            int docLang=bar->maximum()-bar->minimum() +
                        bar->pageStep();
            double position= ( double ) ( pos.y() ) /
                             ( double ) ( uframe->height() );
            bar->setValue ( ( int ) ( docLang*position-height() /2+
                                      ( *it )->height() /2 ) );
            uname->setSelection ( username.length(),text.length()-
                                  username.length() );
            break;
        }
    }
    prevText=text;
}

void ONMainWindow::slotUnameEntered()
{
    QString text=uname->text();
    if ( useLdap )
    {
        UserButton* user=NULL;
        QList<UserButton*>::iterator it;
        QList<UserButton*>::iterator endit=names.end();
        for ( it=names.begin(); it!=endit; it++ )
        {
            QString username= ( *it )->username();
            if ( username==text )
            {
                user=*it;
                break;
            }
        }
        showPass ( user );
    }
    else
    {
        SessionButton* sess=NULL;
        QList<SessionButton*>::iterator it;
        QList<SessionButton*>::iterator endit=sessionExplorer->getSessionsList()->end();
        for ( it=sessionExplorer->getSessionsList()->begin(); it!=endit; it++ )
        {
            QString name= ( *it )->name();
            if ( name==text )
            {
                sess=*it;
                break;
            }
        }
        if ( sess )
            slotSelectedFromList ( sess );
    }
}


void ONMainWindow::readUsers()
{
#ifdef USELDAP
    if ( ! initLdapSession() )
    {
        QMessageBox::critical ( 0l,tr ( "Error" ),
                                tr ( "Please check LDAP settings" ),
                                QMessageBox::Ok,QMessageBox::NoButton );

        slotConfig();
        return;
    }


    list<string> attr;
    attr.push_back ( "uidNumber" );
    attr.push_back ( "uid" );
    attr.push_back ( "cn" );
    attr.push_back ( "jpegPhoto" );


    list<LDAPBinEntry> result;
    try
    {
        ld->binSearch ( ldapDn.toStdString(),attr,
                        "objectClass=posixAccount",result );
    }
    catch ( LDAPExeption e )
    {
        QString message="Exception in: ";
        message=message+e.err_type.c_str();
        message=message+" : "+e.err_str.c_str();
        QMessageBox::critical ( 0l,tr ( "Error" ),
                                message,QMessageBox::Ok,
                                QMessageBox::NoButton );
        QMessageBox::critical ( 0l,tr ( "Error" ),
                                tr ( "Please check LDAP settings" ),
                                QMessageBox::Ok,QMessageBox::NoButton );
        slotConfig();
        return;
    }

    list<LDAPBinEntry>::iterator it=result.begin();
    list<LDAPBinEntry>::iterator end=result.end();

    for ( ; it!=end; ++it )
    {
        user u;
        QString uin=LDAPSession::getBinAttrValues (
                        *it,"uidNumber" ).front().getData();
        u.uin=uin.toUInt();
        if ( u.uin<firstUid || u.uin>lastUid )
        {
            continue;
        }
        u.uid=LDAPSession::getBinAttrValues ( *it,
                                              "uid" ).front().getData();
        u.name=u.name.fromUtf8 ( LDAPSession::getBinAttrValues ( *it,
                                 "cn" ).front().getData() );
        list<ByteArray> lst=LDAPSession::getBinAttrValues (
                                *it,"jpegPhoto" );
        if ( lst.size() )
        {
            u.foto.loadFromData ( ( const uchar* ) (
                                      lst.front().getData() ),
                                  lst.front().length() );
        }
        userList.append ( u );
    }
    qSort ( userList.begin(),userList.end(),user::lessThen );
    delete ld;
    ld=0;
    displayUsers();
    if ( defaultUser )
    {
        defaultUser=false;
        for ( int i=0; i<userList.size(); ++i )
        {
            if ( userList[i].uid ==defaultUserName )
            {
                uname->setText ( defaultUserName );
                slotUnameChanged ( defaultUserName );
                QTimer::singleShot (
                    100, this,
                    SLOT ( slotUnameEntered() ) );
                break;
            }
        }
    }
#endif
}


void ONMainWindow::slotConfig()
{
    if ( !startMaximized && !startHidden && !embedMode )
    {
        X2goSettings st ( "sizes" );

        st.setting()->setValue ( "mainwindow/size",
                                 QVariant ( size() ) );
        st.setting()->setValue ( "mainwindow/pos",QVariant ( pos() ) );
        st.setting()->sync();
    }
    if ( ld )
        delete ld;
    ld=0;

#if defined (Q_OS_WIN) || defined (Q_OS_DARWIN)
    bool oldDisableInput = false;
    {
        X2goSettings st ("settings");
        oldDisableInput = st.setting ()->value ("pulse/norecord",
                                                (QVariant) false).toBool ();
    }
#endif /* defined (Q_OS_WIN) || defined (Q_OS_DARWIN) */

    ConfigDialog dlg ( this );
    if ( dlg.exec() ==QDialog::Accepted )
    {
        int i;

#if defined (Q_OS_WIN) || defined (Q_OS_DARWIN)
        if(!systemDisablePA)
        {
            X2goSettings st ("settings");
            bool disablePA = st.setting ()->value ("pulse/disable",
                                                     (QVariant) false).toBool ();
            if(!pulseManager && !disablePA)
	    {
	        x2goDebug<<"PA manager not inited yet, starting PA thread";
		QTimer::singleShot (10, this, SLOT (pulseManagerWrapper ()));
	    }
	    else if(pulseManager && disablePA)
	    {
	        if(pulseManager->is_server_running())
		{
		    x2goDebug<<"Stopping PulseAudio";
		    pulseManager->shutdown();
		}
	    }
	    else if(pulseManager && !disablePA)//pulse is already inited and not disabled by config dialog
	    {
                bool newDisableInput = st.setting ()->value ("pulse/norecord",
                                                         (QVariant) false).toBool ();
                if(systemDisablePARecord)
		    newDisableInput=true;
                if (oldDisableInput != newDisableInput) {
                    bool ret = pulseManager->set_record (!newDisableInput);

                    if (!ret) {
                      x2goDebug << "Failed to change recording status of PulseManager. PulseAudio not started?" << Qt::endl;
                    }
                    pulseManager->restart ();
                }
	    }
	}
#endif /* defined (Q_OS_WIN) || defined (Q_OS_DARWIN) */

        if ( passForm->isVisible() && !embedMode )
        {
            if (brokerMode  && (!config.brokerAuthenticated))
            {
                return;
            }
            slotClosePass();
        }
        if ( sessionStatusDlg->isVisible() || embedMode )
        {
            trayIconInit();
            //if session is running or embed mode, save changes,
            //but not accept
            //
            return;
        }
        if ( !embedMode )
        {

            for ( i=0; i<names.size(); ++i )
                names[i]->close();

            sessionExplorer->cleanSessions();

            userList.clear();
        }
        loadSettings();
        trayIconInit();
        if ( useLdap )
        {
            act_new->setEnabled ( false );
            act_edit->setEnabled ( false );
            u->setText ( tr ( "Login:" ) );
            QTimer::singleShot ( 1, this, SLOT ( readUsers() ) );
        }
        else
        {
            act_new->setEnabled ( true );
            act_edit->setEnabled ( true );
            u->setText ( tr ( "Session:" ) );
            QTimer::singleShot ( 1, this,
                                 SLOT ( slotReadSessions() ) );
        }
        slotResize ( fr->size() );
    }
}



void ONMainWindow::slotReadSessions()
{

    if(brokerMode&&ignoreBrokerSessions)
    {
        x2goDebug<<"Looks like session is already started, not reading sessions";
        return;
    }
    users->show();
    ln->show();

    if(brokerMode)
    {
//         bgLay->removeItem(bgLay->itemAt(3));
        slotResize(QSize(width(), height()));
    }

    X2goSettings *st;

    if (brokerMode)
    {
        if (changeBrokerPass)
            act_changeBrokerPass->setEnabled(true);
        config.key=QString();
        config.user=QString();
        config.sessiondata=QString();

        st=new X2goSettings(config.iniFile,QSettings::IniFormat);
        sessionStatusDlg->hide();
        setEnabled ( true );
        if(config.brokerSyncTimeout*1000!=brokerSyncTimer->interval())
        {
            brokerSyncTimer->setInterval(config.brokerSyncTimeout*1000);
        }
        if(config.brokerSyncTimeout && !(brokerSyncTimer->isActive()))
        {
            brokerSyncTimer->start();
        }
        if(!config.brokerSyncTimeout)
        {
            brokerSyncTimer->stop();
        }
    }
    else
        st= new X2goSettings( "sessions" );

    QStringList slst=st->setting()->childGroups();

    x2goDebug<<"Reading "<<slst.size()<<" sessions from config file.";

    if (brokerMode && (slst.size()==0))
    {
        QString error = tr("No X2Go sessions found, closing.");
        if(!startHidden)
            QMessageBox::critical(this,tr("Error"),error);
        else
            x2goErrorf(1)<<error;
        close();
        return;
    }
    sessionExplorer->updateSessions(slst);
    if ( slst.size() ==0 )
        slotNewSession();
    uname->setText ( "" );
    disconnect ( uname,SIGNAL ( textEdited ( const QString& ) ),this,
                 SLOT ( slotUnameChanged ( const QString& ) ) );
    connect ( uname,SIGNAL ( textEdited ( const QString& ) ),this,
              SLOT ( slotSnameChanged ( const QString& ) ) );

    if(usePGPCard &&brokerMode&&cardReady)
    {
        if(sessionExplorer->getSessionsList()->count()==1)
        {
            slotSelectedFromList(sessionExplorer->getSessionsList()->at(0));
        }
    }

    if ( !defaultSession&& startHidden )
    {
        startHidden=false;
        destroySplash();
        slotResize();
        show();
        activateWindow();
        raise();

    }
    if ( defaultSession )
    {
        bool sfound=false;
        defaultSession=false;
        if ( defaultSessionId.length() >0 )
        {
            for ( int i=0; i< sessionExplorer->getSessionsList()->size(); ++i )
            {
                if ( sessionExplorer->getSessionsList()->at(i)->id() ==defaultSessionId )
                {
                    sessionExplorer->setCurrrentPath(sessionExplorer->getSessionsList()->at(i)->getPath());
                    sessionExplorer->placeButtons();
                    sfound=true;
                    slotSelectedFromList ( sessionExplorer->getSessionsList()->at(i) );
                    break;
                }
            }
        }
        else if(defaultSessionName.length() > 0)
        {
            QString normalDefaultSName=defaultSessionName.split("/",Qt::SkipEmptyParts).join("/");
            for ( int i=0; i<sessionExplorer->getSessionsList()->size(); ++i )
            {
                QString normalName=(sessionExplorer->getSessionsList()->at(i)->getPath()+"/"+sessionExplorer->getSessionsList()->at(i)->name());
                normalName=normalName.split("/",Qt::SkipEmptyParts).join("/");
                if ( normalName == normalDefaultSName )
                {
                    sessionExplorer->setCurrrentPath(sessionExplorer->getSessionsList()->at(i)->getPath());
                    sessionExplorer->placeButtons();
                    sfound=true;
                    uname->setText ( sessionExplorer->getSessionsList()->at(i)->name() );
                    QTimer::singleShot (
                        100, this,
                        SLOT ( slotUnameEntered() ) );
                    slotSnameChanged ( defaultSessionName );
                    break;
                }
            }
        }
        //no session name or id provided
        //started with ".x2go" session file.
        //if this file has only one session, chose it automatically
        else
        {
            if(sessionExplorer->getSessionsList()->size()==1)
            {
                sessionExplorer->setCurrrentPath(sessionExplorer->getSessionsList()->at(0)->getPath());
                sessionExplorer->placeButtons();
                sfound=true;
                slotSelectedFromList ( sessionExplorer->getSessionsList()->at(0) );
            }
        }
        if ( !sfound && startHidden )
        {
            startHidden=false;
            destroySplash();
            slotResize();
            show();
            activateWindow();
            raise();
        }
    }
    delete st;
}


void ONMainWindow::slotNewSession()
{
    if(X2goSettings::centralSettings())
        return;
    QString id=QDateTime::currentDateTime().
               toString ( "yyyyMMddhhmmsszzz" );
    EditConnectionDialog dlg (true, id, this );
    if ( dlg.exec() ==QDialog::Accepted )
    {
        SessionButton* bt=sessionExplorer->createBut ( id );
        sessionExplorer->placeButtons();
        users->ensureVisible ( bt->x(),bt->y(),50,220 );
    }
}

void ONMainWindow::slotManage()
{
    SessionManageDialog dlg ( this );
    dlg.exec();
}

void ONMainWindow::slotCreateSessionIcon()
{
    SessionManageDialog dlg ( this,true );
    dlg.exec();
}


void ONMainWindow::displayToolBar ( bool show )
{
    X2goSettings st1 ( "settings" );

    st1.setting()->setValue ( "toolbar/show",show );
    st1.setting()->sync();
}


bool ONMainWindow::initLdapSession ( bool showError )
{
#ifdef USELDAP

    x2goDebug<<"Initializing LDAP sessions ...";

    try
    {
        ld=new LDAPSession ( ldapServer.toStdString(),
                             ldapPort,"","",true,false );
    }
    catch ( LDAPExeption e )
    {
        QString message="Exception0 in: ";
        message=message+e.err_type.c_str();
        message=message+" : "+e.err_str.c_str();

        x2goDebug<<message;

        if ( ldapServer1.length() )
        {
            try
            {
                ld=new LDAPSession ( ldapServer1.toStdString(),
                                     ldapPort1,"","",
                                     true,false );
            }
            catch ( LDAPExeption e )
            {
                QString message="Exception1 in: ";
                message=message+e.err_type.c_str();
                message=message+" : "+e.err_str.c_str();

                x2goDebug<<message;

                if ( ldapServer2.length() )
                {
                    try
                    {
                        ld=new LDAPSession (
                            ldapServer2.toStdString(),
                            ldapPort2,"","",
                            true,false );
                    }
                    catch ( LDAPExeption e )
                    {
                        QString message=
                            "Exception2 in: ";
                        message=message+
                                e.err_type.c_str();
                        message=message+" : "+
                                e.err_str.c_str();

                        x2goDebug<<message;
                        if ( showError )
                            QMessageBox::critical (
                                0l,tr ( "Error" ),
                                message,
                                QMessageBox::Ok,
                                QMessageBox::
                                NoButton );

                        x2goDebug<<"Returning false, 3rd try failed.";
                        return false;
                    }
                }
                else
                {
                    x2goDebug<<"Returning false.";

                    if ( showError )
                        QMessageBox::critical (
                            0l,tr ( "Error" ),
                            message, QMessageBox::Ok,
                            QMessageBox::NoButton );

                    return false;
                }

            }
        }
        else
        {
            x2goDebug<<"Returning false.";

            if ( showError )
                QMessageBox::critical ( 0l,tr ( "Error" ),
                                        message,QMessageBox::Ok,
                                        QMessageBox::NoButton );

            return false;
        }

    }
    sessionCmd="/usr/bin/startkde";
    LDAPSndSys="ARTS_SERVER";
    LDAPSndStartServer=true;
    startSound=false;
    firstUid=0;
    lastUid=65535;


    list<string> attr;
    attr.push_back ( SESSIONCMD );
    attr.push_back ( FIRSTUID );
    attr.push_back ( LASTUID );

    list<LDAPStringEntry> res;
    QString searchBase="ou=Settings,ou=ON,"+ldapDn;
    QString srch="cn=session settings";
    try
    {
        ld->stringSearch ( searchBase.toStdString(),attr,
                           srch.toStdString(),res );
    }
    catch ( LDAPExeption e )
    {
        QString message="Exception in: ";
        message=message+e.err_type.c_str();
        message=message+" : "+e.err_str.c_str();
        QMessageBox::critical ( 0l,tr ( "Error" ),message,
                                QMessageBox::Ok,
                                QMessageBox::NoButton );
        return false;
    }

    if ( res.size() !=0 )
    {
        LDAPStringEntry entry=res.front();
        list<string> str=LDAPSession::getStringAttrValues (
                             entry,SESSIONCMD );
        if ( str.size() )
        {
            sessionCmd=str.front().c_str();
        }
        str=LDAPSession::getStringAttrValues ( entry,FIRSTUID );
        if ( str.size() )
        {
            firstUid= ( ( QString ) str.front().c_str() ).toInt();
        }
        str=LDAPSession::getStringAttrValues ( entry,LASTUID );
        if ( str.size() )
        {
            lastUid= ( ( QString ) str.front().c_str() ).toInt();
        }
    }
    attr.clear();
    res.clear();
    attr.push_back ( NETSOUNDSYSTEM );
    attr.push_back ( SNDSUPPORT );
    attr.push_back ( SNDPORT );
    attr.push_back ( STARTSNDSERVER );


    srch="cn=sound settings";
    try
    {
        ld->stringSearch ( searchBase.toStdString(),attr,
                           srch.toStdString(),res );
    }
    catch ( LDAPExeption e )
    {
        QString message="Exception in: ";
        message=message+e.err_type.c_str();
        message=message+" : "+e.err_str.c_str();
        QMessageBox::critical ( 0l,tr ( "Error" ),message,
                                QMessageBox::Ok,
                                QMessageBox::NoButton );
        return false;
    }

    if ( res.size() !=0 )
    {
        LDAPStringEntry entry=res.front();
        list<string> str=LDAPSession::getStringAttrValues (
                             entry,NETSOUNDSYSTEM );
        if ( str.size() )
        {
            LDAPSndSys=str.front().c_str();
        }
        if ( LDAPSndSys=="PULSE" )
        {
            LDAPSndSys="pulse";
            LDAPSndStartServer=false;
            LDAPSndPort="4713";
        }
        if ( LDAPSndSys=="ARTS_SERVER" )
        {
            LDAPSndPort="20221";
            LDAPSndSys="arts";
        }
        if ( LDAPSndSys=="ESPEAKER" )
        {
            LDAPSndPort="16001";
            LDAPSndSys="esd";
        }
        str=LDAPSession::getStringAttrValues ( entry,SNDSUPPORT );
        if ( str.size() )
        {
            startSound= ( str.front() =="yes" ) ?true:false;
        }
        str=LDAPSession::getStringAttrValues ( entry,SNDPORT );
        if ( str.size() )
        {
            LDAPSndPort=str.front().c_str();
        }
        str=LDAPSession::getStringAttrValues ( entry,STARTSNDSERVER );
        if ( str.size() )
        {
            LDAPSndStartServer=
                ( str.front() =="yes" ) ?true:false;
        }
    }
#endif
    x2goDebug<<"Initialized LDAP sessions.";
    return true;

}



void ONMainWindow::slotSnameChanged ( const QString& text )
{
    if ( prevText==text )
        return;
    if ( text=="" )
        return;
    QList<SessionButton*>::iterator it;
    QList<SessionButton*>::iterator endit=sessionExplorer->getSessionsList()->end();
    for ( it=sessionExplorer->getSessionsList()->begin(); it!=endit; it++ )
    {
        if((*it)->getPath()!=sessionExplorer->getCurrentPath())
            continue;
        QString name= ( *it )->name();
        if ( name.indexOf ( text,0,Qt::CaseInsensitive ) ==0 )
        {
            QPoint pos= ( *it )->pos();
            uname->setText ( name );
            QScrollBar* bar=users->verticalScrollBar();
            int docLang=bar->maximum()-bar->minimum() +
                        bar->pageStep();
            double position= ( double ) ( pos.y() ) /
                             ( double ) ( uframe->height() );
            bar->setValue ( ( int ) ( docLang*position-height() /
                                      2+ ( *it )->height() /2 ) );
            uname->setSelection ( name.length(),
                                  text.length()-name.length() );
            break;
        }
    }
    prevText=text;
}


void ONMainWindow::slotSelectedFromList ( SessionButton* session )
{
    pass->setText ( "" );
    sessionExplorer->setLastSession(session);
    QString command;
    QString server;
    QString userName;
    bool autologin=false;
    bool krblogin=false;
    bool usebrokerpass=false;
    QString sessIcon;
    QPalette pal;
    QString sessionName;
    if ( !embedMode )
    {
        session->hide();
        pal=users->palette();
        setUsersEnabled ( false );
        sessionName=session->name();

        QString sid=session->id();
        X2goSettings* st;
        if (brokerMode)
        {
            st=new X2goSettings( config.iniFile, QSettings::IniFormat );
        }
        else
        {
            st = new X2goSettings( "sessions" );
        }



        sessIcon = wrap_legacy_resource_URIs (st->setting()->value (sid+"/icon",
                                                                    (QVariant) iconsPath("/128x128/x2gosession.png")
                                                                   ).toString ());
        sessIcon = expandHome(sessIcon);

        command=st->setting()->value (
                    sid+"/command",
                    ( QVariant ) tr ( "KDE" ) ).toString();

        server=st->setting()->value ( sid+"/host",
                                      ( QVariant ) QString()
                                    ).toString();
        userName=st->setting()->value (
                     sid+"/user",
                     ( QVariant ) QString() ).toString();
        if (defaultUser && userName.length()<1)
            userName=defaultUserName;

        if(brokerMode)
            usebrokerpass=st->setting()->value (
                              sid+"/usebrokerpass",
                              false ).toBool();


        sshPort=st->setting()->value (
                    sid+"/sshport",
                    ( QVariant ) defaultSshPort ).toString();
        currentKey=st->setting()->value (
                       sid+"/key",
                       ( QVariant ) QString() ).toString();
        currentKey=expandHome(currentKey);

        autologin=st->setting()->value (
                      sid+"/autologin",
                      ( QVariant ) false ).toBool();
#ifdef Q_OS_LINUX
        if(command =="XDMCP" && st->setting()->value (
                      sid+"/directxdmcp",
                      ( QVariant ) false ).toBool())
             autologin=true;
#endif
        krblogin=st->setting()->value (
                     sid+"/krblogin",
                     ( QVariant ) false ).toBool();
        delete st;
#ifdef Q_OS_WIN
        if ( portable &&
                u3Device.length() >0 )
        {
            currentKey.replace ( "(U3)",u3Device );
        }
#endif
    }
    else
    {
        command=config.command;
        server=config.server;
        userName=config.user;
        sshPort=config.sshport;
        sessIcon=iconsPath("/128x128/x2gosession.png");
        sessionName=config.session;
        currentKey=config.key;
    }

    selectedCommand=command.split("/").last();
    command=transAppName ( command );
    login->setText ( userName );
    x2goDebug << "Creating QPixmap with session icon: '" << sessIcon << "'.";
    QPixmap* pix;

    if (!brokerMode || sessIcon == iconsPath("/128x128/x2gosession.png"))
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
            pix->load(iconsPath("/128x128/x2gosession.png"));
        }
    }

    if ( !miniMode )
    {
        fotoLabel->setPixmap (
            pix->scaled ( 64,64,
                         Qt::IgnoreAspectRatio,
                         Qt::SmoothTransformation ) );
        fotoLabel->setFixedSize ( 64,64 );
    }
    else
    {
        fotoLabel->setPixmap (
            pix->scaled ( 48,48,
                         Qt::IgnoreAspectRatio,
                         Qt::SmoothTransformation ) );
        fotoLabel->setFixedSize ( 48,48 );
    }

    delete pix;
    if(currentKey.length()<=0)
    {
        currentKey=findSshKeyForServer(userName, server, sshPort);
    }


    if ( command=="RDP" )
    {
        command=tr ( "RDP connection" );
    }
    if ( command=="XDMCP" )
    {
        command=tr ( "XDMCP" );
    }
    if ( command=="SHADOW" )
    {
        command=tr ( "Connection to local desktop" );
    }

    QString text="<b>"+sessionName +"</b><br>"+
                 command+tr ( " on " ) +server;
    nameLabel->setText ( text );
    if ( userName.length() <=0 )
        login->setFocus();

    bool nopass=false;
    if ( !embedMode )
        slotShowPassForm();


///////////////////////////////////////////////////

    if ( currentKey.length() >0 )
    {
        nopass=true;
    }
    if(brokerMode &&usebrokerpass)
    {
        pass->setText(config.brokerPass);
        slotSessEnter();
    }
    else if ( currentKey != QString() && currentKey != "" && nopass )
    {
        x2goDebug<<"Starting session with key.";
        slotSessEnter();
    }
    else if ( cardReady || autologin || krblogin  )
    {
        x2goDebug<<"Starting session via Smart Card, SSH Agent or Kerberos token.";
        nopass=true;
        if ( cardReady )
            login->setText ( cardLogin );
        slotSessEnter();
        return;
    }
    if ( startHidden && nopass==false && (!brokerMode) )
    {
        startHidden=false;
        destroySplash();
        slotResize();
        show();
        activateWindow();
        raise();
    }
    if ( embedMode )
    {
        QTimer::singleShot ( 50, this,
                             SLOT ( slotShowPassForm() ) );
    }
}


SshMasterConnection* ONMainWindow::startSshConnection ( QString host, QString port, bool acceptUnknownHosts,
        QString login,
        QString password, bool autologin, bool krbLogin, bool getSrv, bool useproxy,
        SshMasterConnection::ProxyType type,
        QString proxyserver, quint16 proxyport,
        QString proxylogin, QString proxypassword, QString proxyKey,
        bool proxyAutologin, bool proxyKrbLogin)
{
    x2goInfof(8)<<tr("Starting connection to server: ") + host + ":" + port;
    SshMasterConnection* con;

    x2goDebug<<"Starting new ssh connection to server:"<<host<<":"<<port<<" krbLogin: "<<krbLogin;


    if ( usePGPCard/*||useSshAgent*/ )
        autologin=true;
    if ( cardReady )
        cardStarted=true;


    /////key/sshagent/env/

    passForm->setEnabled ( false );
    if(cmdAutologin)
    {
        autologin=true;
    }


    con=new SshMasterConnection (this, host, port.toInt(),acceptUnknownHosts,
                                 login, password,currentKey, autologin, krbLogin,useproxy,
                                 type, proxyserver, proxyport, proxylogin, proxypassword, proxyKey,proxyAutologin, proxyKrbLogin);
    if (!getSrv)
        connect ( con, SIGNAL ( connectionOk(QString) ), this, SLOT ( slotSshConnectionOk() ) );
    else
        connect ( con, SIGNAL ( connectionOk(QString)), this, SLOT ( slotServSshConnectionOk(QString) ) );

    qRegisterMetaType<SshMasterConnection::passphrase_types> ("SshMasterConnection::passphrase_types");

    connect ( con, SIGNAL ( serverAuthError ( int,QString, SshMasterConnection* ) ),this,
              SLOT ( slotSshServerAuthError ( int,QString, SshMasterConnection* ) ) );
    connect ( con, SIGNAL ( needPassPhrase(SshMasterConnection*, SshMasterConnection::passphrase_types)),this,
              SLOT ( slotSshServerAuthPassphrase(SshMasterConnection*, SshMasterConnection::passphrase_types)) );
    connect ( con, SIGNAL ( needChallengeResponse(SshMasterConnection*, QString)),this,
              SLOT ( slotSshServerAuthChallengeResponse(SshMasterConnection*, QString)) );
    connect ( con, SIGNAL ( userAuthError ( QString ) ),this,SLOT ( slotSshUserAuthError ( QString ) ) );
    connect ( con, SIGNAL ( connectionError ( QString,QString ) ), this,
              SLOT ( slotSshConnectionError ( QString,QString ) ) );
    connect ( con, SIGNAL(startInteraction(SshMasterConnection*,QString)),this,
               SLOT(slotSshInteractionStart(SshMasterConnection*,QString)) );
    connect ( con, SIGNAL(updateInteraction(SshMasterConnection*,QString)),this,
              SLOT(slotSshInteractionUpdate(SshMasterConnection*,QString)) );
    connect (con, SIGNAL(finishInteraction(SshMasterConnection*)),this, SLOT(slotSshInteractionFinish(SshMasterConnection*)));
    connect ( interDlg, SIGNAL(textEntered(QString)), con, SLOT(interactionTextEnter(QString)));
    connect ( interDlg, SIGNAL(interrupt()), con, SLOT(interactionInterruptSlot()));
    con->start();
    return con;
}

void ONMainWindow::slotSshConnectionError ( QString message, QString lastSessionError )
{
    x2goErrorf(2)<< tr("Connection error: ") + message + ": " + lastSessionError;
    if ( sshConnection )
    {
        sshConnection->wait();
        delete sshConnection;
        sshConnection=0l;
    }

    if (!startHidden)
    {
        QMessageBox::critical ( 0l,message,lastSessionError,
                                QMessageBox::Ok,
                                QMessageBox::NoButton );
        setEnabled ( true );
        passForm->setEnabled ( true );
        slotShowPassForm();
        pass->setFocus();
        pass->selectAll();


        passForm->setEnabled ( true );
    }
    else
    {
        // In order to get this interaction free, we need to free this from windows and stuff
        //    if ( startHidden )
        //    {
        //        startHidden=false;
        //        slotResize();
        //        show();
        //        activateWindow();
        //        raise();
        //    }

        // completely quit the application
        trayQuit();
    }
}




void ONMainWindow::slotSshConnectionOk()
{
    x2goDebug<<"SSH connection established.";
    passForm->setEnabled ( true );
    if ( useLdap )
    {
        continueLDAPSession();
    }
    else
    {
        if(brokerMode && (connectionType != DEFAULT))
        {
            if(connectionType==SUSPEND)
            {
                suspendSession(suspendTerminateSessionFromBroker);
            }
            else
            {
                termSession(suspendTerminateSessionFromBroker, false);
            }
        }
        else
            continueNormalSession();
    }
}

void ONMainWindow::cleanServerSshConnections()
{
    for(int i=serverSshConnections.size()-1;i>=0;--i)
    {
        if(serverSshConnections[i]!=sshConnection)
        {
            delete serverSshConnections[i];
        }
    }
    serverSshConnections.clear();
}


SshMasterConnection*  ONMainWindow::findServerSshConnection(QString host)
{
    x2goDebug<<"Searching for SSH connections ...";
    for (int i=0; i<serverSshConnections.count(); ++i)
    {
        if (serverSshConnections[i])
        {
            if (serverSshConnections[i]->getHost()==host)
            {
                x2goDebug<<"Found SSH connection.";
                return serverSshConnections[i];
            }
        }
    }
    x2goWarningf(3)<< tr("Couldn't find an SSH connection.");
    return 0l;
}

void ONMainWindow::slotServSshConnectionOk(QString server)
{
    SshMasterConnection* con=findServerSshConnection(server);
    if (!con)
        return;
    x2goDebug<<"Getting sessions on host: " + server;
    con->executeCommand( "x2golistsessions", this, SLOT (slotListAllSessions ( bool,QString,int ) ));
}

void ONMainWindow::slotSshInteractionFinish(SshMasterConnection* connection)
{
    if(interDlg->isInterrupted())
    {
          slotCloseInteractionDialog();
    }
    else
    {
         interDlg->setDisplayMode();
    }
}

void ONMainWindow::slotCloseInteractionDialog()
{
    if(interDlg->getInteractionMode()==InteractionDialog::SESSION)
    {
          x2goDebug<<"Closed SSH Session interaction";
          slotSshUserAuthError("NO_ERROR");
    }
    else
    {
        x2goDebug<<"Closed SSH Broker interaction";
        if(broker)
        {
	    interDlg->hide();
            broker->closeSSHInteractionDialog();
        }
    }
}


void ONMainWindow::slotSshInteractionStart(SshMasterConnection* connection, QString prompt)
{
    sessionStatusDlg->hide();
    interDlg->show();
    interDlg->reset();
    interDlg->appendText(prompt);

    setEnabled(true);
    interDlg->setEnabled(true);
    x2goDebug<<"SSH Session prompt:"<<prompt;
    if(connection==sshConnection)
    {
         x2goDebug<<"SSH Session interaction";
	 interDlg->setInteractionMode(InteractionDialog::SESSION);
    }
    else
    {
	 interDlg->setInteractionMode(InteractionDialog::BROKER);
         x2goDebug<<"SSH Broker interaction";
    }
}

void ONMainWindow::slotSshInteractionUpdate(SshMasterConnection* connection, QString output)
{
    if(connection==sshConnection)
    {
         x2goDebug<<"SSH Session interaction";
	 interDlg->setInteractionMode(InteractionDialog::SESSION);
    }
    else
    {
	 interDlg->setInteractionMode(InteractionDialog::BROKER);
         x2goDebug<<"SSH Broker interaction";
    }
    interDlg->appendText(output);
    x2goDebug<<"SSH Interaction update:"<<output;
}

void ONMainWindow::slotSshServerAuthPassphrase(SshMasterConnection* connection, SshMasterConnection::passphrase_types passphrase_type)
{
    bool ok;
    QString message;

    switch (passphrase_type) {
        case SshMasterConnection::PASSPHRASE_PRIVKEY:
                                                        message = tr ("Enter passphrase to decrypt a key");
                                                        ok = true;
                                                        break;
        case SshMasterConnection::PASSPHRASE_CHALLENGE:
                                                        message = tr ("Verification code:");
                                                        ok = true;
                                                        break;
        case SshMasterConnection::PASSPHRASE_PASSWORD:
                                                        message = tr ("Enter user account password:");
                                                        ok = true;
                                                        break;
        default:
                                                        x2goDebug << "Unknown passphrase type requested! Was: " << passphrase_type << Qt::endl;
                                                        ok = false;
                                                        break;
    }

    if (ok) {
        QString phrase = QInputDialog::getText (0, connection->getUser () + "@" + connection->getHost () + ":" + QString::number (connection->getPort ()),
                                                message, QLineEdit::Password, QString (""), &ok);
        if (!ok) {
            phrase = QString ("");
        }
        connection->setKeyPhrase (phrase);
        if(isHidden ()) {
            show ();
            QTimer::singleShot (1, this, SLOT (hide ()));
        }
    }
}


void ONMainWindow::slotSshServerAuthChallengeResponse(SshMasterConnection* connection, QString challenge)
{
    bool ok = false;
    QString message;

    message = challenge;

    QString phrase = QInputDialog::getText (0, connection->getUser () + "@" + connection->getHost () + ":" + QString::number (connection->getPort ()),
                                            message, QLineEdit::Password, QString(), &ok);
    if (!ok) {
        phrase = QString();
    }
    else {
        if (phrase == QString()) {
            phrase = "";
        }
    }

    connection->setKeyPhrase (phrase);

    if (isHidden ()) {
        show ();
        QTimer::singleShot (1, this, SLOT (hide ()));
    }
}

void ONMainWindow::startBrokerSyncTimer()
{
    if(brokerMode)
    {
        ignoreBrokerSessions=false;
    }

    if(brokerMode && config.brokerSyncTimeout && !(brokerSyncTimer->isActive()))
    {
            brokerSyncTimer->start();
    }
}

void ONMainWindow::slotSshServerAuthError ( int error, QString sshMessage, SshMasterConnection* connection )
{
    interDlg->hide();
    if ( startHidden )
    {
        startHidden=false;
        destroySplash();
        slotResize();
        show();
        activateWindow();
        raise();
    }
    QString errMsg;
    switch ( error )
    {
    case SSH_SERVER_KNOWN_CHANGED:
        errMsg=tr ( "Host key for server changed.\nIt is now: " ) +sshMessage+"\n"+
               tr ( "This can be an indication of a man-in-the-middle attack.\n"
                    "Somebody might be eavesdropping on you.\n"
                    "For security reasons, it is recommended to stop the connection attempt.\n"
                    "Do you want to terminate the connection?\n" );
        if (QMessageBox::warning (0, tr ("Host key verification failed."),
                                  errMsg, QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No) == QMessageBox::Yes)
        {
            connection->writeKnownHosts(false);
            connection->wait();
            if(sshConnection && sshConnection !=connection)
            {
                sshConnection->wait();
                delete sshConnection;
            }
            slotSshUserAuthError ( tr ( "Host key verification failed." ) );
            sshConnection=0;
            startBrokerSyncTimer();
            return;
        }
        else
        {
            errMsg = tr( "If you accept the new host key the security of your "
                         "connection may be compromised.\n"
                         "Do you want to update the host key?" );
        }
        break;
    case SSH_SERVER_FOUND_OTHER:
        errMsg=tr ( "The host key for this server was not found but another "
                    "type of key exists. An attacker might have changed the default server key to "
                    "trick your client into thinking the key does not exist yet.\n"
                    "For security reasons, it is recommended to stop the connection attempt.\n"
                    "Do you want to terminate the connection?\n");
        if (QMessageBox::warning (0, tr ("Host key verification failed."),
                                  errMsg, QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No) == QMessageBox::Yes)
        {
            connection->writeKnownHosts(false);
            connection->wait();
            if(sshConnection && sshConnection !=connection)
            {
                sshConnection->wait();
                delete sshConnection;
            }
            slotSshUserAuthError ( tr ( "Host key verification failed." ) );
            sshConnection=0;
            startBrokerSyncTimer();
            return;
        }
        else
        {
            errMsg = tr( "If you accept the new host key the security of your "
                         "connection may be compromised.\n"
                         "Do you want to update the host key?" );
        }
        break;
    case SSH_SERVER_ERROR:
        connection->writeKnownHosts(false);
        connection->wait();
        if(sshConnection && sshConnection !=connection)
        {
            sshConnection->wait();
            delete sshConnection;
        }
        sshConnection=0;
        slotSshUserAuthError ( sshMessage );
        startBrokerSyncTimer();
        return ;
    case SSH_SERVER_FILE_NOT_FOUND:
        errMsg=tr ( "Could not find known hosts file. "
                    "If you accept the host key here, the file will be automatically created." );
        break;

    case SSH_SERVER_NOT_KNOWN:
        errMsg=tr ( "The server is unknown. Do you trust the host key?\nPublic key hash: " ) +sshMessage;
        break;
    }

    if (QMessageBox::warning (this, tr ("Host key verification failed."),
                              errMsg, QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No) == QMessageBox::No)
    {
        connection->writeKnownHosts(false);
        connection->wait();
        if(sshConnection && sshConnection !=connection)
        {
            sshConnection->wait();
            delete sshConnection;
        }
        sshConnection=0;
        slotSshUserAuthError ( tr ( "Host key verification failed." ) );
        startBrokerSyncTimer();
        return;
    }
    connection->writeKnownHosts(true);
    connection->wait();
    connection->start();
}

void ONMainWindow::slotSshUserAuthError ( QString error )
{
    interDlg->hide();
    if ( sshConnection )
    {
        sshConnection->wait();
        delete sshConnection;
        sshConnection=0l;
    }

    //    if ( startHidden )
    //    {
    //        startHidden=false;
    //        slotResize();
    //        show();
    //        activateWindow();
    //        raise();
    //    }
    // hidden means hidden, we'll close the client afterwards.
    if ( startHidden )
    {
        x2goErrorf(3)<< tr("Authentication failed: ") + error;
        trayQuit();
    }

    if(error != "NO_ERROR")
        QMessageBox::critical (0l, tr ("Authentication failed."),
                               error, QMessageBox::Ok,
                               QMessageBox::NoButton);
    setEnabled ( true );
    passForm->setEnabled ( true );
    slotShowPassForm();
    pass->setFocus();
    pass->selectAll();
    passForm->setEnabled ( true );
    startBrokerSyncTimer();
}

void ONMainWindow::slotSessEnter()
{

    if ( useLdap )
    {
        slotPassEnter();
        return;
    }
    if (brokerMode)
    {
        if (!config.brokerAuthenticated)
        {

            x2goDebug<<"Starting broker request.";
            slotStartBroker();
            return;
        }
    }

    if ((brokerMode || !embedMode) && !(sessionExplorer->getLastSession())) {
        x2goDebug << "No session selected, returning without starting a session.";
        return;
    }

    resumingSession.sessionId=QString();
    resumingSession.server=QString();
    resumingSession.display=QString();
    setStatStatus ( tr ( "connecting" ) );

    if(brokerMode)
    {
        if (config.brokerNoAuth && brokerNoauthWithSessionUsername) {
            config.brokerUser = login->text();
        }
#ifdef Q_OS_LINUX
        X2goSettings* st=new X2goSettings(config.iniFile, QSettings::IniFormat);
        QString sid=sessionExplorer->getLastSession()->id();
        QString cmd=st->setting()->value ( sid+"/command",
                                           ( QVariant ) QString() ).toString();
        directRDP=(st->setting()->value ( sid+"/directrdp",
                                               ( QVariant ) false ).toBool() && cmd == "RDP");

        if (cmd =="RDP" && directRDP)
        {
            x2goDebug<<"Starting direct RDP Session from broker";
            startSession ( sid );
            return;
        }

        directRDP=(st->setting()->value ( sid+"/directxdmcp",
                                               ( QVariant ) false ).toBool() && cmd == "RDP");

        if (cmd =="XDMCP" && directRDP)
        {
            x2goDebug<<"Starting direct RDP Session from broker";
            startSession ( sid );
            return;
        }
#endif
        broker->selectUserSession(sessionExplorer->getLastSession()->id(),login->text());
        config.session=sessionExplorer->getLastSession()->id();
        setStatStatus ( tr ( "Connecting to broker" ) );
        stInfo->insertPlainText ( "broker url: "+config.brokerurl );
        setEnabled ( false );
        uname->hide();
        u->hide();
        return;
    }

    QString sid="";
    if ( !embedMode )
        sid=sessionExplorer->getLastSession()->id();
    startSession ( sid );
}

void ONMainWindow::continueNormalSession()
{
    x2goDebug<<"Continue normal X2Go session";

    if (brokerMode && !shadowSession)
    {
        slotListSessions(true,QString(),0);
        return;
    }
    if ( !shadowSession )
        sshConnection->executeCommand ( "x2golistsessions", this,SLOT ( slotListSessions ( bool, QString,int )));
    else
        sshConnection->executeCommand ( "x2golistdesktops", this,SLOT ( slotListSessions ( bool, QString,int )));

}

void ONMainWindow::continueLDAPSession()
{
    sshConnection->executeCommand ( "x2gogetservers", this, SLOT ( slotGetServers ( bool, QString,int ) ));
}

#ifdef Q_OS_LINUX
void ONMainWindow::startDirectRDP()
{
    if (!(sessionExplorer->getLastSession())) {
        x2goDebug << "No session selected, returning without starting a session.";
        return;
    }

    X2goSettings* st;
    if(brokerMode)
    {
        st=new X2goSettings(config.iniFile, QSettings::IniFormat);
    }
    else
    {
        st=new X2goSettings ( "sessions" );
    }

    QString sid;
    bool freeRDPNew=false;
    if ( !embedMode )
        sid=sessionExplorer->getLastSession()->id();
    else
        sid="embedded";


    bool fullscreen=st->setting()->value ( sid+"/fullscreen",
                                          ( QVariant )
                                          defaultFullscreen ).toBool();
    bool maxRes=st->setting()->value ( sid+"/maxdim",
                                      ( QVariant )
                                      false ).toBool();
    int height=st->setting()->value ( sid+"/height",
                                     ( QVariant ) defaultHeight ).toInt();
    int width=st->setting()->value ( sid+"/width",
                                    ( QVariant ) defaultWidth ).toInt();
    QString sessionType=st->setting()->value ( sid+"/command",
                                         ( QVariant ) "").toString();

    QString client=st->setting()->value ( sid+"/rdpclient",
                                         ( QVariant ) "rdesktop").toString();
    if(client=="xfreerdpnew")
    {
        client="xfreerdp";
        freeRDPNew=true;
    }
    QString host=st->setting()->value ( sid+"/host",
                                       ( QVariant ) "").toString();
    QString port=st->setting()->value ( sid+"/rdpport",
                                       ( QVariant ) "3389").toString();
    QString params=st->setting()->value ( sid+"/directrdpsettings",
                                         ( QVariant ) "").toString();

    QString user,password;
    if ((!brokerMode) || (brokerCredsForSession))
    {
        user=getCurrentUname();
        password=getCurrentPass();
    }
    else
    {
        user=st->setting()->value ( sid+"/user",
                                       ( QVariant ) "").toString();
        if(getCurrentUname().length()>0)
        {
            user=getCurrentUname();
        }
        if(getCurrentPass().length()>0)
        {
            password=getCurrentPass();
        }
    }

    nxproxy=new QProcess;
    proxyErrString="";
    connect ( nxproxy,SIGNAL ( error ( QProcess::ProcessError ) ),this,
              SLOT ( slotProxyError ( QProcess::ProcessError ) ) );
    connect ( nxproxy,SIGNAL ( finished ( int,QProcess::ExitStatus ) ),this,
              SLOT ( slotProxyFinished ( int,QProcess::ExitStatus ) ) );
    connect ( nxproxy,SIGNAL ( readyReadStandardError() ),this,
              SLOT ( slotProxyStderr() ) );
    connect ( nxproxy,SIGNAL ( readyReadStandardOutput() ),this,
              SLOT ( slotProxyStdout() ) );


    QString userOpt;
    QString passOpt;
    QString grOpt;
    QString proxyCmd;
    if(!freeRDPNew)
    {
        if (user.length()>0)
        {
            userOpt=" -u ";
            userOpt+=user+" ";
        }
        if (password.length()>0)
        {
            passOpt=" -p \"";
            passOpt+=password+"\" ";
        }
        if (fullscreen)
        {
            grOpt=" -f ";
        }
        else if (maxRes)
        {
            QDesktopWidget wd;
            grOpt=" -D -g "+QString::number( wd.screenGeometry().width())+"x"+QString::number(wd.screenGeometry().height())+" ";
        }
        else
        {
            grOpt=" -g "+QString::number(width)+"x"+QString::number(height);
        }
        proxyCmd=client +" "+params+ grOpt +userOpt+passOpt + host +":"+port ;
    }
    else
    {
        if (user.length()>0)
        {
            userOpt=" /u:";
            userOpt+=user+" ";
        }
        if (password.length()>0)
        {
            passOpt=" /p:\"";
            passOpt+=password+"\" ";
        }
        if (fullscreen)
        {
            grOpt=" /f ";
        }
        else if (maxRes)
        {
            QDesktopWidget wd;
            grOpt=" /w:"+QString::number( wd.screenGeometry().width())+" /h:"+QString::number(wd.screenGeometry().height())+" ";
        }
        else
        {
            grOpt=" /w:"+QString::number(width)+" /h:"+QString::number(height);
        }
        proxyCmd= client +" "+params+ grOpt +userOpt+passOpt + "/v:"+host +":"+port ;
    }
    if(sessionType=="XDMCP")
    {
        x2goDebug<<"starting direct XDMCP session";
        client=st->setting()->value ( sid+"/xdmcpclient",
                                         ( QVariant ) "Xnest").toString();
        params=st->setting()->value ( sid+"/directxdmcpsettings",
                                         ( QVariant ) "").toString();
        if(client == "Xephyr")
        {
            if (fullscreen)
            {
                grOpt=" -fullscreen ";
            }
            else if (maxRes)
            {
                QDesktopWidget wd;
                grOpt=" -screen "+QString::number( wd.screenGeometry().width())+"x"+QString::number(wd.screenGeometry().height())+" ";
            }
            else
            {
                grOpt=" -screen "+QString::number(width)+"x"+QString::number(height);
            }
        }
        else
        {
            if (maxRes)
            {
                QDesktopWidget wd;
                grOpt=" -geometry "+QString::number( wd.screenGeometry().width())+"x"+QString::number(wd.screenGeometry().height())+" ";
            }
            else
            {
                grOpt=" -geometry "+QString::number(width)+"x"+QString::number(height);
            }
        }
        int p=0;
        while(true)
        {
             QString fname="/tmp/.X"+QString::number(p)+"-lock";
             if(QFile::exists(fname))
             {
                 ++p;
             }
             else
                 break;
        }
        proxyCmd= client +" "+params+ grOpt + " -query "+host +" :"+QString::number(p) ;
        login->setText(tr("XDM"));
        resumingSession.display=tr("XDMCP");
    }
    else
    {
        x2goDebug<<"starting direct RDP session";
        resumingSession.display=tr("RDP");
    }
//     x2goDebug<<"starting direct session with cmd:"<<proxyCmd;
    nxproxy->start ( proxyCmd );
    resumingSession.server=host;
    resumingSession.sessionId=user+"-RDP-";
#if QT_VERSION < 0x040700
    resumingSession.sessionId += QString::number(QDateTime::currentDateTime().toTime_t());
#else
    resumingSession.sessionId += QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch()/1000);
#endif
    resumingSession.sessionId += "_stRRDP_dp24";
    resumingSession.display="RDP";
    resumingSession.command="RDP";
    resumingSession.crTime=QDateTime::currentDateTime().toString(Qt::ISODate);
    showSessionStatus();
    if(brokerMode)
    {
        sendEventToBroker(CONNECTED);
    }

//     QTimer::singleShot ( 30000,this,SLOT ( slotRestartProxy() ) );
    proxyRunning=true;
    delete st;
}

#endif


QString ONMainWindow::findSshKeyForServer(QString user, QString server, QString port)
{
    foreach (sshKey key, cmdSshKeys)
    {
        if(key.server == server && key.user == user && key.port == port)
            return key.key;
    }
    foreach (sshKey key, cmdSshKeys)
    {
        if(key.server == server && key.user == user && key.port.length()<=0)
            return key.key;
    }

    foreach (sshKey key, cmdSshKeys)
    {
        if(key.server == server && key.user.length()<=0 && key.port==port)
            return key.key;
    }

    foreach (sshKey key, cmdSshKeys)
    {
        if(key.server == server && key.user.length()<=0 && key.port.length()<=0)
            return key.key;
    }

    foreach (sshKey key, cmdSshKeys)
    {
        if(key.server.length()<=0 && key.user.length()<=0 && key.port.length()<=0)
            return key.key;
    }
    return QString();
}


bool ONMainWindow::startSession ( const QString& sid, CONTYPE conType )
{
    connectionType=conType;
    setEnabled ( false );
#ifdef Q_OS_LINUX
    directRDP=false;
#endif
    QString passwd;
    QString user;
    QString host;
    bool autologin=false;
    bool krblogin=false;
    bool krbDelegation=false;

    bool useproxy=false;
    SshMasterConnection::ProxyType proxyType= SshMasterConnection::PROXYHTTP;
    QString proxyserver;
    int proxyport=22;
    QString proxylogin;
    QString proxypassword;
    QString proxyKey;
    bool proxyAutologin=false;
    bool proxyKrbLogin=false;

    if(brokerMode)
    {
        brokerSyncTimer->stop();
        ignoreBrokerSessions=true;
    }

    user=getCurrentUname();
    runRemoteCommand=true;
    if(!embedMode)
        shadowSession=false;
    applications.clear();
    removeAppsFromTray();

    if ( managedMode )
    {
        slotListSessions ( true, QString(),0 );
        return true;
    }

    X2goSettings* st;
    if(!brokerMode)
        st=new  X2goSettings( "sessions" );
    else
        st=new X2goSettings(config.iniFile, QSettings::IniFormat);

    passForm->setEnabled ( false );
    if(brokerMode)
    {
        if(connectionType==DEFAULT)
            host=config.serverIp;
        else
            host=suspendTerminateHostFromBroker;
        sshPort=config.sshport;
        x2goDebug<<"Server: "<<host;
        if(host.indexOf("No servers available")!=-1)
        {
            x2goDebug<<"There are no available X2Go servers for this connection";
            QMessageBox::critical (0l,tr("Error"),
                               tr ("There are no available X2Go Servers for this connection. Please try again later or choose another session."), QMessageBox::Ok,
                               QMessageBox::NoButton);

            setEnabled(true);
            passForm->setEnabled(true);
            startBrokerSyncTimer();
            return false;
        }
    }
    else if (embedMode)
    {
        host=config.server;
    }
    else
    {
        host=st->setting()->value ( sid+"/host",
                                    ( QVariant ) QString() ).toString();
    }

    QString cmd=st->setting()->value ( sid+"/command",
                                       ( QVariant ) QString() ).toString();
    autologin=st->setting()->value ( sid+"/autologin",
                                     ( QVariant ) false ).toBool();
    krblogin=st->setting()->value ( sid+"/krblogin",
                                    ( QVariant ) false ).toBool();
    krbDelegation=st->setting()->value ( sid+"/krbdelegation",
                                         ( QVariant ) false ).toBool();
#ifdef Q_OS_LINUX
    directRDP=(st->setting()->value ( sid+"/directrdp",
                                      ( QVariant ) false ).toBool() && cmd == "RDP");


    if (cmd =="RDP" && directRDP)
    {
        startDirectRDP();
        return true;
    }

    directRDP=(st->setting()->value ( sid+"/directxdmcp",
                                      ( QVariant ) false ).toBool() && cmd == "XDMCP");


    if (cmd =="XDMCP" && directRDP)
    {
        startDirectRDP();
        return true;
    }
#endif
    if ( cmd=="SHADOW" )
        shadowSession=true;
    passwd=getCurrentPass();
    if(brokerMode)
    {
        currentKey=config.key;
        sshPort=config.sshport;
    }
    if (sshConnection)
        delete sshConnection;

    if(currentKey.length()<=0)
    {
        currentKey=findSshKeyForServer(user, host, sshPort);
    }

    if (!embedMode) {
        useproxy = (st->setting ()->value (sid + "/usesshproxy",
                                           false).toBool ());

        QString prtype = (st->setting ()->value (sid + "/sshproxytype",
                                                 "SSH").toString ());
        if (prtype.toLower () == "http") {
            proxyType = SshMasterConnection::PROXYHTTP;
        }
        else {
            proxyType = SshMasterConnection::PROXYSSH;
        }

        proxylogin = (st->setting ()->value (sid + "/sshproxyuser",
                                             QString ()).toString ());

        proxyKey = (st->setting ()->value (sid + "/sshproxykeyfile",
                                           QString ()).toString ());
        proxyKey = expandHome (proxyKey);

        proxyserver = (st->setting ()->value (sid + "/sshproxyhost",
                                              QString ()).toString ());

        proxyport = (st->setting ()->value (sid + "/sshproxyport",
                                            22).toInt ());

        proxyAutologin = (st->setting ()->value (sid + "/sshproxyautologin",
                                                 false).toBool ());

        proxyKrbLogin = (st->setting ()->value (sid + "/sshproxykrblogin",
                                                false).toBool ());
    }
    else {
        useproxy = config.useproxy;
        proxyType = config.proxyType;
        proxylogin = config.proxylogin;
        proxyKey = config.proxyKey;
        proxyserver = config.proxyserver;
        proxyport = config.proxyport;
        proxyAutologin = config.proxyAutologin;
        proxyKrbLogin = config.proxyKrbLogin;
    }

    bool proxySamePass=(st->setting()->value (
                            sid+"/sshproxysamepass",
                            false
                        ).toBool() );
    bool proxySameUser (st->setting()->value (
                            sid+"/sshproxysameuser",
                            false
                        ).toBool() );
    bool proxySameKey=(st->setting()->value (
                            sid+"/sshproxysamekey",
                            false
                        ).toBool() );
    if(proxyKey.length()<=0 && proxyType==SshMasterConnection::PROXYSSH)
    {
        proxyKey=findSshKeyForServer(proxylogin, proxyserver, QString::number(proxyport));
    }
    if(proxySameKey)
    {
        proxyKey=currentKey;
    }

    if(proxySameUser)
        proxylogin=user;
    if(proxySamePass)
        proxypassword=passwd;
    else
    {
        if(useproxy && !proxyAutologin && !proxyKrbLogin && proxyKey.length()<=0)
        {
            bool ok;
            bool useBrokerPassForProxy=false;
            bool useBrokerUserForProxy=false;
            if(brokerMode)
            {
                useBrokerPassForProxy=(st->setting()->value (
                    sid+"/usebrokerpassforproxy", false
                ).toBool() );
                useBrokerUserForProxy=(st->setting()->value (
                    sid+"/usebrokeruserforproxy", false
                ).toBool() );
                if(useBrokerUserForProxy)
                {
                    proxylogin=config.brokerUser;
                }
            }
            if(useBrokerPassForProxy)
                proxypassword=config.brokerPass;
            else
                proxypassword=QInputDialog::getText(0,proxylogin+"@"+proxyserver+":"+QString::number(proxyport),
                                                    tr("Enter password for SSH proxy"),QLineEdit::Password,QString(), &ok);
        }
    }

    delete st;

    sshConnection=startSshConnection ( host,sshPort,acceptRsa,user,passwd,autologin, krblogin, false, useproxy,proxyType,proxyserver,
                                       proxyport, proxylogin, proxypassword, proxyKey,proxyAutologin, proxyKrbLogin);
    sshConnection->set_kerberosDelegation(krbDelegation);
    return true;
}


void ONMainWindow::slotListSessions ( bool result,QString output,
                                      int  )
{
    interDlg->hide();
    x2goDebug<<output;
    if ( result==false )
    {
        cardReady=false;
        cardStarted=false;
        QString message=tr ( "<b>Connection failed.</b>\n" ) +output;
        if ( message.indexOf ( "publickey,password" ) !=-1 )
        {
            message=tr ( "<b>Wrong password!</b><br><br>" ) +
                    message;
        }

        if ( !startHidden )
        {
            QMessageBox::critical ( 0l,tr ( "Error" ),message,
                                    QMessageBox::Ok,
                                    QMessageBox::NoButton );
        }
        else
        {
            QString printout = tr( "Connection failed: ")  + output.toLatin1();

            if ( output.indexOf ( "publickey,password" ) !=-1 )
                x2goErrorf(4)<< tr( "Connection failed: ")  + output + tr(" - Wrong password.");
            else
                x2goErrorf(5)<< tr( "Connection failed: ")  + output;
            trayQuit();
        }


// 		currentKey=QString();
        setEnabled ( true );
        passForm->setEnabled ( true );
        slotShowPassForm();
        pass->setFocus();
        pass->selectAll();
        return;
    }

    passForm->hide();
    if ( !embedMode )
    {
        setUsersEnabled ( false );
        uname->setEnabled ( false );
        u->setEnabled ( false );
    }
    if ( managedMode )
    {
        x2goDebug<<"Session data: " + config.sessiondata;
        if ( config.sessiondata.indexOf ( "|S|" ) ==-1 )
        {
            x2goDebug<<"Starting new managed session.";
            startNewSession();
        }
        else
        {
            x2goSession s=getSessionFromString (config.sessiondata);

            /* Check getSessionFromString for what this "invalid" string means. */
            if (s.agentPid == "invalid") {
                startNewSession ();
            }
            else {
                x2goDebug << "Resuming managed session with ID: " + s.sessionId;
                resumeSession (s);
            }
        }
        return;
    }

    QStringList sessions;
    if(!brokerMode || shadowSession)
    {
        sessions=output.trimmed().split ( '\n',
                                          Qt::SkipEmptyParts );
    }
    else
    {
        sessions=config.sessiondata.trimmed().split ( '\n',
                 Qt::SkipEmptyParts );
    }
    if ( shadowSession )
    {
        selectSession ( sessions );
    }
    else
    {
        if ( ( sessions.size() ==0 ) ||
                ( sessions.size() ==1&&sessions[0].length() <5 ) )
            startNewSession();
        else if ( sessions.size() ==1 )
        {
            x2goSession s=getSessionFromString ( sessions[0] );

            QDesktopWidget wd;

            //we already selected the session
            if(brokerMode)
            {
                resumeSession(s);
                return;
            }

            /* Check getSessionFromString for what this "invalid" string means. */
            if ((s.agentPid != "invalid") && (s.status == "S")
                && (isColorDepthOk (wd.depth (), s.colorDepth))
                && (s.command == selectedCommand)&&autoresume)
                resumeSession ( s );
            else
            {
                if ((startHidden) || (s.agentPid == "invalid"))
                    startNewSession();
                else
                    selectSession ( sessions );
            }
        }
        else
        {
            if ( !startHidden )
                selectSession ( sessions );
            else
            {
                for ( int i=0; i<sessions.size(); ++i )
                {
                    x2goSession s=getSessionFromString (
                                      sessions[i] );

                    QDesktopWidget wd;

                    /* Check getSessionFromString for what this "invalid" string means. */
                    if ((s.agentPid != "invalid") && (s.status == "S")
                        && (isColorDepthOk (wd.depth (), s.colorDepth))
                        && (s.command == selectedCommand))
                    {
                        resumeSession ( s );
                        return;
                    }
                }
                startNewSession();
            }
        }
    }
}


x2goSession ONMainWindow::getSessionFromString ( const QString& string )
{
    QStringList lst=string.split ( '|' );

    x2goSession s;

    /*
     * Skip over invalid sessions strings...
     * This can happen if the perl interpreter outputs warning or error messages
     * to stdout.
     */
    if (lst.size () < 10) {
        s.agentPid = "invalid";
        return (s);
    }

    s.agentPid=lst[0];
    s.sessionId=lst[1];
    s.display=lst[2];
    s.server=lst[3];
    s.status=lst[4];
    s.crTime=lst[5];
    s.cookie=lst[6];
    s.clientIp=lst[7];
    s.grPort=lst[8];
    s.sndPort=lst[9];
    if ( lst.count() >13 )
        s.fsPort=lst[13];
    if (brokerMode && lst.count()>14)
        s.brokerUser=lst[14];
    s.colorDepth=0;
    if ( s.sessionId.indexOf ( "_dp" ) !=-1 )
    {
        s.colorDepth=s.sessionId.split ( "_dp" ) [1].toInt();
    }
    s.sessionType=x2goSession::DESKTOP;
    s.command=tr ( "unknown" );
    if ( s.sessionId.indexOf ( "_st" ) !=-1 )
    {
        QString cmdinfo=s.sessionId.split ( "_st" ) [1];
        cmdinfo=cmdinfo.split ( "_" ) [0];
        QChar st=cmdinfo[0];
        if ( st=='R' )
            s.sessionType=x2goSession::ROOTLESS;
        if ( st=='S' )
            s.sessionType=x2goSession::SHADOW;
        if ( st=='K' )
        {
            s.sessionType=x2goSession::KDRIVE;
        }
        if ( st=='A')
        {
            s.sessionType=x2goSession::ROOTLESSKDRIVE;
        }
        QString command=cmdinfo.mid ( 1 );
        if ( command.length() >0 )
            s.command=command;
    }
    return s;
}


void ONMainWindow::sendEventToBroker(ONMainWindow::client_events ev)
{
    if(!config.brokerEvents)
    {
        return;
    }
    if(ev!=ALIVE)
        lastBrokerEvent=ev;
    lastBrokerEventSession=resumingSession.sessionId;
    QString event;
    switch(ev)
    {
        case CONNECTING:
        {
            event="CONNECTING";
            resumingSession.connectedSince=QDateTime::currentDateTime().toTime_t();
            break;
        }
        case CONNECTED:
        {
            event="CONNECTED";
            resumingSession.connectedSince=QDateTime::currentDateTime().toTime_t();
            if(config.brokerLiveEventsTimeout)
            {
                brokerAliveTimer->start(config.brokerLiveEventsTimeout*1000);
            }
            break;
        }
        case SUSPENDING:
        {
            event="SUSPENDING";
            break;
        }
        case TERMINATING:
        {
            event="TERMINATING";
            break;
        }
        case FINISHED:
        {
            event="FINISHED";
            brokerAliveTimer->stop();
            break;
        }
        case ALIVE:
        {
            event="ALIVE";
            break;
        }
    }
    QString sid=sessionExplorer->getLastSession()->id();
    X2goSettings st(config.iniFile, QSettings::IniFormat);
    if(st.setting()->value ( sid+"/command", ( QVariant ) defaultCmd ).toString()=="XDMCP")
    {
        resumingSession.command="XDMCP-"+st.setting()->value ( sid+"/xdmcpserver").toString();
    }

    broker->sendEvent(event, resumingSession.sessionId,resumingSession.server, resumingSession.clientIp, getCurrentUname(),
                      resumingSession.command, resumingSession.display, resumingSession.crTime,
                      QDateTime::currentDateTime().toTime_t()-resumingSession.connectedSince);
}


void ONMainWindow::startNewSession()
{
    newSession=true;
    QString passwd=getCurrentPass();
    QString user=getCurrentUname();

    QString pack;
    bool kdrive=false;
    bool fullscreen;
    int height;
    int width;
    int quality;
    int speed;
    bool usekbd;
    bool rootless=false;
    resumingSession.published=false;
    bool setDPI=defaultSetDPI;
    bool xinerama = false;
    uint dpi=defaultDPI;
    QString layout;
    QString type;
    QString command;
    QString clipMode=defaultClipboardMode;
    QString xdmcpServer;
    runRemoteCommand=true;
    QString host=QString();
    runStartApp=true;
    removeAppsFromTray();
    if ( useLdap )
    {
        pack=defaultPack;
        fullscreen=defaultFullscreen;
        height=defaultHeight;
        width=defaultWidth;
        quality=defaultQuality;
        speed=defaultLink;
        xinerama = defaultXinerama;
        usekbd=defaultSetKbd;
        layout=defaultLayout[0];
        type=defaultKbdType;
        command=defaultCmd;
        shadowSession=false;
        for ( int j=0; j<x2goServers.size(); ++j )
        {
            if ( x2goServers[j].connOk )
            {
                host=x2goServers[j].name;
                break;
            }
        }
        if ( host==QString() )
        {
            QMessageBox::critical ( 0l,tr ( "Error" ),
                                    tr ( "No server available." ),
                                    QMessageBox::Ok,
                                    QMessageBox::NoButton );
            return;
        }
        sshConnection=findServerSshConnection(host);
        if (!sshConnection)
        {
            QMessageBox::critical ( 0l,tr ( "Error" ),
                                    tr ( "Server not available." ),
                                    QMessageBox::Ok,
                                    QMessageBox::NoButton );
            return;
        }
        cleanServerSshConnections();
    }
    else
    {
        X2goSettings* st;

        if (!brokerMode)
            st=new X2goSettings( "sessions" );
        else
            st= new X2goSettings(config.iniFile,QSettings::IniFormat);

        QString sid;
        if ( !embedMode ) {
            if (!(sessionExplorer->getLastSession())) {
                x2goDebug << "No session selected, returning without starting a session.";

                delete st;

                return;
            }

            sid=sessionExplorer->getLastSession()->id();
        }
        else
            sid="embedded";
        pack=st->setting()->value ( sid+"/pack",
                                    ( QVariant ) defaultPack ).toString();
        fullscreen=st->setting()->value ( sid+"/fullscreen",
                                          ( QVariant )
                                          defaultFullscreen ).toBool();

        //if maxdim = true we set maximun display area available for the selected monitor
        if ( st->setting()->value(sid + "/maxdim", (QVariant) false).toBool())
        {
            int selectedScreen = st->setting()->value(sid + "/display", (QVariant) -1).toInt();
            height=QApplication::desktop()->availableGeometry(selectedScreen).height();
            width=QApplication::desktop()->availableGeometry(selectedScreen).width();
        }
        else if(st->setting()->value(sid + "/multidisp", (QVariant) false).toBool())
        {
            //workaround to avoid wm set windows maximized, which will break moving and resizing of window
            width=800;
            height=600;
        }
        else
        {
            height=st->setting()->value ( sid+"/height",
                                          ( QVariant ) defaultHeight ).toInt();
            width=st->setting()->value ( sid+"/width",
                                         ( QVariant ) defaultWidth ).toInt();
        }


        setDPI=st->setting()->value ( sid+"/setdpi",
                                      ( QVariant ) defaultSetDPI ).toBool();
        dpi=st->setting()->value ( sid+"/dpi",
                                   ( QVariant ) defaultDPI ).toUInt();
        quality=st->setting()->value (
                    sid+"/quality",
                    ( QVariant ) defaultQuality ).toInt();
        speed=st->setting()->value ( sid+"/speed",
                                     ( QVariant ) defaultLink ).toInt();

        xinerama = st->setting ()->value (sid + "/xinerama",
                                          static_cast<QVariant>(defaultXinerama)).toBool ();

        clipMode=st->setting()->value ( sid+"/clipboard",
                                        ( QVariant ) defaultClipboardMode ).toString();

        usekbd=st->setting()->value ( sid+"/usekbd",
                                      ( QVariant ) defaultSetKbd ).toBool();
        layout=st->setting()->value ( sid+"/layout",
                                      ( QVariant )
                                      defaultLayout[0] ).toString();
        type=st->setting()->value ( sid+"/type",
                                    ( QVariant )
                                    defaultKbdType ).toString();
        type.replace("\\","");
        type.replace("(","\\(");
        type.replace(")","\\)");
        if ( !embedMode )
        {
            command=st->setting()->value ( sid+"/command",
                                           ( QVariant ) defaultCmd ).toString();

            kdrive=st->setting()->value ( sid+"/kdrive",
                                           ( QVariant ) false ).toBool();

            host=st->setting()->value (
                     sid+"/host",
                     ( QVariant )
                     ( QString ) "localhost" ).toString();

            rootless=st->setting()->value ( sid+"/rootless",
                                            ( QVariant ) false ).toBool();
            resumingSession.published=st->setting()->value ( sid+"/published",
                                      ( QVariant ) false ).toBool();
            xdmcpServer=st->setting()->value ( sid+"/xdmcpserver",
                                               ( QVariant )
                                               "localhost" ).toString();
        }
        else
        {
            command=config.command;
            if ( command=="SHADOW" )
            {
                shadowSession=true;
                runRemoteCommand=false;
            }

            rootless= config.rootless;
            host=config.server;
            startEmbedded=false;
            resumingSession.published=config.published;
            if ( st->setting()->value ( sid+"/startembed",
                                        ( QVariant ) true ).toBool() )
            {
                startEmbedded=true;
                fullscreen=false;
                height=bgFrame->size().height()-stb->height();
                width=bgFrame->size().width();

                if ( height<0 ||width<0 )
                {
                    height=defaultHeight;
                    width=defaultWidth;
                }
            }
            if ( config.confConSpd )
                speed=config.conSpeed;
            if ( config.confCompMet )
                pack=config.compMet;
            if ( config.confImageQ )
                quality=config.imageQ;
            if ( config.confDPI )
            {
                dpi=config.dpi;
                setDPI=true;
            }
            if ( config.confKbd )
            {
                layout=config.kbdLay;
                type=config.kbdType;
                usekbd=true;
            }
        }
        resumingSession.command=command;
        if ( command=="RDP" )
        {
            if (fullscreen) {
                rootless=false;
            } else {
                rootless=true;
            }
        }
        if ( command=="XDMCP" )
        {
            runRemoteCommand=false;
        }
        proxyWinWidth=width;
        proxyWinHeight=height;
#ifdef Q_OS_WIN
        x2goDebug<<"Fullscreen: "<<fullscreen;
        maximizeProxyWin=false;
        xorgMode=WIN;
        if (fullscreen)
            xorgMode=FS;
        if (rootless)
            xorgMode=SAPP;
        xorgWidth=QString::number(width);
        xorgHeight=QString::number(height);
        uint displays=QApplication::desktop()->numScreens();
        if ((!rootless) && (st->setting()->value ( sid+"/multidisp", ( QVariant ) false ).toBool()))
        {
            xorgMode=MULTIDISPLAY;
            uint disp=st->setting()->value ( sid+"/display",( QVariant ) 1 ).toUInt();
            if (disp>displays)
            {
                disp=1;
            }
            localDisplayNumber=disp;
        }
        if(kdrive)
        {
            x2goDebug<<"KDRIVE session don't start X Server";
            slotSetWinServersReady();
        }
        if (! startXorgOnStart && ! kdrive)
            startXOrg();
#endif
        delete st;
    }

    if ( shadowSession )
    {
        runRemoteCommand=false;
    }


    resumingSession.server=host;

    if (defaultLayout.size()>0)
        layout=cbLayout->currentText();


    QString geometry;
    if ( fullscreen )
    {
        geometry="fullscreen";
#ifdef Q_OS_WIN
//        fullscreen=false;
        maximizeProxyWin=true;

        x2goDebug<<"Maximize proxy win: "<<maximizeProxyWin;

#endif
    }
    if ( ! fullscreen )
    {
        geometry=QString::number ( width ) +"x"+
                 QString::number ( height );
        if ( embedMode )
        {
            QPoint position=mapToGlobal ( bgFrame->pos() );
            geometry+="+"+QString::number ( position.x() ) +"+"+
                      QString::number ( position.y() +
                                        stb->height() );
        }

    }
    QString link;
    switch ( speed )
    {
    case MODEM:
        link="modem";
        break;
    case ISDN:
        link="isdn";
        break;
    case ADSL:
        link="adsl";
        break;
    case WAN:
        link="wan";
        break;
    case LAN:
        link="lan";
        break;
    }

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
            if ( pc==pack )
            {
                pack+="-"+QString::number ( quality );
                break;
            }
        }
    }
    file.close();


    if ( selectSessionDlg->isVisible() )
    {
        if ( !embedMode )
            slotCloseSelectDlg();
        else
            selectSessionDlg->hide();
    }
    QDesktopWidget wd;
    QString depth=QString::number ( wd.depth() );
#ifdef Q_OS_DARWIN
    usekbd=0;
    type="query";
#endif
    QString sessTypeStr="D ";
    if ( rootless )
        sessTypeStr="R ";
    if ( shadowSession )
        sessTypeStr="S ";
    if ( resumingSession.published)
    {
        sessTypeStr="P ";
        command="PUBLISHED";
    }
    if(kdrive)
        sessTypeStr="K ";
    if((rootless||resumingSession.published) && kdrive)
    {
        sessTypeStr="A ";
    }
    QString dpiEnv;
    QString xdmcpEnv;
    QString xinerama_env = "X2GO_XINERAMA=";
    if ( runRemoteCommand==false && command=="XDMCP" )
        xdmcpEnv="X2GOXDMCP="+xdmcpServer+" ";
    if ( setDPI )
    {
        dpiEnv="X2GODPI="+QString::number ( dpi ) +" ";
    }

    if (xinerama) {
      xinerama_env += "yes";
    }
    else {
      xinerama_env += "no";
    }

    if(usekbd && kdrive && type=="auto")
    {
        QString kbdl, kbdm, kbdv;
        getClientKeyboardConfig(kbdl,kbdm,kbdv);
        type=kbdm+"/"+kbdl;
        if(kbdv.length()>0)
        {
            type+="\\("+kbdv+"\\)";
        }
    }
    QString cmd=dpiEnv+xdmcpEnv+ xinerama_env + " x2gostartagent "+
                geometry+" "+link+" "+pack+
                " unix-kde-depth_"+depth+" "+layout+" "+type+" ";
    if ( usekbd )
        cmd += "1 ";
    else
        cmd += "0 ";
    QFileInfo f ( command );
    if ( !shadowSession ) {
        cmd+=sessTypeStr+f.fileName();
        cmd+=" "+clipMode;
    }
    else
    {
        cmd+=sessTypeStr+QString::number ( shadowMode ) +"XSHAD"+
             shadowUser+"XSHAD"+shadowDisplay;
    }

    resumingSession.fullscreen=fullscreen;

    x2goDebug<<"Executing remote command: "<<cmd;

    if ( managedMode )
    {
        slotRetResumeSess ( true,config.sessiondata,0 );
        passForm->hide();
        return;
    }

    sshConnection->executeCommand ( cmd, this, SLOT ( slotRetResumeSess ( bool,
                                    QString,int ) ) );
    passForm->hide();
}


void ONMainWindow::resumeSession ( const x2goSession& s )
{
    newSession=false;
    runStartApp=false;
    applications.clear();
    removeAppsFromTray();
    QString passwd=getCurrentPass();
    QString user=getCurrentUname();
    QString host=s.server;
    bool rootless=false;

    QString clipMode=defaultClipboardMode;

    QString pack;
    bool fullscreen;
    int height;
    int width;
    int quality;
    int speed;
    bool xinerama = false;
    bool usekbd;
    QString layout;
    QString type;
    removeAppsFromTray();

    if ( useLdap )
    {
        pack=defaultPack;
        fullscreen=defaultFullscreen;
        height=defaultHeight;
        width=defaultWidth;
        quality=defaultQuality;
        speed=defaultLink;
        xinerama = defaultXinerama;
        usekbd=defaultSetKbd;
        layout=defaultLayout[0];
        type=defaultKbdType;
        sshConnection=findServerSshConnection(host);
        if (!sshConnection)
        {
            QMessageBox::critical ( 0l,tr ( "Error" ),
                                    tr ( "Server not available." ),
                                    QMessageBox::Ok,
                                    QMessageBox::NoButton );
            return;
        }
        cleanServerSshConnections();
    }
    else
    {

        QString sid;
        if ( !embedMode ) {
            if (!(sessionExplorer->getLastSession())) {
                x2goDebug << "No session selected, returning without resuming a session.";
                return;
            }

            sid=sessionExplorer->getLastSession()->id();
        }
        else
            sid="embedded";
        X2goSettings* st;
        if (!brokerMode)
            st=new X2goSettings( "sessions" );
        else
            st=new X2goSettings(config.iniFile,QSettings::IniFormat);

        pack=st->setting()->value ( sid+"/pack",
                                    ( QVariant ) defaultPack ).toString();

        fullscreen=st->setting()->value ( sid+"/fullscreen",
                                          ( QVariant )
                                          defaultFullscreen ).toBool();
        height=st->setting()->value ( sid+"/height",
                                      ( QVariant ) defaultHeight ).toInt();
        width=st->setting()->value ( sid+"/width",
                                     ( QVariant ) defaultWidth ).toInt();
        quality=st->setting()->value ( sid+"/quality",
                                       ( QVariant )
                                       defaultQuality ).toInt();
        speed=st->setting()->value ( sid+"/speed",
                                     ( QVariant ) defaultLink ).toInt();

        xinerama = st->setting ()->value (sid + "/xinerama",
                                          static_cast<QVariant>(defaultXinerama)).toBool ();

        clipMode=st->setting()->value ( sid+"/clipboard",
                                        ( QVariant ) defaultClipboardMode ).toString();

        usekbd=st->setting()->value ( sid+"/usekbd",
                                      ( QVariant ) defaultSetKbd ).toBool();
        layout=st->setting()->value ( sid+"/layout",
                                      ( QVariant )
                                      defaultLayout[0] ).toString();
        type=st->setting()->value ( sid+"/type",
                                    ( QVariant )
                                    defaultKbdType ).toString();
        rootless=st->setting()->value ( sid+"/rootless",
                                        ( QVariant ) false ).toBool();

        if ( brokerMode )
        {
            host = config.serverIp;
        }
        else if ( embedMode )
        {
            startEmbedded=false;
            if ( st->setting()->value ( sid+"/startembed",
                                        ( QVariant ) true ).toBool() )
            {
                fullscreen=false;
                startEmbedded=true;
                height=bgFrame->size().height()-stb->height();
                width=bgFrame->size().width();
                if ( height<0 ||width<0 )
                {
                    height=defaultHeight;
                    width=defaultWidth;
                }
            }
            rootless=config.rootless;
            host=config.server;
            if ( config.confConSpd )
                speed=config.conSpeed;
            if ( config.confCompMet )
                pack=config.compMet;
            if ( config.confImageQ )
                quality=config.imageQ;
            if ( config.confKbd )
            {
                layout=config.kbdLay;
                type=config.kbdType;
                usekbd=true;
            }
        }
        else
        {
            host=st->setting()->value ( sid+"/host",
                                        ( QVariant ) s.server ).toString();
        }
        delete st;
    }

    if (defaultLayout.size()>0)
        layout=cbLayout->currentText();

    QString geometry;
    proxyWinWidth=width;
    proxyWinHeight=height;

#ifdef Q_OS_WIN
    maximizeProxyWin=false;
// #ifdef CFGCLIENT
    xorgMode=WIN;
    if (fullscreen)
        xorgMode=FS;
    if (rootless)
        xorgMode=SAPP;
    xorgWidth=QString::number(width);
    xorgHeight=QString::number(height);
    if(s.sessionType == x2goSession::KDRIVE || s.sessionType == x2goSession::ROOTLESSKDRIVE)
    {
        x2goDebug<<"KDRIVE session, don't start X-Server";
        slotSetWinServersReady();
    }
    if (! startXorgOnStart  && (s.sessionType != x2goSession::KDRIVE) && (s.sessionType != x2goSession::ROOTLESSKDRIVE))
        startXOrg();
// #endif
#else /* defined (Q_OS_WIN) */
    UNUSED (rootless);
#endif /* defined (Q_OS_WIN) */

    if ( fullscreen )
    {
        geometry="fullscreen";
#ifdef Q_OS_WIN
//        fullscreen=false;
        maximizeProxyWin=true;

        x2goDebug<<"Maximize proxy win: "<<maximizeProxyWin;

#endif
    }
    if ( !fullscreen )
    {
        geometry=QString::number ( width ) +"x"+
                 QString::number ( height );
    }
    resumingSession.fullscreen=fullscreen;
    QString link;
    switch ( speed )
    {
    case MODEM:
        link="modem";
        break;
    case ISDN:
        link="isdn";
        break;
    case ADSL:
        link="adsl";
        break;
    case WAN:
        link="wan";
        break;
    case LAN:
        link="lan";
        break;
    }

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
            if ( pc==pack )
            {
                pack+="-"+QString::number ( quality );
                break;
            }
        }
    }
    file.close();

#ifdef Q_OS_DARWIN
    usekbd=0;
    type="query";
#endif

    if (s.sessionId.indexOf("RPUBLISHED")!=-1 || s.sessionId.indexOf("APUBLISHED")!=-1)
    {
        resumingSession.published=true;
        sbApps->setDisabled(true);
    }
    else
        resumingSession.published=false;



    if ( selectSessionDlg->isVisible() )
    {
        if ( !embedMode )
            slotCloseSelectDlg();
        else
            selectSessionDlg->hide();
    }
    if(usekbd && (s.sessionType == x2goSession::KDRIVE || s.sessionType == x2goSession::ROOTLESSKDRIVE) && type=="auto")
    {
        QString kbdl, kbdm, kbdv;
        getClientKeyboardConfig(kbdl,kbdm,kbdv);
        type=kbdm+"/"+kbdl;
        if(kbdv.length()>0)
        {
            type+="\\("+kbdv+"\\)";
        }
    }
    QString cmd="x2goresume-session "+s.sessionId+" "+geometry+
                " "+link+" "+pack+" "+layout+
                " "+type+" ";
    if ( usekbd )
        cmd += "1";
    else
        cmd += "0";
    cmd +=" "+clipMode;

    QString xinerama_opt;
    if (xinerama) {
      xinerama_opt = "yes";
    }
    else {
      xinerama_opt = "no";
    }

    cmd += " " + xinerama_opt;

    sshConnection->executeCommand ( cmd, this,  SLOT ( slotRetResumeSess ( bool, QString,
                                    int ) ));
    resumingSession=s;
    passForm->hide();
}

/**
 * @brief ONMainWindow::setTrayIconToSessionIcon
 * @param info: message to be displayed in tray icon message
 *
 * Sets the tray session icon picture as the tray icon picture and
 * shows a tray icon information message about what X2Go Client
 * is currently doing.
 *
 * This message gives the user some feedback while the X2Go session
 * window is displayed.
 *
 */
void ONMainWindow::setTrayIconToSessionIcon(QString info) {

    //set session icon to tray icon
    if (trayIcon && sessionExplorer->getLastSession()) {

        X2goSettings* st;

        if (!brokerMode)
            st=new X2goSettings( "sessions" );
        else
            st= new X2goSettings(config.iniFile,QSettings::IniFormat);

        QString sid;
        if ( !embedMode )
            sid=sessionExplorer->getLastSession()->id();
        else
            sid="embedded";

        if (!keepTrayIcon) {
            QString sessIcon = wrap_legacy_resource_URIs (expandHome(st->setting()->value(sid + "/icon",
                                        (QVariant) QString(iconsPath("/128x128/x2go.png"))).toString()));
            if (!brokerMode || sessIcon == iconsPath("/128x128/x2gosession.png"))
            {
                trayIcon->setIcon(QIcon (sessIcon));
            }
            else
            {
                QPixmap pix;
                if(sessIcon.indexOf("file://")!=-1)
                {
                    //load icon from file URL
                    pix.load(sessIcon.replace("file://",""));
                }
                else
                    pix.loadFromData(QByteArray::fromBase64(sessIcon.toLatin1()));
                if(pix.isNull())
                {
                    //loading icon has failed, load default icon
                    pix.load(iconsPath("/128x128/x2gosession.png"));
                }
                trayIcon->setIcon(QIcon (pix));
            }
        }

        QString name=st->setting()->value ( sid +"/name").toString() ;

        //send a information notification about the connection is done
        trayIcon->showMessage("X2Go - " + name, info, QSystemTrayIcon::Information, 15000);

        delete (st);
    }

}

void ONMainWindow::selectSession ( QStringList& sessions )
{
    setEnabled ( true );
    sessionStatusDlg->hide();
    passForm->hide();

    if ( !shadowSession )
    {
        x2goDebug<<"No shadow session.";
        if ( !miniMode )
            selectSesDlgLayout->setContentsMargins ( 25,25,10,10 );

        bNew->show();
        bSusp->show();
        bTerm->show();
        sOk->show();
        sCancel->show();

        desktopFilter->hide();
        desktopFilterCb->hide();
        bShadow->hide();
        bShadowView->hide();
        bCancel->hide();

// 		model->clear();
        model->removeRows ( 0,model->rowCount() );
        selectSessionLabel->setText ( tr ( "Select session:" ) );
        selectedSessions.clear();
        QFontMetrics fm ( sessTv->font() );
        for ( int row = 0; row < sessions.size(); ++row )
        {
            x2goDebug<<"Decoding session string:" + sessions[row];
            x2goSession s=getSessionFromString ( sessions[row] );

            /* Check getSessionFromString for what this "invalid" string means. */
            if (s.agentPid == "invalid") {
                continue;
            }

            selectedSessions.append ( s );
            QStandardItem *item;

            if(!brokerMode)
                item= new QStandardItem ( s.display );
            else
                item= new QStandardItem ( s.brokerUser );
            model->setItem ( row,S_DISPLAY,item );

            if ( s.status=="R" )
                item= new QStandardItem ( tr ( "running" ) );
            else
                item= new QStandardItem ( tr ( "suspended" ) );
            model->setItem ( row,S_STATUS,item );

            item= new QStandardItem ( transAppName ( s.command ) );
            model->setItem ( row,S_COMMAND,item );

            QString type=tr ( "Desktop" );
            if ( s.sessionType==x2goSession::ROOTLESS )
                type=tr ( "single application" );
            if ( s.sessionType==x2goSession::SHADOW )
                type=tr ( "shadow session" );
            if ( s.sessionType==x2goSession::KDRIVE )
                type=tr ( "X2GoKDrive session" );
            if ( s.sessionType==x2goSession::ROOTLESSKDRIVE )
                type=tr ( "X2GoKDrive single application" );


            item= new QStandardItem ( type );
            model->setItem ( row,S_TYPE,item );

            QString formatted_time = QDateTime::fromString (s.crTime, Qt::ISODate).toString ();
            item = new QStandardItem (formatted_time);
            model->setItem ( row,S_CRTIME,item );
            item= new QStandardItem ( s.server );
            model->setItem ( row,S_SERVER,item );
            item= new QStandardItem ( s.clientIp );
            model->setItem ( row,S_IP,item );
            item= new QStandardItem ( s.sessionId );
            model->setItem ( row,S_ID,item );
            for ( int j=0; j<8; ++j )
            {
                QString txt=
                    model->index ( row,j ).data().toString();
                if ( sessTv->header()->sectionSize ( j ) <
                        fm.width ( txt ) +6 )
                {
                    sessTv->header()->resizeSection (
                        j,fm.width ( txt ) +6 );
                }
            }
        }
    }
    else
    {
        shadowMode=SHADOW_VIEWONLY;
        selectedDesktops.clear();
        selectedDesktops=sessions;
        if ( sessions.size() ==0 )
        {
            QMessageBox::information ( this,tr ( "Information" ),
                                       tr ( "No accessible desktop "
                                            "found." ) );
            slotCloseSelectDlg();
            return;
        }
        sessTv->setModel ( ( QAbstractItemModel* ) modelDesktop );
        desktopFilter->show();
        desktopFilterCb->show();
        sOk->hide();
        sCancel->hide();
        bShadow->show();
        bCancel->show();
        bShadowView->show();
        desktopFilter->setText ( tr ( "Filter" ) );
        sessions.sort();
        if ( !miniMode )
            selectSesDlgLayout->setContentsMargins ( 25,25,25,25 );
        bNew->hide();
        bSusp->hide();
        bTerm->hide();
        selectSessionLabel->setText ( tr ( "Select desktop:" ) );
        filterDesktops ( "" );
        desktopFilter->setFocus();
        desktopFilter->selectAll();
        if ( !embedMode )
        {
            X2goSettings* st;

            if (!brokerMode)
            {
                if (!(sessionExplorer->getLastSession())) {
                    x2goDebug << "No session selected, returning without starting a shadow session.";
                    return;
                }

                st=new X2goSettings( "sessions" );

                QString sid=sessionExplorer->getLastSession()->id();
                QString suser = st->setting()->value(sid + "/shadowuser", (QVariant) QString()).toString();
                QString sdisplay = st->setting()->value(sid + "/shadowdisplay", (QVariant) QString()).toString();
                bool fullAccess= st->setting()->value(sid + "/shadowfullaccess", (QVariant) false).toBool();

                delete (st);

                if(suser != QString() && sdisplay != QString())
                {
                    shadowUser=suser;
                    shadowDisplay=sdisplay;
                    if(fullAccess)
                        shadowMode=SHADOW_FULL;
                    else
                        shadowMode=SHADOW_VIEWONLY;
                    startNewSession();
                    return;
                }
            }
        }
    }


    sessTv->setCurrentIndex ( sessTv->model()->index ( 0, 0 ) );
    sessTv->setFocus();
    selectSessionDlg->show();

    if( startHidden && shadowSession && sessTv->model()->rowCount() == 1 )
        slotShadowSess();
}

void ONMainWindow::slotCloseSelectDlg()
{
    selectSessionDlg->hide();
    if ( !embedMode )
    {
        u->setEnabled ( true );
        uname->setEnabled ( true );
    }
    slotShowPassForm();
}



void ONMainWindow::slotActivated ( const QModelIndex& index )
{
    if ( !shadowSession )
    {
        QString status=sessTv->model()->index (
                           index.row(),
                           S_STATUS ).data().toString();
        if ( status==tr ( "running" ) )
        {
            bSusp->setEnabled ( true );
            sOk->setEnabled ( false );
            running_label->show ();
        }
        else
        {
            bSusp->setEnabled ( false );
            sOk->setEnabled ( true );
            running_label->hide ();
        }
        bTerm->setEnabled ( true );
        if ( status==QString() )
        {
            sOk->setEnabled ( false );
            bTerm->setEnabled ( false );
            running_label->hide ();
        }
    }
    else
    {
        QString user=sessTv->model()->index (
                         index.row(),
                         D_USER ).data().toString();
        bShadowView->setEnabled ( true );
        bShadow->setEnabled ( true );
    }
}


void ONMainWindow::slotResumeSess()
{
    x2goSession s=getSelectedSession();
    if(brokerMode)
    {
        broker->resumeSession(s.sessionId, s.server);
        return;
    }
    QDesktopWidget wd;
    if ( isColorDepthOk ( wd.depth(),s.colorDepth ) )
    {
        if ( s.status=="R" && ! resumeAfterSuspending)
        {
            resumeAfterSuspending=true;
            slotSuspendSess();
            return;
        }
        resumeAfterSuspending=false;
        resumeSession ( s );
    }
    else
    {
        int res;
        res=QMessageBox::warning (
                0l,tr ( "Warning" ),
                tr ("Your current color depth is different from "
                    "the session's color depth. This may cause "
                    "problems reconnecting to this session and "
                    "in most cases <b>you will lose the "
                    "session</b> and have to start a new one! ")
              + tr ("It's highly recommended to change the color "
                    "depth of your display to %n bit(s) and "
                    "restart your X.Org Server before you "
                    "reconnect to this X2Go session.",
                    "%n will be replaced with a number",
                    s.colorDepth)
              + tr ("<br />"
                    "Do you want to resume this session anyway?"),
                tr ( "Yes" ),
                tr ( "No" ) );
        if ( res==0 )
            resumeSession ( s );
    }

}

void ONMainWindow::suspendFromBroker(const QString& sid)
{
    if(proxyRunning && sid ==resumingSession.sessionId)
    {
        x2goDebug<<"Suspending session from broker";
        sendEventToBroker(SUSPENDING);
        suspendSession(sid);
    }
}


void ONMainWindow::terminateFromBroker(const QString& sid)
{
    if(proxyRunning && sid ==resumingSession.sessionId)
    {
        x2goDebug<<"Suspending session from broker";
        sendEventToBroker(TERMINATING);
        termSession(sid,false);
    }
}


void ONMainWindow::slotSuspendSess()
{

#ifdef Q_OS_LINUX
    if (directRDP)
    {
        nxproxy->terminate();
        proxyRunning=false;
        return;
    }
#endif
    QString passwd;
    QString user=getCurrentUname();

    passwd=getCurrentPass();

    selectSessionDlg->setEnabled ( false );


    QString sessId=sessTv->model()->index (
                       sessTv->currentIndex().row(),
                       S_ID ).data().toString();
    QString host=sessTv->model()->index (
                     sessTv->currentIndex().row(),
                     S_SERVER ).data().toString();


    if ( !useLdap )
    {
        if ( !brokerMode )
        {
            if ( embedMode )
            {
                host=config.server;
            }
            else
            {
                X2goSettings st ( "sessions" );

                if (!(sessionExplorer->getLastSession())) {
                    x2goDebug << "No session selected, returning without suspending a session.";
                    return;
                }

                QString sid=sessionExplorer->getLastSession()->id();
                host=st.setting()->value ( sid+"/host",
                                           ( QVariant ) host ).toString();
            }
        }
    }
    else
    {
        sshConnection=findServerSshConnection(host);
        if (!sshConnection)
        {
            QMessageBox::critical ( 0l,tr ( "Error" ),
                                    tr ( "Server not available." ),
                                    QMessageBox::Ok,
                                    QMessageBox::NoButton );
            return;
        }
    }

    if(brokerMode)
    {
       suspendBrokerSession(sessId, host);
    }
    else
    {
       suspendSession ( sessId );
    }
}


void ONMainWindow::slotSuspendSessFromSt()
{

    if(brokerMode)
    {
        sendEventToBroker(SUSPENDING);
    }
#ifdef Q_OS_LINUX
    if (directRDP)
    {
        nxproxy->terminate();
        proxyRunning=false;
        return;
    }
#endif
    QString passwd;
    QString user=getCurrentUname();
    passwd=getCurrentPass();
    setStatStatus ( tr ( "suspending" ) );


    sbExp->setEnabled ( false );

    if ( !shadowSession )
        suspendSession ( resumingSession.sessionId );
    else
        termSession ( resumingSession.sessionId,false );
}

void ONMainWindow::slotTermSessFromSt()
{
#ifdef Q_OS_LINUX
    if (directRDP)
    {

        if(brokerMode)
        {
            sendEventToBroker(TERMINATING);
        }
        x2goDebug<<"Terminating direct RDP session.";

        nxproxy->terminate();
        proxyRunning=false;
        return;
    }
#endif


    if ( !shadowSession )
    {
        if ( termSession ( resumingSession.sessionId ) )
        {
            sbExp->setEnabled ( false );
            if(brokerMode)
            {
                sendEventToBroker(TERMINATING);
            }
            setStatStatus ( tr ( "terminating" ) );
        }
    }
    else
    {
        sbExp->setEnabled ( false );
        if(brokerMode)
        {
            sendEventToBroker(TERMINATING);
        }
        termSession ( resumingSession.sessionId,false );
    }
}


void ONMainWindow::slotRetSuspSess ( bool result, QString output,
                                     int )
{
    setEnabled ( true );
    if ( result==false )
    {
        QString message=tr ( "<b>Connection failed.</b>\n" ) +output;
        if ( message.indexOf ( "publickey,password" ) !=-1 )
        {
            message=tr (
                        "<b>Wrong password!</b><br><br>" ) +message;
        }

        QMessageBox::critical ( 0l,tr ( "Error" ),message,
                                QMessageBox::Ok,
                                QMessageBox::NoButton );
    }
    else
    {
        if ( selectSessionDlg->isVisible() )
        {
            ( ( QStandardItemModel* )
              ( sessTv->model() ) )->item (
                  sessTv->currentIndex().row(),
                  S_STATUS )->setData (
                      QVariant ( ( QString ) tr ( "suspended" ) ),
                      Qt::DisplayRole );
            bSusp->setEnabled ( false );
            sOk->setEnabled ( true );
            running_label->hide ();
        }
    }
    if ( selectSessionDlg->isVisible() )
        selectSessionDlg->setEnabled ( true );
    if (resumeAfterSuspending)
    {
        slotResumeSess();
    }
    else
    {
        startBrokerSyncTimer();
    }
}



void ONMainWindow::slotTermSess()
{

#ifdef Q_OS_LINUX
    if (directRDP)
    {
        nxproxy->terminate();
        proxyRunning=false;
        return;
    }
#endif

    selectSessionDlg->setEnabled ( false );


    QString sessId=sessTv->model()->index ( sessTv->currentIndex().row(),
                                            S_ID ).data().toString();
    QString host=sessTv->model()->index (
                                            sessTv->currentIndex().row(),
                                            S_SERVER ).data().toString();

    if ( !useLdap )
    {
        if ( !embedMode )
        {
            X2goSettings st ( "sessions" );

            if (!(sessionExplorer->getLastSession())) {
                x2goDebug << "No session selected, returning without terminating a session.";
                return;
            }

            QString sid=sessionExplorer->getLastSession()->id();
        }
    }
    else
    {
        sshConnection=findServerSshConnection(host);
        if (!sshConnection)
        {
            QMessageBox::critical ( 0l,tr ( "Error" ),
                                    tr ( "Server not available." ),
                                    QMessageBox::Ok,
                                    QMessageBox::NoButton );
            return;
        }
    }
    if(brokerMode)
    {
        termBrokerSession(sessId, host);
    }
    else
    {
        termSession ( sessId );
    }
}


void ONMainWindow::slotNewSess()
{
    startNewSession();
}


void ONMainWindow::slotRetTermSess ( bool result,  QString output,
                                     int )
{
    setEnabled ( true );
    if ( result==false )
    {
        QString message=tr ( "<b>Connection failed.</b>\n" ) +output;
        if ( message.indexOf ( "publickey,password" ) !=-1 )
        {
            message=tr (
                        "<b>Wrong password!</b><br><br>" ) +message;
        }

        QMessageBox::critical ( 0l,tr ( "Error" ),
                                message,QMessageBox::Ok,
                                QMessageBox::NoButton );
    }
    else
    {
        if ( selectSessionDlg->isVisible()  )
        {
            sessTv->model()->removeRow (
                sessTv->currentIndex().row() );
            slotActivated ( sessTv->currentIndex() );
        }
    }
    if ( selectSessionDlg->isVisible() )
        selectSessionDlg->setEnabled ( true );
    startBrokerSyncTimer();
}

void ONMainWindow::slotRetResumeSess ( bool result,
                                       QString output,
                                       int )
{

    x2goDebug<<"Agent output: "<<output;

    if ( result==false )
    {
        QString message=tr ( "<b>Connection failed.</b>\n" ) +output;
        if ( message.indexOf ( "publickey,password" ) !=-1 )
        {
            message=tr (
                        "<b>Wrong Password!</b><br><br>" ) +message;
        }
        if ( output.indexOf ( "LIMIT" ) !=-1 )
        {
            QString sessions=output.mid (
                                 output.indexOf ( "LIMIT" ) +6 );

            message="Sessions limit reached:"+sessions;
        }
        if ( output.indexOf ( "ACCESS DENIED" ) !=-1 )
        {
            message="Access denied for user";
        }

        QMessageBox::critical ( 0l,tr ( "Error" ),
                                message,QMessageBox::Ok,
                                QMessageBox::NoButton );
        slotShowPassForm();
        return;
    }

    output.replace ( " ","" );
    QString passwd=getCurrentPass();
    QString user=getCurrentUname();
    QString host;

    bool sound=true;
    bool kdrive=false;
    bool rootless=false;
    int sndSystem=PULSE;
    QString sndPort;
#if !defined (Q_OS_WIN) && !defined (Q_OS_DARWIN)
    sndPort="4713";
#endif /* !defined (Q_OS_WIN) && !defined (Q_OS_DARWIN) */
    bool startSoundServer=true;
    bool sshSndTunnel=true;

    if ( useLdap )
    {
        sound=startSound;
        startSoundServer=LDAPSndStartServer;
        if ( LDAPSndSys=="arts" )
            sndSystem=ARTS;
        if ( LDAPSndSys=="esd" )
            sndSystem=ESD;
        sndPort=LDAPSndPort;
    }
    else
    {
        QString sid;
        if ( !embedMode ) {
            if (!(sessionExplorer->getLastSession())) {
                x2goDebug << "No session selected, this should not happen since we already started resuming a session.";
                return;
            }

            sid=sessionExplorer->getLastSession()->id();
        }
        else
            sid="embedded";

        X2goSettings* st;
        if (!brokerMode)
            st= new X2goSettings( "sessions" );
        else
            st= new X2goSettings(config.iniFile,QSettings::IniFormat);

        kdrive=st->setting()->value ( sid+"/kdrive",
                                     ( QVariant ) false ).toBool();

        rootless=st->setting()->value ( sid+"/rootless",
                                     ( QVariant ) false ).toBool();

        sound=st->setting()->value ( sid+"/sound",
                                     ( QVariant ) true ).toBool();
        QString sndsys=st->setting()->value (
                           sid+"/soundsystem",
                           ( QVariant ) "pulse" ).toString();
        if ( sndsys=="arts" )
            sndSystem=ARTS;
        if ( sndsys=="esd" )
            sndSystem=ESD;
#if !defined (Q_OS_WIN) && !defined (Q_OS_DARWIN)
        sndPort=st->setting()->value ( sid+"/sndport" ).toString();
#endif /* !defined (Q_OS_WIN) && !defined (Q_OS_DARWIN) */
        startSoundServer=st->setting()->value (
                             sid+"/startsoundsystem",
                             true ).toBool();

        if ( embedMode&&config.confSnd )
        {
            sound=config.useSnd;
        }


#if !defined (Q_OS_WIN) && !defined (Q_OS_DARWIN)
        bool defPort=st->setting()->value ( sid+
                                            "/defsndport",true ).toBool();
        if ( defPort )
        {
            switch ( sndSystem )
            {
            case PULSE:
                sndPort="4713";
                break;
            case ESD:
                sndPort="16001";
                break;
            }
        }
#endif /* !defined (Q_OS_WIN) && !defined (Q_OS_DARWIN) */
        sshSndTunnel=st->setting()->value ( sid+"/soundtunnel",
                                            true ).toBool();

#if defined (Q_OS_WIN) || defined (Q_OS_DARWIN)
        if (sound)
        {
            if(!pulseManager || !(pulseManager->is_server_running()))
            {
                X2goSettings st ("settings");
                bool disablePA = st.setting ()->value ("pulse/disable",
                                                   (QVariant) false).toBool ();
                if(!systemDisablePA && ! disablePA)
                {
                    show_RichText_WarningMsgBox (tr("PulseAudio is not running"),
                                                 tr("Sound output will be disabled for this session. Please enable PulseAudio in the configuration dialog or disable sound in the session settings"),true);
                }
                sound=false;
            }
            else
            {
                switch (sndSystem)
                {
                    case PULSE:
                        sndPort = QString::number (pulseManager->get_pulse_port ());
                    break;
                    case ESD:
                        sndPort = QString::number (pulseManager->get_esd_port ());
                    break;
               }
            }
        }

#endif /* defined (Q_OS_WIN) || defined (Q_OS_DARWIN) */

        delete st;

    }

    //Will be used in runCommand
    startSessSound=sound;
    startSessSndSystem=sndSystem;

    if ( newSession )
    {
        QString sString=output.trimmed();
        sString.replace ( '\n','|' );
        host=resumingSession.server;
        resumingSession=getNewSessionFromString ( sString );
        if(kdrive)
        {
            resumingSession.sessionType=x2goSession::KDRIVE;
        }
        if(kdrive&&(rootless||resumingSession.published))
        {
            resumingSession.sessionType=x2goSession::ROOTLESSKDRIVE;
        }
        resumingSession.server=host;
        resumingSession.crTime=QDateTime::currentDateTime().toString (
                                   Qt::ISODate );
        if ( managedMode )
        {
            //replace session data for future resuming
            config.sessiondata=resumingSession.agentPid+"|"+
                               resumingSession.sessionId+"|"+
                               resumingSession.display+"|"+
                               resumingSession.server+"|"+
                               "S|"+
                               resumingSession.crTime+"|"+
                               resumingSession.cookie+"|"+
                               resumingSession.clientIp+"|"+
                               resumingSession.grPort+"|"+
                               resumingSession.sndPort+"|"+
                               resumingSession.crTime+"|"+
                               user+"|"+
                               "0|"+
                               resumingSession.fsPort;
        }

        //change the trayicon picture
        if (debugging) {
            setTrayIconToSessionIcon(tr("New session started") + ": " + resumingSession.sessionId);
        } else {
            setTrayIconToSessionIcon(tr("New session started") + ".");
        }

    }
    else
    {

        if(resumingSession.sessionType==x2goSession::KDRIVE||resumingSession.sessionType==x2goSession::ROOTLESSKDRIVE)
        {
            qDebug()<<"resuming kdrive session";
        }
        else
        {
            qDebug()<<"resuming normal session";
        }
        host=resumingSession.server;
        QStringList outputLines=output.split("\n",Qt::SkipEmptyParts);
        foreach(QString line,outputLines)
        {
            if (line.indexOf("gr_port=")!=-1)
            {
                resumingSession.grPort=line.replace("gr_port=","");

                x2goDebug<<"New gr_port: "<<resumingSession.grPort;

            }
            if (line.indexOf("sound_port=")!=-1)
            {
                resumingSession.sndPort=line.replace("sound_port=","");

                x2goDebug<<"New sound_port: "<<resumingSession.sndPort;

            }
            if (line.indexOf("fs_port=")!=-1)
            {
                resumingSession.fsPort=line.replace("fs_port=","");

                x2goDebug<<"New fs_port: "<<resumingSession.fsPort;

            }
        }
        if (resumingSession.published)
            readApplications();

        //change the trayicon picture
        if (debugging) {
            setTrayIconToSessionIcon(tr("Session resumed") + ": " + resumingSession.sessionId);
        } else {
            setTrayIconToSessionIcon(tr("Session resumed") + ".");
        }

    }
    if ( !useLdap )
    {
        if ( brokerMode )
        {
            host=config.serverIp;
        }
        else if ( embedMode )
        {
            host=config.server;
        }
        else
        {
            X2goSettings st ( "sessions" );

            QString sid=sessionExplorer->getLastSession()->id();
            host=st.setting()->value ( sid+"/host",
                                       ( QVariant ) host ).toString();
        }
        resumingSession.server=host;
    }


    localGraphicPort=resumingSession.grPort;
    int iport=localGraphicPort.toInt() +1000;
    while ( iport == resumingSession.sndPort.toInt() ||
            iport == resumingSession.fsPort.toInt() ||
            isServerRunning ( iport ) )
        ++iport;
    localGraphicPort=QString::number ( iport );

    sshConnection->startTunnel ( "localhost",resumingSession.grPort.toInt(),"localhost",
                                 localGraphicPort.toInt(), false, this,  SLOT ( slotTunnelOk(int) ), SLOT ( slotTunnelFailed ( bool,
                                         QString,int ) ) );
    if ( shadowSession )
        return;

    sndTunnel=0l;
    if ( sound )
    {
        if ( sndSystem==PULSE )
        {
            startSoundServer=false;
            QString scmd;
            if ( !sshSndTunnel )
                scmd="echo \"default-server=$(echo "
                     "\"$SSH_CLIENT\" | awk '{print $1}'):"+
                     sndPort+
                     "\" > \"${HOME}/.x2go/C-"+
                     resumingSession.sessionId+
                     "/.pulse-client.conf\""
                     ";echo \"cookie-file=${HOME}/.x2go/C-"+
                     resumingSession.sessionId+
                     "/.pulse-cookie"+
                     "\" >> \"${HOME}/.x2go/C-"+
                     resumingSession.sessionId+
                     "/.pulse-client.conf\""
                     ";echo \"autospawn=no\" >>"
                     "\"${HOME}/.x2go/C-" +
                     resumingSession.sessionId +
                     "/.pulse-client.conf\"";
            else
                scmd="echo \"default-server=localhost:"+
                     resumingSession.sndPort+
                     "\" > \"${HOME}/.x2go/C-"+
                     resumingSession.sessionId+
                     "/.pulse-client.conf\""
                     ";echo \"cookie-file=${HOME}/.x2go/C-"+
                     resumingSession.sessionId+
                     "/.pulse-cookie"+
                     "\" >> \"${HOME}/.x2go/C-"+
                     resumingSession.sessionId+
                     "/.pulse-client.conf\""
                     ";echo \"autospawn=no\" >>"
                     "\"${HOME}/.x2go/C-" +
                     resumingSession.sessionId +
                     "/.pulse-client.conf\"";

            sshConnection->executeCommand (scmd);

            bool sysPulse=false;
#ifdef Q_OS_LINUX
            loadPulseModuleNativeProtocol();
            QFile file ( "/etc/default/pulseaudio" );
            if ( file.open ( QIODevice::ReadOnly |
                             QIODevice::Text ) )
            {

                while ( !file.atEnd() )
                {
                    QByteArray line = file.readLine();
                    int pos=line.indexOf (
                                "PULSEAUDIO_SYSTEM_START=1"
                            );
                    if ( pos!=-1 )
                    {
                        int commentPos=line.indexOf (
                                           "#" );
                        if ( commentPos==-1 ||
                                commentPos>pos )
                        {
                            sysPulse=true;
                            break;
                        }
                    }
                }
                file.close();
            }
#endif
            QString pulsecookie_filename = "";
            if ( sysPulse )
            {
                if ( QFile::exists("/run/pulse/.config/pulse/cookie") ) {
                    pulsecookie_filename = "/run/pulse/.config/pulse/cookie";
                }
                else if ( QFile::exists("/run/pulse/.pulse-cookie") ) {
                    pulsecookie_filename = "/run/pulse/.pulse-cookie";
                }
                else if ( QFile::exists("/var/run/pulse/.config/pulse/cookie") ) {
                    pulsecookie_filename = "/var/run/pulse/.config/pulse/cookie";
                }
                else if ( QFile::exists("/var/run/pulse/.pulse-cookie") ) {
                    pulsecookie_filename = "/var/run/pulse/.pulse-cookie";
                }
                if ( pulsecookie_filename.length() > 0 )
                {
                    sshConnection->copyFile(
                        pulsecookie_filename,
                        "$HOME/.x2go/C-"+
                        resumingSession.sessionId+
                        "/.pulse-cookie", this, SLOT ( slotPCookieReady ( bool, QString,int )));
                }
            }
            else
            {
#if !defined (Q_OS_WIN) && !defined (Q_OS_DARWIN)
                if ( QFile::exists(homeDir+"/.config/pulse/cookie") ) {
                    pulsecookie_filename = homeDir+"/.config/pulse/cookie";
                }
                else if ( QFile::exists(homeDir+"/.pulse-cookie") ) {
                    pulsecookie_filename = homeDir+"/.pulse-cookie";
                }
                if ( pulsecookie_filename.length() > 0 )
                {
                    sshConnection->copyFile(pulsecookie_filename,
                                            "$HOME/.x2go/C-"+
                                            resumingSession.sessionId+
                                            "/.pulse-cookie", this, SLOT ( slotPCookieReady ( bool, QString,int )));
                }
#else /* !defined (Q_OS_WIN) && !defined (Q_OS_DARWIN) */
                QString cooFile = QDir::toNativeSeparators (QDir (pulseManager->get_pulse_dir ().absolutePath () +
                                                                  "/.pulse-cookie").absolutePath ());
                QString destFile="$HOME/.x2go/C-"+
                                 resumingSession.sessionId+
                                 "/.pulse-cookie";
                sshConnection->copyFile(cooFile,
                                        destFile, this, SLOT ( slotPCookieReady ( bool, QString,int )));
#endif /* !defined (Q_OS_WIN) && !defined (Q_OS_DARWIN) */
            }
        }
        if ( sndSystem==ESD )
        {
#if !defined (Q_OS_WIN) && !defined (Q_OS_DARWIN)
            sshConnection->copyFile(homeDir+"/.esd_auth",
                                    "$HOME/.esd_auth" );
#else /* !defined (Q_OS_WIN) && !defined (Q_OS_DARWIN) */
            QString cooFile = QDir::toNativeSeparators (QDir (pulseManager->get_pulse_dir ().absolutePath () +
                                                              "/.esd_auth").absolutePath ());
            QString destFile="$HOME/.esd_auth";
            sshConnection->copyFile(cooFile,
                                    destFile );
#endif /* defined (Q_OS_LINUX) */
        }
/* Windows and Darwin are covered by PulseManager. */
#if !defined (Q_OS_WIN) && !defined (Q_OS_DARWIN)
        if ( startSoundServer )
        {
            soundServer=new QProcess ( this );
            QString acmd="artsd",ecmd="esd";
            if ( sndSystem==ESD )
                soundServer->start (
                    ecmd+
                    " -tcp -nobeeps -bind localhost -port "+
                    resumingSession.sndPort );
            if ( sndSystem==ARTS )
                soundServer->start ( acmd+" -u -N -p "+
                                     resumingSession.sndPort );
            sndPort=resumingSession.sndPort;
        }
#endif /* !defined (Q_OS_WIN) && !defined (Q_OS_DARWIN) */
        if ( sshSndTunnel )
        {
            sndTunnel=sshConnection->startTunnel (
                          "localhost",
                          resumingSession.sndPort.toInt(),"127.0.0.1",
                          sndPort.toInt(),true,this,NULL, SLOT (
                              slotSndTunnelFailed ( bool,
                                                    QString,
                                                    int ) ));
        }
    }
}



x2goSession ONMainWindow::getSelectedSession()
{
    QString sessId=sessTv->model()->index ( sessTv->currentIndex().row(),
                                            S_ID ).data().toString();
    for ( int i=0; i<selectedSessions.size(); ++i )
    {
        if ( selectedSessions[i].sessionId==sessId )
            return selectedSessions[i];
    }
    return selectedSessions[0]; //warning !!!!! undefined session
}


void ONMainWindow::slotTunnelOk(int)
{

#ifdef Q_OS_WIN
    //waiting for X
    if ( !winServersReady )
    {

        x2goDebug<<"Waiting for win-servers.";

        QTimer::singleShot ( 100, this, SLOT ( slotTunnelOk() ) );
        return;
    }
#endif

    showExport=false;
    QString nxroot=homeDir +"/.x2go";
    QString dirpath=nxroot+"/S-"+resumingSession.sessionId;
    QDir d ( dirpath );
    if ( !d.exists() )
        if ( !d.mkpath ( dirpath ) )
        {
            if (startHidden)
            {
                x2goErrorf(6)<< tr( "Unable to create folder: ")  + dirpath;
                trayQuit();
            }

            QMessageBox::critical ( 0l,tr ( "Error" ), tr ( "Unable to create folder: " ) + dirpath,
                                    QMessageBox::Ok,
                                    QMessageBox::NoButton );
            slotShowPassForm();
//             if ( tunnel )
//                 delete tunnel;
//             if ( sndTunnel )
//                 delete sndTunnel;
//             if ( fsTunnel )
//                 delete fsTunnel;
//             if ( soundServer )
//                 delete soundServer;
            tunnel=sndTunnel=fsTunnel=0l;
            soundServer=0l;
            nxproxy=0l;
            return;
        }
#ifdef Q_OS_WIN
    dirpath=wapiShortFileName ( dirpath );
    nxroot=wapiShortFileName ( nxroot );
#endif
    QFile file ( dirpath+"/options" );
    if ( !file.open ( QIODevice::WriteOnly | QIODevice::Text ) )
    {
        if (startHidden)
        {
            x2goErrorf(7)<< tr( "Unable to write file: " ) + dirpath + "/options";
            trayQuit();
        }

        QMessageBox::critical ( 0l,tr ( "Error" ), tr( "Unable to write file: " ) + dirpath + "/options",
                                QMessageBox::Ok,
                                QMessageBox::NoButton );
        slotShowPassForm();
        return;
    }

    QTextStream out ( &file );
#ifdef Q_OS_WIN
    dirpath=cygwinPath ( dirpath );
    nxroot=cygwinPath ( nxroot );
#endif
    out << "nx/nx,root="<<nxroot<<",connect=localhost,cookie="<<
        resumingSession.cookie<<",port="
        <<localGraphicPort/*resumingSession.grPort*/<<
        ",errors="<<dirpath<<"/sessions:"<<
        resumingSession.display;
    file.close();
    xmodExecuted=false;


    qDebug()<<"RESUMING SESSION is KDRIVE: "<<((resumingSession.sessionType== x2goSession::KDRIVE)||(resumingSession.sessionType== x2goSession::ROOTLESSKDRIVE));

    nxproxy=new QProcess;
    proxyErrString="";
    QStringList env = QProcess::systemEnvironment();
#if defined ( Q_OS_WIN ) || defined ( Q_OS_DARWIN )
    int dispInd=-1;
#endif
    for ( int l=0; l<env.size(); ++l )
    {
// 		x2goDebug<<env[l]<<Qt::endl;

#if defined ( Q_OS_WIN ) || defined ( Q_OS_DARWIN )
        if ( env[l].indexOf ( "DISPLAY" ) ==0 )
        {
            dispInd=l;
        }
#endif
    }
    env << "NX_CLIENT="+QCoreApplication::applicationFilePath ();

#if defined ( Q_OS_WIN ) || defined ( Q_OS_DARWIN )
    // On Mac OS X, we want to make sure that DISPLAY is set to a proper value,
    // but at the same time don't want to set the value ourselves but keep
    // the provided one.

    QString disp="0";
    if(resumingSession.sessionType!= x2goSession::KDRIVE && resumingSession.sessionType!= x2goSession::ROOTLESSKDRIVE)
    {
        disp=getXDisplay();
        if ( disp==QString() )
        {
            //slotProxyerror ( QProcess::FailedToStart );
            return;
        }
    }
#endif // Q_OS_WIN || Q_OS_DARWIN
#if defined ( Q_OS_WIN )
    if ( dispInd==-1 )
    {
        env <<"DISPLAY=localhost:"+disp;

        /*		x2goDebug<<"new env DISPLAY"<<"DISPLAY=localhost:"+disp<<Qt::endl;*/

    }
    else
    {
        env[dispInd]="DISPLAY=localhost:"+disp;

        /*		x2goDebug<<"existing env DISPLAY("<<dispInd<<
        		") DISPLAY=localhost:"+disp<<Qt::endl;*/

    }
#endif // Q_OS_WIN
#if defined ( Q_OS_DARWIN )
    // setting /usr/X11/bin to find xauth
    // /usr/X11R6/bin is added for compatibility reasons with OS X 10.4.
    /* FIXME: don't just overwrite this stuff, use add_to_path () instead. */
    env.insert (
        0,
        "PATH=/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin:/usr/X11/bin:/usr/X11R6/bin" );
    // Set the NX base dir to bundle/exe/, used for finding nxauth.
    {
        QDir tmpDir (appDir);
        tmpDir.cd ("../exe");
        env.append ("NX_SYSTEM=" + tmpDir.absolutePath ());
    }
    if (dispInd == -1)
    {

        x2goDebug<< "No DISPLAY variable found in global environment, using autodetected setting.";

        env.append ("DISPLAY=" + disp);
    }
#endif
    nxproxy->setEnvironment ( env );

    connect ( nxproxy,SIGNAL ( error ( QProcess::ProcessError ) ),this,
              SLOT ( slotProxyError ( QProcess::ProcessError ) ) );
    connect ( nxproxy,SIGNAL ( finished ( int,QProcess::ExitStatus ) ),this,
              SLOT ( slotProxyFinished ( int,QProcess::ExitStatus ) ) );
    connect ( nxproxy,SIGNAL ( readyReadStandardError() ),this,
              SLOT ( slotProxyStderr() ) );
    connect ( nxproxy,SIGNAL ( readyReadStandardOutput() ),this,
              SLOT ( slotProxyStdout() ) );
    QString proxyCmd="nxproxy -S nx/nx,options="+dirpath+"/options:"+
                     resumingSession.display;
#ifdef Q_OS_WIN
    if(! QFile::exists(appDir+"/nxproxy.exe"))
    {
        if(QFile::exists(appDir+"/bin/nxproxy.exe"))
        {
            proxyCmd="bin\\"+proxyCmd;
        }
    }
    x2goDebug<<"NX proxy: "<<proxyCmd;
#endif
#ifdef Q_OS_DARWIN
    //run nxproxy from bundle
    QDir dir ( appDir );
    dir.cdUp();
    dir.cd ( "exe" );
    proxyCmd="\""+dir.absolutePath() +"/\""+proxyCmd;
#endif //Q_OS_DARWIN


    if(resumingSession.sessionType==x2goSession::KDRIVE || resumingSession.sessionType==x2goSession::ROOTLESSKDRIVE)
    {
        bool randr=false;
        bool fs=false;
        QString width;
        QString height;
        bool multidisp=false;
        QString dispNumber="1";
        QString clipboard="both";
        bool noresize=false;
        if (!embedMode )
        {
            if (!useLdap)
            {
                X2goSettings *st;
                QString sid;
                if ( !embedMode ) {
                    if (!(sessionExplorer->getLastSession())) {
                        x2goDebug << "No session selected, not searching for proxy window.";
                        return;
                    }

                    sid=sessionExplorer->getLastSession()->id();
                }
                else
                    sid="embedded";

                if (brokerMode)
                    st=new X2goSettings(config.iniFile,QSettings::IniFormat);
                else
                    st= new X2goSettings( "sessions" );
                randr=st->setting()->value ( sid+"/xinerama",
                                             ( QVariant ) defaultXinerama ).toBool();
                fs=st->setting()->value ( sid+"/fullscreen", ( QVariant ) defaultFullscreen).toBool();

                width=st->setting()->value ( sid+"/width", ( QVariant ) "800").toString();
                height=st->setting()->value ( sid+"/height", ( QVariant ) "600").toString();
                clipboard=st->setting()->value ( sid+"/clipboard", ( QVariant ) "both").toString();
                noresize=st->setting()->value ( sid+"/noresize",   ( QVariant ) false).toBool();


                if (st->setting()->value ( sid+"/multidisp", ( QVariant ) false ).toBool())
                {
                    dispNumber=st->setting()->value ( sid+"/display", (QVariant ) "1").toString();
                    multidisp=st->setting()->value ( sid+"/multidisp", ( QVariant ) false ).toBool();
                }
                delete st;
            }
        }

        QStringList options;
#ifdef Q_OS_WIN
        //restore real path, as we not using cygwin for x2gokdriveclient
        nxroot=homeDir +"/.x2go";
        dirpath=nxroot+"/S-"+resumingSession.sessionId;
#endif
        proxyCmd="x2gokdriveclient";
        if(ONMainWindow::debugging)
        {
            options <<"--debug";
        }
        options<<"--connect"<<"localhost"<<"--port"<<localGraphicPort<<"--title"<<resumingSession.sessionId<<"-S"<<"nx/nx,options="+dirpath+
        "/options:"+resumingSession.display<<"--selection"<<clipboard;
        if(noresize)
            options<<"--noresize";
        if(resumingSession.sessionType==x2goSession::ROOTLESSKDRIVE)
        {
            options << "--rootless";
        }
        if(randr)
        {
            options<<"--randr";
        }
        if(fs)
        {
            options<<"--fs";
        }
        else
            if(multidisp)
            {
                options<<"--screen"<<dispNumber;
            }
            else
            {
                options<<"--width"<<width<<"--height"<<height;
            }
        if (thinMode) {
          options << "--disable-menubar";
        }

        x2goDebug<<"Starting KDrive Client, command: " + proxyCmd<<options;
        nxproxy->start ( proxyCmd, options );
    }
    else
    {
        x2goDebug<<"Starting NX proxy, command: " + proxyCmd;
        nxproxy->start ( proxyCmd);
    }

    proxyRunning=true;
    if(brokerMode)
    {
        sendEventToBroker(CONNECTING);
    }

// always search for proxy window on linux. On Windows only in window mode
#ifdef Q_OS_WIN
    if (xorgMode==WIN) {
#endif
//don't search proxy for kdrive sessions
        if(!(resumingSession.sessionType==x2goSession::KDRIVE || resumingSession.sessionType==x2goSession::ROOTLESSKDRIVE))
            proxyWinTimer->start ( 300 );
#ifdef Q_OS_WIN
    }
#endif
    if ( embedMode )
    {
//         proxyWinTimer->start ( 300 );
        if ( !startEmbedded )
        {
            act_embedContol->setText (
                tr ( "Attach X2Go window" ) );
        }
    }
#ifdef Q_OS_WIN
    else
    {
// #ifdef CFGCLIENT
//         // if using XMing, we must find proxy win for case, that we should make it fullscreen
        //         if(useInternalX&& (internalX==XMING))
// #endif
//         proxyWinTimer->start ( 300 );
    }
#endif

    showSessionStatus();
    QTimer::singleShot ( 30000,this,SLOT ( slotRestartProxy() ) );

}

void ONMainWindow::slotTunnelFailed ( bool result,  QString output,
                                      int )
{
    if ( result==false )
    {
        if ( !managedMode )
        {
            QString message=tr ( "Unable to create SSH tunnel for X2Go session (NX) startup:\n" )
                            +output;
            QMessageBox::critical ( 0l,tr ( "Error" ),message,
                                    QMessageBox::Ok,
                                    QMessageBox::NoButton );
        }
//         if ( tunnel )
//             delete tunnel;
//         if ( sndTunnel )
//             delete sndTunnel;
//         if ( fsTunnel )
//             delete fsTunnel;
//         if ( soundServer )
//             delete soundServer;
        tunnel=sndTunnel=fsTunnel=0l;
        soundServer=0l;
        nxproxy=0l;
        proxyRunning=false;
        if ( !managedMode )
            slotShowPassForm();
    }
}

void ONMainWindow::slotSndTunnelFailed ( bool result,  QString output,
        int )
{
    if ( result==false )
    {
        if ( !managedMode )
        {
            QString message=tr ( "Unable to create SSH tunnel for audio data:\n" )
                            +output;
            QMessageBox::warning ( 0l,tr ( "Warning" ),message,
                                   QMessageBox::Ok,
                                   QMessageBox::NoButton );
        }
        sndTunnel=0l;
    }
}


#ifdef Q_OS_DARWIN
void ONMainWindow::slotSetModMap()
{
    if (!nxproxy) {
        return;
    }
    if (kbMap.isEmpty ()) {
        QProcess pr (this);
        QProcessEnvironment tmp_env = QProcessEnvironment::systemEnvironment ();
        QString path_val = tmp_env.value ("PATH");

        /* Let's set a reasonable default value if none is provided. */
        if (path_val.isEmpty ()) {
            /* Prefer the default MacPorts prefix. */
            path_val = MACPORTS_PREFIX "/bin:" MACPORTS_PREFIX "/sbin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin:/usr/local/sbin:/usr/X11R6/bin:/opt/X11/bin";
            tmp_env.insert ("PATH", path_val);
        }
        else {
            /* Search for and add specific directories to the PATH value, if necessary. */
            QStringList to_back, to_front;
            to_back << "/opt/X11/bin";
            to_front << MACPORTS_PREFIX "/bin" << "/usr/local/bin";

            path_val = add_to_path (path_val, to_back);
            path_val = add_to_path (path_val, to_front, false);

            /* Insert will overwrite the value automatically. */
            tmp_env.insert ("PATH", path_val);

            /*
             * Also alter our own environment so we can actually
             * execute xmodmap and friends later on.
             */
            qputenv ("PATH", path_val.toUtf8 ());
        }

        pr.setProcessEnvironment (tmp_env);

        QStringList key_map_fetch_args;
        key_map_fetch_args << "-pke";
        pr.start ("xmodmap", key_map_fetch_args);
        bool key_map_fetch_ret = pr.waitForStarted ();

        if (!key_map_fetch_ret) {
            handle_xmodmap_error (pr);
        }
        else {
            key_map_fetch_ret = pr.waitForFinished ();

            if (!key_map_fetch_ret) {
                handle_xmodmap_error (pr);
            }

            kbMap = pr.readAllStandardOutput ();

            QStringList mod_fetch_args;
            mod_fetch_args << "-pm";
            pr.start ("xmodmap", mod_fetch_args);
            bool mod_fetch_ret = pr.waitForStarted ();

            if (!mod_fetch_ret) {
                handle_xmodmap_error (pr);
            }
            else {
                mod_fetch_ret = pr.waitForFinished ();

                if (!mod_fetch_ret) {
                    handle_xmodmap_error (pr);
                }

                QString modifiers = pr.readAllStandardOutput ();
                x2goDebug << "modifiers: " << modifiers;

                /* Reset all modifiers first. */
                kbMap += "clear shift\nclear lock\nclear control\nclear mod1\nclear mod2\nclear mod3\nclear mod4\nclear mod5\n";

                /* And set them back again. */
                QStringList lines = modifiers.split ("\n", Qt::SkipEmptyParts);
                for (int i = 0; i < lines.count (); ++i) {
                    QStringList parts = lines[i].split (" ", Qt::SkipEmptyParts);
                    if (parts.count () < 2) {
                        continue;
                    }

                    QString mod = parts[0];
                    if ((mod == "shift") || (mod == "lock") || (mod == "control") || (mod == "mod1") || (mod == "mod2") || (mod == "mod3") || (mod == "mod4") || (mod == "mod5")) {
                        for (int j = 1; j < parts.count (); ++j) {
                            if (parts[j].indexOf ("(") == -1) {
                                kbMap += "add " + mod + " = " + parts[j] + "\n";
                            }
                        }
                    }
                }
            }
        }
    }

    if (kbMap.isEmpty ()) {
        x2goWarningf (9) << "Could not fetch keyboard map via xmodmap. Internal error. Not synchronizing with remote side.";
    }
    else {
        /* Send modified map to server. */
        QString cmd = "export DISPLAY=\":" + resumingSession.display + "\"; echo \"" + kbMap + "\" | xmodmap -";
        sshConnection->executeCommand (cmd);
    }
}

void ONMainWindow::handle_xmodmap_error (QProcess &proc) {
  QString main_text ("xmodmap ");
  QString informative_text;

  QProcessEnvironment proc_env = QProcessEnvironment::systemEnvironment ();

  /* If the process has a special env, fetch it. */
  if (!(proc.processEnvironment ().isEmpty ())) {
    proc_env = proc.processEnvironment ();
  }

  switch (proc.error ()) {
    case QProcess::FailedToStart: {
      main_text += tr ("failed to start.");
      informative_text += tr ("This likely means the binary is not available.\n"
                              "The current search path is: ");

      QString path_val = proc_env.value ("PATH", "unknown");

      /* Add a newline every 100 characters. */
      for (std::size_t i = 100; i < static_cast<std::size_t> (path_val.size ()); i += 100) {
          path_val.insert (i, "\n");
      }

      informative_text += path_val;
      break;
    }
    case QProcess::Crashed: {
      main_text += tr ("returned a non-zero exit code or crashed otherwise.");
      informative_text += tr ("Execution failed, exit code was: ");
      informative_text += QString::number (proc.exitCode ());
      break;
    }
    case QProcess::Timedout: {
      main_text += tr ("didn't start up in time.");
      informative_text = tr ("This error shouldn't come up.");
      break;
    }
    case QProcess::WriteError: {
      main_text += tr ("didn't accept a write operation.");
      informative_text = tr ("It is probably not running correctly or crashed in-between.");
      break;
    }
    case QProcess::ReadError: {
      main_text = tr ("Unable to read from xmodmap.");
      informative_text = tr ("It is probably not running correctly or crashed in-between.");
      break;
    }
    case QProcess::UnknownError: {
      main_text += tr ("encountered an unknown error during start up or execution.");
      break;
    }
    default: {
      main_text += tr ("experienced an undefined error.");
      break;
    }
  }

  if (!informative_text.isEmpty ()) {
    informative_text += "\n\n";
  }

  informative_text += tr ("X2Go Client will now terminate.\n\n"
                          "File a bug report as outlined on the <a href=\"http://wiki.x2go.org/doku.php/wiki:bugs\">bugs wiki page</a>.");

  show_RichText_ErrorMsgBox (main_text, informative_text);
  trayQuit ();
}
#endif

void ONMainWindow::slotProxyError ( QProcess::ProcessError err )
{
#ifdef Q_OS_LINUX
    if(err==QProcess::FailedToStart && directRDP)
    {
        QString main_text = tr("Failed to start RDP or XDMCP client");
        QString informative_text = tr ("Check session settings and ensure that selected client is installed on your system.");

        show_RichText_ErrorMsgBox (main_text, informative_text);
    }
#endif
    slotProxyFinished ( -1,QProcess::CrashExit );
}


void ONMainWindow::slotProxyFinished ( int,QProcess::ExitStatus )
{
    if(brokerMode)
    {
        ignoreBrokerSessions=false;
        sendEventToBroker(FINISHED);
    }
#ifdef Q_OS_DARWIN
    if (modMapTimer) {
      disconnect (modMapTimer, SIGNAL (timeout ()), this, SLOT (slotSetModMap ()));
      modMapTimer->stop ();
      delete (modMapTimer);
      modMapTimer = 0;
    }
    kbMap = QString ();
//fixes bug, when mainwindow inputs not accepting focus under mac
    setFocus ();
#endif
    //set tray icon to default
    if (trayIcon && !keepTrayIcon)
        trayIcon->setIcon(QIcon ( iconsPath("/128x128/x2go.png") ));


    if ( embedMode )
    {
        proxyWinTimer->stop();
        setEmbedSessionActionsEnabled ( false );
    }
#ifdef Q_OS_WIN
    else
        proxyWinTimer->stop();
    if (! startXorgOnStart)
    {
        if (xorg)
        {
            if (xorg->state() ==QProcess::Running)
            {
                xorg->terminate();
                delete xorg;
                xorg=0;
            }
        }
    }
#endif
    if ( closeEventSent )
        return;
    if ( soundServer )
        delete soundServer;
    if ( spoolTimer )
        delete spoolTimer;

    x2goDebug<<"Deleting Proxy." ;

    disconnect ( nxproxy,SIGNAL ( error ( QProcess::ProcessError ) ),this,
                 SLOT ( slotProxyError ( QProcess::ProcessError ) ) );
    disconnect ( nxproxy,SIGNAL ( finished ( int,QProcess::ExitStatus ) ),this,
                 SLOT ( slotProxyFinished ( int,QProcess::ExitStatus ) ) );
    disconnect ( nxproxy,SIGNAL ( readyReadStandardError() ),this,
                 SLOT ( slotProxyStderr() ) );
    disconnect ( nxproxy,SIGNAL ( readyReadStandardOutput() ),this,
                 SLOT ( slotProxyStdout() ) );
    proxyRunning=false;
    if (trayEnabled)
    {
        trayIconActiveConnectionMenu->setTitle(tr("Not connected"));
        trayIconActiveConnectionMenu->setEnabled(false);
        if (trayMaxDiscon)
            showNormal();
    }
    trayAutoHidden=false;
    bool emergencyExit=false;

    if(proxyErrString.indexOf("No data received from remote proxy")!=-1)
    {
        emergencyExit=true;
        x2goWarningf(4)<< tr( "Emergency exit." );
    }

    if ( nxproxy )
    {
        if ( nxproxy->state() ==QProcess::Running )
        {
            emergencyExit=true;
            x2goWarningf(5)<< tr( "Waiting for proxy to exit." );
            if ( !nxproxy->waitForFinished ( 3000 ) )
            {
                x2goWarningf(6)<< tr( "Proxy didn't terminate after 3 seconds. Killing the proxy." );
                nxproxy->kill();
            }
        }

#ifdef Q_OS_LINUX
        if (directRDP)
            nxproxy=0;
#endif
    }
    x2goDebug<<"Waiting for proxy to exit.";

    spoolTimer=0l;
    tunnel=sndTunnel=fsTunnel=0l;
    soundServer=0l;
    nxproxy=0l;
    proxyWinId=0;

#ifdef Q_OS_LINUX
    if (directRDP)
    {
        if (closeDisconnect)
        {
            close();
        }
        pass->setText ( "" );
        QTimer::singleShot ( 2000,this,
                             SLOT ( slotShowPassForm() ) );
        return;
    }
#endif
    if ( !emergencyExit && !shadowSession && !usePGPCard && ! ( embedMode &&
            ( config.checkexitstatus==false ) ) )
    {
        x2goDebug<<"Checking exit status.";
        check_cmd_status();
    }
    else
    {
        x2goDebug<<"Deleting SSH connection instance.";
        delete sshConnection;
        x2goDebug<<"Deleted SSH connection instance." ;
        sshConnection=0;
        if (startHidden)
        {
            x2goInfof(9) << tr("Closing X2Go Client because it was started in hidden mode.");
            close();
        }
        else if (closeDisconnect)
        {
            x2goInfof(10) << tr("Closing X2Go Client because the --close-disconnect parameter was passed.");
            close();
        }
    }

    if ( readExportsFrom!=QString() )
    {
        exportTimer->stop();
        if ( extLogin )
        {
            currentKey=QString();
        }
    }
    if ( printSupport )
        cleanPrintSpool();
    if ( !restartResume )
    {
        if ( brokerMode && (!config.brokerAutologoff) )
        {
            x2goDebug<<"Re-reading user's session profiles from broker.";
            QTimer::singleShot ( 2000,broker,
                                 SLOT ( getUserSessions() ) );
        }
        else if ( brokerMode && config.brokerAutologoff )
        {
            x2goDebug<<"Logging off from broker as requested via command line.";
            QTimer::singleShot(1, this,SLOT(slotGetBrokerAuth()));
        }
        else if ( !embedMode )
        {
            pass->setText ( "" );
            QTimer::singleShot ( 2000,this,
                                 SLOT ( slotShowPassForm() ) );
        }
    }
    else
    {
        restartResume=false;
        sessionStatusDlg->hide();
        resumeSession ( resumingSession );
    }
    x2goDebug<<"Finished proxy.";
    setStatStatus ( tr ( "Finished" ) );
#ifdef Q_OS_DARWIN
//fixes bug, when mainwindow inputs not accepting focus under mac
    setFocus();
#endif

}


void ONMainWindow::slotProxyStderr()
{
    QString reserr;
    if ( nxproxy )
        reserr= nxproxy->readAllStandardError();
    proxyErrString+=reserr;

    x2goDebug<<"Proxy wrote on stderr: "<<reserr;

    if(debugging)
    {
        QFile fl(homeDir+"/.x2go/S-"+resumingSession.sessionId+"/session.log");
        fl.open(QIODevice::WriteOnly|QIODevice::Append);
        fl.write(reserr.toLocal8Bit());
        fl.close();
    }

    stInfo->insertPlainText ( reserr );
    stInfo->ensureCursorVisible();
    if ( reserr.indexOf (
        "Connecting to remote host 'localhost:"+
        /*resumingSession.grPort*/ localGraphicPort ) !=-1 )
    {
        setStatStatus ( tr ( "connecting" ) );
    }

    if ( reserr.indexOf (
                "Connection to remote proxy 'localhost:"+
                /*resumingSession.grPort*/
                localGraphicPort+"' established" ) !=-1 )
    {
        if ( newSession )
        {
            setStatStatus ( tr ( "starting" ) );
        }
        else
        {
            setStatStatus ( tr ( "resuming" ) );
        }
    }

    if ( reserr.indexOf (
                "Established X server connection" ) !=-1 )
    {
        if(brokerMode)
        {
            sendEventToBroker(CONNECTED);
        }
        setStatStatus ( tr ( "running" ) );
        destroySplash();
        if (trayEnabled)
        {
            if (!useLdap)
                trayIconActiveConnectionMenu->setTitle(sessionExplorer->getLastSession()->name());
            else
                trayIconActiveConnectionMenu->setTitle(lastUser->username());
            trayIconActiveConnectionMenu->setEnabled(true);
            if (trayMinCon && !trayAutoHidden)
            {
                trayAutoHidden=true;
                hide();
            }
        }
        if ( embedMode )
            setEmbedSessionActionsEnabled ( true );
        disconnect ( sbSusp,SIGNAL ( clicked() ),this,
                     SLOT ( slotTestSessionStatus() ) );
        disconnect ( sbSusp,SIGNAL ( clicked() ),this,
                     SLOT ( slotSuspendSessFromSt() ) );
        connect ( sbSusp,SIGNAL ( clicked() ),this,
                  SLOT ( slotSuspendSessFromSt() ) );
        if ( !showExport )
        {
            showExport=true;
            /*connect ( sbExp,SIGNAL ( clicked() ),this,
              SLOT ( slot_exportDirectory() ) );*/
            sbExp->setEnabled ( true );
            exportDefaultDirs();
            if ( readExportsFrom!=QString() )
            {
                exportTimer->start ( 2000 );
            }
        }
        sbSusp->setToolTip ( tr ( "Suspend" ) );
#ifdef Q_OS_DARWIN
        // Only start this once...
        if (!modMapTimer) {
            modMapTimer = new QTimer (this);
            connect (modMapTimer, SIGNAL (timeout ()), this, SLOT (slotSetModMap ()));
            modMapTimer->start (10000);
            slotSetModMap ();
        }
#endif
        if ( newSession )
        {
            runCommand();
            newSession=false;
        }
    }
    if ( reserr.indexOf (
                tr ( "Connection timeout, aborting" ) ) !=-1 )
        setStatStatus ( tr ( "aborting" ) );
}


void ONMainWindow::slotProxyStdout()
{
    QString resout ( nxproxy->readAllStandardOutput() );

    x2goDebug<<"Proxy wrote on stdout: "<<resout;
}

void ONMainWindow::setFocus()
{

#ifdef Q_OS_DARWIN
    bool userFocus=false;
    bool passFocus=false;
    if(passForm && passForm->isVisible())
    {
        if(pass && pass->isVisible()&& pass->hasFocus())
        {
            passFocus=true;
        }
        if(login && login->isVisible()&& login->hasFocus())
        {
            userFocus=true;
        }
    }
#endif
    QWidget::setFocus();
#ifdef Q_OS_DARWIN
    if(userFocus)
    {
        login->setFocus();
    }
    if(passFocus)
    {
        pass->setFocus();
    }
#endif

}


void ONMainWindow::slotShowPassForm()
{
#ifdef Q_OS_DARWIN
//fixes bug, when mainwindow inputs not accepting focus under mac
    x2goDebug<<"Setting focus."<<Qt::endl;
    QTimer::singleShot(500, this, SLOT(setFocus()));
    setFocus();
#endif
    if ( !useLdap )
    {
        loginPrompt->show();
        login->show();
    }
    else
    {
        loginPrompt->hide();
        login->hide();
    }
    setEnabled ( true );
    if ( !embedMode )
    {
        u->hide();
        uname->hide();
    }
    sessionStatusDlg->hide();
    selectSessionDlg->hide();
    setEnabled ( true );
    if ( isPassShown )
    {
        passForm->show();
        passForm->setEnabled ( true );
    }
    isPassShown=true;
    login->setEnabled ( true );
    if ( login->text().length() >0 )
    {
        pass->setFocus();
        pass->selectAll();
    }
    else
    {
        login->setFocus();
        login->selectAll();
    }


    if ( !embedMode )
    {
        u->setEnabled ( true );
    }
    else
    {
        if ( config.user.length() >0 )
            login->setEnabled ( false );
    }
}


void ONMainWindow::showSessionStatus()
{
    setStatStatus();
}


void ONMainWindow::slotShowAdvancedStat()
{
    if ( !miniMode )
    {
        if ( sbAdv->isChecked() )
        {
            sessionStatusDlg->setFixedSize (
                sessionStatusDlg->width(),
                sessionStatusDlg->height() *2 );
        }
        else
        {
            sessionStatusDlg->setFixedSize (
                sessionStatusDlg->sizeHint() );
            stInfo->hide();
        }
    }
    else
    {
        if ( sbAdv->isChecked() )
        {
            sessionStatusDlg->setFixedSize ( 310,300 );
        }
        else
        {
            stInfo->hide();
            sessionStatusDlg->setFixedSize ( 310,200 );
        }
    }


// 	username->invalidate();


    if ( sbAdv->isChecked() )
    {
        stInfo->show();
    }

    X2goSettings st ( "settings" );
    st.setting()->setValue ( "showStatus",
                             ( QVariant ) sbAdv->isChecked() );
    st.setting()->sync();
}




void ONMainWindow::slotResumeDoubleClick ( const QModelIndex& )
{
    if ( !shadowSession )
        slotResumeSess();
}

void ONMainWindow::suspendBrokerSession(const QString& sessId, const QString& host)
{
    suspendTerminateHostFromBroker=host;
    suspendTerminateSessionFromBroker=sessId;
    startSession(config.session, SUSPEND);
}

void ONMainWindow::termBrokerSession(const QString& sessId, const QString& host)
{
    if(QMessageBox::warning (
        this,tr ( "Warning" ),
                                     tr (
                                         "Are you sure you want to terminate "
                                         "this session?\n"
                                         "Unsaved documents will be lost." ),
                             QMessageBox::Yes,QMessageBox::No )!= QMessageBox::Yes)
    {
        setEnabled(true);
        selectSessionDlg->setEnabled ( true );
        return;
    }

    suspendTerminateHostFromBroker=host;
    suspendTerminateSessionFromBroker=sessId;
    startSession(config.session, TERMINATE);
}


void ONMainWindow::suspendSession ( QString sessId )
{

    sshConnection->executeCommand ( "x2gosuspend-session "+sessId, this,  SLOT ( slotRetSuspSess ( bool,  QString,
                                    int ) ) );
}


bool ONMainWindow::termSession ( QString sessId, bool warn )
{
    if ( warn )
    {
        bool hide_after=false;
        if (isHidden())
        {
            showNormal();
            hide_after=true;
        }
        int answer=QMessageBox::warning (
                       this,tr ( "Warning" ),
                       tr (
                           "Are you sure you want to terminate "
                           "this session?\n"
                           "Unsaved documents will be lost." ),
                       QMessageBox::Yes,QMessageBox::No );
        if (hide_after)
            hide();

        if ( answer !=
                QMessageBox::Yes )
        {
            if ( selectSessionDlg->isVisible() )
                selectSessionDlg->setEnabled ( true );
            return false;
        }
    }
    if ( shadowSession )
    {
        nxproxy->terminate();
        return true;
    }
    x2goDebug<<"Terminating session.";
    sshConnection->executeCommand ( "x2goterminate-session "+sessId, this, SLOT ( slotRetTermSess ( bool,
                                    QString,int) )  );
    proxyRunning=false;
    return true;
}



void ONMainWindow::setStatStatus ( QString status )
{
    setEnabled ( true );
    passForm->hide();
    selectSessionDlg->hide();
    if ( status == QString() )
        status=statusString;
    else
        statusString=status;
    QString tstr;
    if ( statusLabel )
        statusLabel->setText ( QString() );
    if ( resumingSession.sessionId!=QString() )
    {
        QDateTime dt=QDateTime::fromString ( resumingSession.crTime, Qt::ISODate );
        tstr=dt.toString();
    }
    if ( !embedMode || !proxyWinEmbedded )
    {
        if(!brokerMode)
        {
            statusBar()->showMessage ( "");
            statusBar()->hide();
        }
        QString srv;
        if ( brokerMode )
        {
            srv=config.serverIp;
        }
        else if ( embedMode )
        {
            srv=config.server;
        }
        else
        {
            srv=resumingSession.server;
        }
        slVal->setText ( resumingSession.sessionId+"\n"+
                         srv+"\n"+
                         getCurrentUname() +"\n"+
                         resumingSession.display+
                         "\n"+tstr+"\n"+status );

        slVal->setFixedSize ( slVal->sizeHint() );
        sessionStatusDlg->show();
        if (resumingSession.published)
            sbApps->show();
        else
            sbApps->hide();
    }
    else
    {

        QString srv;
        if ( brokerMode )
        {
            srv=config.serverIp;
        }
        else
        {
            srv=config.server;
        }
        QString message=getCurrentUname() +"@"+
                        srv+
                        ", "+tr ( "Session" ) +": "+
                        resumingSession.sessionId+", "+
                        tr ( "Display" ) +": "+
                        resumingSession.display+", "+
                        tr ( "Creation time" ) +": "+tstr;
        if ( statusLabel )
        {
            statusLabel->setText ( "   "+message );
        }
        else
        {
            if ( config.showstatusbar )
            {
                statusBar()->show();
                statusBar()->showMessage ( message );
            }
        }
        sessionStatusDlg->hide();
    }
    if(splash)
    {
        splash->showMessage(status, Qt::AlignLeft|Qt::AlignBottom, Qt::white);
    }
}


void ONMainWindow::slotRestartProxy()
{
    if ( !sessionStatusDlg->isVisible() )
        return;
    if ( proxyErrString.indexOf (
                "Established X server connection" ) ==-1 )
    {
        stInfo->insertPlainText (
            tr (
                "Connection timeout, aborting" ) );
        if ( nxproxy )
            nxproxy->terminate();
        proxyRunning=false;
        restartResume=true;
    }
}


void ONMainWindow::slotTestSessionStatus()
{

    if ( !sessionStatusDlg->isVisible() )
        return;
    if ( proxyErrString.indexOf (
                "Established X server connection" ) ==-1 )
    {
        stInfo->insertPlainText (
            tr ( "Connection timeout, aborting" ) );
        if ( nxproxy )
            nxproxy->terminate();
        proxyRunning=false;
    }
}


x2goSession ONMainWindow::getNewSessionFromString ( const QString& string )
{
    QStringList lst=string.split ( '|' );
    x2goSession s;
    s.display=lst[0];
    s.cookie=lst[1];
    s.agentPid=lst[2];
    s.sessionId=lst[3];
    s.grPort=lst[4];
    s.sndPort=lst[5];
    if ( lst.count() >6 )
        s.fsPort=lst[6];
    return s;
}

#if defined (Q_OS_DARWIN) || defined (Q_OS_WIN)
void ONMainWindow::pulseManagerWrapper () {
#ifdef Q_OS_WIN
  if (!embedMode || !config.confSnd ||
      (config.confSnd && config.useSnd))
#endif /* defined (Q_OS_WIN) */
  {
    X2goSettings st ("settings");
    bool disablePulse = st.setting ()->value ("pulse/disable",
                                                     (QVariant) false).toBool ();
    if(disablePulse||systemDisablePA)
    {
        x2goDebug<<"Not starting PulseAudio";
        return;
    }
    pulseManagerThread = new QThread (0);
    pulseManager = new PulseManager ();
    connect(pulseManager, SIGNAL(sig_pulse_user_warning(bool, const QString&, const QString&, bool)),
            this, SLOT(slotShowPAMSGDialog(bool, const QString&, const QString&, bool)));

    bool disableInput = st.setting ()->value ("pulse/norecord",
                                                     (QVariant) false).toBool ();
    pulseManager->set_record (!disableInput);
    pulseManager->set_debug (debugging);

    pulseManager->moveToThread (pulseManagerThread);

    /*
     * Spawn PulseManager::start() once the thread started up successfully.
     * Another means of doing that would be via
     * QMetaObject::invokeMethod (pulseManager, "start", Qt::QueuedConnection);
     */
    connect (pulseManagerThread, SIGNAL (started ()), pulseManager, SLOT (start ()));

    pulseManagerThread->start ();
  }
}
#endif /* defined (Q_OS_DARWIN) || defined (Q_OS_WIN) */

void ONMainWindow::slotAppDialog()
{
    AppDialog dlg(this);
    dlg.exec();
}

void ONMainWindow::runCommand()
{

    if ( runRemoteCommand )
    {
        /* 1st override PATH and determine the base path to x2goruncommand.
         * Then in SlotRunCommand, call x2goruncommand without overriding PATH.
         * This ensures that the PATH is never overridden for the actual user
         * session.
         * Fixes: #1100
         */
        sshConnection->executeCommand ( "x2gobasepath", this,
                                        SLOT ( SlotRunCommand ( bool,
                                        QString,
                                        int )), true);
    }
}


void ONMainWindow::runApplication(QString exec)
{
    QString cmd = "PULSE_CLIENTCONFIG=\"${HOME}/.x2go/C-"
                + resumingSession.sessionId+"/.pulse-client.conf\" DISPLAY=:"
                + resumingSession.display
                + " setsid " + exec + " 1> /dev/null 2>/dev/null & exit";

    sshConnection->executeCommand (cmd, 0, 0, false);
}

void ONMainWindow::SlotRunCommand(bool, QString output, int)
{
    QString passwd=getCurrentPass();
    QString user=getCurrentUname();
    QString host=resumingSession.server;
    QString command;
    QString sessionType="D";
    QString rdpOpts,rdpServer;
    bool rdpFS=false;
    QString rdpWidth;
    QString rdpHeight;
    bool rootless=false;
    resumingSession.published=false;
    if ( !embedMode )
    {
        X2goSettings* st;
        if (!brokerMode)
            st=new X2goSettings( "sessions" );
        else
            st=new X2goSettings(config.iniFile, QSettings::IniFormat);


        if ( useLdap )
            command=sessionCmd;
        else
        {
            if (!(sessionExplorer->getLastSession())) {
                x2goDebug << "No session selected, not running command.";
                return;
            }

            QString sid=sessionExplorer->getLastSession()->id();
            command=st->setting()->value (
                        sid+"/command",
                        ( QVariant ) tr ( "KDE" ) ).toString();
            rdpOpts=st->setting()->value (
                        sid+"/rdpoptions",
                        ( QVariant ) "" ).toString();
            if ( user != "" ) {
                rdpOpts.replace("X2GO_USER", user);
            }
            if ( passwd != "" ) {
                rdpOpts.replace("X2GO_PASSWORD", passwd);
            }
            rdpServer=st->setting()->value (
                          sid+"/rdpserver",
                          ( QVariant ) "" ).toString();
            rootless=st->setting()->value ( sid+"/rootless",
                                            ( QVariant ) false ).toBool();
            resumingSession.published=st->setting()->value ( sid+"/published",
                                      ( QVariant ) false ).toBool();

            rdpFS=st->setting()->value (
                      sid+"/fullscreen",
                      ( QVariant ) defaultFullscreen ).toBool();
            rdpHeight=st->setting()->value (
                          sid+"/height",
                          ( QVariant ) defaultHeight ).toString();
            rdpWidth=st->setting()->value (
                         sid+"/width",
                         ( QVariant ) defaultWidth ).toString();

        }
        delete st;
    }
    else
    {
        command=config.command;
        rootless=config.rootless;
        resumingSession.published=config.published;
    }

    resumingSession.command=command;

    if ( rootless )
        sessionType="R";
    if ( resumingSession.published )
    {
        sessionType="P";
        command="PUBLISHED";
    }

    if ( command=="KDE" )
    {
        command="startkde";
    }
    else if ( command=="GNOME" )
    {
        command="gnome-session";
    }
    else if ( command=="UNITY" )
    {
        command="unity";
    }
    else if ( command=="XFCE" )
    {
        command="xfce4-session";
    }
    else if ( command=="MATE" )
    {
        command="mate-session";
    }
    else if ( command=="LXDE" )
    {
        command="startlxde";
    }
    else if (command == "LXQt")
    {
        command = "startlxqt";
    }
    // Note that there are multiple possible Cinnamon commands that the server
    // may call.
    else if ( command=="CINNAMON" )
    {
        command="cinnamon-session";
    }
    else if ( command=="TRINITY" )
    {
        command="starttrinity";
    }
    else if ( command=="OPENBOX" )
    {
        command="openbox-session";
    }
    else if ( command=="ICEWM" )
    {
        command="icewm-session";
    }
    else if ( command=="RDP" )
    {
        command="rdesktop ";
        if ( rdpFS )
        {
            command+=" -f ";
            sessionType="D";
            rootless=false;
        } else {
            command+=" -g "+rdpWidth+"x"+rdpHeight;
            rootless=true;
        }
        command+=" "+rdpOpts+ " "+rdpServer;

    }

    if ( managedMode )
        return;


    QString cmd;

    command.replace ( " ","X2GO_SPACE_CHAR" );
    QString krbFwString;

    if(sshConnection->useKerberos() && sshConnection->get_kerberosDelegation())
    {
        krbFwString="KRB5CCNAME=`echo $KRB5CCNAME |sed 's/FILE://g'` \
        KRBFL=$HOME/.x2go/C-"+resumingSession.sessionId+"/krb5cc ;\
        cp -a $KRB5CCNAME $KRBFL;KRB5CCNAME=$KRBFL ";
    }

    if ( !startSessSound  || startSessSndSystem==PULSE )
    {
        cmd=krbFwString+"setsid " + output + "/bin/x2goruncommand "+
            resumingSession.display+" "+
            resumingSession.agentPid + " " +
            resumingSession.sessionId+" "+
            resumingSession.sndPort+ " "+ command+" nosnd "+
            sessionType +" 1> /dev/null 2>/dev/null & exit";
        if ( startSessSndSystem ==PULSE )
        {
            cmd="export PULSE_CLIENTCONFIG=\"${HOME}/.x2go/C-"+
                resumingSession.sessionId+
                "/.pulse-client.conf\";"+cmd;
        }
    }
    else
    {
        switch ( startSessSndSystem )
        {
        case ESD:
            cmd=krbFwString+"setsid " + output + "/bin/x2goruncommand "+
                resumingSession.display+" "+
                resumingSession.agentPid + " " +
                resumingSession.sessionId+" "+
                resumingSession.sndPort+ " "+
                command+" esd "+
                sessionType +" 1> /dev/null 2>/dev/null & exit";
            break;
        case ARTS:
            cmd=krbFwString+"setsid " + output + "/bin/x2goruncommand "+
                resumingSession.display+" "+
                resumingSession.agentPid + " " +
                resumingSession.sessionId+" "+
                resumingSession.sndPort+ " "+
                command+" arts "+
                sessionType +" 1> /dev/null 2>/dev/null & exit";
            break;

        }
    }

    sshConnection->executeCommand ( cmd, this,
                                    SLOT ( slotRetRunCommand ( bool,
                                    QString,
                                    int )), false);
}

void ONMainWindow::slotRetRunCommand ( bool result, QString output,
                                       int )
{
    if ( result==false )
    {
        QString message=tr ( "<b>Connection failed.</b>\n:\n" ) +output;
        if ( message.indexOf ( "publickey,password" ) !=-1 )
        {
            message=tr ( "<b>Wrong password!</b><br><br>" ) +
                    message;
        }
        QMessageBox::critical ( 0l,tr ( "Error" ),message,
                                QMessageBox::Ok,
                                QMessageBox::NoButton );
    }
    else
    {
        if (resumingSession.published)
            readApplications();
    }
}

void ONMainWindow::readApplications()
{
    sshConnection->executeCommand ( "x2gogetapps", this,  SLOT ( slotReadApplications ( bool,
                                    QString,
                                    int) ));
    sbApps->setEnabled(false);
}

void ONMainWindow::slotReadApplications(bool result, QString output,
                                        int)
{
    if ( result==false )
    {
        QString message=tr ( "<b>Connection failed.</b>\n:\n" ) +output;
        if ( message.indexOf ( "publickey,password" ) !=-1 )
        {
            message=tr ( "<b>Wrong password!</b><br><br>" ) +
                    message;
        }
        QMessageBox::critical ( 0l,tr ( "Error" ),message,
                                QMessageBox::Ok,
                                QMessageBox::NoButton );
        return;
    }
    sbApps->setEnabled(true);
    applications.clear();
    QString locallong=QLocale::system().name();
    QString localshort=QLocale::system().name().split("_")[0];
    QStringList startAppsFound;

    foreach(QString appstr, output.split("</desktop>",Qt::SkipEmptyParts))
    {
        bool localcomment=false;
        bool localname=false;
        Application app;
        app.category=Application::OTHER;
        QStringList lines=appstr.split("\n", Qt::SkipEmptyParts);
        for (int i=0; i<lines.count(); ++i)
        {
            QString line=lines[i];
            if (line.indexOf("Name["+localshort+"]=")!=-1  || line.indexOf("Name["+locallong+"]=")!=-1)
            {
                app.name=QString::fromUtf8(line.split("=")[1].toLatin1());

                //                 x2goDebug<<"local name: "<<app.name<<Qt::endl;

                localname=true;
            }
            if (line.indexOf("Comment["+localshort+"]=")!=-1 || line.indexOf("Comment["+locallong+"]=")!=-1)
            {
                app.comment=QString::fromUtf8(line.split("=")[1].toLatin1());
                //                 x2goDebug<<"local comment: "<<app.comment<<Qt::endl;
                localcomment=true;
            }
            if (line.indexOf("Name=")!=-1 && !localname)
            {
                app.name=line.split("=")[1];
                for (int i=0; i<autostartApps.length(); ++i)
                {
                    if (app.name==autostartApps[i])
                        startAppsFound.append(app.name);
                    //                 x2goDebug<<"name: "<<app.name<<Qt::endl;
                }
            }
            if (line.indexOf("Comment=")!=-1 && !localcomment)
            {
                app.comment=line.split("=")[1];
                //                 x2goDebug<<"comment: "<<app.comment<<Qt::endl;
            }
            if (line.indexOf("Exec=")!=-1)
            {
                app.exec=line.split("=")[1];
                app.exec.replace(" %f","",Qt::CaseInsensitive);
                app.exec.replace(" %u","",Qt::CaseInsensitive);
                app.exec.replace("%f","",Qt::CaseInsensitive);
                app.exec.replace("%u","",Qt::CaseInsensitive);
                app.exec.replace("%i","",Qt::CaseInsensitive);
                app.exec.replace("%c",app.name,Qt::CaseInsensitive);
                for (int i=0; i<autostartApps.length(); ++i)
                {
                    if (app.exec==autostartApps[i])
                        startAppsFound.append(app.exec);
                    //                 x2goDebug<<"exec: "<<app.exec<<Qt::endl;
                }
            }
            if (line.indexOf("Categories=")!=-1)
            {
                if (line.indexOf("Audio")!=-1)
                    app.category=Application::MULTIMEDIA;
                if (line.indexOf("Vide")!=-1)
                    app.category=Application::MULTIMEDIA;
                if (line.indexOf("Development")!=-1)
                    app.category=Application::DEVELOPMENT;
                if (line.indexOf("Education")!=-1)
                    app.category=Application::EDUCATION;
                if (line.indexOf("Game")!=-1)
                    app.category=Application::GAME;
                if (line.indexOf("Graphics")!=-1)
                    app.category=Application::GRAPHICS;
                if (line.indexOf("Network")!=-1)
                    app.category=Application::NETWORK;
                if (line.indexOf("Office")!=-1)
                    app.category=Application::OFFICE;
                if (line.indexOf("Settings")!=-1)
                    app.category=Application::SETTINGS;
                if (line.indexOf("System")!=-1)
                    app.category=Application::SYSTEM;
                if (line.indexOf("Utility")!=-1)
                    app.category=Application::UTILITY;
                if (line.indexOf("X2Go-Top",0,Qt::CaseInsensitive)!=-1)
                    app.category=Application::TOP;
            }
            if (line.indexOf("<icon>")!=-1)
            {
                bool isSvg=false;
                line=lines[++i];
                QByteArray pic;
                while (line.indexOf("</icon>")==-1)
                {
                    pic+=QByteArray::fromBase64(line.toLatin1());
                    line=lines[++i];
                    if (QString(QByteArray::fromBase64(line.toLatin1())).indexOf("</svg>",Qt::CaseInsensitive)!=-1)
                    {
                        isSvg=true;
                    }
                }
                if (!isSvg)
                    app.icon.loadFromData(pic);
                else
                {
                    QPixmap pix(32,32);
                    QSvgRenderer svgRenderer( pic );
                    QPainter pixPainter(&pix);
                    svgRenderer.render(&pixPainter);
                    app.icon=pix;
                }
            }
        }
        if (app.name.length()>0)
        {
            if (app.comment.length()<=0)
                app.comment=app.name;
            applications.append(app);
        }
    }

    qSort(applications.begin(), applications.end(),Application::lessThen);
    plugAppsInTray();
    if (runStartApp && autostartApps.length()>0)
    {
        for (int i=0; i<autostartApps.length(); ++i)
        {
            bool startAppFound = false;
            for (int j=0; j<startAppsFound.length(); ++j)
            {
                if (startAppsFound[j] == autostartApps[i])
                {
                    startAppFound = true;
                }
            }
            if (!startAppFound) {
                x2goDebug<<"Autostart application "<<autostartApps[i]<< " not found in desktop files.";
            }
            else
            {
                runApplication(autostartApps[i]);
            }
        }
    }
    else
    {
        if(embedMode)
        {
            act_showApps->setVisible(true);
            slotAppDialog();
        }
    }
}


bool ONMainWindow::parseParameter ( QString param )
{
#if defined (Q_OS_UNIX)
    if ("--bypass-cleanup-helper" == param) {
        return (true);
    }
#endif /* defined (Q_OS_UNIX) */

//open session file with .x2go extension
    if ( param.endsWith(".x2go", Qt::CaseInsensitive) )
    {
        ONMainWindow::sessionCfg=expandHome(param);
        defaultSession=true;
        return true;
    }

    if ( param=="--help" )
    {
        showHelp();
        return false;
    }

    if ( param=="--help-pack" )
    {
        showHelpPack();
        return false;
    }

    if ( param=="--version" || param=="-v")
    {
        showVersion();
        return false;
    }

    if ( param=="--changelog" )
    {
        showChangelog();
        return false;
    }

    if ( param=="--git-info" )
    {
        showGit();
        return false;
    }

    if (param == "--debug")
    {
        ONMainWindow::debugging = true;
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
        QLoggingCategory::setFilterRules("default.debug=true");
#endif
        return true;
    }

    if ("--libssh-debug" == param) {
        ONMainWindow::libssh_debugging = true;
        return true;
    }

    if ("--libssh-packetlog" == param) {
        ONMainWindow::libssh_packetlog = true;
        return true;
    }

#if defined (Q_OS_DARWIN) || defined (Q_OS_WIN)
    if (param == "--disable-pulse")
    {
        systemDisablePA=true;
        return true;
    }
    if (param == "--disable-pulse-record")
    {
        systemDisablePARecord=true;
        return true;
    }
#endif /* defined (Q_OS_DARWIN) || defined (Q_OS_WIN) */

    if (param == "--no-autoresume")
    {
        autoresume=false;
        return true;
    }

    if ( param == "--portable" )
    {
        ONMainWindow::portable=true;
        return true;
    }
    if ( param == "--clean-all-files" )
    {
        cleanAllFiles=true;
        return true;
    }
    if (param == "--connectivity-test")
    {
        connTest=true;
        return true;
    }

    if ( param=="--no-menu" )
    {
        drawMenu=false;
        return true;
    }

    if ( param=="--maximize" )
    {
        startMaximized=true;
        return true;
    }
    if ( param=="--xinerama" )
    {
        defaultXinerama=true;
        return true;
    }
    if (param == "--thinclient")
    {
        thinMode=true;
        startMaximized=true;
        return true;
    }
    if (param == "--haltbt")
    {
        showHaltBtn=true;
        return true;
    }
    if (param == "--broker-logoutbt")
    {
        showBrokerLogoutBtn=true;
        return true;
    }
    if ( param=="--hide" )
    {
        startHidden=true;
        return true;
    }
    if ( param=="--keep-trayicon" )
    {
        keepTrayIcon=true;
        return true;
    }
    if ( param=="--hide-foldersharing" )
    {
        hideFolderSharing=true;
        return true;
    }
    if ( param=="--pgp-card" )
    {
        usePGPCard=true;
        return true;
    }
    if ( param=="--ldap-printing" )
    {
        LDAPPrintSupport=true;
        return true;
    }
    if ( param=="--add-to-known-hosts" )
    {
        acceptRsa=true;
        return true;
    }
    if ( param=="--no-session-edit" )
    {
        noSessionEdit=true;
        return true;
    }
    if ( param=="--change-broker-pass")
    {
        changeBrokerPass=true;
        return true;
    }
    if ( param == "--autologin")
    {
        cmdAutologin=true;
        return true;
    }

    if ( param == "--broker-autologin")
    {
        config.brokerAutologin=true;
        return true;
    }
    if ( param == "--broker-krblogin")
    {
        config.brokerKrbLogin=true;
        return true;
    }

    if ( param == "--broker-autologoff")
    {
        config.brokerAutologoff=true;
        return true;
    }

    if ( param == "--broker-noauth")
    {
        config.brokerNoAuth=true;
        return true;
    }

    if ( param=="--broker-noauth-with-session-username" )
    {
        brokerNoauthWithSessionUsername=true;
        return true;
    }

    if ( param=="--broker-use-creds-for-session" )
    {
        brokerCredsForSession=true;
        return true;
    }

    //force to show trayicon
    if (param == "--tray-icon")
    {
        forceToShowTrayicon = true;
        return true;
    }

    if ( param=="--close-disconnect" )
    {
        closeDisconnect=true;
        return true;
    }

    QString setting,value;
    QStringList vals=param.split ( "=" );
    if ( vals.size() <2 )
    {
        printError ( param );
        return false;
    }
    setting=vals[0];
    vals.removeFirst();
    value=vals.join ( "=" );
    if ( setting=="--link" )
    {
        return linkParameter ( value );
    }
    if ( setting=="--clipboard" )
    {
        return clipboardParameter ( value );
    }
    if ( setting=="--sound" )
    {
        return soundParameter ( value );
    }
    if ( setting=="--geometry" )
    {
        return geometry_par ( value );
    }
    if ( setting=="--pack" )
    {
        return packParameter ( value );
    }
    if ( setting=="--kbd-layout" )
    {
        defaultLayout=value.split(",",Qt::SkipEmptyParts);
        if (defaultLayout.size()==0)
            defaultLayout<<tr("us");
        return true;
    }
    if ( setting=="--session" )
    {
        defaultSession=true;
        defaultSessionName=value;
        return true;
    }
    if ( setting=="--session-conf" )
    {
        ONMainWindow::sessionCfg=expandHome(value);
        return true;
    }
    if ( setting=="--sessionid" )
    {
        defaultSession=true;
        defaultSessionId=value;
        return true;
    }
    if ( setting=="--user" )
    {
        defaultUser=true;
        defaultUserName=value;
        return true;
    }
    if ( setting=="--kbd-type" )
    {
        defaultKbdType=value;
        return true;
    }
    if ( setting=="--set-kbd" )
    {
        return setKbd_par ( value );
    }
    if ( setting=="--ldap" )
    {
        return ldapParameter ( value );
    }
    if ( setting=="--ldap1" )
    {
        return ldap1Parameter ( value );
    }
    if ( setting=="--ldap2" )
    {
        return ldap2Parameter ( value );
    }
    if ( setting=="--command" )
    {
        defaultCmd=value;
        return true;
    }
    if ( setting=="--read-exports-from" )
    {
        readExportsFrom=expandHome(value);
        return true;
    }
    if ( setting=="--external-login" )
    {
        extLogin=true;
        readLoginsFrom=expandHome(value);
        return true;
    }
    if ( setting=="--ssh-port" )
    {
        defaultSshPort=value;
        return true;
    }
    if ( setting=="--dpi" )
    {
        defaultSetDPI=true;
        defaultDPI=value.toUInt();
        return true;
    }
    if ( setting=="--client-ssh-port" )
    {
        clientSshPort=value;
        return true;
    }
    if ( setting == "--embed-into" )
    {
        embedMode=true;
        embedParent=value.toLong();
        return true;
    }
    if ( setting == "--broker-url")
    {
        brokerMode=true;
        noSessionEdit=true;
        config.brokerurl=value;
        return true;
    }
    if( setting == "--splash")
    {
        splashPix=value;
        return true;
    }
    if ( setting == "--broker-cacertfile")
    {
        config.brokerCaCertFile=expandHome(value);
        return true;
    }
    if ( setting == "--broker-ssh-key")
    {
        config.brokerSshKey=expandHome(value);
        return true;
    }
    if ( setting == "--ssh-key")
    {
        sshKey key;
        QStringList parts=value.split(":");
        QString authPart;
        switch(parts.length())
        {
        case 1:
            key.key=expandHome(parts[0]);
            break;
        case 2:
            key.key=expandHome(parts[1]);
            authPart=parts[0];
            break;
        case 3:
            authPart=parts[0];
            key.key=expandHome(parts[2]);
            key.port=parts[1];
            break;
        }
        if(authPart.length()>0)
        {
            if(authPart.indexOf("@")!=-1)
            {
                key.user=authPart.split("@")[0];
                key.server=authPart.split("@")[1];
            }
            else
                key.server=authPart;
        }
        cmdSshKeys<<key;
        return true;
    }
    if ( setting == "--broker-name")
    {
        config.brokerName=value;
        return true;
    }
    if ( setting == "--autostart")
    {
        autostartApps.append(value.split(','));

        /* Fix up by trimming whitespace. */
        for (QStringList::iterator it = autostartApps.begin (); it != autostartApps.end (); ++it) {
            *it = it->trimmed ();
        }

        return true;
    }
    if ( setting == "--auth-id")
    {
        QFile file(expandHome(value));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            printError ( param + tr(" (can't open file)"));
            return false;
        }
        QTextStream in(&file);
        config.brokerUserId = in.readLine();
        return true;
    }
    if (setting == "--support-menu")
    {
        value = expandHome(value);
        if (! QFile::exists(value))
        {
            printError( param + tr(" (file does not exist)"));
            return false;
        }
        supportMenuFile=value;
        return true;
    }
    if (setting == "--background")
    {
        value = expandHome(value);
        if (! QFile::exists(value))
        {
            printError( param + tr(" (file does not exist)"));
            return false;
        }
        BGFile=value;
        return true;
    }
    if (setting == "--session-icon")
    {
        value=expandHome(value);
        if (! QFile::exists(value))
        {
            printError( param + tr(" (file does not exist)"));
            return false;
        }
        SPixFile=value;
        return true;
    }
    if (setting == "--home")
    {
        QDir dr;

        value = expandHome(value);

#ifdef Q_OS_WIN
        int find=value.indexOf("(");
        int lind=value.indexOf(")");
        if (find!=-1 && lind !=-1)
        {
            QString label=value.mid(find+1,lind-find-1);
            x2goDebug<<"Searching for drive with label: "<<label;

            QString drive=wapiGetDriveByLabel(label);
            value.replace("("+label+")",drive);
            x2goDebug<<"New path: "<<value;
        }
#endif
        if (! dr.exists(value))
        {
            printError( param + tr(" (directory does not exist)"));
            return false;
        }
        homeDir=value;
        portableDataPath=value;
        return true;
    }
    if (setting == "--branding")
    {
        value = expandHome(value);
        if (! QFile::exists(value))
        {
            printError( param + tr(" (file does not exist)"));
            return false;
        }
        OnFile=value;
        return true;
    }
    if( setting == "--resources")
    {
        return parseResourceUrl(value);
    }
#if defined(Q_OS_WIN)
    if ("--xserver-start-limit" == setting) {
        bool conv_ret = false;
        signed long long conv = value.toLongLong (&conv_ret);

        if (conv_ret) {
            if (0 >= conv) {
                x2goDebug << "Not limiting X.Org Server starts.";

                x_start_limit_ = 0;
            }
            else {
                if (SIZE_MAX < conv) {
                    conv = SIZE_MAX;

                    x2goDebug << "Clamped X.Org Server start limit to SIZE_MAX.";
                }

                x_start_limit_ = conv;

                x2goDebug << "Limiting X.Org Server starts to " << x_start_limit_ << " tries.";
            }
        }
        else {
            x2goDebug << "Conversion for --xserver-start-limit value " << value << " failed; assuming default of 3.";

            x_start_limit_ = 3;
        }

        return (true);
    }
#endif /* defined(Q_OS_WIN) */

    printError ( param );
    return false;
}


bool ONMainWindow::linkParameter ( QString value )
{
    if ( value=="modem" )
        defaultLink=MODEM;
    else if ( value=="isdn" )
        defaultLink=ISDN;
    else if ( value=="adsl" )
        defaultLink=ADSL;
    else if ( value=="wan" )
        defaultLink=WAN;
    else if ( value=="lan" )
        defaultLink=LAN;
    else
    {
        printError( tr (
                        "Invalid value for parameter \"--link\"."
                    ).toLocal8Bit().data() );
        return false;
    }
    return true;
}

bool ONMainWindow::clipboardParameter ( QString value )
{
    if ( value=="both"  || value=="client" || value=="server"||value == "none")
    {
        defaultClipboardMode=value;
        return true;
    }
    else
    {
        printError( tr (
                        "Invalid value for parameter \"--clipboard\"."
                    ).toLocal8Bit().data() );
        return false;
    }

}

bool ONMainWindow::soundParameter ( QString val )
{
    if ( val=="1" )
        defaultUseSound=true;
    else if ( val=="0" )
        defaultUseSound=false;
    else
    {
        printError( tr ( "Invalid value for "
                         "parameter \"--sound\"." ).toLocal8Bit().data() );
        return false;
    }
    return true;
}

bool ONMainWindow::geometry_par ( QString val )
{
    if ( val=="fullscreen" )
        defaultFullscreen=true;
    else
    {
        QStringList res=val.split ( "x" );
        if ( res.size() !=2 )
        {
            printError( tr (
                            "Invalid value for parameter \"--geometry\"." ).
                        toLocal8Bit().data() );
            return false;
        }
        bool o1,o2;
        defaultWidth=res[0].toInt ( &o1 );
        defaultHeight=res[1].toInt ( &o2 );
        if ( ! ( defaultWidth >0 && defaultHeight >0 && o1 && o2 ) )
        {
            printError( tr (
                            "Invalid value for parameter \"--geometry\"." ).
                        toLocal8Bit().data() );
            return false;
        }
    }
    return true;
}

bool ONMainWindow::setKbd_par ( QString val )
{
    if ( val=="1" )
        defaultSetKbd=true;
    else if ( val=="0" )
        defaultSetKbd=false;
    else
    {
        printError( tr (
                        "Invalid value for parameter \"--set-kbd\"." ).
                    toLocal8Bit().data() );
        return false;
    }
    return true;
}

bool ONMainWindow::ldapParameter ( QString val )
{
    QString ldapstring=val;
    useLdap=true;
    ldapstring.replace ( "\"","" );
    QStringList lst=ldapstring.split ( ':',Qt::SkipEmptyParts );
    if ( lst.size() !=3 )
    {
        printError( tr (
                        "Invalid value for parameter \"--ldap\"." ).
                    toLocal8Bit().data() );
        return false;
    }
    ldapOnly=true;
    ldapServer=lst[0];
    ldapPort=lst[1].toInt();
    ldapDn=lst[2];


    return true;
}

bool ONMainWindow::ldap1Parameter ( QString val )
{
    QString ldapstring=val;
    ldapstring.replace ( "\"","" );
    QStringList lst=ldapstring.split ( ':',Qt::SkipEmptyParts );
    if ( lst.size() !=2 )
    {
        printError( tr (
                        "Invalid value for parameter \"--ldap1\"." ).
                    toLocal8Bit().data() );
        return false;
    }
    ldapServer1=lst[0];
    ldapPort1=lst[1].toInt();

    return true;
}

bool ONMainWindow::ldap2Parameter ( QString val )
{
    QString ldapstring=val;
    ldapstring.replace ( "\"","" );
    QStringList lst=ldapstring.split ( ':',Qt::SkipEmptyParts );
    if ( lst.size() !=2 )
    {
        printError(
            tr (
                "Invalid value for parameter \"--ldap2\"." ).
            toLocal8Bit().data() );
        return false;
    }
    ldapServer2=lst[0];
    ldapPort2=lst[1].toInt();

    return true;
}


bool ONMainWindow::packParameter ( QString val )
{

    QFile file ( ":/txt/packs" );
    if ( !file.open ( QIODevice::ReadOnly | QIODevice::Text ) )
        return true;
    QTextStream in ( &file );
    while ( !in.atEnd() )
    {
        QString pc=in.readLine();
        if ( pc.indexOf ( "-%" ) !=-1 )
        {
            pc=pc.left ( pc.indexOf ( "-%" ) );

            QStringList pctails=val.split ( "-" );
            QString pcq=pctails[pctails.size()-1];
            pctails.removeLast();

            if ( pctails.join ( "-" ) ==pc )
            {
                bool ok;
                int v=pcq.toInt ( &ok );
                if ( ok && v>=0 && v<=9 )
                {
                    defaultPack=pc;
                    defaultQuality=v;
                    return true;
                }
                else
                    break;
            }
        }
        else
        {
            if ( pc==val )
            {
                defaultPack=val;
                return true;
            }
        }
    }
    file.close();
    printError ( tr("Invalid value for parameter \"--pack\"." ));
    return false;
}


void ONMainWindow::printError ( QString param )
{
    if( !startHidden )
    {
        qCritical ( "%s", ( tr ( "Wrong parameter: " ) +param ).
                    toLocal8Bit().data() );
        if (!startHidden && !haveTerminal)
        {
            QMessageBox::critical(0,tr("Error"), tr ( "Wrong parameter: " ) +param);
        }
    }
    else
    {
        x2goErrorf(8)<<tr("Wrong parameter: ")<<param;
    }
}

void ONMainWindow::showHelp ()
{
    QString out = help::pretty_print ();
    if (!startHidden && !haveTerminal) {
        HelpDialog dlg (this);
        dlg.setWindowTitle (tr ("Help"));
        dlg.setText (help::pretty_print (false));
        dlg.exec ();
    }
}

void ONMainWindow::showHelpPack()
{
    qCritical ( "%s",tr (
                    "Available pack methods:" ).toLocal8Bit().data() );
    QFile file ( ":/txt/packs" );
    if ( !file.open ( QIODevice::ReadOnly | QIODevice::Text ) )
        return;
    QTextStream in ( &file );
    QString msg;
    while ( !in.atEnd() )
    {
        QString pc=in.readLine();
        if ( pc.indexOf ( "-%" ) !=-1 )
        {
            pc=pc.left ( pc.indexOf ( "-%" ) );
            pc+="-[0-9]";
        }
        msg+=pc+"\n";
    }
    file.close();
    qCritical().nospace () << qPrintable (msg);
    if (!startHidden && !haveTerminal)
    {
        HelpDialog dlg(this);
        dlg.setWindowTitle(tr("Pack Methods"));
        dlg.setText(msg);
        dlg.exec();
    }
}

void ONMainWindow::showTextFile(QString fname, QString title)
{
    QFile file ( fname );
    if ( !file.open ( QIODevice::ReadOnly | QIODevice::Text ) )
        return;
    QTextStream in ( &file );
    QString msg=in.readAll();
    file.close();
    qCritical().nospace () << qPrintable (msg);
    if (!startHidden && !haveTerminal)
    {
        HelpDialog dlg(this);
        dlg.setWindowTitle(title);
        dlg.setText(msg);
        dlg.exec();
    }

}


void ONMainWindow::showChangelog()
{
    if(!QFile::exists(":/txt/changelog"))
    {
        qCritical()<<tr("Option is not available in this build.");
        return;
    }
    showTextFile(":/txt/changelog", tr("Changelog"));
}

void ONMainWindow::showGit()
{
    if(!QFile::exists(":/txt/git-info"))
    {
        qCritical()<<tr("Option is not available in this build.");
        return;
    }
    showTextFile(":/txt/git-info", tr("Git Info"));
}

void ONMainWindow::showVersion()
{
    qCritical().nospace () << VERSION;

    if (!startHidden && !haveTerminal)
    {
        slotAbout();
    }
}


void ONMainWindow::slotGetServers ( bool result, QString output,
                                    int )
{
    if ( result==false )
    {
        cardReady=false;
        cardStarted=false;

        QString message=tr ( "<b>Connection failed.</b>\n" ) +output;
        if ( message.indexOf ( "publickey,password" ) !=-1 )
        {
            message=tr ( "<b>Wrong password!</b><br><br>" ) +
                    message;
        }

        QMessageBox::critical ( 0l,tr ( "Error" ),message,
                                QMessageBox::Ok,
                                QMessageBox::NoButton );
        setEnabled ( true );
        passForm->setEnabled ( true );
        pass->setFocus();
        pass->selectAll();
        return;
    }

    passForm->hide();
    setUsersEnabled ( false );
    uname->setEnabled ( false );
    u->setEnabled ( false );
    QStringList servers=output.trimmed().split ( '\n' );
    for ( int i=0; i<servers.size(); ++i )
    {
        QStringList lst=servers[i].simplified().split ( ' ' );
        if ( lst.size() >1 )
        {
            for ( int j=0; j<x2goServers.size(); ++j )
                if ( x2goServers[j].name==lst[0] )
                {
                    x2goServers[j].sess=
                        lst[1].toInt() *
                        x2goServers[j].factor;

                    x2goDebug<<x2goServers[j].name<<
                             ": sessions "<<
                             lst[1].toInt() <<
                             ", multiplied "<<x2goServers[j].sess;

                    break;
                }
        }
    }

    qSort ( x2goServers.begin(),x2goServers.end(),serv::lt );

    listedSessions.clear();
    retSessions=0;
    if (sshConnection)
    {
        delete sshConnection;
        sshConnection=0;
    }
    QString passwd;
    QString user=getCurrentUname();
    passwd=getCurrentPass();
    for (int i=0; i< serverSshConnections.count(); ++i)
    {
        if (serverSshConnections[i])
            delete serverSshConnections[i];
    }
    serverSshConnections.clear();
    for ( int j=0; j<x2goServers.size(); ++j )
    {
        QString host=x2goServers[j].name;
        sshPort=x2goServers[j].sshPort;
        serverSshConnections<<startSshConnection ( host,sshPort,acceptRsa,user,passwd,true,false,true);
    }
}


void ONMainWindow::slotListAllSessions ( bool result,QString output,
        int )
{
    bool last=false;

    ++retSessions;
    if ( retSessions == x2goServers.size() )
        last=true;

    if ( result==false )
    {
        QString message=tr ( "<b>Connection failed.</b>\n" ) +output;
        if ( message.indexOf ( "publickey,password" ) !=-1 )
        {
            message=tr ( "<b>Wrong password!</b><br><br>" ) +
                    message;
        }

        QMessageBox::critical ( 0l,tr ( "Error" ),message,
                                QMessageBox::Ok,
                                QMessageBox::NoButton );
        QString sv=output.split ( ":" ) [0];
        for ( int j=0; j<x2goServers.size(); ++j )
        {
            if ( x2goServers[j].name==sv )
            {
                x2goServers[j].connOk=false;
            }
        }
    }
    else
    {
        listedSessions+=output.trimmed().split ( '\n',
                        Qt::SkipEmptyParts );
    }
    if ( last )
    {
        if ( listedSessions.size() ==0||
                ( listedSessions.size() ==1 &&
                  listedSessions[0].length() <5 ) )
        {
            x2goDebug<<"Starting new session.";
            startNewSession();
        }
        else if ( listedSessions.size() ==1 )
        {
            x2goDebug<<"Already having a session.";
            x2goSession s=getSessionFromString (
                              listedSessions[0] );

            x2goDebug<<"Will proceed with this session.";

            QDesktopWidget wd;

            /* Check getSessionFromString for what this "invalid" string means. */
            if ((s.agentPid != "invalid") && (s.status == "S")
                && (isColorDepthOk (wd.depth (), s.colorDepth)))
            {
                resumeSession ( s );
            }
            else
            {
                x2goDebug<<"Please select one session ...";
                selectSession ( listedSessions );
            }
        }
        else
        {
            selectSession ( listedSessions );
        }
    }
}

void ONMainWindow::slotResize()
{
    if ( startHidden )
    {
        hide();
        return;
    }
    if ( !startMaximized && !mwMax )
    {
        resize ( mwSize );
        move ( mwPos );

        show();
    }
    else
        showMaximized();
}

void ONMainWindow::slotExportDirectory()
{
    if ( shadowSession )
        return;

    bool hide_after=false;
    if (isHidden())
    {
        showNormal();
        hide_after=true;
    }
    QString path;
    if ( !useLdap && !embedMode )
    {
        ExportDialog dlg ( sessionExplorer->getLastSession()->id(),this );
        if ( dlg.exec() ==QDialog::Accepted )
            path=dlg.getExport();
    }
    else

        path= QFileDialog::getExistingDirectory (
                  this,QString(),
                  homeDir );
    if (hide_after)
        hide();
#ifdef Q_OS_WIN
    if ( ONMainWindow::getPortable() &&
            ONMainWindow::U3DevicePath().length() >0 )
    {
        path.replace ( "(U3)",u3Device );
    }

    path=cygwinPath ( wapiShortFileName ( path ) );
#endif
    if ( path!=QString() )
        exportDirs ( path );
}


void ONMainWindow::exportDirs ( QString exports,bool removable )
{
    if ( shadowSession )
        return;
    if ( embedMode )
    {
        if ( config.confFS  && ! ( config.useFs ) )
        {
            return;
        }
    }
    fsExportKeyReady=false;
    directory dr;


    dr.dirList=exports;
    dr.key=createKeyBundle();

    // Key creation failure or the like.
    if (dr.key.isEmpty ()) {
      return;
    }

    QString passwd;

    x2goDebug<<"Key created on: "<<dr.key;

    passwd=getCurrentPass();

    fsInTun=false;
    if ( !useLdap )
    {
        if ( !embedMode )
        {
            X2goSettings st ( "sessions" );

            if (!(sessionExplorer->getLastSession())) {
                x2goDebug << "No session selected, not exporting directories.";
                return;
            }

            QString sid=sessionExplorer->getLastSession()->id();

            fsInTun=st.setting()->value ( sid+"/fstunnel",
                                          ( QVariant ) true ).toBool();
        }
        else
            fsInTun=true;
    }
    if ( fsInTun )
    {
        if ( fsTunnel==0l )
            if ( startSshFsTunnel() )
                return;
    }


    QString uname=getCurrentUname();

    /* For the destination, fetch the base name only. */
    QFileInfo tmp_file_info (dr.key);
    QString dst = tmp_file_info.fileName ();

    dst="~"+uname +"/.x2go/ssh/"+dst;
    dr.dstKey=dst;
    dr.isRemovable=removable;
    exportDir.append ( dr );
    QString keyFile=dr.key;
    sshConnection->copyFile ( keyFile,dst, this,  SLOT ( slotCopyKey ( bool, QString,int ) ));

}


void ONMainWindow::exportDefaultDirs()
{
    QStringList dirs;
    bool clientPrinting= ( useLdap && LDAPPrintSupport );

    if ( !useLdap )
    {
        if ( !embedMode )
        {
            if (!(sessionExplorer->getLastSession())) {
                x2goDebug << "No session selected, not exporting default directories.";
                return;
            }

            X2goSettings* st;
            if (!brokerMode)
                st= new X2goSettings( "sessions" );
            else
                st= new X2goSettings(config.iniFile,QSettings::IniFormat);

            clientPrinting= st->setting()->value (
                                sessionExplorer->getLastSession()->id() +
                                "/print", true ).toBool();

            QString exd=st->setting()->value (
                            sessionExplorer->getLastSession()->id() +"/export",
                            ( QVariant ) QString() ).toString();
            QStringList lst=exd.split ( ";",
                                        Qt::SkipEmptyParts );
            for ( int i=0; i<lst.size(); ++i )
            {
                //check if directory is in Windows client format
                QChar splitChar='#';
                if(lst[i].indexOf(splitChar)==-1)
                {
                    //Linux client format
                    splitChar=':';
                }
                QStringList tails=lst[i].split (
                                      splitChar,
                                      Qt::SkipEmptyParts );
                if(tails.length()!=2)
                {
                    //wrong format
                    continue;
                }
                if ( tails[1]=="1" )
                {
#ifdef Q_OS_WIN
                    if ( ONMainWindow::getPortable() &&
                            ONMainWindow::U3DevicePath (
                            ).length() >0 )
                    {
                        tails[0].replace (
                            "(U3)",u3Device );
                    }
#endif

                    tails[0].replace("<HOME>",QDir::homePath());
                    if(!QFile::exists(tails[0]))
                    {
                        x2goDebug<<"Path "<<tails[0]<<" not found, trying to create it";
                        QDir dr;
                        if(!dr.mkpath(tails[0]))
                        {
                            x2goDebug<<"Failed to create "<<tails[0];
                            continue;
                        }
                    }

#ifdef Q_OS_WIN
                    tails[0]=cygwinPath (
                                 wapiShortFileName (
                                     tails[0] ) );
#endif
                    dirs+=tails[0];
                }
            }
            delete st;
        }
        else
        {
            clientPrinting=true;
            if ( config.confFS )
            {
                clientPrinting=config.useFs;
            }
        }
    }

    if ( clientPrinting )
    {
        QString path= homeDir +
                      "/.x2go/S-"+
                      resumingSession.sessionId +"/spool";
        QDir spooldir;
        if ( !spooldir.exists ( path ) )
        {
            if ( !spooldir.mkpath ( path ) )
            {
                QString message=
                    tr (
                        "Unable to create directory:" ) + path;
                QMessageBox::critical ( 0l,tr (
                                            "Error" ),message,
                                        QMessageBox::Ok,
                                        QMessageBox::NoButton );

            }
        }
        spoolDir=path;
        QFile::setPermissions (
            path,QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner );
#ifdef Q_OS_WIN
        wapiSetFilePermissions(path);
        path=cygwinPath (
                 wapiShortFileName (
                     path ) );
#endif

        path+="__PRINT_SPOOL_";
        dirs+=path;
        printSupport=true;
        if ( spoolTimer )
            delete spoolTimer;
        spoolTimer=new QTimer ( this );
        connect ( spoolTimer,SIGNAL ( timeout() ),this,
                  SLOT ( slotCheckPrintSpool() ) );
        spoolTimer->start ( 2000 );
    }
    if ( dirs.size() <=0 )
        return;

    exportDirs ( dirs.join ( ":" ) );
}

void ONMainWindow::slotCopyKey ( bool result, QString output, int pid)
{
    fsExportKey=sshConnection->getSourceFile(pid);

    x2goDebug<<"Exported key: "<<fsExportKey;

    QFile::remove ( fsExportKey );
    x2goDebug<<"Key removed.";

    if ( result==false )
    {
        QString message=tr ( "<b>Connection failed.</b>\n" ) +output;
        if ( message.indexOf ( "publickey,password" ) !=-1 )
        {
            message=tr ( "<b>Wrong password!</b><br><br>" ) +
                    message;
        }

        if (!startHidden)
        {
            QMessageBox::critical ( 0l,tr ( "Error" ),message,
                                    QMessageBox::Ok,
                                    QMessageBox::NoButton );
        }
        else
        {
            QString printout = tr( "Connection failed: ")  + output.toLatin1();

            if ( output.indexOf ( "publickey,password" ) !=-1 )
                x2goErrorf(11)<< tr( "Connection failed: ")  + output + tr(" - Wrong password.");
            else
                x2goErrorf(12)<< tr( "Connection failed: ")  + output;
            trayQuit();
        }

        QFile::remove ( fsExportKey+".pub" );
        return;
    }
    fsExportKeyReady=true;

    //start reverse mounting if RSA Key and FS tunnel are ready
    //start only once from slotFsTunnelOk() or slotCopyKey().
    if ( !fsInTun || fsTunReady )
        startX2goMount();

}

directory* ONMainWindow::getExpDir ( QString key )
{
    for ( int i=0; i<exportDir.size(); ++i )
    {
        if ( exportDir[i].key==key )
            return &exportDir[i];
    }
    return 0l;
}




void ONMainWindow::slotRetExportDir ( bool result,QString output,
                                      int pid)
{
    x2goDebug<<"Post-cleanup for startX2goMount triggered."<<Qt::endl;
    QString key;
    for ( int i=0; i<exportDir.size(); ++i )
        if ( exportDir[i].pid==pid )
        {
            key=exportDir[i].key;
            exportDir.removeAt ( i );
            break;
        }

    if ( result==false )
    {
        QString message=tr ( "<b>Connection failed.</b>\n" ) +output;
        x2goDebug<<"startX2goMount failed to mount client-side folder, reason: "<<message<<Qt::endl;
        if ( message.indexOf ( "publickey,password" ) !=-1 )
        {
            message=tr ( "<b>Wrong password!</b><br><br>" ) +
                    message;
        }

        QMessageBox::critical ( 0l,tr ( "Error" ),message,
                                QMessageBox::Ok,
                                QMessageBox::NoButton );
    }
    QFile file ( key+".pub" );
    x2goDebug << "Deactivating public key from " << QString (key + ".pub") << " again.";
    if ( !file.open ( QIODevice::ReadOnly | QIODevice::Text ) )
    {
        printSshDError_noExportPubKey();
        QFile::remove
        ( key+".pub" );
        return;
    }

    QByteArray line = file.readLine();
    file.close();

    QDir authorized_keys_dir (homeDir);
    authorized_keys_dir = QDir (authorized_keys_dir.absolutePath () + "/.x2go/.ssh/");

    QFile authorized_keys_file (authorized_keys_dir.absolutePath () + "/authorized_keys");

    /*
     * We do not try to create the file first.
     * This has been already done in startX2goMount().
     * We wouldn't be here if that failed.
     */
    if (!authorized_keys_file.open (QIODevice::ReadOnly | QIODevice::Text)) {
      printSshDError_noAuthorizedKeysFile ();
      QFile::remove (key + ".pub");
      return;
    }

    QTemporaryFile tfile (authorized_keys_file.fileName ());
    tfile.open ();
    tfile.setPermissions (QFile::ReadOwner | QFile::WriteOwner);
    tfile.setAutoRemove (true);
    QTextStream out (&tfile);

    /*
     * Copy the content of the authorized_keys file to our new temporary file
     * and remove the public authorized key for the current "session" again.
     */
    while (!authorized_keys_file.atEnd ()) {
      QByteArray newline = authorized_keys_file.readLine ();
      if (newline != line)
        out << newline;
    }

    authorized_keys_file.close ();
    tfile.close ();

    authorized_keys_file.remove ();

    tfile.copy (authorized_keys_file.fileName ());
    QFile::remove (key + ".pub");
}


void ONMainWindow::slotExtTimer()
{

    if ( QFile::permissions ( readLoginsFrom ) !=
            ( QFile::ReadUser|QFile::WriteUser|QFile::ExeUser|
              QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner ) )
    {

        x2goDebug<<"Wrong permissions on "<<readLoginsFrom <<":";
        x2goDebug<< ( int ) ( QFile::permissions (
                                  readLoginsFrom+"/." ) )
                 <<"must be"<< ( int ) ( QFile::ReadUser|QFile::WriteUser
                                         |QFile::ExeUser|QFile::ReadOwner|
                                         QFile::WriteOwner|
                                         QFile::ExeOwner ) <<Qt::endl;

        if ( extLogin )
            extTimer->stop();
        return;
    }
    QString loginDir;
    QString logoutDir;
    QDir dir ( readLoginsFrom );
    QStringList list = dir.entryList ( QDir::Files );
    for ( int i=0; i<list.size(); ++i )
    {
        QFile file ( readLoginsFrom+"/"+list[i] );
        if ( !file.open ( QIODevice::ReadOnly | QIODevice::Text ) )
            continue;
        if ( !file.atEnd() )
        {
            QByteArray line = file.readLine();
            QString ln ( line );
            QStringList args=ln.split ( "=",
                                        Qt::SkipEmptyParts );
            if ( args.size() >1 )
            {
                if ( args[0]=="login" )
                {
                    args[1].replace ( "\n","" );
                    if ( args[1].size() )
                        loginDir=args[1];
                }
                if ( args[0]=="logout" )
                {

                    x2goDebug<<"External logout.";

                    args[1].replace ( "\n","" );
                    if ( args[1].size() )
                        logoutDir=args[1];
                }
            }
        }
        file.close();
        file.remove();
    }
    if ( exportTimer->isActive() ) //running session
    {
        if ( logoutDir != QString() )
        {
            x2goDebug<<"External logout received";
            externalLogout ( logoutDir );
        }
    }
    else
    {
        if ( loginDir != QString() )
        {
            x2goDebug<<"External login.";
            externalLogin ( loginDir );
        }
    }
}


void ONMainWindow::slotExportTimer()
{

    if ( QFile::permissions ( readExportsFrom ) != ( QFile::ReadUser|
            QFile::WriteUser|
            QFile::ExeUser|
            QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner ) )
    {

        x2goDebug<<"Wrong permissions on "<<
                 readExportsFrom <<":"<<Qt::endl;
        x2goDebug<< ( int ) ( QFile::permissions (
                                  readExportsFrom+"/." ) )
                 <<"must be"<< ( int ) ( QFile::ReadUser|QFile::WriteUser
                                         |QFile::ExeUser|QFile::ReadOwner|
                                         QFile::WriteOwner|
                                         QFile::ExeOwner ) <<Qt::endl;
        exportTimer->stop();
        return;
    }

    QDir dir ( readExportsFrom );
    QStringList list = dir.entryList ( QDir::Files );
    QString expList;
    QString unexpList;
    QString loginDir;
    QString logoutDir;
    for ( int i=0; i<list.size(); ++i )
    {
        QFile file ( readExportsFrom+"/"+list[i] );
        if ( !file.open ( QIODevice::ReadOnly | QIODevice::Text ) )
            continue;
        if ( !file.atEnd() )
        {
            QByteArray line = file.readLine();
            QString ln ( line );
            QStringList args=ln.split ( "=",
                                        Qt::SkipEmptyParts );
            if ( args.size() >1 )
            {
                if ( args[0]=="export" )
                {
                    args[1].replace ( "\n","" );
                    if ( args[1].size() )
                        expList+=":"+args[1];
                }
                if ( args[0]=="unexport" )
                {
                    args[1].replace ( "\n","" );
                    if ( args[1].size() )
                        unexpList+=":"+args[1];
                }
            }
        }
        file.close();
        file.remove();
    }
    QStringList args=expList.split ( ":",Qt::SkipEmptyParts );
    expList=args.join ( ":" );
    if ( expList.size() >0 )
    {
        exportDirs ( expList,true );
    }
    args.clear();
    args=unexpList.split ( ":",Qt::SkipEmptyParts );

    QString passwd=getCurrentPass();
    QString user=getCurrentUname();
    QString host=resumingSession.server;
    QString sessionId=resumingSession.sessionId;

    for ( int i=0; i<args.size(); ++i )
    {
        sshConnection->executeCommand ( "x2goumount_session "+
                                        sessionId+" "+args[i] );
    }
}

void ONMainWindow::slotAboutQt()
{
    QMessageBox::aboutQt ( this );
}

void ONMainWindow::slotSupport()
{
    QFile file(supportMenuFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    QString sup;
    while (!in.atEnd())
    {
        sup+=in.readLine();
    }
    QMessageBox::information (this,tr ( "Support" ),sup);
}

void ONMainWindow::slotAbout()
{
    /*
     * This (extra) comment (for translators) looks fucked up, and it is, but
     * this can't be changed.
     *
     * Adding asterisks in front of a line will cause them to literally appear
     * in the comment, so I'd rather just drop them.
     *
     * Adding any whitespace in front of a line will lead to one whitespace
     * character surviving the lupdate scan, so we have to drop them as well.
     *
     * NOT adding an escape character to the end of a line will make lupdate
     * stupidly replace the newline character with a whitespace character.
     */
    /*:
Translators, feel free to add a localized version of your translation
including a copyright notice and your name(s). Remember to keep a <br>
at the end, though.\n\n\
\
English example for a potential translation:\n\n\
\
<br>(C) 2005-2023 by <b>obviously nice</b>: \
Oleksandr Shneyder, Heinz-Markus Graesing<br>\
English version: (C) 20AB-20CD Trans Lator <trans@la.tor><br>
     */
    QString aboutStr=tr ("<br>(C) 2005-2023 by <b>obviously nice</b>: "
                         "Oleksandr Shneyder, Heinz-Markus Graesing<br>" );
    aboutStr+=
        tr (
            "<br>This is a client to access the X2Go network-based "
            "computing environment. This client will be able "
            "to connect to X2Go Server(s) and start, stop, "
            "resume and terminate (running) desktop sessions. "
            "X2Go Client stores different server connections "
            "and may automatically request authentication "
            "data from LDAP directories. Furthermore, it can be "
            "used as a fullscreen login screen (replacement for "
            "login managers like XDM). Please visit <a "
            "href=\"http://www.x2go.org\">the project home page at "
            "x2go.org</a> for further information." );
    QMessageBox::about (
        this,tr ( "About X2Go Client" ),
        tr ( "<b>X2Go Client v. " ) +VERSION+
        "</b> (Qt - "+qVersion() +")"+
        aboutStr );
}



void ONMainWindow::slotRereadUsers()
{
    if ( !useLdap )
        return;
#ifdef USELDAP

    if ( ld )
    {
        delete ld;
        ld=0;
    }


    if ( ! initLdapSession ( false ) )
    {
        return;
    }


    list<string> attr;
    attr.push_back ( "uidNumber" );
    attr.push_back ( "uid" );


    list<LDAPBinEntry> result;
    try
    {
        ld->binSearch ( ldapDn.toStdString(),attr,
                        "objectClass=posixAccount",result );
    }
    catch ( LDAPExeption e )
    {
        QString message="Exception in: ";
        message=message+e.err_type.c_str();
        message=message+" : "+e.err_str.c_str();
        QMessageBox::critical ( 0l,tr ( "Error" ),message,
                                QMessageBox::Ok,QMessageBox::NoButton );
        QMessageBox::critical ( 0l,tr ( "Error" ),
                                tr ( "Please check LDAP Settings." ),
                                QMessageBox::Ok,QMessageBox::NoButton );
        slotConfig();
        return;
    }

    list<LDAPBinEntry>::iterator it=result.begin();
    list<LDAPBinEntry>::iterator end=result.end();

    for ( ; it!=end; ++it )
    {
        user u;
        QString uin=LDAPSession::getBinAttrValues (
                        *it,"uidNumber" ).front().getData();
        u.uin=uin.toUInt();
        if ( u.uin<firstUid || u.uin>lastUid )
        {
            continue;
        }
        u.uid=LDAPSession::getBinAttrValues (
                  *it,"uid" ).front().getData();
        if ( !findInList ( u.uid ) )
        {
            reloadUsers();
            return;
        }
    }
#endif
}

void ONMainWindow::reloadUsers()
{
    int i;
    for ( i=0; i<names.size(); ++i )
        names[i]->close();

    userList.clear();

    sessionExplorer->cleanSessions();


    loadSettings();
    if ( useLdap )
    {
        act_new->setEnabled ( false );
        act_edit->setEnabled ( false );
        u->setText ( tr ( "Login:" ) );
        QTimer::singleShot ( 1, this, SLOT ( readUsers() ) );
    }
    else
    {
        act_new->setEnabled ( true );
        act_edit->setEnabled ( true );
        u->setText ( tr ( "Session:" ) );
        QTimer::singleShot ( 1, this, SLOT ( slotReadSessions() ) );
    }
    slotResize ( fr->size() );
}


bool ONMainWindow::findInList ( const QString& uid )
{
    for ( int i=0; i<userList.size(); ++i )
    {
        if ( userList[i].uid==uid )
            return true;
    }
    return false;
}

void ONMainWindow::setUsersEnabled ( bool enable )
{

    if ( useLdap )
    {
        QScrollBar* bar=users->verticalScrollBar();
        bar->setEnabled ( enable );
        int upos=bar->value();
        QDesktopWidget dw;
        int height=dw.screenGeometry ( fr ).height();
        QList<UserButton*>::iterator it;
        QList<UserButton*>::iterator endit=names.end();
        if ( !enable )
        {
            for ( it=names.begin(); it!=endit; it++ )
            {
                QPoint pos= ( *it )->pos();
                if ( ( pos.y() >upos-height ) &&
                        ( pos.y() <upos+height ) )
                    ( *it )->setEnabled ( false );
                if ( pos.y() >upos+height )
                    break;
            }
        }
        else
        {
            for ( it=names.begin(); it!=endit; it++ )
            {
                if ( ! ( *it )->isEnabled() )
                    ( *it )->setEnabled ( true );
            }
        }
    }
    else
    {
        users->setEnabled ( enable );
        sessionExplorer->setEnable(enable);
    }
}


void ONMainWindow::externalLogin ( const QString& loginDir )
{
    QFile file ( loginDir+"/username" );
    QString user;

    if ( !file.open ( QIODevice::ReadOnly | QIODevice::Text ) )
        return;
    QTextStream in ( &file );
    while ( !in.atEnd() )
    {
        user=in.readLine();
        break;
    }
    file.close();


    if ( passForm->isVisible() )
        slotClosePass();
    uname->setText ( user );
    slotUnameEntered();
    currentKey=loginDir+"/dsa.key";
    extStarted=true;
    slotPassEnter();
}


void ONMainWindow::externalLogout ( const QString& )
{
    if ( extStarted )
    {
        extStarted=false;
        currentKey=QString();
        if ( nxproxy )
            if ( nxproxy->state() ==QProcess::Running )
                nxproxy->terminate();
        proxyRunning=false;
    }
}


void ONMainWindow::slotStartPGPAuth()
{
    scDaemon=new QProcess ( this );

    connect ( scDaemon,SIGNAL ( readyReadStandardError() ),this,
              SLOT ( slotScDaemonStdErr() ) );
    connect ( scDaemon,SIGNAL ( readyReadStandardOutput() ),this,
              SLOT ( slotScDaemonStdOut() ) );
    connect ( scDaemon,SIGNAL ( finished ( int,QProcess::ExitStatus ) ),
              this,
              SLOT (
                  slotScDaemonFinished ( int, QProcess::ExitStatus ) ) );
    connect (scDaemon, SIGNAL (error (QProcess::ProcessError)), this,
             SLOT (slotScDaemonError (QProcess::ProcessError)));
    scDaemon->start ( "pcsc_scan -n");

}


void ONMainWindow::slotScDaemonStdErr()
{
    QString stdOut ( scDaemon->readAllStandardError() );
    stdOut=stdOut.simplified();
    x2goDebug<<"PCSC error: "<<stdOut;
}

void ONMainWindow::slotScDaemonStdOut()
{
    QString stdOut ( scDaemon->readAllStandardOutput() );
    stdOut=stdOut.simplified();

    x2goDebug<<"PCSC out: |"<<stdOut<<"|";
    if(stdOut.indexOf("Card inserted")!=-1)
    {
        x2goDebug<<"GOT NEW CARD!!!!!";
        startGPG();
    }
    if(stdOut.indexOf("Card removed")!=-1)
    {
       cardReady=false;
       if ( cardStarted )
       {
           cardStarted=false;
           if ( nxproxy )
              if ( nxproxy->state() ==QProcess::Running )
              {
                  x2goDebug<<"Suspending session ...";
                  slotSuspendSessFromSt();
                  x2goDebug<<"Suspended session.";
              }
       }
    }
}

void ONMainWindow::slotScDaemonFinished ( int , QProcess::ExitStatus )
{
    //this should not happen, restart...
    scDaemon=0l;
    x2goDebug<<"SCDAEMON finished."<<Qt::endl;
    slotStartPGPAuth();
}

void ONMainWindow::slotScDaemonError (QProcess::ProcessError error) {
    QString main_text ("pcsc_scan ");
    QString informative_text;

    switch (error) {
        case QProcess::FailedToStart: {
            main_text += tr ("failed to start.");
            informative_text = tr ("Check whether the package providing \"pcsc_scan\" is installed.\n"
                                   "The current search path is: ");

            break;
        }
        case QProcess::Crashed: {
            /* This means the process exited with a non-zero exit code.
             * It didn't really crash at all. Everything is fine.
             * Just restart it. */
            return;
        }
        case QProcess::Timedout: {
            main_text += tr ("didn't start yet.");
            informative_text = tr ("This error shouldn't come up.");
            break;
        }
        case QProcess::WriteError: {
            main_text += tr ("didn't accept a write operation.");
            informative_text = tr ("It is probably not running correctly or crashed in-between.");
            break;
        }
        case QProcess::ReadError: {
            main_text = tr ("Unable to read from scdaemon.");
            informative_text = tr ("It is probably not running correctly or crashed in-between.");
            break;
        }
        case QProcess::UnknownError: {
            main_text += tr ("encountered an unknown error during start up or execution.");
            break;
        }
        default: {
            main_text += tr ("experienced an undefined error.");
            break;
        }
    }

    if (!informative_text.isEmpty ()) {
        informative_text += "\n\n";
    }

    informative_text += tr ("X2Go Client will now terminate.\n\n"
                            "File a bug report as outlined on the <a href=\"http://wiki.x2go.org/doku.php/wiki:bugs\">bugs wiki page</a>.");

    show_RichText_ErrorMsgBox (main_text, informative_text);
    trayQuit ();
}

void ONMainWindow::slotShowPAMSGDialog(bool error, const QString& main_text, const QString& info_text, bool modal)
{
    if(error)
    {
        show_RichText_ErrorMsgBox(main_text, info_text,modal);
    }
    else
    {
        show_RichText_WarningMsgBox(main_text, info_text, modal);
    }
}


void ONMainWindow::slotGpgError()
{
    QString stdOut ( gpg->readAllStandardError() );
    stdOut=stdOut.simplified();

    x2goDebug<<"GPG error: "<<stdOut;

    if ( stdOut.indexOf ( "failed" ) !=-1 )
    {
        QMessageBox::critical ( 0l,tr ( "Error" ),
                                tr ( "No valid card found." ),
                                QMessageBox::Ok,
                                QMessageBox::NoButton );
        gpg->kill();
    }
}


void ONMainWindow::slotGpgFinished ( int exitCode,
                                     QProcess::ExitStatus exitStatus )
{

    x2goDebug<<"GPG finished, exit code: "<<exitCode;
    x2goDebug<<"GPG finished, exit status:"<<exitStatus;

    if ( exitStatus==0 )
    {
        QString stdOut ( gpg->readAllStandardOutput() );
        stdOut.chop ( 1 );

        x2goDebug<<"GPG out: "<<stdOut;

        QStringList lines=stdOut.split ( "\n" );
        QString login;
        QString appId;
        QString authKey;
        for ( int i=0; i<lines.count(); ++i )
        {
            if ( lines[i].indexOf ( "Application ID" ) !=-1 )
            {
                appId=lines[i].split ( ":" ) [1];
            }
            else if ( lines[i].indexOf ( "Login data" ) !=-1 )
            {
                login=lines[i].split ( ":" ) [1];
            }
            else if ( lines[i].indexOf (
                          "Authentication key" ) !=-1 )
            {
                authKey=lines[i].split ( ":" ) [1];
                break;
            }
        }
        appId=appId.simplified();
        login=login.simplified();
        authKey=authKey.simplified();

        x2goDebug<<"Card data: "<<appId<<login<<authKey;

        if ( login=="[not set]" || authKey == "[none]" )
        {

            x2goErrorf(13)<<tr("Card not configured.");

            QMessageBox::critical (
                0l,tr ( "Error" ),
                tr (
                    "This card is unknown to the X2Go system." ),
                QMessageBox::Ok,
                QMessageBox::NoButton );
        }
        else
        {
            cardReady=true;
            cardLogin=login;
            //card is ready
            GPGCardLogin(login);
        }
    }
    gpg=0l;
}

void ONMainWindow::startGPG()
{

        QStringList arguments;
        gpg=new QProcess ( this );
        arguments<<"--card-status";
        connect ( gpg,SIGNAL ( readyReadStandardError() ),
                  this,SLOT ( slotGpgError() ) );
        connect ( gpg,SIGNAL ( finished ( int,
                                          QProcess::ExitStatus ) ),this,
                  SLOT ( slotGpgFinished ( int,
                                           QProcess::ExitStatus ) ) );
        gpg->start ( "gpg",arguments );
        x2goDebug<<"gpg started";

}

void ONMainWindow::GPGCardLogin ( const QString& cardLogin)
{
    if ( !useLdap )
    {
         if ( passForm->isVisible() && !brokerMode)
         {
             if ( passForm->isEnabled() )
             {
                 if ( login->isEnabled() )
                 {
                     login->setText ( cardLogin );
                     slotSessEnter();
                 }
             }
        }
        if(brokerMode && (!config.brokerAutologoff))
        {
            broker->getUserSessions();
        }
    }
    else
    {
        if ( passForm->isVisible() )
            slotClosePass();
        uname->setText ( cardLogin );
        slotUnameEntered();
        slotPassEnter();
    }
}


#if defined ( Q_OS_DARWIN )
QString ONMainWindow::getXDisplay()
{
    QLocalSocket unixSocket (this);
    QString xsocket (getenv ("DISPLAY"));

    if (xsocket.isEmpty ())
    {
        // Mac OS X 10.4 compatibility mode.
        // There, it is possible no $DISPLAY variable is set.
        // Start X11 manually. First, find a free display number.

        x2goDebug<< "Entering 10.4 compat mode, checking for free X11 display.";


        int xFreeDisp = 0;
        QDir xtmpdir ("/tmp/.X11-unix");

        if (xtmpdir.exists ())
        {
            xtmpdir.setFilter (QDir::Files | QDir::System | QDir::NoSymLinks | QDir::NoDotAndDotDot);
            xtmpdir.setSorting (QDir::Name);

            QFileInfoList xtmpdirList = xtmpdir.entryInfoList ();
            bool foundFreeDisp = FALSE;
            xFreeDisp = -1;

            for (int i = 0; (i < 2000) && (!foundFreeDisp); ++i)
            {
                QFileInfo xtmpdirFile (xtmpdir.absolutePath () + "/X" + QString::number (i));

                if ((!xtmpdirFile.exists ()) && (!xtmpdirFile.isSymLink ()))
                {
                    xFreeDisp = i;
                    foundFreeDisp = TRUE;
                }
            }
        }

        // Control flow will go to error condition if no free display port has been found.
        if (xFreeDisp != -1)
        {
            xsocket = "/tmp/.X11-unix/X" + QString::number (xFreeDisp);

            x2goDebug<< "Successfully detected free socket " << xsocket << ".";
        }

        if (!(xsocket.isEmpty ()))
        {
            QString xname = ConfigDialog::getXDarwinDirectory () + "/Contents/MacOS/X11";
            QString xopt = ":" + QString::number (xFreeDisp);
            QProcessEnvironment env = QProcessEnvironment::systemEnvironment ();
            QProcess* startx = new QProcess (this);

            x2goDebug<< "Starting the X server on free display port.";

            /* FIXME: why not passing our current environment, maybe extended via add_to_path ()? */
            env.insert (0, "PATH=/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin:/usr/X11R6/bin");

            startx->setProcessEnvironment (env);
            startx->start (xname + QString (" ") + xopt, QIODevice::NotOpen);
            if (startx->waitForStarted (3000))
            {

                x2goDebug<< "Sleeping for three seconds";
                int sleeptime = 3;
                while ((sleeptime = sleep (sleeptime))) {};

                x2goDebug<< "Leaving OS X 10.4 compat mode.";

            }
        }
    }

    // OS X >= 10.5 starts the X11 server automatically, as soon as the
    // launchd UNIX socket is accessed.
    // On user login, the DISPLAY environment variable is set to this said existing
    // socket.
    // By now, we should have a socket, even on 10.4. Test, if connecting works.
    // Note: common sense may tell you to change this if into an else. Don't.
    // We do not want to skip this part, if coming from the compat section above.
    if (!(xsocket.isEmpty ()))
    {
        if (xsocket[0] == ':')
        {
            // Be backwards compatible with 10.4.
            // Delete the ":" character.
            xsocket.remove (0, 1);
            // xsocket may now contain the display value (one integer),
            // or something like "0.0" - we're only interested in the
            // display value, so keep the first char only.
            if (xsocket.indexOf (".") != -1)
            {
                xsocket = xsocket.left (xsocket.indexOf ("."));
            }
            // Prepend the well-known socket path.
            xsocket.prepend ("/tmp/.X11-unix/X");

            x2goDebug<< "xsocket in compat mode: " << xsocket;

        }

        unixSocket.connectToServer (xsocket);

        if (unixSocket.waitForConnected (10000))
        {
            unixSocket.disconnectFromServer ();

            // Mac OS X 10.4 compat: nxproxy expects
            // a DISPLAY variable like ":0", passing
            // an UNIX socket will just make it error out.
            // Instead of altering the nxproxy code, which does
            // already try to connect to "/tmp/.X11-unix/Xi" with
            // i = display number, pass ":i" as DISPLAY.
            if (xsocket.left (16).compare ("/tmp/.x11-unix/x", Qt::CaseInsensitive) == 0)
            {
                bool ok = FALSE;
                int tmp = -1;

                xsocket = xsocket.mid (16);
                tmp = xsocket.toInt (&ok);

                if (ok)
                {
                    x2goDebug<<"Returning " << QString (":") + xsocket;
                    return (QString (":") + xsocket);
                }
            }
            else
            {
                return (xsocket);
            }
        }
    }
    // And if not, error out.
    show_XQuartz_start_error ();
    slotConfig();
    return QString();
}
#endif

#ifdef Q_OS_WIN
QString ONMainWindow::getXDisplay()
{
    if ( !isServerRunning ( 6000+xDisplay ) )
    {
        QMessageBox::critical (
            this,QString(),
            tr (
                "Can't start X.Org Server.\nPlease check your installation." )
        );
        close();
    }
    return QString::number ( xDisplay );

}

QString ONMainWindow::cygwinPath ( const QString& winPath )
{
    QString cPath="/cygdrive/"+winPath;
    cPath.replace ( "\\","/" );
    cPath.replace ( ":","" );
    return cPath;
}
#endif

bool ONMainWindow::isColorDepthOk ( int disp, int sess )
{
    if ( sess==0 )
        return true;
    if ( disp==sess )
        return true;
    if ( ( disp == 24 || disp == 32 ) && ( sess == 24 || sess == 32 ) )
        return true;
    return false;
}

#ifndef Q_OS_LINUX
void ONMainWindow::setWidgetStyle ( QWidget* widget )
{
    widget->setStyle ( widgetExtraStyle );
}
#else
void ONMainWindow::setWidgetStyle ( QWidget* )
{
}
#endif

QString ONMainWindow::internAppName ( const QString& transAppName, bool* found )
{
    if ( found )
        *found=false;
    int ind=_transApplicationsNames.indexOf ( transAppName );
    if ( ind!=-1 )
    {
        if ( found )
            *found=true;
        return _internApplicationsNames[ind];
    }
    return transAppName;
}


QString ONMainWindow::transAppName ( const QString& internAppName, bool* found )
{
    if ( found )
        *found=false;
    int ind=_internApplicationsNames.indexOf ( internAppName );
    if ( ind!=-1 )
    {
        if ( found )
            *found=true;
        return _transApplicationsNames[ind];
    }
    return internAppName;
}

void ONMainWindow::addToAppNames ( QString intName, QString transName )
{
    _internApplicationsNames.append ( intName );
    _transApplicationsNames.append ( transName );
}


void ONMainWindow::slotExecXmodmap()
{
}

void ONMainWindow::check_cmd_status()
{
    QString passwd;
    QString user=getCurrentUname();
    QString host=resumingSession.server;
    passwd=getCurrentPass();

    sshConnection->executeCommand ( "x2gocmdexitmessage "+
                                    resumingSession.sessionId , this, SLOT(slotCmdMessage(bool, QString, int)));
}

void ONMainWindow::slotCmdMessage ( bool result,QString output,
                                    int)
{
    x2goDebug<<"Command message: " + output;
    if ( result==false )
    {
        cardReady=false;
        cardStarted=false;
        QString message=tr ( "<b>Connection failed.</b>\n" ) +output;
        if ( message.indexOf ( "publickey,password" ) !=-1 )
        {
            message=tr ( "<b>Wrong password!</b><br><br>" ) +
                    message;
        }

        QMessageBox::critical ( 0l,tr ( "Error" ),message,
                                QMessageBox::Ok,
                                QMessageBox::NoButton );
        setEnabled ( true );
        passForm->setEnabled ( true );
        pass->setFocus();
        pass->selectAll();
    }
    if ( output.indexOf ( "X2GORUNCOMMAND ERR NOEXEC:" ) !=-1 )
    {
        QString cmd=output;
        cmd.replace ( "X2GORUNCOMMAND ERR NOEXEC:","" );

        if(startHidden)
        {
            x2goErrorf(14)<< tr( "Unable to execute: ") + cmd;
        }
        else
        {
            QMessageBox::critical ( 0l,tr ( "Error" ),
                                    tr ( "Unable to execute: " ) +
                                    cmd,QMessageBox::Ok,
                                    QMessageBox::NoButton );
        }
    }
    if(sshConnection)
        delete sshConnection;
    sshConnection=0;
    if (startHidden)
    {
        x2goInfof(11) << tr("Closing X2Go Client because it was started in hidden mode.");
        close();
    }
    else if (closeDisconnect)
    {
        x2goInfof(12) << tr("Closing X2Go Client because the --close-disconnect parameter was passed.");
        close();
    }
}


int ONMainWindow::startSshFsTunnel()
{
    fsTunReady=false;

    x2goDebug<<"Starting Folder Sharing tunnel for: "<<resumingSession.sessionId;
    x2goDebug<<"FS port: "<<resumingSession.fsPort;


    if ( resumingSession.fsPort.length() <=0 )
    {
        QString message=tr ("Remote server does not "
                            "support file system exports "
                            "through SSH tunnels.\n"
                            "Please update your x2goserver "
                            "package." );
        slotFsTunnelFailed ( false,message,0 );
        return 1;
    }
    QString passwd=getCurrentPass();
    QString uname=getCurrentUname();

    fsTunnel=sshConnection->startTunnel ( "localhost",resumingSession.fsPort.toUInt(),"127.0.0.1",
                                          clientSshPort.toInt(), true, this, SLOT ( slotFsTunnelOk(int)), SLOT ( slotFsTunnelFailed ( bool,
                                                  QString,int ) ) );
    return 0;
}

void ONMainWindow::slotFsTunnelFailed ( bool result,  QString output,
                                        int)
{
    if ( result==false )
    {
        if ( !managedMode )
        {

            QString message=tr ( "Unable to create SSH tunnel for Folder Sharing and Printing support:\n" )
                            +output;
            QMessageBox::critical ( 0l,tr ( "Error" ),message,
                                    QMessageBox::Ok,
                                    QMessageBox::NoButton );
        }
        fsTunnel=0l;
        fsTunReady=false;
    }
}


void ONMainWindow::slotFsTunnelOk(int)
{
    x2goDebug<<"FS tunnel through SSH seems to be up and running ..."<<Qt::endl;

    fsTunReady=true;
    //start reverse mounting if RSA Key and FS tunnel are ready
    //start only once from slotFsTunnelOk() or slotCopyKey().
    if ( fsExportKeyReady )
        startX2goMount();
}


void ONMainWindow::startX2goMount()
{
    QFile file ( fsExportKey+".pub" );
    if ( !file.open ( QIODevice::ReadOnly | QIODevice::Text ) )
    {
        QString message=tr ( "Unable to read:\n" ) +fsExportKey+".pub";
        QMessageBox::critical ( 0l,tr ( "Error" ),message,
                                QMessageBox::Ok,
                                QMessageBox::NoButton );
        QFile::remove
        ( fsExportKey+".pub" );
        return;
    }

    QByteArray line = file.readLine();
    file.close();

    QDir authorized_keys_dir (homeDir);
    authorized_keys_dir = QDir (authorized_keys_dir.absolutePath () + "/.x2go/.ssh/");

    QFile authorized_keys_file (authorized_keys_dir.absolutePath () + "/authorized_keys");

    x2goDebug << "Potentially creating dir " << authorized_keys_dir.absolutePath ();
    authorized_keys_dir.mkpath (authorized_keys_dir.absolutePath ());

    x2goDebug << "Potentially creating file " << authorized_keys_file.fileName ();
    if (!authorized_keys_file.open (QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
      QString message = tr ("Unable to create or append to file: ") + authorized_keys_file.fileName ();
      QMessageBox::critical (0l, tr ("Error"), message,
                             QMessageBox::Ok, QMessageBox::NoButton);
      QFile::remove (fsExportKey + ".pub");
      return;
    }

#ifdef Q_OS_UNIX
    QFile::Permissions authorized_keys_file_perm = authorized_keys_file.permissions ();
    QFile::Permissions authorized_keys_file_target_perm = QFile::ReadOwner | QFile::WriteOwner;

    bool permission_error = false;

    /*
     * Try to set the permissions if they are wrong.
     * (sshd would disallow such a file.)
     */
    if (authorized_keys_file_perm != authorized_keys_file_target_perm) {
      if (!authorized_keys_file.setPermissions (authorized_keys_file_target_perm)) {
        /* FIXME: use a function for this... */
        QString message = tr ("Unable to change the permissions of file: ") + authorized_keys_file.fileName ();
        message += "\n" + tr ("This is an error because sshd would deny such a file.");
        QMessageBox::critical (NULL, tr ("Error"), message,
                               QMessageBox::Ok, QMessageBox::NoButton);
        permission_error = true;
      }
    }

    QFile::Permissions authorized_keys_dir_perm = QFile (authorized_keys_dir.absolutePath ()).permissions ();
    QFile::Permissions authorized_keys_dir_target_perm = QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner;

    /*
     * Try to set the permissions if they are wrong.
     * (sshd would disallow such a directory.)
     */
    if (authorized_keys_dir_perm != authorized_keys_dir_target_perm) {
      if (!QFile (authorized_keys_dir.absolutePath ()).setPermissions (authorized_keys_dir_target_perm)) {
        /* FIXME: use a function for this... */
        QString message = tr ("Unable to change the permissions of directory: ") + authorized_keys_dir.absolutePath ();
        message += "\n" + tr ("This is an error because sshd would deny such a directory.");
        QMessageBox::critical (NULL, tr ("Error"), message,
                               QMessageBox::Ok, QMessageBox::NoButton);
        permission_error = true;
      }
    }

    if (permission_error) {
      QFile::remove (fsExportKey + ".pub");
      return;
    }
#endif /* defined (Q_OS_UNIX) */

    directory* dir = getExpDir (fsExportKey);
    bool rem = dir->isRemovable;
    if (!dir) {
      return;
    }

    QTextStream out (&authorized_keys_file);
    out << line;
    authorized_keys_file.close ();

    x2goDebug << "Temporarily activated public key from file " << QString (fsExportKey + ".pub") << ".";

    QString passwd=getCurrentPass();
    QString user=getCurrentUname();
    QString host=resumingSession.server;
    QString sessionId=resumingSession.sessionId;

    QStringList env=QProcess::systemEnvironment();


    QString cuser;
#if defined (Q_OS_WIN)
    cuser=wapiGetUserName();
#else
    for ( int i=0; i<env.size(); ++i )
    {
        QStringList ls=env[i].split ( "=" );
        if ( ls[0]=="USER" )

        {
            cuser=ls[1];
            break;
        }
    }
#endif
    QString cmd;
    QString dirs=dir->dirList;

    if ( !fsInTun && clientSshPort!="22" )
    {
        dirs=dirs+"__SSH_PORT__"+clientSshPort;
    }
    if ( fsInTun )
    {
        dirs=dirs+"__REVERSESSH_PORT__"+resumingSession.fsPort;
    }
    if ( !rem )
        cmd = "x2gomountdirs dir " + sessionId + " \"" + cuser +
              "\" " + dir->dstKey + " " + dirs;
    else
        cmd = "x2gomountdirs rem " + sessionId + " \"" + cuser +
              "\" " + dir->dstKey + " " + dirs;

#ifdef Q_OS_WIN

    cmd="chmod 600 "+dir->dstKey+"&&"+cmd;
#endif



    X2goSettings st ( "sessions" );

    if ( !useLdap )
    {
        QString sid;
        if ( !embedMode ) {
            if (!(sessionExplorer->getLastSession())) {
                x2goDebug << "No session selected, not calling startx2gomount command.";
                return;
            }

            sid=sessionExplorer->getLastSession()->id();
        }
        else
            sid="embedded";
        if ( st.setting()->value (
                    sid+"/useiconv", ( QVariant ) false ).toBool() )
        {
            QString toCode=st.setting()->value (
                               sid+"/iconvto",
                               ( QVariant ) "UTF-8" ).toString();

#ifdef Q_OS_WIN
            QString fromCode=st.setting()->value (
                                 sid+"/iconvfrom",
                                 ( QVariant ) tr (
                                     "WINDOWS-1252" ) ).toString();
#endif
#ifdef Q_OS_DARWIN
            QString fromCode=st.setting()->value (
                                 sid+"/iconvfrom",
                                 ( QVariant )
                                 "UTF-8" ).toString();
#endif
#ifdef Q_OS_LINUX
            QString fromCode=st.setting()->value (
                                 sid+"/iconvfrom",
                                 ( QVariant ) tr (
                                     "ISO8859-1" ) ).toString();
#endif
            cmd="export X2GO_ICONV=modules=iconv,from_code="+
                fromCode+
                ",to_code="+toCode+"&&"+cmd;
        }
    }

    x2goDebug<<"Calling startX2goMount command."<<Qt::endl;
    dir->pid=sshConnection->executeCommand(cmd,this,SLOT ( slotRetExportDir ( bool,
                                           QString,int) ));
}

void ONMainWindow::slotCheckPrintSpool()
{
    QDir dir ( spoolDir );
    QStringList list = dir.entryList ( QDir::Files );
    for ( int i=0; i<list.size(); ++i )
    {
        if ( !list[i].endsWith ( ".ready" ) )
            continue;
        QFile file ( spoolDir+"/"+list[i] );
        if ( !file.open ( QIODevice::ReadOnly | QIODevice::Text ) )
            continue;
        bool startProc=false;
        QString fname,title;
        if ( !file.atEnd() )
        {
            QByteArray line = file.readLine();
            QString fn ( line );
            fn.replace ( "\n","" );
            fname=fn;
            if ( !file.atEnd() )
            {
                line = file.readLine();
                title=line;
                title.replace ( "\n","" );
            }
            startProc=true;
        }
        file.close();
        file.remove();
        if ( startProc )
            new PrintProcess ( spoolDir+"/"+fname,title ,this );

    }
}


void ONMainWindow::cleanPrintSpool()
{
    QDir dir ( spoolDir );
    QStringList list = dir.entryList ( QDir::Files );
    for ( int i=0; i<list.size(); ++i )
    {
        QFile::remove ( spoolDir+"/"+list[i] );
    }
}


void ONMainWindow::cleanAskPass()
{
    QString path=homeDir +"/.x2go/ssh/";
    QDir dir ( path );
    QStringList list = dir.entryList ( QDir::Files );
    for ( int i=0; i<list.size(); ++i )
    {
        if ( list[i].startsWith ( "askpass" ) )
            QFile::remove ( path+list[i] );
    }

}


#ifdef Q_OS_WIN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#endif


bool ONMainWindow::isServerRunning ( int port )
{
#ifdef Q_OS_WIN
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct sockaddr_in saServer;
    hostent* localHost;
    char* localIP;
    int iResult;
    WSADATA wsaData;

    struct in_addr addr = {{ }};

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {

        x2goDebug<<"WARNING: WSAStartup failed: "<< iResult;

        return false;
    }

    addr.s_addr = inet_addr("127.0.0.1");
    if (addr.s_addr == INADDR_NONE)
    {

        x2goDebug<< "WARNING: The IPv4 address entered was invalid.\n";

        return false;
    }


    localHost = gethostbyaddr((char*)&addr,4, AF_INET);
    if (!localHost)
    {

        x2goDebug<<"WARNING: gethostbyaddr failed: "<<WSAGetLastError();

        return false;
    }
    x2goDebug<<"Resolved localhost.";


    localIP = inet_ntoa (*(struct in_addr *)*localHost->h_addr_list);

    saServer.sin_family = AF_INET;
    saServer.sin_addr.s_addr = inet_addr(localIP);
    saServer.sin_port = htons(port);

    ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ConnectSocket == INVALID_SOCKET)
    {
        x2goDebug<<"WARNING: Socket failed with error: "<< WSAGetLastError();

        return false;
    }

    iResult = ::connect( ConnectSocket, (SOCKADDR*) &saServer, sizeof(saServer));
    if (iResult == SOCKET_ERROR)
    {
        closesocket(ConnectSocket);

        x2goDebug<<"Port is free: "<<port;

        return false;
    }
    closesocket(ConnectSocket);

    x2goDebug<<"Port already in use: "<<port;

    return true;
#endif
    QTcpSocket tcpSocket ( 0 );
    tcpSocket.connectToHost ( "127.0.0.1",port );

    if ( tcpSocket.waitForConnected ( 1000 ) )
    {
        tcpSocket.close();
        return true;
    }
    return false;
}
#ifdef Q_OS_WIN
void ONMainWindow::slotCheckXOrgLog()
{
    xorgLogMutex.lock();
    if ( xorgLogFile.length() <=0 )
    {
        xorgLogMutex.unlock();
        return;
    }
    QFile file ( xorgLogFile );
    if ( !file.open ( QIODevice::ReadOnly | QIODevice::Text ) )
    {
        xorgLogMutex.unlock();
        return;
    }

    QTextStream in ( &file );
    while ( !in.atEnd() )
    {
        QString line = in.readLine();
        if ( line.indexOf ( "successfully opened the display" ) !=-1 )
        {
            xorgLogTimer->stop();
            slotSetWinServersReady();
            xorgLogMutex.unlock();
            return;
        }
    }
    xorgLogMutex.unlock();
}

void ONMainWindow::startXOrg (std::size_t start_offset)
{
    xDisplay += start_offset;

    while ( isServerRunning ( 6000+xDisplay ) )
        ++xDisplay;
    QString dispString;
    QTextStream ( &dispString ) <<":"<<xDisplay;

    QStringList args;
    QString exec;
    if (internalX==XMING)
        exec=appDir+"\\xming\\Xming.exe";
    if (internalX==VCXSRV)
        exec=appDir+"\\vcxsrv\\vcxsrv.exe";
    winServersReady=false;

    x2goDebug<<"Using internal X.Org Server: "<<useInternalX;

//#ifdef CFGCLIENT
    if (!useInternalX || internalX!=XMING)
    {
        if (!useInternalX)
            exec=xorgExe;
        QString cmdLine;
        if (startXorgOnStart)
            cmdLine=xorgOptions;
        else
        {
            switch (xorgMode)
            {
            case FS:
                cmdLine=xorgFSOptions;
                break;
            case SAPP:
                cmdLine=xorgSAppOptions;
                break;
            case WIN:
                cmdLine=xorgWinOptions;

                x2goDebug<<"WxH:"<<xorgWidth<<"x"<<xorgHeight;

                cmdLine.replace("%w",xorgWidth);
                cmdLine.replace("%h",xorgHeight);

                x2goDebug<<cmdLine;
                break;
            case MULTIDISPLAY:
                cmdLine=xorgMDOptions+QString::number(localDisplayNumber);
                break;
            }
        }
        QStringList options=cmdLine.split(" ",Qt::SkipEmptyParts);
        QString option;
        foreach(option,options)
        {
            args<< option;
        }

        if (useInternalX) {
            /*
             * -nopn lets the server terminate if it couldn't bind on all interfaces.
             * Only VcXsrv supports this option, though, and we don't want have external
             * X servers fail to start due to unknown options, so only add the option
             * if using the internal X server and the internal copy is VcXsrv-based.
             */
            args << "-nopn";

            /*
             * -silent-dup-error makes VcXsrv and Cygwin/X error out silently if either:
             *   - the global(!) MUTEX for the X display number is already taken by another
             *     process
             *   - binding to the socket (mostly TCP, though maybe increasingly UNIX since
             *     newer Windows version [10 with the April 2018 update] started supporting
             *     this natively) failed.
             *
             * The latter also triggers if some other process is listening on the selected
             * end point, which also covers other applications.
             *
             * The key part here is "silently": without this option, the process keeps
             * running - showing a dialog to the user. For X2Go Client, it looks as though
             * the server was started successfully and if connecting to the socket end
             * point also worked out, we get a false-positive in X.Org Server startup
             * detection.
             *
             * With this option, no dialog pops up and the X.Org Server terminates
             * immediately.
             *
             * FIXME: we currently only use this option when the internal X.Org Server
             * option is used (the default) and the variant is VcXsrv. Given that Cygwin/X
             * also supports the option, it would be nice to use it for external X.Org
             * Server settings as well - but we don't know their type.
             */
            args << "-silent-dup-error";
        }

        args<<dispString;
    }
//#endif
    xorg=new QProcess ( 0 );
    if (useInternalX && (internalX==XMING))
    {

        QString workingDir=appDir+"\\xming";
        QStringList env=QProcess::systemEnvironment();
        env<<"GLWIN_ENABLE_DEBUG=0";
        xorgLogMutex.lock();
        xorgLogFile=homeDir+"/.x2go/xorg";
        QDir dr ( homeDir );
        dr.mkpath ( xorgLogFile );
        xorgLogFile=wapiShortFileName ( xorgLogFile ) +"\\xorg.log."+
                    QString::number ( xDisplay );
        if ( QFile::exists ( xorgLogFile ) )
            QFile::remove ( xorgLogFile );
        xorgLogMutex.unlock();
//run xming with clipboard support
        args<<dispString<<"-multiwindow"<<"-notrayicon"<<"-clipboard"<<
            "-logfile"<<xorgLogFile;
        xorg->setEnvironment ( env );
        xorg-> setWorkingDirectory ( workingDir);
    }


    x2goDebug<<"Running "<<exec<<" "<<args.join(" ");
    x2goDebug << "Trying to start X.Org Server on DISPLAY: " << xDisplay;

    xorg->start ( exec, args );


    if ( !xorg->waitForStarted ( 3000 ) )
    {
        QMessageBox::critical (
            0,QString(),
            tr ( "Can't start X.Org Server." )
            + "\n"
            + tr ( "Please check your installation." ) );
        close();
    }
// #ifdef CFGCLIENT
    if ( !useInternalX || internalX!= XMING)
    {
        //check connection in slot and launch setWinServerReady
        waitingForX=0;
        x_start_tries_ += 1;

        QTimer::singleShot(1000, this, SLOT(slotCheckXOrgConnection()));
    }
// #endif
}

void ONMainWindow::slotCheckXOrgConnection()
{
    ++waitingForX;

    /* Before we try to connect to the socket, let's check if the server actually is still running. */
    if (xorg->state () != QProcess::Running) {
        /*
         * Process died (crashed, terminated, whatever). We need to restart it, unless we already tried
         * to do so multiple times unsuccessfully.
         */
        if ((x_start_limit_) && (x_start_limit_ < x_start_tries_)) {
            x2goDebug << "Unable to start X.Org Server for " << x_start_limit_ << " times, terminating.";

            QMessageBox::critical (NULL, QString(),
                                   tr ("X.Org Server did not launch correctly after %n tries.", "%n will be substituted with the current number of tries", x_start_tries_)
                                   + "\n"
                                   + tr ("Please check your installation."));

            close ();

            return;
        }

        x2goDebug << "Trying to re-start X.Org Server. Try count: " << x_start_tries_;

        delete xorg;
        xorg = NULL;

        startXOrg (1);

        return;
    }

    if (isServerRunning(6000+xDisplay))
    {
        x2goDebug<<"X.Org Server started on DISPLAY " << xDisplay << ".";

        slotSetWinServersReady();
    }
    else
    {
        if (waitingForX > 10)
        {
            /*
             * Timeout reached. If we tried starting the X.Org Server less times than the limit,
             * continue doing so (with a higher DISPLAY value).
             * Otherwise error out.
             */
            if ((!(x_start_limit_)) || (x_start_limit_ >= x_start_tries_)) {
                /*
                 * Server might still be running here, but deleting the QProcess object
                 * should kill it.
                 */
                x2goDebug << "Timeout reached waiting for the X.Org Server to open a listening TCP socket." << Qt::endl
                          << "Restarting on higher DISPLAY port. Try count: " << x_start_tries_;

                xorg->terminate ();
                delete xorg;
                xorg = NULL;

                startXOrg (1);

                return;
            }
            else {
                QMessageBox::critical (NULL, QString(),
                                       tr ("Can't start X.Org Server.")
                                       + "\n"
                                       + tr ("Please check your installation."));
                close();

                return;
            }
        }
        else
        {

            x2goDebug<<"Waiting for X.Org Server to start.";

            QTimer::singleShot(1000, this, SLOT(slotCheckXOrgConnection()));
        }
    }
}

WinServerStarter::WinServerStarter ( daemon server, ONMainWindow * par ) :
    QThread ( 0 ), mode (server), parent (par), ssh_key_type_ (ONMainWindow::RSA_KEY_TYPE)
{
}

void WinServerStarter::run()
{
    switch ( mode )
    {
    case SSH:
        parent->startSshd(ssh_key_type_);
        break;
    case X:
        parent->startXOrg();
        break;
    }
}

void WinServerStarter::set_ssh_key_type (ONMainWindow::key_types key_type) {
  ssh_key_type_ = key_type;
}

ONMainWindow::key_types WinServerStarter::get_ssh_key_type () {
  return (ssh_key_type_);
}


void ONMainWindow::startWinServers(ONMainWindow::key_types key_type)
{
    key_type = check_key_type (key_type);

    x2goDebug<<"Starting helper servers for Windows ...";

    QString etcDir=homeDir+"/.x2go/etc";
    QDir dr ( homeDir );

    WinServerStarter* xStarter = new WinServerStarter ( WinServerStarter::X,
            this );
    WinServerStarter* sshStarter = new WinServerStarter (
        WinServerStarter::SSH, this );

    if ( !embedMode || !config.confFS || ( config.confFS && config.useFs ) )
    {

        dr.mkpath ( etcDir );
        UNUSED (generateKey (key_type, true));
        sshStarter->set_ssh_key_type (key_type);
        sshStarter->start();
    }

// #ifdef CFGCLIENT

    //x2goDebug<<"Xorg settings: "<< startXorgOnStart <<" useXming: "<< useXming;

    if ( useInternalX && (internalX== XMING))
    {
// #endif
        xStarter->start();
        xorgLogTimer=new QTimer ( this );
        connect ( xorgLogTimer,SIGNAL ( timeout() ),this,
                  SLOT ( slotCheckXOrgLog() ) );
        xorgLogTimer->start ( 500 );
// #ifdef CFGCLIENT
    }
    else
    {
        if (startXorgOnStart)
        {
            startXOrg();
        }
    }
// #endif
}

bool ONMainWindow::haveCygwinEntry()
{
    QSettings CygwSt ( "HKEY_CURRENT_USER\\Software"
                       "\\Cygwin",
                       QSettings::NativeFormat );
    return ( CygwSt.childGroups().count() >0||CygwSt.childKeys().count() );
}



void ONMainWindow::saveCygnusSettings()
{
    if ( ONMainWindow::portable )
    {
        if ( haveCygwinEntry() )
        {
            x2goDebug<<"Cygnus Solutions entry exists.";
            cyEntry=true;
        }
        else
        {
            x2goDebug<<"Cygnus Solutions entry does not exist.";
            cyEntry=false;
        }
    }
}

void ONMainWindow::restoreCygnusSettings()
{
    if ( ONMainWindow::portable )
    {
        if ( !cyEntry )
        {
            removeCygwinEntry();
        }
    }
}

void ONMainWindow::removeCygwinEntry()
{
    QSettings st ( "HKEY_CURRENT_USER\\Software"
                   "\\Cygwin",
                   QSettings::NativeFormat );
    x2goDebug<<"Removing cygwin entry from registry.";
    st.remove ( "" );
    st.sync();

}


// #ifdef CFGCLIENT
void ONMainWindow::xorgSettings()
{

    x2goDebug<<"Getting X.Org Server settings.";


    X2goSettings st ( "settings" );

    useInternalX=(st.setting()->value("useintx",true).toBool());

    xorgExe=(st.setting()->value("xexec","C:\\program files\\vcxsrv\\vcxsrv.exe").toString());
    xorgOptions=(st.setting()->value("options","-multiwindow -notrayicon -clipboard").toString());
    startXorgOnStart=(st.setting()->value("onstart",true).toBool());
    xorgWinOptions=(st.setting()->value("optionswin","-screen 0 %wx%h -notrayicon -clipboard").toString());
    xorgFSOptions=(st.setting()->value("optionsfs","-fullscreen -notrayicon -clipboard").toString());
    xorgSAppOptions=(st.setting()->value("optionssingle","-multiwindow -notrayicon -clipboard").toString());
    xorgMDOptions=(st.setting()->value("optionswholedisplay","-nodecoration -notrayicon -clipboard -screen 0 @").toString());


    if (QFile::exists(appDir+"\\vcxsrv"))
        internalX=VCXSRV;
    if (QFile::exists(appDir+"\\xming"))
        internalX=XMING;

    QString primClip;
    if(st.setting()->value("noprimaryclip",false).toBool() && internalX==VCXSRV)
        primClip=" -noprimary";

    if (useInternalX)
    {
        startXorgOnStart=(internalX==XMING);
        xorgOptions="-multiwindow -notrayicon -clipboard"+primClip;
        if (internalX==VCXSRV)
        {
// 	xorgWinOptions="-screen 0 %wx%h -notrayicon -clipboard";
            xorgWinOptions="-multiwindow -notrayicon -clipboard"+primClip;
            xorgFSOptions="-fullscreen -notrayicon -clipboard"+primClip;
            xorgSAppOptions="-multiwindow -notrayicon -clipboard"+primClip;
            xorgMDOptions="-nodecoration -notrayicon -clipboard"+primClip+" -screen 0 @";
        }
    }

}
// #endif

void ONMainWindow::slotSetWinServersReady()
{

    x2goDebug<<"All helper servers for Windows are started.";

    winServersReady=true;
    restoreCygnusSettings();
}

#include <windows.h>
#include<sstream>
#endif

void ONMainWindow::generateEtcFiles()
{
    QString etcDir=homeDir+"/.x2go/etc";
    QString varDir = homeDir + "/.x2go/var";
    QDir dr ( homeDir );
    dr.mkpath ( etcDir );
    dr.mkpath (varDir);
    QFile file ( etcDir +"/sshd_config" );
    if ( !file.open ( QIODevice::WriteOnly | QIODevice::Text ) )
        return;
    QString authKeyPath = homeDir + "/.x2go/.ssh/authorized_keys";
#ifdef Q_OS_WIN
    authKeyPath = cygwinPath (authKeyPath);
    authKeyPath.replace (wapiGetUserName (), "%u");
    varDir = cygwinPath (varDir);
    x2goDebug << varDir << " cygwin var path";
#endif /* defined (Q_OS_WIN) */
    QTextStream out ( &file );
    out<<"StrictModes no\n"<<
         "PidFile \"" + varDir + "/sshd.pid\"\n" <<
         "AuthorizedKeysFile \"" << authKeyPath << "\"\n";
#ifdef Q_OS_WIN
    QString shell="/bash";
    if(QFile::exists(appDir+"/bash.exe"))
    {
        shell="/bash";
    }
    else if(QFile::exists(appDir+"/sh.exe"))
    {
        shell="/sh";
    }
    else if(QFile::exists(appDir+"/bin/bash.exe"))
    {
        shell="/bin/bash";
    }
    else if(QFile::exists(appDir+"/bin/sh.exe"))
    {
        shell="/bin/sh";
    }
    x2goDebug<<"Subsystem shell: "<<shell;

    QString sftp="/sftp-server";
    if(QFile::exists(appDir+"/sftp-server.exe"))
    {
        sftp="/sftp-server";
    }
    else if(QFile::exists(appDir+"/bin/sftp-server.exe"))
    {
        shell="/bin/sftp-server";
    }
    x2goDebug<<"Subsystem sftp: "<<sftp;
    out << "Subsystem shell "<< wapiShortFileName ( appDir) +shell+"\n"<<
           "ListenAddress 127.0.0.1\n" <<
           "Subsystem sftp "<< wapiShortFileName ( appDir) +sftp+"\n";
#else
    /*
     * We need to find the sftp-server binary.
     * This turns out to be surprisingly difficult, because there is no standard place
     * for this binary. Instead, every distribution installs it where they see fit.
     * Needless to say, we're screwed...
     */

    QString sftp_binary;
    QStringList common_sftp_dirs;
    common_sftp_dirs << "/usr/lib/openssh" /* Debian and Ubuntu */
                     << "/usr/libexec/openssh" /* Fedora, CentOS, hopefully also RHEL */
                     << "/usr/lib/ssh" /* Mageia, OpenSUSE, SLE{S,D} < 12 x86, SLE{S,D} 12, Arch */
                     << "/usr/lib64/ssh" /* SLE{S,D} < 12 x86_64 */
                     << "/usr/lib64/misc" /* Gentoo *64 with 17.1 profile - has no lib compat-symlink. */
                     << "/usr/lib/misc" /* Gentoo */
                     << "/run/current-system/sw/bin" /* NixOS */
                     << "/usr/libexec"; /* Slackware, OS X */

#if QT_VERSION < 0x050000
    QProcessEnvironment tmp_env = QProcessEnvironment::systemEnvironment ();
    QString path_val = tmp_env.value ("PATH");

    path_val = add_to_path (path_val, common_sftp_dirs);

    /* Just in case we bundle sftp-server ourselves. */
    sftp_binary = find_binary (appDir, "sftp-server");

    if (sftp_binary.isEmpty ()) {
      sftp_binary = find_binary (path_val, "sftp-server");
    }
#else /* QT_VERSION < 0x050000 */
    QStringList search_paths;
    search_paths << appDir;

    sftp_binary = QStandardPaths::findExecutable ("sftp-server", search_paths);

    if (sftp_binary.isEmpty ()) {
      search_paths = QStringList ();

      sftp_binary = QStandardPaths::findExecutable ("sftp-server", search_paths);

      if (sftp_binary.isEmpty ()) {
        search_paths = common_sftp_dirs;

        sftp_binary = QStandardPaths::findExecutable ("sftp-server", search_paths);
      }
    }
#endif /* QT_VERSION < 0x050000 */

    if (sftp_binary.isEmpty ()) {
      x2goErrorf (31) << "Unable to find the sftp-server binary. Neither bundled, nor found in $PATH nor additional directories.";
      show_RichText_ErrorMsgBox (tr ("Unable to find the sftp-server binary. Neither bundled, nor found in $PATH nor additional directories."),
                                 tr ("If you are using a Linux-based operating system, please ask your system administrator "
                                     "to install the package containing the sftp-server binary. Common names are <b>openssh</b>, "
                                     "<b>openssh-server</b> or <b>openssh-sftp-server</b> depending upon distribution.\n\n"
                                     "If the sftp-server binary is installed on your system, please report a bug "
                                     "mentioning its path on:\n"
                                     "<center><a href=\"https://wiki.x2go.org/doku.php/wiki:bugs\">"
                                       "https://wiki.x2go.org/doku.php/wiki:bugs"
                                     "</a></center>\n"),
                                 true);
      abort ();
    }

    out << "Subsystem sftp " << sftp_binary << "\n";
#endif

    /* The log file in startSshd() is specific to Windows. */
#ifdef Q_OS_WIN
    if (debugging){
        out<<"LogLevel DEBUG1\n";
    }
#endif
    file.close();
    x2goDebug<<etcDir +"/sshd_config created.";
}

ONMainWindow::key_types ONMainWindow::check_key_type (ONMainWindow::key_types key_type) {
  ONMainWindow::key_types ret = key_type;

  switch (key_type) {
    case RSA_KEY_TYPE:
                           break;
    case DSA_KEY_TYPE:
                           break;
    case ECDSA_KEY_TYPE:
                           break;
    case ED25519_KEY_TYPE:
                           break;
    default:
                           ret = UNKNOWN_KEY_TYPE;
  }

  if (UNKNOWN_KEY_TYPE == ret) {
    QMessageBox::critical (this, tr ("SSH key type selection error"),
                           tr ("Unknown SSH key selected.")
                           + "\n"
                           + tr ("Terminating application."));
    close ();
  }

  return (ret);
}

QString ONMainWindow::key_type_to_string (ONMainWindow::key_types key_type) {
  ONMainWindow::key_types sanitized_key_type = check_key_type (key_type);
  QString ret ("");

  switch (sanitized_key_type) {
    case RSA_KEY_TYPE:
                           ret = "rsa";
                           break;
    case DSA_KEY_TYPE:
                           ret = "dsa";
                           break;
    case ECDSA_KEY_TYPE:
                           ret = "ecdsa";
                           break;
    case ED25519_KEY_TYPE:
                           ret = "ed25519";
                           break;
    default:
                           ret = "unknown";
  }

  return (ret);
}

std::size_t ONMainWindow::default_size_for_key_type (ONMainWindow::key_types key_type) {
  ONMainWindow::key_types sanitized_key_type = check_key_type (key_type);
  std::size_t ret = 0;

  switch (sanitized_key_type) {
    case RSA_KEY_TYPE:
                           ret = 4096;
                           break;
    case DSA_KEY_TYPE:
                           ret = 1024;
                           break;
    case ECDSA_KEY_TYPE:
                           ret = 384;
                           break;
    case ED25519_KEY_TYPE:
                           /* Fixed key length, flag will be unused. */
                           ret = 0;
                           break;
    default:
                           ret = 0;
  }

  return (ret);
}

QString ONMainWindow::generateKey (ONMainWindow::key_types key_type, bool host_key) {
  QString stringified_key_type (key_type_to_string (key_type));
  std::size_t key_bits = default_size_for_key_type (key_type);

  QString base_dir (homeDir);
  QString private_key_file ("");

  if (host_key) {
    base_dir += "/.x2go/etc/";
  }
  else {
    base_dir += "/.x2go/ssh/gen/";
  }

  {
    QDir dir (homeDir);
    if (!(dir.mkpath (base_dir))) {
      QMessageBox::critical (this, tr ("SSH key base directory creation error"),
                             tr ("Unable to create SSH key base directory '%1'.").arg (base_dir)
                             + "\n"
                             + tr ("Terminating application."));
      close ();
    }
  }

  private_key_file = base_dir;
#ifdef Q_OS_WIN
  QString private_key_file_cygwin = cygwinPath (wapiShortFileName (base_dir));
#endif

  {
    QString tmp_to_add ("");

    if (host_key) {
      tmp_to_add = "/ssh_host_" + stringified_key_type + "_key";
    }
    else {
      QTemporaryFile temp_file (base_dir + "/key");
      temp_file.open ();

      /* Extract base name. */
      QFileInfo tmp_file_info (temp_file.fileName ());
      tmp_to_add = tmp_file_info.fileName ();

      /* Clean up again. We don't need the temporary file anymore. */
      temp_file.setAutoRemove (false);
      temp_file.close ();
      temp_file.remove ();
    }

    private_key_file += tmp_to_add;
#ifdef Q_OS_WIN
    private_key_file_cygwin += tmp_to_add;
#endif
  }

  QString public_key_file (private_key_file + ".pub");

  if ((!(QFile::exists (private_key_file))) || (!(QFile::exists (public_key_file)))) {
    x2goDebug << "Generating SSH key. Type: " << stringified_key_type.toUpper ()
              << "; Location: "
              <<
#ifdef Q_OS_WIN
                 private_key_file_cygwin
#else
                 private_key_file
#endif
              ;

    QStringList args;

    QString comment = "X2Go Client " + stringified_key_type.toUpper () + " ";

    if (host_key) {
      comment += "host";
    }
    else {
      comment += "user";
    }

    comment += " key";

    args << "-t"
         << stringified_key_type
         << "-b"
         << QString::number (key_bits)
         << "-N"
         << ""
         << "-C"
         << comment
         << "-f"
         << private_key_file;

#ifdef Q_OS_WIN
    QString keygen="ssh-keygen";
    if(QFile::exists(appDir+"/ssh-keygen.exe"))
    {
        keygen="ssh-keygen";
    }
    else if(QFile::exists(appDir+"/bin/ssh-keygen.exe"))
    {
        keygen="bin\\ssh-keygen";
    }
    x2goDebug<<"ssh-keygen: "<<keygen;
    const int keygen_ret = QProcess::execute (keygen, args);
#else
    const int keygen_ret = QProcess::execute ("ssh-keygen", args);
#endif

    if (-2 == keygen_ret) {
      QMessageBox::critical (this, tr ("ssh-keygen launching error"),
                             tr ("Unable to start the ssh-keygen binary.")
                             + "\n"
                             + tr ("Terminating application."));
      close ();
    }

    if (-1 == keygen_ret) {
      QMessageBox::critical (this, tr ("ssh-keygen crashed"),
                             tr ("The ssh-keygen binary crashed.")
                             + "\n"
                             + tr ("Terminating application."));
      close ();
    }

    if (0 != keygen_ret) {
      QMessageBox::critical (this, tr ("ssh-keygen program error"),
                             tr ("The ssh-keygen binary did not exit cleanly.")
                             + " "
                             + tr ("It was probably called with unknown arguments.")
                             + "\n"
                             + tr ("Terminating application."));
      close ();
    }
  }

  return (private_key_file);
}

QString ONMainWindow::createKeyBundle (key_types key_type) {
  /*
   * I spent multiple hours on trying to understand this function
   * and directory exporting in general, so I'd better document
   * this.
   *
   * This function first generates a new RSA private-public key
   * pair as ~/.x2go/ssh/gen/key.XXXXX{,.pub}.
   *
   * Then, the SSH daemon's public host key is read and appended
   * to the *private* SSH key file after a marker looking like
   * this: "----BEGIN RSA IDENTITY----"
   *
   * Later on, this *private* SSH key file is transferred to the
   * remote server, which parses it in the "x2gomountdirs" perl
   * script and extracts the public key (used for logging in
   * to the client machine) and the public *host* key, used to
   * circumvent the "untrusted host" message by SSH by
   * explicitly giving the aforementioned public *host* key as
   * the only element in a fake "authorized_keys" file. Again,
   * this is all happening server-side.
   *
   * The *public* key part generated here is then taken and
   * later added to the "authorized_keys" file on the client
   * side, to allow auto-logins via the generated and transferred
   * private SSH key.
   */

  QString stringified_key_type (key_type_to_string (key_type));

  QString user_key = generateKey (key_type);

  /*
   * Now taking the *host* pub key here...
   */
  const QString host_pub_key_file_name ("ssh_host_" + stringified_key_type + "_key.pub");
  QFile rsa (homeDir + "/.x2go/etc/" + host_pub_key_file_name);
#ifdef Q_OS_WIN
  rsa.setFileName (wapiShortFileName (homeDir + "\\.x2go\\etc\\" + host_pub_key_file_name));
#endif

  if (!(rsa.open (QIODevice::ReadOnly | QIODevice::Text))) {
    x2goDebug << "Unable to open public host key file.";
#ifdef Q_OS_UNIX
    x2goDebug << "Creating a new one.";
    QString tmp_file_name (generateKey (key_type, true));

    rsa.setFileName (tmp_file_name + ".pub");
    if (!(rsa.open (QIODevice::ReadOnly | QIODevice::Text))) {
      x2goErrorf (9) << tr ("Unable to open newly generated %1 public host key file.").arg (stringified_key_type.toUpper ());
      return (QString());
    }
#else
    printSshDError_noHostPubKey ();
    return (QString());
#endif
  }

  if (!(startSshd (key_type))) {
    x2goDebug << "Failed to start OpenSSH Server pro-actively.";
    return (QString());
  }

  QByteArray rsa_pub;

  if (!(rsa.atEnd ())) {
    rsa_pub = rsa.readLine ();
  }
  else {
    x2goErrorf (9) << tr ("%1 public host key file empty.").arg (stringified_key_type.toUpper ());
    return (QString());
  }

  QFile file (user_key);
  if (!(file.open (QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))) {
    x2goErrorf (10) << tr ("Cannot open key: ") << user_key;
    return (user_key);
  }

  /* Append public host key into private user key file. */
  QTextStream out (&file);

  /*
   * The string here should be changed, but this requires
   * changes to X2Go Server as well.
   * As such, I'll be not changing it here for now.
   */
  out << "----BEGIN RSA IDENTITY----" << rsa_pub;
  file.close ();
  return (user_key);
}

bool ONMainWindow::startSshd(ONMainWindow::key_types key_type)
{
    if ( embedMode && config.confFS && !config.useFs )
    {
        return false;
    }

    const QString stringified_key_type = key_type_to_string (key_type);

    /* Don't start sshd, if it's already running. */
#ifdef Q_OS_WIN
    if (winSshdStarted)
#else /* defined (Q_OS_WIN) */
    if (sshd)
#endif /* defined (Q_OS_WIN) */
    {
        if (isServerRunning (clientSshPort.toInt ())) {
            return (true);
        }
    }

    /*
     * Pro-actively (re-)create sshd_config file, we'll need it for sshd
     * and more importantly make sure that it's up-to-date.
     */
    generateEtcFiles ();

    clientSshPort = "7022";
    QString etcDir=homeDir+"/.x2go/etc";

    int port=clientSshPort.toInt();
    //clientSshPort have initvalue
    while ( isServerRunning ( port ) )
        ++port;
    clientSshPort=QString::number ( port );
#ifdef Q_OS_WIN
    std::string sshdExec="\\sshd.exe";
    if(QFile::exists(appDir+"/sshd.exe"))
    {
        sshdExec="\\sshd.exe";
    }
    else if(QFile::exists(appDir+"/bin/sshd.exe"))
    {
        sshdExec="\\bin\\sshd.exe";
    }
    x2goDebug<<"SSHD executable: "<<sshdExec.c_str();
    std::string clientdir=wapiShortFileName ( appDir ).toStdString();
    std::stringstream strm;
    std::string config="\""+cygwinPath(etcDir+"/sshd_config").toStdString()+"\"";
    std::string key="\""+cygwinPath(etcDir+"/ssh_host_" + stringified_key_type + "_key").toStdString()+"\"";

    // generate a unique sshLog filepath, and create its directory
    if (debugging)
    {
        QDir* sshLogsDir= new QDir( homeDir+"/.x2go/sshLogs" );
        if (!sshLogsDir->exists())
            sshLogsDir->mkpath(".");

        QTemporaryFile* sshLogTemp=new QTemporaryFile ( sshLogsDir->absolutePath()+"/XXXXXX.log" );
        sshLogTemp->open();
        sshLog=sshLogTemp->fileName();
        sshLogTemp->close();

        delete sshLogsDir;
        delete sshLogTemp;
        x2goDebug<<"Logging cygwin sshd to: "<<sshLog;
    }

    strm<<clientdir<<sshdExec<<" -D -p "<<clientSshPort.toInt()<<" -f "<< config <<" -h "<<key;
    if (debugging){
        strm<<" -E "<<"\""<<sshLog.toStdString()<<"\"";
    }

    STARTUPINFOA si;
    std::string desktopName="x2go_";
    desktopName+=getenv ( "USERNAME" );
    char* desktop=new char[desktopName.size() +1];
    strcpy ( desktop,desktopName.c_str() );

    x2goDebug<<"Creating desktop: "<<desktop;

    if ( !CreateDesktopA (
                desktop,
                0,
                0,
                0,
                GENERIC_ALL,
                0
            ) )
    {
        strcpy ( desktop,"" );

        x2goDebug<<"Desktop creation failed, using default.";

    }
    ZeroMemory ( &si, sizeof ( si ) );
    ZeroMemory ( &sshd, sizeof ( sshd ) );
    si.lpDesktop=desktop;
    si.cb = sizeof ( si );
    const BOOL ret = CreateProcessA ( NULL,  // No module name (use command line)
                                      ( LPSTR ) strm.str().c_str(),  // Command line
                                      NULL,           // Process handle not inheritable
                                      NULL,           // Thread handle not inheritable
                                      TRUE,          // Set handle inheritance to FALSE
                                      CREATE_NO_WINDOW/*|CREATE_NEW_PROCESS_GROUP*/,
                                      //creation flags
                                      NULL,           // Use parent's environment block
                                      clientdir.c_str(), // Starting directory
                                      &si,            // Pointer to STARTUPINFO structure
                                      &sshd );// Pointer to PROCESS_INFORMATION structure

    /* Test successful SSH daemon startup for at most 5 seconds. */
    if (ret) {
        QTime sleep_time = QTime::currentTime ().addSecs (5);

        while (QTime::currentTime () < sleep_time) {
            if (isServerRunning (clientSshPort.toInt ())) {
                winSshdStarted = true;

                break;
            }

            QCoreApplication::processEvents (QEventLoop::AllEvents, 100);
        }
    }
    else {
        winSshdStarted = false;
    }

    delete []desktop;
#else // defined (Q_OS_WIN)
    sshd=new QProcess ( this );

    QString binary ("/usr/sbin/sshd");
#ifndef Q_OS_UNIX
    binary = appDir + "/sshd";
#endif /* !(defined (Q_OS_UNIX)) */

    QStringList arguments;
    arguments<<"-f"<<etcDir +"/sshd_config"<< "-h" <<
             etcDir+"/ssh_host_" + stringified_key_type + "_key"<<"-D"<<"-p"<<clientSshPort;

    sshd->start (binary, arguments);

    /* Allow sshd a grace time of 5 seconds to come up. */
    QTime sleep_time = QTime::currentTime ().addSecs (5);
    while (QTime::currentTime () < sleep_time) {
        if (QProcess::Running == sshd->state ()) {
            /*
             * Additionally, spin up to 3 seconds to give sshd
             * the opportunity to launch its listening socket.
             */
            QTime new_sleep_time = QTime::currentTime ().addSecs (3);

            while (QTime::currentTime () < new_sleep_time) {
                if (isServerRunning (clientSshPort.toInt ())) {
                    break;
                }

                QCoreApplication::processEvents (QEventLoop::AllEvents, 100);
            }

            break;
        }

        QCoreApplication::processEvents (QEventLoop::AllEvents, 100);
    }
#endif /* defined (Q_OS_WIN) */

    /* Check to see if connecting to the listening socket works. */
    if (!isServerRunning (clientSshPort.toInt ())) {
        printSshDError_startupFailure ();
        x2goDebug << "Failed to start user mode OpenSSH server.";
        return (false);
    }
    else {
        x2goDebug << "User mode OpenSSH server started successfully.";
        return (true);
    }
}

void ONMainWindow::setProxyWinTitle()
{

    if (embedMode)
        return;

    QString title;

    if (!useLdap) {
        if (!(sessionExplorer->getLastSession())) {
            x2goDebug << "No session selected, not setting proxy window title.";
            return;
        }

        title=sessionExplorer->getLastSession()->name();
    }
    else
        title=getCurrentUname()+"@"+resumingSession.server;

    QPixmap pixmap;

    if (useLdap)
        pixmap=lastUser->foto();
    else
        pixmap=*(sessionExplorer->getLastSession()->sessIcon());

#ifdef Q_OS_LINUX

    XStoreName(QX11Info::display(), proxyWinId, title.toLocal8Bit().data());

    XWMHints* win_hints;


    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "XPM");


    int rez;

    if (image)
        XFreePixmap(QX11Info::display(),image);
    if (shape)
        XFreePixmap(QX11Info::display(),shape);


    rez=XpmCreatePixmapFromBuffer(QX11Info::display(), proxyWinId, bytes.data(),
                                  (Pixmap *) &image, (Pixmap *) &shape, NULL);
    if (!rez)
    {

        win_hints = XAllocWMHints();
        if (win_hints)
        {
            win_hints->flags = IconPixmapHint|IconMaskHint;
            win_hints->icon_pixmap = image;
            win_hints->icon_mask = shape;
            XSetWMHints(QX11Info::display(), proxyWinId, win_hints);
            XFree(win_hints);
        }
    }

#endif
#ifdef Q_OS_WIN
    wapiSetWindowText((HWND)proxyWinId, title);
//       wapiSetWindowIcon((HWND)proxyWinId, pixmap);
#endif
}

void ONMainWindow::slotSetProxyWinFullscreen()
{

#ifdef Q_OS_LINUX

    QRect geom=QApplication::desktop()->screenGeometry(localDisplayNumber-1);
    Atom atom = XInternAtom ( QX11Info::display(), "_NET_WM_STATE_FULLSCREEN", True );
    XChangeProperty (
        QX11Info::display(), proxyWinId,
        XInternAtom ( QX11Info::display(), "_NET_WM_STATE", True ),
        XA_ATOM,  32,  PropModeReplace,
        (unsigned char*) &atom,  1 );

    XMapWindow(QX11Info::display(), proxyWinId);

    XSync(QX11Info::display(),false);
    XEvent event;
    event.xclient.type = ClientMessage;
    event.xclient.serial = 0;
    event.xclient.send_event = True;
    event.xclient.display = QX11Info::display();
    event.xclient.window = proxyWinId;
    event.xclient.message_type = XInternAtom(QX11Info::display(),"_NET_WM_STATE",False);
    event.xclient.format = 32;
    event.xclient.data.l[0] = 1;
    event.xclient.data.l[1] = XInternAtom(QX11Info::display(),"_NET_WM_STATE_FULLSCREEN",False);
    event.xclient.data.l[2] = 0;
    event.xclient.data.l[3] = 1;
    event.xclient.data.l[4] = 0;
    Status st;
    st=XSendEvent(QX11Info::display(), DefaultRootWindow(QX11Info::display()),
                  False, SubstructureNotifyMask ,&event);
    if(!st)
    {
        x2goDebug<<"Couldn't fetch fullscreen setting.";
    }
    XSync(QX11Info::display(),false);
    XMapWindow(QX11Info::display(), proxyWinId);

    QString geoStr = QString("%1").arg(geom.width()) + "x"+ QString("%1").arg(geom.height());



    sshConnection->executeCommand("DISPLAY=:"+resumingSession.display+" xrandr --output default --mode "+geoStr);
#endif
}


void ONMainWindow::resizeProxyWinOnDisplay(int disp)
{
    QRect geom=QApplication::desktop()->screenGeometry(disp-1);

    QString geoStr =
        "(x: " + QString("%1").arg(geom.x()) +
        ", y: "+ QString("%1").arg(geom.y()) +
        ", w: "+ QString("%1").arg(geom.width()) +
        ", h: "+ QString("%1").arg(geom.height());
    x2goDebug<<"Resizing proxy window to fit display: " + QString("%1").arg(disp) + " " + geoStr;

#ifdef Q_OS_LINUX
    XSync(QX11Info::display(),false);
    XMoveResizeWindow(QX11Info::display(), proxyWinId, geom.x(), geom.y(), 800, 600);
    XMapWindow(QX11Info::display(), proxyWinId);
    XFlush(QX11Info::display());
#endif
#ifdef Q_OS_WIN
    dispGeometry=geom;
#endif
    QTimer::singleShot(500, this, SLOT(slotSetProxyWinFullscreen()));
}

QRect ONMainWindow::proxyWinGeometry()
{
#ifdef Q_OS_WIN
    QRect proxyRect;
    if (!wapiWindowRectWithoutDecoration((HWND)proxyWinId,proxyRect))
        return QRect();
    return proxyRect;
#endif
#ifdef Q_OS_LINUX
    QRect proxyRect;
    Window root;
    int x,y;
    uint w,h,border,depth;
    if (XGetGeometry(QX11Info::display(), proxyWinId, &root,&x,&y,&w,&h,&border,&depth))
    {

        int realx,realy;
        Window child;
        XTranslateCoordinates(QX11Info::display(), proxyWinId, root, 0, 0, &realx, &realy, &child);
        proxyRect.setRect(realx, realy, w,h);
    }
    return proxyRect;
#endif
    return QRect();
}

void ONMainWindow::slotConfigXinerama()
{
    QRect newGeometry=proxyWinGeometry();
    if (newGeometry.isNull())
    {
        x2goWarningf(7)<< tr("Error getting window geometry. (Did you close the window?)");
        xineramaTimer->stop();
        return;
    }
    if (newGeometry==lastDisplayGeometry)
        return;
    lastDisplayGeometry=newGeometry;

    QString geoStr =
        "(x: " + QString("%1").arg(lastDisplayGeometry.x()) +
        ", y: "+ QString("%1").arg(lastDisplayGeometry.y()) +
        ", w: "+ QString("%1").arg(lastDisplayGeometry.width()) +
        ", h: "+ QString("%1").arg(lastDisplayGeometry.height());
    x2goDebug<<"New proxy geometry: " + geoStr;

    QDesktopWidget* root=QApplication::desktop();
    QList<QRect> newXineramaScreens;
    for (int i=0; i< root->numScreens(); ++i)
    {
        QRect intersection;
        if (resumingSession.fullscreen)
        {
            x2goDebug<<"correcting display geometry for full screen session";
            lastDisplayGeometry.setX(0);
            lastDisplayGeometry.setY(0);
            intersection=root->screenGeometry(i);
        }
        else
            intersection=root->screenGeometry(i).intersected(lastDisplayGeometry);
        if (!intersection.isNull())
        {
            //            x2goDebug<<"intersected with "<<i<<": "<<intersection<<Qt::endl;
            intersection.moveLeft(intersection.x()-lastDisplayGeometry.x());
            intersection.moveTop(intersection.y()-lastDisplayGeometry.y());

            //            x2goDebug<<"xinerama screen: "<<intersection<<Qt::endl;
            newXineramaScreens<<intersection;
        }
    }
    if (xineramaScreens != newXineramaScreens)
    {
        xineramaScreens=newXineramaScreens;
        //        x2goDebug<<"xinerama screen changed, new screens: "<<xineramaScreens<<Qt::endl;

        xineramaTimer->stop();
        QStringList screens;
        foreach (QRect disp, xineramaScreens)
            screens<<QString::number(disp.x())+" "+QString::number(disp.y())+" "+QString::number(disp.width())+
                   " "+QString::number(disp.height());
        QString cmd="export DISPLAY=:"+resumingSession.display+" ;printf %b '\\''"+screens.join("\\n")+"'\\'' >  $HOME/.x2go/C-"+
                    resumingSession.sessionId+"/xinerama.conf";

        sshConnection->executeCommand(cmd, this, SLOT(slotXineramaConfigured()));
    }
}

void ONMainWindow::slotXineramaConfigured()
{
    if (resumingSession.fullscreen)
        return;
    if (xinSizeInc == -1)
        xinSizeInc=1;
    else
        xinSizeInc=-1;
#ifdef Q_OS_LINUX
    lastDisplayGeometry.setWidth(lastDisplayGeometry.width()+xinSizeInc);
    XSync(QX11Info::display(),false);
    XResizeWindow(QX11Info::display(), proxyWinId,
                  lastDisplayGeometry.width(),lastDisplayGeometry.height());
    XSync(QX11Info::display(),false);
#endif
#ifdef Q_OS_WIN
    QRect geom;
    wapiWindowRect ( (HWND) proxyWinId, geom );
    wapiMoveWindow( (HWND) proxyWinId, geom.x(), geom.y(), geom.width()+xinSizeInc, geom.height(),true);
    lastDisplayGeometry=proxyWinGeometry();
#endif
    xineramaTimer->start(500);
}

void ONMainWindow::setProxyWinNotResizable()
{
#ifdef Q_OS_WIN
    switch(wapiIsWinResizeable((HWND)proxyWinId))
    {
        case -1:
            x2goDebug<<"Proxy window is closed, stop checking properties";
            return;
        case 1:
            wapiSetWinNotResizable((HWND)proxyWinId);
        default:
            QTimer::singleShot(1000, this, SLOT(setProxyWinNotResizable()));
    }
#endif
#ifdef Q_OS_LINUX
    XWindowAttributes wattr;
    if(! XGetWindowAttributes(QX11Info::display(), proxyWinId, &wattr))
    {
        x2goDebug<<"Proxy window is closed, stop checking properties";
        return;
    }
    if(wattr.map_state != IsViewable)
    {
        QTimer::singleShot(1000, this, SLOT(setProxyWinNotResizable()));
        return;
    }

    XSizeHints rhints;
    long rflags;
    if(!XGetWMNormalHints(QX11Info::display(), proxyWinId, &rhints, &rflags))
    {
        x2goDebug<<"Failed to get hints for proxy window, stop checking properties";
        return;
    }
    if(rhints.min_width==proxyWinWidth && rhints.max_width==proxyWinWidth && rhints.min_height==proxyWinHeight && rhints.max_height==proxyWinHeight)
    {
        QTimer::singleShot(1000, this, SLOT(setProxyWinNotResizable()));
        return;
    }

    XSizeHints* hints;
    XSync(QX11Info::display(),false);
    hints=XAllocSizeHints();
    hints->flags= PMinSize|PMaxSize;
    hints->base_width=hints->max_width=hints->min_width=proxyWinWidth;
    hints->base_height=hints->max_height=hints->min_height=proxyWinHeight;
//     x2goDebug<<"Setting X11 hints on proxy window";
    XSetWMNormalHints(QX11Info::display(), proxyWinId, hints);
    XSync(QX11Info::display(),false);
    XFlush(QX11Info::display());
    XFree(hints);
    QTimer::singleShot(1000, this, SLOT(setProxyWinNotResizable()));
#endif
}


void ONMainWindow::slotFindProxyWin()
{
#ifndef Q_OS_DARWIN
    x2goDebug<<"Searching proxy window: X2GO-" + resumingSession.sessionId;

    proxyWinId=findWindow ( "X2GO-"+resumingSession.sessionId );
    bool xinerama=defaultXinerama;
    if ( proxyWinId )
    {
        x2goDebug<<"Proxy window found: " + QString("%1").arg(proxyWinId);

        setProxyWinTitle();
        proxyWinTimer->stop();
        if (!embedMode && !(resumingSession.sessionType==x2goSession::KDRIVE) && !(resumingSession.sessionType==x2goSession::ROOTLESSKDRIVE))
        {
            if (!useLdap)
            {
                X2goSettings *st;
                QString sid;
                if ( !embedMode ) {
                    if (!(sessionExplorer->getLastSession())) {
                        x2goDebug << "No session selected, not searching for proxy window.";
                        return;
                    }

                    sid=sessionExplorer->getLastSession()->id();
                }
                else
                    sid="embedded";

                if (brokerMode)
                    st=new X2goSettings(config.iniFile,QSettings::IniFormat);
                else
                    st= new X2goSettings( "sessions" );
                xinerama=st->setting()->value ( sid+"/xinerama",
                                                ( QVariant ) defaultXinerama ).toBool();
#ifndef Q_OS_WIN
                uint displays=QApplication::desktop()->numScreens();
                if (st->setting()->value ( sid+"/multidisp",
                                           ( QVariant ) false ).toBool())
                {
                    uint disp=st->setting()->value ( sid+"/display",
                                                     ( QVariant ) 1 ).toUInt();
                    delete st;
                    if (disp>displays)
                    {
                        disp=1;
                    }
                    localDisplayNumber=disp;
                    resizeProxyWinOnDisplay(disp);
                    return;
                }
#endif
                if (st->setting()->value ( sid+"/noresize",
                               ( QVariant ) false ).toBool())
                {
                    x2goDebug<<"making proxy window not resizable";
                    setProxyWinNotResizable();
                }

                delete st;
            }
            if (xinerama)
            {
                x2goDebug<<"Starting Xinerama timer.";

                lastDisplayGeometry=QRect();
                xineramaScreens.clear();
                xineramaTimer->start(500);
            }
        }

        if ( embedMode )
        {
            x2goDebug<<"Checking rootless config.";

            if ( config.rootless )
            {
                x2goDebug<<"Window is rootless.";
                act_embedContol->setEnabled ( false );
            }
            else
                slotAttachProxyWindow();
        }

#ifdef Q_OS_WIN
        x2goDebug<<"Maximize proxy window: "<<maximizeProxyWin;

        if ( !startEmbedded &&!(resumingSession.sessionType==x2goSession::KDRIVE)&&!(resumingSession.sessionType==x2goSession::ROOTLESSKDRIVE))
        {
            if ( maximizeProxyWin )
            {
                QDesktopWidget dw;

                x2goDebug<<"Putting proxy window into fullscreen.";
                wapiSetFSWindow ( ( HWND ) proxyWinId,
                                  dw.screenGeometry ( this ) );
                /*wapiShowWindow ( ( HWND ) proxyWinId,
                   WAPI_SHOWMAXIMIZED );*/
            }
            else
            {
                wapiMoveWindow ( ( HWND ) proxyWinId,0,0,
                                 proxyWinWidth,
                                 proxyWinHeight,true );
            }
        }
#endif
    }
#endif
}


QString ONMainWindow::getCurrentUname()
{
    return login->text();
}

QString ONMainWindow::getCurrentPass()
{
    return pass->text();
}

void ONMainWindow::slotDetachProxyWindow()
{
    proxyWinEmbedded=false;
    bgFrame->show();
    setStatStatus();
    act_embedContol->setText ( tr ( "Attach X2Go window" ) );
    act_embedContol->setIcon ( QIcon ( iconsPath("/32x32/attach.png" ) ));
#ifdef Q_OS_LINUX
    //if QX11EmbedContainer cannot embed window, check if window exists
    //and reconnect
    if ( !embedControlChanged )
    {
        slotFindProxyWin();
        x2goDebug<<"Proxy window detached, proxywin is: "<<proxyWinId;
    }
#endif
    embedControlChanged=false;
}


void ONMainWindow::slotAttachProxyWindow()
{
    x2goDebug<<"slotAttachProxyWindow.";

    if ( startEmbedded )
    {
        embedControlChanged=false;
        bgFrame->hide();
        proxyWinEmbedded=true;
        setStatStatus();
        act_embedContol->setText ( tr ( "Detach X2Go window" ) );
        act_embedContol->setIcon (
            QIcon ( iconsPath("/32x32/detach.png" ) ));
        QTimer::singleShot ( 100, this, SLOT ( slotEmbedWindow() ) );
    }
    else
    {

        x2goDebug<<"Start embedded was false.";

        startEmbedded=true;
    }

}

void ONMainWindow::slotEmbedWindow()
{
#ifndef Q_OS_DARWIN
    QTimer::singleShot ( 1000, this,
                         SLOT ( slotActivateWindow() ) );

#endif
}

void ONMainWindow::setEmbedSessionActionsEnabled ( bool enable )
{
    act_shareFolder->setEnabled ( enable );
    if(!enable)
        act_showApps->setVisible(enable);
    act_suspend->setEnabled ( enable );
    act_terminate->setEnabled ( enable );
    act_embedContol->setEnabled ( enable );
    act_reconnect->setEnabled ( !enable );
}

void ONMainWindow::slotEmbedControlAction()
{
#ifndef Q_OS_DARWIN
    embedControlChanged=true;
    if ( !proxyWinEmbedded )
    {
        slotAttachProxyWindow();
    }
#endif
}

void ONMainWindow::slotEmbedIntoParentWindow()
{
#ifndef Q_OS_DARWIN
// 	embedInto ( embedParent );
#endif
}


void ONMainWindow::processSessionConfig()
{
    bool haveKey=false;
    UNUSED (haveKey);

    config.command="KDE";
    config.brokerNoAuth=false;
    config.sshport="22";
    config.session=tr ( "X2Go Session" );
    config.checkexitstatus=true;
    config.showtermbutton=true;
    config.showexpbutton=true;
    config.showconfig=true;
    config.showextconfig=true;
    config.showtoolbar=true;
    config.showstatusbar=true;
    config.kbdType=getDefaultKbdType();
    config.kbdLay=getDefaultLayout()[0];


    config.confSnd=false;
    config.confFS=false;
    config.confConSpd=false;
    config.confCompMet=false;
    config.confImageQ=false;
    config.confDPI=false;
    config.confKbd=false;

    QStringList lines=m_x2goconfig.split ( "\n" );


    for ( int i=0; i<lines.count(); ++i )
    {
        QString line = lines[i];

        // strip left/right whitespaces, important for plugin settings via x2goconfig
        line.remove(QRegExp("^\\s+"));
        line.remove(QRegExp("\\s+$"));

        if ( ( line=="-----BEGIN DSA PRIVATE KEY-----" ) ||
                ( line=="-----BEGIN RSA PRIVATE KEY-----" ) ||
                ( line=="-----BEGIN OPENSSH PRIVATE KEY-----" ))
        {
            while ( i<lines.count() )
                config.key+=lines[i++] +"\n";
            haveKey=true;
        }
        else
            processCfgLine ( line );
    }
    act_terminate->setVisible ( config.showtermbutton );
    act_shareFolder->setVisible ( config.showexpbutton );
    act_set->setVisible ( config.showconfig );
    if (!config.showstatusbar)
    {
        statusBar()->hide();
    }
    if ( managedMode )
    {
        QTimer::singleShot ( 500, this, SLOT ( slotStartBroker() ) );
        return;
    }
    slotSelectedFromList ( ( SessionButton* ) 0 );
}


void ONMainWindow::processCfgLine ( QString line )
{
    QStringList lst=line.split ( "=" );
    if ( lst[0]=="command" )
    {
        config.command=lst[1];
        if ( config.command=="SHADOW" )
        {
            shadowSession=true;
            runRemoteCommand=false;
        }

        return;
    }
    if ( lst[0]=="server" )
    {
        config.server=lst[1];
        return;
    }
    if ( lst[0]=="session" )
    {
        config.session=lst[1];
        return;
    }
    if ( lst[0]=="sshport" )
    {
        config.sshport=lst[1];
        return;
    }
    if ( lst[0]=="user" )
    {
        config.user=lst[1];
        return;
    }
    if ( lst[0]=="rootless" )
    {
        if ( lst[1]=="true" )
            config.rootless=true;
        else
            config.rootless=false;
        return;
    }
    if ( lst[0]=="published" )
    {
        if ( lst[1]=="true" )
            config.published=true;
        else
            config.published=false;
        return;
    }
    if ( lst[0]=="checkexitstatus" )
    {
        if ( lst[1]=="true" )
            config.checkexitstatus=true;
        else
            config.checkexitstatus=false;
        return;
    }
    if ( lst[0]=="showtermbutton" )
    {
        if ( lst[1]=="true" )
            config.showtermbutton=true;
        else
            config.showtermbutton=false;
        return;
    }
    if ( lst[0]=="showexpbutton" )
    {
        if ( lst[1]=="true" )
            config.showexpbutton=true;
        else
            config.showexpbutton=false;
        return;
    }
    if ( lst[0]=="showconfig" )
    {
        if ( lst[1]=="true" )
            config.showconfig=true;
        else
            config.showconfig=false;
        return;
    }
    if ( lst[0]=="showextconfig" )
    {
        if ( lst[1]=="true" )
            config.showextconfig=true;
        else
            config.showextconfig=false;
        return;
    }
    if ( lst[0]=="showstatusbar" )
    {
        if ( lst[1]=="true" )
            config.showstatusbar=true;
        else
            config.showstatusbar=false;
        return;
    }
    if ( lst[0]=="showtoolbar" )
    {
        if ( lst[1]=="true" )
            config.showtoolbar=true;
        else
            config.showtoolbar=false;
        return;
    }

    if ( lst[0]=="sound" )
    {
        config.confSnd=true;
        if ( lst[1]=="true" )
            config.useSnd=true;
        else
            config.useSnd=false;
        return;
    }
    if ( lst[0]=="exportfs" )
    {
        config.confFS=true;
        if ( lst[1]=="true" )
            config.useFs=true;
        else
            config.useFs=false;
        return;
    }

    if ( lst[0]=="speed" )
    {
        config.confConSpd=true;
        config.conSpeed=ADSL;

        if ( lst[1]=="modem" )
            config.conSpeed=MODEM;
        else if ( lst[1]=="isdn" )
            config.conSpeed=ISDN;
        else if ( lst[1]=="adsl" )
            config.conSpeed=ADSL;
        else if ( lst[1]=="wan" )
            config.conSpeed=WAN;
        else if ( lst[1]=="lan" )
            config.conSpeed=LAN;
        else
        {
            qCritical (
                "%s",tr (
                    "Invalid value for argument \"speed\""
                ).toLocal8Bit().data() );
        }
        return;
    }
    if ( lst[0]=="compression" )
    {
        config.confCompMet=true;
        config.compMet=lst[1];
        return;
    }
    if ( lst[0]=="quality" )
    {
        config.confImageQ=true;
        config.imageQ=lst[1].toInt();
        return;
    }
    if ( lst[0]=="dpi" )
    {
        config.confDPI=true;
        config.dpi=lst[1].toInt();
        return;
    }
    if ( lst[0]=="kbdlayout" )
    {
        config.confKbd=true;
        config.kbdLay=lst[1];
        return;
    }
    if ( lst[0]=="kbdtype" )
    {
        config.confKbd=true;
        config.kbdType=lst[1];
        return;
    }
    if ( lst[0]=="brokerurl" )
    {
        config.brokerurl=lst[1];
        managedMode=true;
        acceptRsa=true;
    }
    if ( lst[0]=="cookie" )
    {
        config.cookie=lst[1];
        return;
    }
    if ( lst[0]=="x2gosession" )
    {
        config.sessiondata=lst[1];
        return;
    }
    if ( lst[0]=="connectionts" )
    {
        config.connectionts=lst[1];
        return;
    }
    if (lst[0] == "usesshproxy")
    {
        config.useproxy = true;
        if (lst[1].toLower () == "true") {
          config.useproxy = true;
        }
        else {
          config.useproxy = false;
        }
        return;
    }
    if (lst[0] == "sshproxytype")
    {
        if (lst[1].toLower () == "http") {
            config.proxyType = SshMasterConnection::PROXYHTTP;
        }
        else {
            config.proxyType = SshMasterConnection::PROXYSSH;
        }
        return;
    }
    if (lst[0] == "sshproxyuser")
    {
        config.proxylogin = lst[1];
        return;
    }
    if (lst[0] == "sshproxyhost")
    {
        config.proxyserver = lst[1];
        return;
    }
    if (lst[0] == "sshproxyport")
    {
        config.proxyport = lst[1].toInt ();
        return;
    }
    if (lst[0] == "sshproxyautologin")
    {
        if (lst[1].toLower () == "true") {
            config.proxyAutologin = true;
        }
        else {
            config.proxyAutologin = false;
        }
        return;
    }
    if (lst[0] == "sshproxykrblogin")
    {
        if (lst[1].toLower () == "true") {
            config.proxyKrbLogin = true;
        }
        else {
            config.proxyKrbLogin = false;
        }
        return;
    }
    if (lst[0] == "sshproxykeyfile")
    {
        config.proxyKey = lst[1];
        return;
    }
}

void ONMainWindow::slotChangeKbdLayout(const QString& layout)
{
#ifdef Q_OS_LINUX
    QStringList args;
    args<<"-layout"<<layout;

    x2goDebug<<"Running setxkbmap with params: "<<args.join(" ");

    QProcess::startDetached("setxkbmap",args);
#else
    /* Silence warning. */
    UNUSED (layout);
#endif /* defined (Q_OS_LINUX) */
}

void ONMainWindow::initPassDlg()
{
    passForm = new SVGFrame (images_resource_path( "/svg/passform.svg"),
                              false,bgFrame );
#ifdef Q_OS_WIN
    passForm->setMainWidget ( ( QWidget* ) this );
#endif
    username->addWidget ( passForm );
    passForm->hide();
    setWidgetStyle ( passForm );
    if ( !miniMode )
        passForm->setFixedSize ( passForm->sizeHint() );
    else
        passForm->setFixedSize ( 310,180 );
    QPalette pal=passForm->palette();
    pal.setBrush ( QPalette::Window, QColor ( 255,255,255,0 ) );
    pal.setColor ( QPalette::Active, QPalette::WindowText, QPalette::Mid );
    pal.setColor ( QPalette::Active, QPalette::ButtonText, QPalette::Mid );
    pal.setColor ( QPalette::Active, QPalette::Text, QPalette::Mid );
    pal.setColor ( QPalette::Inactive, QPalette::WindowText, QPalette::Mid );
    pal.setColor ( QPalette::Inactive, QPalette::ButtonText, QPalette::Mid );
    pal.setColor ( QPalette::Inactive, QPalette::Text, QPalette::Mid );
    passForm->setPalette ( pal );

    pal.setColor ( QPalette::Button, QColor ( 255,255,255,0 ) );
    pal.setColor ( QPalette::Window, QColor ( 255,255,255,255 ) );
    pal.setColor ( QPalette::Base, QColor ( 255,255,255,255 ) );


    QFont fnt=passForm->font();
    if ( miniMode )
        fnt.setPointSize ( 9 );
    passForm->setFont ( fnt );

    fotoLabel=new QLabel ( passForm );
    fotoLabel->hide();

    nameLabel=new QLabel ( "",passForm );
    nameLabel->hide();

    loginPrompt=new QLabel ( tr ( "Login:" ),passForm );
    passPrompt=new QLabel ( tr ( "Password:" ),passForm );
    layoutPrompt=new QLabel ( tr ( "Keyboard layout:" ),passForm );

    login=new ClickLineEdit ( passForm );
    setWidgetStyle ( login );
    login->setFrame ( false );
    login->setEnabled ( false );

    login->hide();
    loginPrompt->hide();

    pass=new ClickLineEdit ( passForm );
    setWidgetStyle ( pass );
    pass->setFrame ( false );
    fnt.setBold ( true );
    pass->setFont ( fnt );
    pass->setEchoMode ( QLineEdit::Password );
    pass->setFocus();

#ifdef Q_OS_LINUX
    connect ( login,SIGNAL ( clicked() ),this,
              SLOT ( slotActivateWindow() ) );
    connect ( pass,SIGNAL ( clicked() ),this,
              SLOT ( slotActivateWindow() ) );
#endif

    pass->hide();
    passPrompt->hide();


    cbLayout=new QComboBox(passForm);
    cbLayout->addItems(defaultLayout);
    cbLayout->setFocusPolicy(Qt::NoFocus);
    cbLayout->setFrame(false);
    setWidgetStyle(cbLayout);
    cbLayout->hide();
    layoutPrompt->hide();
    QHBoxLayout* cbLayoutLay=new QHBoxLayout();
    cbLayoutLay->addWidget(cbLayout);
    cbLayoutLay->addStretch();


    ok=new QPushButton ( tr ( "Ok" ),passForm );
    setWidgetStyle ( ok );
    cancel=new QPushButton ( tr ( "Cancel" ),passForm );
    setWidgetStyle ( cancel );
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    cancel->setShortcut (QKeySequence::Cancel);
#else /* QT_VERSION >= QT_VERSION_CHECK(5, 6, 0) */
#ifdef Q_OS_DARWIN
    QAction *cancel_action = new QAction (passForm);
    QList<QKeySequence> tmp_shortcuts;
    tmp_shortcuts << Qt::Key_Escape << (Qt::CTRL + Qt::Key_Period);
    cancel_action->setShortcuts (tmp_shortcuts);
    connect (cancel_action, SIGNAL (triggered ()), cancel, SLOT (animateClick ()));
    cancel->addAction (cancel_action);
#else /* defined (Q_OS_DARWIN) */
    cancel->setShortcut (QKeySequence (Qt::Key_Escape));
#endif /* defined (Q_OS_DARWIN) */
#endif /* QT_VERSION >= QT_VERSION_CHECK(5, 6, 0) */
    ok->hide();
    cancel->hide();



    cbLayout->setPalette ( pal );
    ok->setPalette ( pal );
    cancel->setPalette ( pal );



    ok->setFixedSize ( ok->sizeHint() );
    cancel->setFixedSize ( cancel->sizeHint() );

    QVBoxLayout *layout=new QVBoxLayout ( passForm );
    QHBoxLayout *labelLay=new QHBoxLayout();
    QHBoxLayout *inputLay=new QHBoxLayout();
    QHBoxLayout *buttonLay=new QHBoxLayout();

    labelLay->setSpacing ( 20 );
    inputLay->setSpacing ( 10 );
    layout->setContentsMargins ( 20,20,10,10 );
    layout->addLayout ( labelLay );
    layout->addStretch();
    layout->addLayout ( inputLay );
    layout->addStretch();
    layout->addLayout ( buttonLay );

    labelLay->addWidget ( fotoLabel );
    labelLay->addWidget ( nameLabel );
    labelLay->addStretch();

    QVBoxLayout* il1=new QVBoxLayout();
    il1->addWidget ( loginPrompt );
    il1->addWidget ( passPrompt );
    il1->addWidget ( layoutPrompt );

    QVBoxLayout* il2=new QVBoxLayout();
    il2->addWidget ( login );
    il2->addWidget ( pass );
    il2->addLayout ( cbLayoutLay );
    inputLay->addLayout ( il1 );
    inputLay->addLayout ( il2 );
    inputLay->addStretch();

    buttonLay->addStretch();
    buttonLay->addWidget ( ok );
    buttonLay->addWidget ( cancel );
    buttonLay->addStretch();

    pal.setColor ( QPalette::Base, QColor ( 239,239,239,255 ) );
    login->setPalette ( pal );
    pass->setPalette ( pal );

    connect ( ok,SIGNAL ( clicked() ),this, SLOT ( slotSessEnter() ) );
    connect ( cancel,SIGNAL ( clicked() ),this, SLOT ( slotClosePass() ) );
    connect ( pass,SIGNAL ( returnPressed() ),this,
              SLOT ( slotSessEnter() ) );
    connect ( login,SIGNAL ( returnPressed() ),pass, SLOT ( selectAll() ) );
    connect ( login,SIGNAL ( returnPressed() ),pass, SLOT ( setFocus() ) );

    passPrompt->show();
    pass->show();
    ok->show();
    cancel->show();
    fotoLabel->show();
    nameLabel->show();
    if ( !useLdap )
    {
        login->show();
        loginPrompt->show();
    }
    if ( embedMode )
    {
        cancel->setEnabled ( false );
#ifdef Q_OS_WIN
        QRect r;
        wapiWindowRect ( (HWND)ok->winId(),r );
#endif
    }
    if (defaultLayout.size()>1)
    {
        layoutPrompt->show();
        cbLayout->show();
        slotChangeKbdLayout(cbLayout->currentText());
        connect (cbLayout,SIGNAL(currentIndexChanged(QString)),this,SLOT(slotChangeKbdLayout(QString)));
    }
}


void ONMainWindow::initStatusDlg()
{
    sessionStatusDlg = new SVGFrame ( images_resource_path("/svg/passform.svg"),
                                      false,bgFrame );
    sessionStatusDlg->hide();
    if ( !miniMode )
        sessionStatusDlg->setFixedSize (
            sessionStatusDlg->sizeHint() );
    else
        sessionStatusDlg->setFixedSize ( 310,200 );
    QFont fnt=sessionStatusDlg->font();
    if ( miniMode )
        fnt.setPointSize ( 9 );
    sessionStatusDlg->setFont ( fnt );
    username->addWidget ( sessionStatusDlg );
    QPalette pal=sessionStatusDlg->palette();
    pal.setBrush ( QPalette::Window, QColor ( 0,0,0,0 ) );
    pal.setColor ( QPalette::Active, QPalette::WindowText, QPalette::Mid );
    pal.setColor ( QPalette::Active, QPalette::ButtonText, QPalette::Mid );
    pal.setColor ( QPalette::Active, QPalette::Text, QPalette::Mid );
    pal.setColor ( QPalette::Inactive, QPalette::WindowText, QPalette::Mid );
    pal.setColor ( QPalette::Inactive, QPalette::ButtonText, QPalette::Mid );
    pal.setColor ( QPalette::Inactive, QPalette::Text, QPalette::Mid );

    sessionStatusDlg->setPalette ( pal );

    slName=new QLabel ( sessionStatusDlg );
    slVal=new QLabel ( sessionStatusDlg );

    slName->setText (
        tr (
            "<b>Session ID:<br>Server:<br>Username:"
            "<br>Display:<br>Creation time:<br>Status:</b>" ) );
    slName->setFixedSize ( slName->sizeHint() );
    slName->hide();

    slVal->hide();
    slVal->setFixedHeight ( slName->sizeHint().height() );

    sbApps=new QToolButton (sessionStatusDlg );
    sbApps->setToolTip(tr ( "Applications ..." ));
    sbApps->setIcon(QPixmap(iconsPath("/32x32/apps.png")));
    sbApps->setAutoRaise(true);
    sbApps->setFocusPolicy(Qt::NoFocus);

    sbExp=new QToolButton (sessionStatusDlg );
    sbExp->setIcon(QPixmap(iconsPath("/32x32/open_dir.png")));
    sbExp->setToolTip (tr ("Share folder ..." ));
    sbExp->setAutoRaise(true);
    sbExp->setFocusPolicy(Qt::NoFocus);

    sbSusp=new QToolButton (sessionStatusDlg );
    sbSusp->setIcon(QPixmap(iconsPath("/32x32/suspend_session.png")));
    sbSusp->setToolTip(tr ( "Abort" ));
    sbSusp->setAutoRaise(true);
    sbSusp->setFocusPolicy(Qt::NoFocus);


    sbTerm=new QToolButton (sessionStatusDlg );
    sbTerm->setIcon(QPixmap(iconsPath("/32x32/stop_session.png")));
    sbTerm->setToolTip(tr ( "Terminate" ));
    sbTerm->setAutoRaise(true);
    sbTerm->setFocusPolicy(Qt::NoFocus);


    sbAdv=new QCheckBox ( tr ( "Show details" ),sessionStatusDlg );
    setWidgetStyle ( sbTerm );
    setWidgetStyle ( sbApps );
    setWidgetStyle ( sbExp );
    setWidgetStyle ( sbSusp );
    setWidgetStyle ( sbAdv );

    sbAdv->setFixedSize ( sbAdv->sizeHint() );
    sbApps->setFixedSize ( 32,32 );
    sbSusp->setFixedSize ( 32,32 );
    sbTerm->setFixedSize ( 32,32 );
    sbExp->setFixedSize ( 32,32 );

    /*
        sbApps->setFocusPolicy(Qt::NoFocus);
        sbSusp->setFocusPolicy(Qt::NoFocus);
        sbTerm->setFocusPolicy(Qt::NoFocus);
        sbExp->setFocusPolicy(Qt::NoFocus);*/

    sbAdv->hide();
    sbSusp->hide();
    sbTerm->hide();
    sbExp->hide();
    sbApps->hide();


    pal.setColor ( QPalette::Button, QColor ( 255,255,255,0 ) );
    pal.setColor ( QPalette::Window, QColor ( 255,255,255,255 ) );
    pal.setColor ( QPalette::Base, QColor ( 255,255,255,255 ) );

    sbAdv->setPalette ( pal );
    sbApps->setPalette ( pal );
    sbSusp->setPalette ( pal );
    sbTerm->setPalette ( pal );
    sbExp->setPalette ( pal );

    stInfo=new QTextEdit ( sessionStatusDlg );
    setWidgetStyle ( stInfo );
    setWidgetStyle ( stInfo->verticalScrollBar() );
    stInfo->setReadOnly ( true );
    stInfo->hide();
    stInfo->setFrameStyle ( QFrame::StyledPanel|QFrame::Plain );
    stInfo->setPalette ( pal );

    sbExp->setEnabled ( false );

    connect ( sbSusp,SIGNAL ( clicked() ),this,
              SLOT ( slotTestSessionStatus() ) );
    connect ( sbTerm,SIGNAL ( clicked() ),this,
              SLOT ( slotTermSessFromSt() ) );
    connect ( sbAdv,SIGNAL ( clicked() ),this,
              SLOT ( slotShowAdvancedStat() ) );
    connect ( sbExp,SIGNAL ( clicked() ),this,
              SLOT ( slotExportDirectory() ) );
    connect ( sbApps,SIGNAL ( clicked() ),this,
              SLOT ( slotAppDialog()) );

    QVBoxLayout* layout=new QVBoxLayout ( sessionStatusDlg );
    QHBoxLayout* ll=new QHBoxLayout();
    ll->addWidget ( slName );
    ll->addWidget ( slVal );
    ll->addStretch();
    ll->setSpacing ( 10 );
    if ( !miniMode )
        layout->setContentsMargins ( 25,25,10,10 );
    else
        layout->setContentsMargins ( 10,10,10,10 );

    QHBoxLayout* bl=new QHBoxLayout();
    bl->addStretch();
    bl->addWidget ( sbApps );
    bl->addWidget ( sbExp );
    bl->addWidget ( sbSusp );
    bl->addWidget ( sbTerm );
//     bl->addStretch();
    layout->addLayout ( ll );
    layout->addStretch();
    layout->addWidget ( stInfo );
    layout->addWidget ( sbAdv );
    layout->addStretch();
    layout->addLayout ( bl );
    layout->setSizeConstraint( QLayout::SetFixedSize );


    slName->show();
    slVal->show();
    sbAdv->show();
    if ( !embedMode )
    {
        sbSusp->show();
        sbTerm->show();
        if (! hideFolderSharing )
            sbExp->show();
    }

    X2goSettings st ( "settings" );


    if ( st.setting()->value ( "showStatus", ( QVariant ) false ).toBool() )
    {
        sbAdv->setChecked ( true );
        slotShowAdvancedStat();
    }
#ifdef Q_OS_WIN
    if ( embedMode )
    {
        QRect r;
        wapiWindowRect ( (HWND)sbAdv->winId(),r );
        wapiWindowRect ( (HWND)stInfo->verticalScrollBar ()->winId(),r );
    }
#endif

}


void ONMainWindow::initSelectSessDlg()
{
    selectSessionDlg = new SVGFrame (images_resource_path( "/svg/passform.svg"),
                                      false,bgFrame );
    username->addWidget ( selectSessionDlg );
    setWidgetStyle ( selectSessionDlg );
    if ( !miniMode ) {
        QSize hint_size = selectSessionDlg->sizeHint ();
        selectSessionDlg->setMinimumSize (QSize (hint_size.width (), (hint_size.height () + 100)));
        selectSessionDlg->setFixedSize (selectSessionDlg->minimumSize ());
    }
    else
        selectSessionDlg->setFixedSize ( 310,240 );
    QPalette pal=selectSessionDlg->palette();
    pal.setBrush ( QPalette::Window, QColor ( 255,255,255,0 ) );
    pal.setColor ( QPalette::Active, QPalette::WindowText, QPalette::Mid );
    pal.setColor ( QPalette::Active, QPalette::ButtonText, QPalette::Mid );
    pal.setColor ( QPalette::Active, QPalette::Text, QPalette::Mid );
    pal.setColor ( QPalette::Inactive, QPalette::WindowText, QPalette::Mid );
    pal.setColor ( QPalette::Inactive, QPalette::ButtonText, QPalette::Mid );
    pal.setColor ( QPalette::Inactive, QPalette::Text, QPalette::Mid );

    selectSessionDlg->setPalette ( pal );

    pal.setColor ( QPalette::Button, QColor ( 255,255,255,0 ) );
    pal.setColor ( QPalette::Window, QColor ( 255,255,255,255 ) );
    pal.setColor ( QPalette::Base, QColor ( 255,255,255,255 ) );

    QFont fnt=selectSessionDlg->font();
    if ( miniMode )
        fnt.setPointSize ( 9 );
    selectSessionDlg->setFont ( fnt );
    selectSessionLabel=new QLabel ( tr ( "Select session:" ),
                                    selectSessionDlg );
    running_label = new QLabel (tr ("<b>Note:</b> the currently selected session "
                                     "is already running.<br />To forcefully "
                                     "resume it, suspend it first."),
                                 selectSessionDlg);
    running_label->setWordWrap (true);
    sOk=new QPushButton ( tr ( "Resume" ),selectSessionDlg );
    setWidgetStyle ( sOk );
    sCancel=new QPushButton ( tr ( "Cancel" ),selectSessionDlg );
    setWidgetStyle ( sCancel );
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    sCancel->setShortcut (QKeySequence::Cancel);
#else /* QT_VERSION >= QT_VERSION_CHECK(5, 6, 0) */
#ifdef Q_OS_DARWIN
    QAction *cancel_action = new QAction (selectSessionDlg);
    QList<QKeySequence> tmp_shortcuts;
    tmp_shortcuts << Qt::Key_Escape << (Qt::CTRL + Qt::Key_Period);
    cancel_action->setShortcuts (tmp_shortcuts);
    connect (cancel_action, SIGNAL (triggered ()), sCancel, SLOT (animateClick ()));
    sCancel->addAction (cancel_action);
#else /* defined (Q_OS_DARWIN) */
    sCancel->setShortcut (QKeySequence (Qt::Key_Escape));
#endif /* defined (Q_OS_DARWIN) */
#endif /* QT_VERSION >= QT_VERSION_CHECK(5, 6, 0) */
    bCancel=new QPushButton ( tr ( "Cancel" ),selectSessionDlg );
    setWidgetStyle ( bCancel );

    bSusp=new QPushButton ( tr ( "Suspend" ),selectSessionDlg );
    setWidgetStyle ( bSusp );
    bTerm=new QPushButton ( tr ( "Terminate" ),selectSessionDlg );
    setWidgetStyle ( bTerm );

    bNew=new QPushButton ( tr ( "New" ),selectSessionDlg );
    setWidgetStyle ( bNew );

    bShadow=new QPushButton ( tr ( "Full access" ),selectSessionDlg );
    setWidgetStyle ( bShadow );

    bShadowView=new QPushButton ( tr ( "View only" ),selectSessionDlg );
    setWidgetStyle ( bShadowView );

    sOk->setPalette ( pal );
    sCancel->setPalette ( pal );

    connect ( sCancel,SIGNAL ( clicked() ),this,
              SLOT ( slotCloseSelectDlg() ) );
    connect ( bCancel,SIGNAL ( clicked() ),this,
              SLOT ( slotCloseSelectDlg() ) );

    selectSessionDlg->show();
    sOk->setFixedSize ( ok->sizeHint() );
    sCancel->setFixedSize ( cancel->sizeHint() );
    int bmaxw=bNew->size().width();
    if ( bSusp->size().width() >bmaxw )
        bmaxw=bSusp->size().width();
    if ( bTerm->size().width() >bmaxw )
        bmaxw=bTerm->size().width();

    bNew->setFixedWidth ( bmaxw );
    bSusp->setFixedWidth ( bmaxw );
    bTerm->setFixedWidth ( bmaxw );



    sOk->setEnabled (false);
    sCancel->setEnabled ( true );
    selectSessionDlg->setEnabled ( true );
    setEnabled ( true );

    sessTv=new SessTreeView ( selectSessionDlg );
    setWidgetStyle ( sessTv );
    setWidgetStyle ( sessTv->horizontalScrollBar() );
    setWidgetStyle ( sessTv->verticalScrollBar() );
    sessTv->setItemsExpandable ( false );
    sessTv->setRootIsDecorated ( false );

    model=new QStandardItemModel ( sessionExplorer->getSessionsList()->size(), 8 );
    if(!brokerMode)
        model->setHeaderData ( S_DISPLAY,Qt::Horizontal,
                               QVariant ( ( QString ) tr ( "Display" ) ) );
    else
    {
        model->setHeaderData ( S_DISPLAY,Qt::Horizontal,
                               QVariant ( ( QString ) tr ( "User" ) ) );
    }
    model->setHeaderData ( S_STATUS,Qt::Horizontal,
                           QVariant ( ( QString ) tr ( "Status" ) ) );
    model->setHeaderData ( S_COMMAND,Qt::Horizontal,
                           QVariant ( ( QString ) tr ( "Command" ) ) );
    model->setHeaderData ( S_TYPE,Qt::Horizontal,
                           QVariant ( ( QString ) tr ( "Type" ) ) );
    model->setHeaderData ( S_SERVER,Qt::Horizontal,
                           QVariant ( ( QString ) tr ( "Server" ) ) );
    model->setHeaderData (
        S_CRTIME,Qt::Horizontal,
        QVariant ( ( QString ) tr ( "Creation time" ) ) );
    model->setHeaderData ( S_IP,Qt::Horizontal,
                           QVariant ( ( QString ) tr ( "Client IP" ) ) );
    model->setHeaderData ( S_ID,Qt::Horizontal,
                           QVariant ( ( QString ) tr ( "Session ID" ) ) );

    modelDesktop=new QStandardItemModel ( sessionExplorer->getSessionsList()->size(), 2 );
    modelDesktop->setHeaderData ( D_USER,Qt::Horizontal,
                                  QVariant ( ( QString ) tr ( "User" ) ) );
    modelDesktop->setHeaderData (
        D_DISPLAY,Qt::Horizontal,
        QVariant ( ( QString ) tr ( "Display" ) ) );

    sessTv->setModel ( ( QAbstractItemModel* ) model );

    QFontMetrics fm ( sessTv->font() );
    sessTv->setEditTriggers ( QAbstractItemView::NoEditTriggers );
    sessTv->setPalette ( pal );

    sessTv->setModel ( ( QAbstractItemModel* ) model );

    bNew->setPalette ( pal );
    bShadow->setPalette ( pal );
    bShadowView->setPalette ( pal );
    bSusp->setPalette ( pal );
    bTerm->setPalette ( pal );
    sessTv->setFrameStyle ( QFrame::StyledPanel|QFrame::Plain );
    bSusp->setEnabled ( false );
    bTerm->setEnabled ( false );
    bShadow->setEnabled ( false );
    selectSessionLabel->hide();
    running_label->hide ();
    bCancel->setPalette ( pal );
    bCancel->hide();

    desktopFilter=new QLineEdit ( selectSessionDlg );
    setWidgetStyle ( desktopFilter );
// 	desktopFilter->setFrame ( false );

    desktopFilterCb=new QCheckBox ( tr ( "Only my desktops" ),
                                    selectSessionDlg );
    desktopFilterCb->hide();

    QVBoxLayout* layout=new QVBoxLayout ( selectSessionDlg );
    QHBoxLayout* filterLay=new QHBoxLayout();
    QHBoxLayout* blay=new QHBoxLayout();
    QVBoxLayout* alay=new QVBoxLayout();
    QHBoxLayout* tvlay=new QHBoxLayout();

    selectSesDlgLayout=layout;

    layout->addWidget ( selectSessionLabel );
    layout->addLayout ( filterLay );
    layout->addLayout ( tvlay );
    layout->addStretch ();
    layout->addWidget (running_label);
    layout->addStretch ();
    layout->addLayout ( blay );

    filterLay->addWidget ( desktopFilter );
    filterLay->addWidget ( desktopFilterCb );

    alay->addWidget ( bSusp );
    alay->addWidget ( bTerm );
    alay->addWidget ( bShadowView );
    alay->addWidget ( bShadow );
    alay->addStretch();
    alay->addWidget ( bNew );
    alay->addWidget ( bCancel );

    tvlay->addWidget ( sessTv );
    tvlay->addLayout ( alay );

    blay->addStretch();
    blay->addWidget ( sOk );
    blay->addWidget ( sCancel );
    blay->addStretch();
    if ( !miniMode )
        layout->setContentsMargins ( 25,25,10,10 );
    else
        layout->setContentsMargins ( 10,10,10,10 );



    sOk->hide();
    sCancel->hide();
    bNew->hide();
    bSusp->hide();
    bTerm->hide();

    connect ( sessTv,SIGNAL ( selected ( const QModelIndex& ) ),
              this,SLOT ( slotActivated ( const QModelIndex& ) ) );

    connect ( sOk,SIGNAL ( clicked() ),this, SLOT ( slotResumeSess() ) );
    connect ( sessTv,SIGNAL ( doubleClicked ( const QModelIndex& ) ),
              this,SLOT ( slotResumeDoubleClick ( const QModelIndex& ) ) );

    if(!brokerMode)
    {
        connect ( bNew,SIGNAL ( clicked() ),this, SLOT ( slotNewSess() ) );
    }
    else
    {
        connect ( bNew,SIGNAL ( clicked() ),broker, SIGNAL ( sessionSelected() ) );
    }

    connect ( bSusp,SIGNAL ( clicked() ),this, SLOT ( slotSuspendSess() ) );
    connect ( bTerm,SIGNAL ( clicked() ),this, SLOT ( slotTermSess() ) );

    connect ( bShadow,SIGNAL ( clicked() ),this,
              SLOT ( slotShadowSess() ) );
    connect ( bShadowView,SIGNAL ( clicked() ),this,
              SLOT ( slotShadowViewSess() ) );

    connect ( desktopFilter,SIGNAL ( textEdited ( const QString& ) ),this,
              SLOT ( slotDesktopFilterChanged ( const QString& ) ) );
    connect ( desktopFilterCb,SIGNAL ( stateChanged ( int ) ),
              this,
              SLOT ( slotDesktopFilterCb ( int ) ) );

    selectSessionLabel->show();
    sOk->show();
    sCancel->show();
    bNew->show();
    bSusp->show();
    bTerm->show();
    sessTv->show();
    selectSessionDlg->hide();
#ifdef Q_OS_WIN
    if ( embedMode )
    {
        QRect r;
        wapiWindowRect ( (HWND)sOk->winId(),r );
        wapiWindowRect ( (HWND)sessTv->verticalScrollBar ()->winId(),r );
        wapiWindowRect ( (HWND)sessTv->horizontalScrollBar ()->winId(),r );
        wapiWindowRect ( (HWND)sessTv->header ()->viewport()->winId(),r );
    }
#endif

}


void ONMainWindow::printSshDError_startupFailure()
{
    if ( closeEventSent )
        return;
    QString error_message;

    error_message = tr ("SSH daemon could not be started.\n\n");

    QString detailed_error_message = tr ("You have enabled Remote Printing or File Sharing.\n"
                                         "These features require a running and functioning SSH server on your computer.\n"
                                         "<b>Printing and File Sharing will be disabled for this session.</b>\n\n");
#ifdef Q_OS_WIN
    detailed_error_message += tr ("Normally, this should not happen as X2Go Client for Windows "
                                  "ships its own internal SSH server.\n\n"

                                  "If you see this message, please report a bug on:\n");
#else /* defined (Q_OS_WIN) */
    detailed_error_message += tr ("The SSH server failed to start.\n\n"

                                  "Please report a bug on:\n");
#endif /* defined (Q_OS_WIN) */
    detailed_error_message += tr ("<center><a href=\"https://wiki.x2go.org/doku.php/wiki:bugs\">"
                                      "https://wiki.x2go.org/doku.php/wiki:bugs"
                                  "</a></center>\n");
    detailed_error_message += tr ("Disabling Remote Printing or File Sharing support "
                                  "in the session settings will get rid of this message.");

    Non_Modal_MessageBox::critical (0l, "X2Go Client",
                                    error_message, detailed_error_message, true,
                                    QMessageBox::Ok, QMessageBox::NoButton);
}

void ONMainWindow::printSshDError_noHostPubKey()
{
    if ( closeEventSent )
        return;

    QString error_message = tr ("SSH daemon failed to open its public host key.");

    QString detailed_error_message = tr ("You have enabled Remote Printing or File Sharing.\n"
                                         "These features require a running and functioning SSH server on your computer.\n"
                                         "<b>Printing and File Sharing will be disabled for this session.</b>\n\n");
#ifdef Q_OS_WIN
    detailed_error_message += tr ("Normally, this should not happen as X2Go Client for Windows "
                                  "ships its own internal SSH server and automatically "
                                  "generates the required keys.\n\n"

                                  "If you see this message, please report a bug on:\n");
#else /* defined (Q_OS_WIN) */
    detailed_error_message += tr ("X2Go Client was unable to create SSH host keys.\n\n"

                                  "Please report a bug on:\n");
#endif /* defined (Q_OS_WIN) */
    detailed_error_message += tr ("<center><a href=\"https://wiki.x2go.org/doku.php/wiki:bugs\">"
                                      "https://wiki.x2go.org/doku.php/wiki:bugs"
                                  "</a></center>\n");

    detailed_error_message += tr ("Disabling Remote Printing or File Sharing support "
                                  "in the session settings will get rid of this message.");

    Non_Modal_MessageBox::critical (0l, "X2Go Client", error_message, detailed_error_message,
                                    true,
                                    QMessageBox::Ok, QMessageBox::NoButton);
}

void ONMainWindow::printSshDError_noExportPubKey()
{
    if ( closeEventSent )
        return;
    QMessageBox::critical ( 0l,tr ( "SSH Error" ),
                            tr ( "SSH daemon failed to open the application's public key\n"
                                 "used for exporting folders and printers."
                               ),
                            QMessageBox::Ok,QMessageBox::NoButton );
}

void ONMainWindow::printSshDError_noAuthorizedKeysFile()
{
    if ( closeEventSent )
        return;
    QMessageBox::critical ( 0l,tr ( "SSH Error" ),
                            tr ( "SSH daemon failed to open the application's\n"
                                 "authorized_keys file."
                               ),
                            QMessageBox::Ok,QMessageBox::NoButton );
}


void ONMainWindow::slotPCookieReady (	bool result,
                                        QString ,
                                        int )
{
    /* Silence warning. */
    UNUSED (result);
}


void ONMainWindow::loadPulseModuleNativeProtocol()
{
    QProcess* proc=new QProcess ( this );
    QStringList args;
    args<<"load-module"<<"module-native-protocol-tcp";
    proc->start ( "pactl",args );
    proc->waitForFinished ( 3000 );
}

void ONMainWindow::slotEmbedToolBar()
{
    if ( statusLabel )
    {
        delete statusLabel;
        statusLabel=0;
    }
    if ( embedTbVisible )
    {
        stb->clear();
        act_embedToolBar->setIcon (
            QIcon ( iconsPath("/16x16/tbshow.png" )) );
        stb->addAction ( act_embedToolBar );
        stb->setToolButtonStyle ( Qt::ToolButtonIconOnly );
        stb->widgetForAction (
            act_embedToolBar )->setFixedHeight ( 16 );
        act_embedToolBar->setText ( tr ( "Restore toolbar" ) );
        statusLabel=new QLabel;
        stb->addWidget ( statusLabel );
#ifndef Q_OS_WIN
        if(!brokerMode)
            statusBar()->hide();
#endif
    }
    else
    {
        initEmbedToolBar();
        act_embedToolBar->setIcon (
            QIcon ( iconsPath("/32x32/tbhide.png" )) );
        act_embedToolBar->setText ( tr ( "Minimize toolbar" ) );
    }
    embedTbVisible=!embedTbVisible;
    if ( proxyWinEmbedded )
        setStatStatus();
    X2goSettings st ( "sessions" );
    st.setting()->setValue ( "embedded/tbvisible", embedTbVisible );
    st.setting()->sync();
}

void ONMainWindow::initEmbedToolBar()
{
    stb->addAction ( act_embedToolBar );
    stb->addSeparator();
    stb->setToolButtonStyle ( Qt::ToolButtonTextUnderIcon );
    stb->addAction ( act_shareFolder );
    stb->addAction ( act_showApps );
    stb->addAction ( act_reconnect );
    stb->addAction ( act_suspend );
    stb->addAction ( act_terminate );
    stb->addSeparator();
    stb->addAction ( act_embedContol );
    stb->addSeparator();
    stb->addAction ( act_set );
    stb->addAction ( act_abclient );
}

void ONMainWindow::slotEmbedToolBarToolTip()
{
    if ( !showTbTooltip )
        return;
    QWidget* widg=stb->widgetForAction (
                      act_embedToolBar );
    QToolTip::showText ( this->mapToGlobal ( QPoint ( 6,6 ) ),
                         tr ( "<br><b>&nbsp;&nbsp;&nbsp;Click this "
                              "button&nbsp;&nbsp;&nbsp;<br>"
                              "&nbsp;&nbsp;&nbsp;to restore toolbar"
                              "&nbsp;&nbsp;&nbsp;</b><br>" ),
                         widg );
}


void ONMainWindow::slotActivateWindow()
{
    if ( embedMode )
    {
        QApplication::setActiveWindow ( this ) ;
        activateWindow();


        /*		x2goDebug<<"focus:"<<pass->hasFocus();
                x2goDebug<<"activ:"<<pass->isActiveWindow();*/

        QTimer::singleShot ( 50, this,
                             SLOT ( slotEmbedToolBarToolTip() ) );
    }
}

#ifndef Q_OS_WIN
void ONMainWindow::mouseReleaseEvent ( QMouseEvent * event )
{
    QMainWindow::mouseReleaseEvent ( event );
    slotActivateWindow();
}
#endif

void ONMainWindow::slotHideEmbedToolBarToolTip()
{
    showTbTooltip=false;
    QToolTip::hideText();
}


void ONMainWindow::slotDesktopFilterChanged ( const QString& text )
{
    filterDesktops ( text );
}


void ONMainWindow::slotDesktopFilterCb ( int state )
{
    if ( state==Qt::Checked )
    {
        filterDesktops ( getCurrentUname(),true );
        desktopFilter->setEnabled ( false );
    }
    else
    {
        filterDesktops ( desktopFilter->text() );
        desktopFilter->setEnabled ( true );
    }
}


void ONMainWindow::filterDesktops ( const QString& filter, bool strict )
{
    modelDesktop->setRowCount ( 0 );
    bShadow->setEnabled ( false );
    bShadowView->setEnabled ( false );
    QFontMetrics fm ( sessTv->font() );
    uint nextRow=0;
    for ( int row = 0; row < selectedDesktops.size(); ++row )
    {
        QStringList desktop=selectedDesktops[row].split ( "@" );
        if ( filter==tr ( "Filter" ) ||filter.length() <=0||
                ( strict && desktop[0]==filter )  ||
                ( !strict && desktop[0].startsWith ( filter ) ) )
        {
            QStandardItem *item;
            item= new QStandardItem ( desktop[0] );
            modelDesktop->setItem ( nextRow,D_USER,item );
            item= new QStandardItem ( desktop[1] );
            modelDesktop->setItem ( nextRow++,D_DISPLAY,item );
            for ( int j=0; j<2; ++j )
            {
                QString txt=
                    modelDesktop->index (
                        row,j ).data().toString();
                if ( sessTv->header()->sectionSize ( j ) <
                        fm.width ( txt ) +6 )
                {
                    sessTv->header()->resizeSection (
                        j,fm.width ( txt ) +6 );
                }
            }
        }
    }
}


void ONMainWindow::slotShadowSess()
{
    shadowMode=SHADOW_FULL;
    slotShadowViewSess();
}

void ONMainWindow::slotShadowViewSess()
{
    shadowUser=sessTv->model()->index ( sessTv->currentIndex().row(),
                                        D_USER ).data().toString();
    shadowDisplay=sessTv->model()->index ( sessTv->currentIndex().row(),
                                           D_DISPLAY ).data().toString();
    startNewSession();
}


void ONMainWindow::slotReconnectSession()
{
    if ( !managedMode )
        slotSelectedFromList ( ( SessionButton* ) 0 );
    else
    {
        setEnabled ( false );
    }
}


QSize ONMainWindow::getEmbedAreaSize()
{
    if ( embedTbVisible && config.showstatusbar )
        statusBar()->show();
    QSize sz=bgFrame->size();
//     sz.setHeight(sz.height()-statusBar()->size().height());
    if(!brokerMode)
        statusBar()->hide();
    return sz;
}


void ONMainWindow::slotStartBroker()
{
    config.brokerPass=pass->text();
    config.brokerUser=login->text();
    setStatStatus ( tr ( "Connecting to broker" ) );
    stInfo->insertPlainText ( "broker url: "+config.brokerurl );
    setEnabled ( false );
    if(!usePGPCard)
        broker->getUserSessions();
}

void ONMainWindow::slotGetBrokerSession()
{
    startSession ( config.session);
}

void ONMainWindow::slotStartNewBrokerSession ( )
{
    if ( managedMode )
    {
        setEnabled ( true );
        slotSelectedFromList ( ( SessionButton* ) 0 );
    }
}

#ifdef Q_OS_WIN
QString ONMainWindow::u3DataPath()
{
    QStringList env=QProcess::systemEnvironment();
    QString dpath;
    for ( int i=0; i<env.size(); ++i )
    {
        //check if we have U3 System
        if ( env[i].indexOf ( "U3_APP_DATA_PATH=" ) ==0 )
        {
            dpath=env[i];
        }
        if ( env[i].indexOf ( "U3_DEVICE_PATH=" ) ==0 )
        {
            u3Device=env[i];
            u3Device.replace ( "U3_DEVICE_PATH=","" );
        }

    }

    if ( dpath.length() >0 )
    {
        dpath.replace ( "U3_APP_DATA_PATH=","" );
        portableDataPath=dpath;
        return dpath;
    }
    return QString();
}

#endif

void ONMainWindow::cleanPortable()
{
    removeDir ( homeDir +"/.ssh" );
    removeDir ( homeDir +"/ssh" );
    removeDir ( homeDir+"/.x2go" );
    if (cleanAllFiles)
        removeDir(homeDir+"/.x2goclient");
}

void ONMainWindow::removeDir ( QString path )
{

    x2goDebug<<"removeDir, entering: " <<path;

    QDir dr ( path );
    QStringList files=dr.entryList ( QDir::Files );
    for ( int i=0; i<files.size(); ++i )
    {
        if ( files[i]!="known_hosts" || cleanAllFiles)
        {

            x2goDebug<<"Cleaning file: "<<path+"/"+files[i];

            dr.remove ( path+"/"+files[i] );
        }
    }
    QStringList dirs=dr.entryList ( QDir::AllDirs|QDir::NoDotAndDotDot );
    for ( int i=0; i<dirs.size(); ++i )
    {
        removeDir ( path+"/"+dirs[i] );
    }
    dr.rmdir ( path );
}

#ifdef Q_OS_LINUX
long ONMainWindow::X11FindWindow ( QString text, long rootWin )
{
    Window    wParent;
    Window    wRoot;
    Window   *child_list;
    unsigned  nChildren;
    long proxyId=0;
    if ( !rootWin )
        rootWin= XDefaultRootWindow ( QX11Info::display() );

    if ( XQueryTree ( QX11Info::display(),rootWin,&wRoot,&wParent,
                      &child_list,&nChildren ) )
    {
        for ( uint i=0; i<nChildren; ++i )
        {
            char *wname;
            if ( XFetchName ( QX11Info::display(),
                              child_list[i],&wname ) )
            {
                QString title ( wname );
                XFree ( wname );
                if ( title==text )
                {
                    proxyId=child_list[i];
                    break;
                }
            }
            proxyId=X11FindWindow ( text, child_list[i] );
            if ( proxyId )
                break;
        }
        XFree ( child_list );
    }
    return proxyId;
}
#endif

long ONMainWindow::findWindow ( QString text )
{
    x2goDebug<<"Searching window with title: " + text;
#ifdef Q_OS_LINUX
    return X11FindWindow ( text );
#endif
#ifdef Q_OS_WIN
    return ( long ) wapiFindWindow (text);
#endif
    return 0;
}

void ONMainWindow::slotInitLibssh () {
  /* Initialize libssh. This must be done outside of any threading context. */
  x2goDebug << "libssh not initialized yet. Initializing.";
  ssh_threads_set_callbacks(ssh_threads_get_pthread ());
  if (ssh_init ()) {
      x2goDebug << "Cannot initialize libssh.";
      QMessageBox::critical (this, tr ("libssh initialization failure"),
                             tr ("Unable to initialize libssh."));
      trayQuit ();
  }
}


void ONMainWindow::slotResLoadRequestFinished(QNetworkReply* reply)
{
    if(reply->error()!=QNetworkReply::NoError)
    {
        x2goDebug<<"Failed to load resource file "<<reply->url()<<reply->errorString();
        if(resReply.length()==1)
        {
            reply->deleteLater();
            resourcesLoaded=true;
            initUI();
            return;
        }
    }
    else
    {
//         x2goDebug<<reply->url().toString()<<" ...OK";
        if(resReply.length()==1)
        {
            QTemporaryFile fl;
            fl.open();
            resourceTmpDir= new QDir(fl.fileName()+"_d");
            fl.close();
            resourceDir=resourceTmpDir->path();
            x2goDebug<<"resources path:"<<resourceDir;
            resourceTmpDir->mkpath(resourceTmpDir->path());
            QString data=reply->readAll();
            QStringList lines=data.split("\n");
            foreach(QString line, lines)
            {
                if((line.indexOf("<file>")!=-1 )&&(line.indexOf("</file>")!=-1))
                {
                    line.replace("<file>","");
                    line.replace("</file>","");
                    line=line.trimmed();
                    resReply<<resourceLoader->get (QNetworkRequest(QUrl(resourceURL+"/"+line)));
                }
            }
        }
        else
        {
            QString fpath=resourceDir+reply->url().toString().replace(resourceURL,"");
            QStringList parts=fpath.split("/",Qt::SkipEmptyParts);
            QString fname=parts.takeLast();
            QString path="/"+parts.join("/");
            resourceTmpDir->mkpath(path);
            QFile file(path+"/"+fname);
            file.open(QIODevice::WriteOnly);
            file.write(reply->readAll());
            file.close();
        }
    }
    foreach(QNetworkReply* r, resReply)
    {
        if(!r->isFinished())
        {
            return;
        }
    }
    foreach(QNetworkReply* r, resReply)
        r->deleteLater();
    resourcesLoaded=true;
    initUI();
    return;
}

bool ONMainWindow::parseResourceUrl(const QString& url)
{
    x2goDebug<<"parsing resource url:"<<url;
    if((url.indexOf("http://")!=-1) ||(url.indexOf("https://")!=-1))
    {
        resourcesLoaded=false;
        resourceLoader=new QNetworkAccessManager ( this );

        connect ( resourceLoader,SIGNAL ( finished (QNetworkReply*) ),this,
                  SLOT ( slotResLoadRequestFinished (QNetworkReply*) ) );
        x2goDebug<<"downloading:"<<url+"/resources.qrc";
        resReply<<resourceLoader->get (QNetworkRequest(QUrl(url+"/resources.qrc")));
        resourceURL=url;
        return true;
    }

    resourceDir=url;
    if(url.indexOf("file://")==0)
    {
        resourceDir.replace("file://","");
    }
    QDir d(resourceDir);
    if(!d.exists())
    {
        resourceDir=QString();
        printError(tr("Directory not exists:")+" "+url+". "+tr("Using built in resources"));
    }
    else
    {
        x2goDebug<<"Using alternative resource directory:"<<resourceDir;
    }
    return true;
}

void ONMainWindow::getClientKeyboardConfig(QString& layout, QString& model, QString& variant)
{
    layout = "us";
    model = "pc105";
    variant = "";
#ifdef Q_OS_DARWIN
    //TODO:implement this for Mac
    return;
#endif
#ifdef Q_OS_WIN
    layout = wapiGetKeyboardLayout();
#else
    QProcess proc;
    proc.start("setxkbmap",QStringList() << "-query");
    if (!proc.waitForStarted())
    {
        x2goDebug<<"Failed to start xkbcomp"<<proc.error();
        return;
    }
    if (!proc.waitForFinished())
    {
        x2goDebug<<"Failed to execute xkbcomp"<<proc.error();
        return;
    }
    QStringList lines=QString(proc.readAllStandardOutput()).split("\n");
    QString line;
    foreach (line, lines)
    {
        line.replace(" ","");
        line.replace("\t","");
        if(line.indexOf("model:")!=-1)
        {
            model=line;
            model.replace("model:","");
        }
        if(line.indexOf("layout:")!=-1)
        {
            layout=line;
            layout.replace("layout:","");
        }
        if(line.indexOf("variant:")!=-1)
        {
            variant=line;
            variant.replace("variant:","");
        }
    }
    x2goDebug<<"xkbcomp keyboard config - model:"<<model<<" layout: "<<layout<<" variant: "<<variant;
#endif
}
