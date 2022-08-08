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

#ifndef ONMAINWINDOW_H
#define ONMAINWINDOW_H

#include "x2goclientconfig.h"
//#include "CallbackInterface.h"
#include <QMainWindow>
#include <QList>
#include <QPushButton>
#include <QPixmap>
#include <QProcess>
#include <QTreeView>
#include "LDAPSession.h"
#include <QToolBar>
#include <QSystemTrayIcon>
#include <QTranslator>
#include <QLocale>
#include <QProcessEnvironment>
#include <QDirIterator>
#include <QApplication>
#include <QDesktopWidget>
#include <libssh/callbacks.h>
#include "sshmasterconnection.h"
#include "non_modal_messagebox.h"


#ifdef Q_OS_WIN
#include <windows.h>
#include <QSysInfo>
#endif

#if defined (Q_OS_DARWIN) || defined (Q_OS_WIN)
#include "pulsemanager.h"
#endif /* defined (Q_OS_DARWIN) || defined (Q_OS_WIN) */

/**
@author Oleksandr Shneyder
*/
class QToolButton;
class QTemporaryFile;
class QLineEdit;
class QFrame;
class QVBoxLayout;
class QHBoxLayout;
class QScrollArea;
class UserButton;
class QTextEdit;
class SessionButton;
class QLabel;
class QProcess;
class QFile;
class SVGFrame;
class SessionButton;
class QAction;
class QCheckBox;
class QModelIndex;
class SshMasterConnection;
class IMGFrame;
class QStandardItemModel;
class HttpBrokerClient;
class QMenu;
class QComboBox;
class InteractionDialog;
class QNetworkAccessManager;
class QNetworkReply;
class SessionExplorer;
class QSplashScreen;
struct user
{
    int uin;
    QString uid;
    QString name;
    QPixmap foto;
    static bool lessThen ( user u1,user u2 )
    {
        return u1.uid < u2.uid;
    }
};

struct directory
{
    QString key;
    QString dstKey;
    QString dirList;
    bool isRemovable;
    int pid;
};

struct serv
{
    QString name;
    float factor;
    float sess;
    bool connOk;
    bool operator < ( const struct serv it )
    {
        return ( it.sess < sess );
    }
    static bool lt ( const struct serv it, const struct serv it1 )
    {
        return it.sess<it1.sess;
    }
    QString sshPort;
};

struct Application
{
    QString name;
    QString comment;
    QString exec;
    QPixmap icon;
    enum {MULTIMEDIA, DEVELOPMENT, EDUCATION, GAME,
          GRAPHICS, NETWORK, OFFICE,
          SETTINGS, SYSTEM, UTILITY, OTHER, TOP
         } category;
    static bool lessThen(Application t1, Application t2)
    {
        return (t1.name.compare(t2.name,Qt::CaseInsensitive)<0);
    }
};

struct x2goSession
{
    QString agentPid;
    QString sessionId;
    QString display;
    QString server;
    QString status;
    QString crTime;
    QString cookie;
    QString clientIp;
    QString grPort;
    QString sndPort;
    QString fsPort;
    QString brokerUser;
    uint connectedSince;
    bool published;
    int colorDepth;
    bool fullscreen;
    enum {DESKTOP,ROOTLESS,SHADOW, KDRIVE, ROOTLESSKDRIVE} sessionType;
    QString command;
    void operator = ( const x2goSession& s );
};

struct ConfigFile
{
    QString session;
    QString user;
    QString brokerUser;
    QString brokerPass;
    QString brokerUserId;
    QString brokerName;
    QString sshBrokerBin;
    bool brokerAuthenticated;
    bool brokerNoAuth;
    bool brokerAutologin;
    bool brokerAutologoff;
    bool brokerKrbLogin;
    bool brokerEvents; //Send events to broker and get control commands
    uint brokerLiveEventsTimeout; //(seconds)How often send alive events, 0 - do not send
    uint brokerSyncTimeout; //(seconds)How often synchronize with broker, 0 - not synchronize
    QString brokerSshKey;
    QString brokerCaCertFile;
    QString iniFile;
    QString server;
    QString serverIp;//Can be different from server (use for load balancing)
    QString sshport;
    QString command;
    QString key;
    bool rootless;
    QString cookie;
    QString connectionts;
    QString brokerurl;
    QString sessiondata;
    bool checkexitstatus;
    bool showtermbutton;
    bool showexpbutton;
    bool showextconfig;
    bool showconfig;
    bool showstatusbar;
    bool showtoolbar;

    //if true - use cfg values, else default or client settings
    bool confSnd;
    bool confFS;
    bool confConSpd;
    bool confCompMet;
    bool confImageQ;
    bool confDPI;
    bool confKbd;
    //
    bool useSnd;
    bool useFs;
    bool published;
    int conSpeed;
    QString compMet;
    int imageQ;
    int dpi;
    QString kbdLay;
    QString kbdType;
    //
    bool useproxy;
    SshMasterConnection::ProxyType proxyType;
    QString proxyserver;
    int proxyport;
    QString proxylogin;
    QString proxypassword;
    QString proxyKey;
    bool proxyAutologin;
    bool proxyKrbLogin;

};


struct sshKey
{
    QString server;
    QString port;
    QString user;
    QString key;
};


class SessTreeView : public QTreeView
{
    Q_OBJECT

public:
    SessTreeView ( QWidget* parent = 0 ) : QTreeView ( parent ) {}

    virtual void selectionChanged ( const QItemSelection& selected,
                                    const QItemSelection& deselected ) {
        emit this->selected ( currentIndex() );
        QTreeView::selectionChanged ( selected, deselected );
    }

Q_SIGNALS:
    void selected ( const QModelIndex& index );
};


class ClickLineEdit;
class ONMainWindow : public QMainWindow
{
    friend class HttpBrokerClient;
    friend class SessionButton;
    Q_OBJECT
public:
    enum
    {
        S_DISPLAY,
        S_STATUS,
        S_COMMAND,
        S_TYPE,
        S_SERVER,
        S_CRTIME,
        S_IP,
        S_ID
    };
    enum
    {
        MODEM,
        ISDN,
        ADSL,
        WAN,
        LAN
    };
    enum
    {
        D_USER,
        D_DISPLAY
    };
    enum
    {
        SHADOW_VIEWONLY,
        SHADOW_FULL
    };
    enum
    {
        PULSE,
        ARTS,
        ESD
    };

    enum key_types {
        RSA_KEY_TYPE,
        DSA_KEY_TYPE,
        ECDSA_KEY_TYPE,
        ED25519_KEY_TYPE,
        UNKNOWN_KEY_TYPE
    };

    enum client_events {
        CONNECTING,
        CONNECTED,
        SUSPENDING,
        TERMINATING,
        FINISHED,
        ALIVE
    };

    static bool debugging;
    static bool libssh_debugging;
    static bool libssh_packetlog;

    static bool portable;
    ONMainWindow ( QWidget *parent = 0 );
    ~ONMainWindow();
    static void installTranslator();
    QString iconsPath ( const QString &fname ) const;
    QString images_resource_path (const QString &filename, const QString &base = "") const;
    static bool isServerRunning ( int port );
    void startNewSession();
    void suspendSession ( QString sessId );
    void suspendBrokerSession ( const QString& sessId, const QString& host );
    bool termSession ( QString sessId,
                       bool warn=true );
    void termBrokerSession ( const QString& sessId, const QString& host );
    InteractionDialog* getInteractionDialog()
    {
      return interDlg;
    }
    void setStatStatus ( QString status=QString::null );
    x2goSession getNewSessionFromString ( const QString& string );
    void runCommand();
    long findWindow ( QString text );
    bool retUseLdap()
    {
        return useLdap;
    }
    bool retMiniMode()
    {
        return miniMode;
    }
    QString retLdapServer()
    {
        return ldapServer;
    }
    int retLdapPort()
    {
        return ldapPort;
    }
    QString retLdapDn()
    {
        return ldapDn;
    }
    QString retLdapServer1()
    {
        return ldapServer1;
    }
    int retLdapPort1()
    {
        return ldapPort1;
    }
    QString retLdapServer2()
    {
        return ldapServer2;
    }
    int retLdapPort2()
    {
        return ldapPort2;
    }
    QHBoxLayout* mainLayout()
    {
        return mainL;
    }
    QWidget* mainWidget()
    {
        return ( QWidget* ) fr;
    }

    static bool getPortable()
    {
        return portable;
    }
    static QString getHomeDirectory()
    {
        return homeDir;
    }
    bool getShowAdvOption()
    {
        return config.showextconfig;
    }
    bool getUsePGPCard()
    {
        return usePGPCard;
    }
    QString getCardLogin()
    {
        return cardLogin;
    }
    QString getDefaultCmd()
    {
        return defaultCmd;
    }
    QString getDefaultSshPort()
    {
        return defaultSshPort;
    }
    QString getDefaultClipboardMode()
    {
        return defaultClipboardMode;
    }
    QString getDefaultKbdType()
    {
        return defaultKbdType;
    }
    QStringList getDefaultLayout()
    {
        return defaultLayout;
    }
    QString getDefaultPack()
    {
        return defaultPack;
    }
    int getDefaultQuality()
    {
        return defaultQuality;
    }

    uint getDefaultDPI()
    {
        return defaultDPI;
    }

    bool getDefaultSetDPI()
    {
        return defaultSetDPI;
    }

    bool getEmbedMode()
    {
        return embedMode;
    }

    int getDefaultLink()
    {
        return defaultLink;
    }
    int getDefaultWidth()
    {
        return defaultWidth;
    }
    int getDefaultHeight()
    {
        return defaultHeight;
    }
    bool getDefaultSetKbd()
    {
        return defaultSetKbd;
    }
    bool getDefaultUseSound()
    {
        return defaultUseSound;
    }
    bool getDefaultFullscreen()
    {
        return defaultFullscreen;
    }
    bool sessionEditEnabled()
    {
        return !noSessionEdit;
    }
    const QList<Application>& getApplications()
    {
        return applications;
    }
    static QString getSessionConf()
    {
        return sessionCfg;
    }
    bool getHideFolderSharing()
    {
        return hideFolderSharing;
    }

    SessionExplorer* getSessionExplorer()
    {
        return sessionExplorer;
    }

    bool getBrokerMode()
    {
        return brokerMode;
    }

    bool getMiniMode()
    {
        return miniMode;
    }

    bool getAcceptRSA()
    {
        return acceptRsa;
    }

    QScrollArea* getUsersArea()
    {
        return users;
    }

    QFrame* getUsersFrame()
    {
        return uframe;
    }

    IMGFrame* getCentralFrame()
    {
        return fr;
    }

    ConfigFile* getConfig()
    {
        return &config;
    }

#if defined (Q_OS_DARWIN) || defined (Q_OS_WIN)
    bool getSystemDisablePARecord()
    {
        return systemDisablePARecord;
    }
    bool getSystemDisablePA()
    {
        return systemDisablePA;
    }
#endif /* defined (Q_OS_DARWIN) || defined (Q_OS_WIN) */

    void runApplication(QString exec);


    SshMasterConnection* findServerSshConnection(QString host);
    void cleanServerSshConnections();

    bool parseResourceUrl(const QString& url);
    void showHelp();
    void showVersion();
    void showTextFile(QString file, QString title);
    void showGit();
    void showChangelog();
    void showHelpPack();
    void exportDirs ( QString exports,bool removable=false );
    void reloadUsers();
    void setWidgetStyle ( QWidget* widget );
    QStringList internApplicationsNames()
    {
        return _internApplicationsNames;
    }
    QStringList transApplicationsNames()
    {
        return _transApplicationsNames;
    }
    QString transAppName ( const QString& internAppName,
                           bool* found=0l );
    QString internAppName ( const QString& transAppName,
                            bool* found=0l );
    void setEmbedSessionActionsEnabled ( bool enable );
    bool startSshd (key_types key_type = RSA_KEY_TYPE);
    QSize getEmbedAreaSize();
    void setBrokerStatus(const QString& text, bool error=false);
    bool isPassFormHidden();
    bool isSelectFormHidden();
#ifdef Q_OS_WIN
    static QString cygwinPath ( const QString& winPath );
    void startXOrg(std::size_t start_offset = 0);
    static bool haveCygwinEntry();
    static void removeCygwinEntry();
    static QString U3DevicePath()
    {
        return u3Device;
    }
#endif


private:
    InteractionDialog* interDlg;
    QString m_x2goconfig;
    QStringList _internApplicationsNames;
    QStringList _transApplicationsNames;
    QString portableDataPath;
    QString proxyErrString;
    bool haveTerminal;
    bool proxyRunning;
    bool drawMenu;
    bool extStarted;
    bool startMaximized;
    bool closeDisconnect;
    bool startHidden;
    QString splashPix;
    QSplashScreen* splash;
    bool keepTrayIcon;
    bool hideFolderSharing;
    bool brokerNoauthWithSessionUsername;
    bool brokerCredsForSession;
    bool defaultUseSound;
    bool defaultXinerama;
    bool cardStarted;
    bool defaultSetKbd;
    bool autoresume;
    bool showExport;
    bool usePGPCard;
    bool miniMode;
    bool managedMode;
    bool brokerMode;
    bool changeBrokerPass;
    bool connTest;
    bool embedMode;
    bool thinMode;
    QString statusString;
    QStringList autostartApps;
    bool cmdAutologin;
    int defaultLink;
    int defaultQuality;
    int defaultWidth;
    int defaultHeight;
    bool defaultFullscreen;
    bool acceptRsa;
    bool startEmbedded;
    bool extLogin;
    bool printSupport;
    bool showTbTooltip;
    bool noSessionEdit;
    bool cleanAllFiles;
    bool PGPInited;
    bool resumeAfterSuspending;
    QString sshPort;
    QString clientSshPort;
    QString defaultSshPort;
    QString resourceDir;
    QString resourceURL;
    QDir* resourceTmpDir;
    QList<QNetworkReply*> resReply;
#ifdef Q_OS_WIN
    QString sshLog;
#endif
    QVBoxLayout* selectSesDlgLayout;
    SshMasterConnection* sshConnection;
    QList<SshMasterConnection*> serverSshConnections;
    bool closeEventSent;
    int shadowMode;
    QString shadowUser;
    QString shadowDisplay;
    QString defaultPack;
    QStringList defaultLayout;
    QString selectedLayout;
    QString defaultKbdType;
    QString defaultClipboardMode;
    QString defaultCmd;
    bool defaultSetDPI;
    uint defaultDPI;
    QStringList listedSessions;
    QString appDir;
    QString localGraphicPort;
    static QString homeDir;
    int retSessions;
    QList<serv> x2goServers;
    QList<Application> applications;
    QList<sshKey> cmdSshKeys;
    bool forceToShowTrayicon; //true if --tray-icon passed in command line

    QPushButton* bSusp;
    QPushButton* bTerm;
    QPushButton* bNew;
    QPushButton* bShadow;
    QPushButton* bShadowView;
    QPushButton* bCancel;


    QPushButton *bBrokerLogout;


    QLabel* selectSessionLabel;
    QLabel* running_label;
    SessTreeView* sessTv;

    QLineEdit* desktopFilter;
    QCheckBox* desktopFilterCb;

    SessionExplorer* sessionExplorer;

    IMGFrame* fr;
    SVGFrame *bgFrame;
    SVGFrame *on;
    QLineEdit* uname;
    ClickLineEdit* pass;
    ClickLineEdit* login;
    QFrame* uframe;
    SVGFrame *passForm;
    QSize mwSize;
    bool mwMax;
    QPoint mwPos;
    SVGFrame *selectSessionDlg;
    SVGFrame *sessionStatusDlg;
    QLabel* u;
    QLabel* fotoLabel;
    QLabel* nameLabel;
    QLabel* passPrompt;
    QLabel* loginPrompt;
    QLabel* layoutPrompt;
    QLabel* slName;
    QLabel* slVal;
    QComboBox* cbLayout;
    QPushButton* ok;
    QPushButton* cancel;
    QString readExportsFrom;
    QString readLoginsFrom;
    QPushButton* sOk;
    QToolButton* sbSusp;
    QToolButton* sbExp;
    QToolButton* sbTerm;
    QToolButton* sbApps;
    QCheckBox* sbAdv;
    QPushButton* sCancel;
    QString resolution;
    QString kdeIconsPath;
    QScrollArea* users;
    QVBoxLayout* userl;
    QHBoxLayout* mainL;
    QHBoxLayout* bgLay;
    QList<UserButton*> names;
    UserButton* lastUser;
    QString prevText;
    QString onserver;
    QString id;
    QString selectedCommand;
    QString currentKey;
    QTimer *exportTimer;
    QTimer *extTimer;
    QTimer *spoolTimer;
    QTimer *proxyWinTimer;
    QTimer *xineramaTimer;
    QTimer *brokerAliveTimer;
    QTimer *brokerSyncTimer;
    short xinSizeInc;
    QRect lastDisplayGeometry;
    QList <QRect> xineramaScreens;
    QStyle* widgetExtraStyle;
    bool isPassShown;
    bool xmodExecuted;
    long proxyWinId;
    bool embedControlChanged;
    bool embedTbVisible;
    QLabel* statusLabel;
    ConfigFile config;
    QStandardItemModel* model;
    QStandardItemModel* modelDesktop;

    QAction *act_set;
    QAction *act_abclient;
    QAction *act_support;
    QAction *act_shareFolder;
    QAction *act_showApps;
    QAction *act_suspend;
    QAction *act_terminate;
    QAction *act_reconnect;
    QAction *act_embedContol;
    QAction *act_embedToolBar;
    QAction *act_changeBrokerPass;
    QAction *act_testCon;
    QList <QAction*> topActions;

    QToolBar *stb;

    QString sessionStatus;
    QString spoolDir;
    QString sessionRes;
    QHBoxLayout* username;
    QList <user> userList;
    QList <directory> exportDir;
    QString nick;
    QString nfsPort;
    QString mntPort;
    static QString sessionCfg;
    QProcess* ssh;
    QProcess* soundServer;
    QProcess* scDaemon;
    QProcess* gpg;
    LDAPSession* ld;
    long embedParent;
    long embedChild;
    bool proxyWinEmbedded;
    bool useLdap;
    bool showToolBar;
    bool showHaltBtn;
    bool showBrokerLogoutBtn;
    bool newSession;
    bool runStartApp;
    bool ldapOnly;
#ifdef Q_OS_LINUX
    bool directRDP;
#endif

    bool startSessSound;
    int startSessSndSystem;

    bool fsInTun;
    bool fsTunReady;

    QString fsExportKey;
    bool fsExportKeyReady;

    QString ldapServer;
    int ldapPort;
    QString ldapServer1;
    int ldapPort1;
    QString ldapServer2;
    int ldapPort2;
    QString ldapDn;
    QString sessionCmd;

    QString supportMenuFile;
    QString BGFile;
    QString OnFile;
    QString SPixFile;

    QString LDAPSndSys;
    QString LDAPSndPort;
    bool LDAPSndStartServer;

    bool  LDAPPrintSupport;

    QAction *act_edit;
    QAction *act_new;
    QAction *act_sessicon;
    QProcess *nxproxy;
#ifndef Q_OS_WIN
    QProcess *sshd;
#else
    QProcess *xorg;
    PROCESS_INFORMATION sshd;
    bool winSshdStarted;
    static QString u3Device;

    int xDisplay;
    int sshdPort;
    bool winServersReady;
    QString oldEtcDir;
    QString oldBinDir;
    QString oldTmpDir;

    bool cyEntry;

    bool maximizeProxyWin;
    int proxyWinWidth;
    int proxyWinHeight;
    QTimer* xorgLogTimer;
    QString xorgLogFile;
    QMutex xorgLogMutex;
#endif
    QString lastFreeServer;
    QString cardLogin;
    QTextEdit* stInfo;
    int localDisplayNumber;

#if defined (Q_OS_DARWIN) || defined (Q_OS_WIN)
    QThread *pulseManagerThread;
    PulseManager *pulseManager;
    bool systemDisablePARecord;
    bool systemDisablePA;
#endif /* defined (Q_OS_DARWIN) || defined (Q_OS_WIN) */


    SVGFrame* ln;
    int tunnel;
    int sndTunnel;
    int fsTunnel;
    QList<x2goSession> selectedSessions;
    QStringList selectedDesktops;
    x2goSession resumingSession;
    bool startSound;
    bool restartResume;
    bool runRemoteCommand;
    bool shadowSession;
    int firstUid;
    int lastUid;
    bool cardReady;
    HttpBrokerClient* broker;
    client_events lastBrokerEvent;
    QString lastBrokerEventSession;


#if defined ( Q_OS_WIN) //&& defined (CFGCLIENT )
    void xorgSettings();
    bool startXorgOnStart;
    bool useInternalX;
    enum {VCXSRV, XMING} internalX;
    QString xorgExe;
    QString xorgOptions;
    QString xorgWinOptions;
    QString xorgFSOptions;
    QString xorgSAppOptions;
    QString xorgMDOptions;
    enum {WIN,FS,SAPP,MULTIDISPLAY} xorgMode;
    QString xorgWidth;
    QString xorgHeight;
    int waitingForX;
    std::size_t x_start_tries_;
    std::size_t x_start_limit_;
    QRect dispGeometry;
#endif

#ifdef Q_OS_LINUX
    long image, shape;
#endif

    // Tray icon stuff based on patch from Joachim Langenbach <joachim@falaba.de>
    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;
    QMenu *trayIconActiveConnectionMenu;

    QAction* appSeparator;
    QMenu* appMenu[Application::OTHER+1];

    bool trayEnabled;
    bool trayMinToTray;
    bool trayNoclose;
    bool trayMinCon;
    bool trayMaxDiscon;
    bool trayAutoHidden;
    bool resourcesLoaded;
    QNetworkAccessManager* resourceLoader;

    //server connection type
    // DEFAULT - start X2GO session
    // SUSPEND - open connection to suspend broker session
    // TERMINATE - open connection to terminate broker session
    typedef enum {DEFAULT, SUSPEND, TERMINATE} CONTYPE;

    CONTYPE connectionType;
    QString suspendTerminateHostFromBroker;
    QString suspendTerminateSessionFromBroker;

    void initUI();
    void initSplash();
    void destroySplash();
    void sendEventToBroker(client_events ev);
    void suspendFromBroker(const QString& sid);
    void terminateFromBroker(const QString& sid);
    QString findSshKeyForServer(QString user, QString server, QString port);
    void loadSettings();
    void showPass ( UserButton* user );
    void clean();
    bool defaultSession;
    QString defaultSessionName;
    QString defaultSessionId;
    QString defaultUserName;
    bool defaultUser;
    QString getKdeIconsPath();
    QString findTheme ( QString theme );
    bool initLdapSession ( bool showBox=true );
    bool startSession ( const QString& id, CONTYPE conType=DEFAULT);
    x2goSession getSessionFromString ( const QString& string );
    void resumeSession ( const x2goSession& s );
    void selectSession ( QStringList& sessions );
    x2goSession getSelectedSession();
    bool parseParameter ( QString param );
    bool linkParameter ( QString value );
    bool clipboardParameter ( QString value );
    bool geometry_par ( QString value );
    bool setKbd_par ( QString value );
    bool ldapParameter ( QString value );
    bool ldap1Parameter ( QString value );
    bool ldap2Parameter ( QString value );
    bool packParameter ( QString value );
    bool soundParameter ( QString val );
    void printError ( QString param );
    void exportDefaultDirs();
    directory* getExpDir ( QString key );
    bool findInList ( const QString& uid );
    void setUsersEnabled ( bool enable );
    void externalLogout ( const QString& logoutDir );
    void externalLogin ( const QString& loginDir );
    void startGPG();
    void GPGCardLogin ( const QString& cardLogin);
    void closeClient();
    void continueNormalSession();
    void continueLDAPSession();
    SshMasterConnection* startSshConnection ( QString host, QString port,
            bool acceptUnknownHosts, QString login,
            QString password, bool autologin, bool krbLogin, bool getSrv=false, bool useproxy=false,
            SshMasterConnection::ProxyType type=SshMasterConnection::PROXYSSH,
            QString proxyserver=QString::null, quint16 proxyport=0,
            QString proxylogin=QString::null, QString proxypassword=QString::null, QString proxyKey=QString::null,
            bool proxyAutologin=false, bool proxyKrbLogin=false );
    void setProxyWinTitle();
    QRect proxyWinGeometry();
    void readApplications();
    void removeAppsFromTray();
    void plugAppsInTray();
    QMenu* initTrayAppMenu(QString text, QPixmap icon);
    void setTrayIconToSessionIcon(QString info);
    /*
     * Tries to get the most suitable translator for the running system.
     *
     * The first parameter file_name_start denotes the start of a potential
     * translation file name. Locale values will be appended to this.
     *
     * On Qt 4.7 and lower, only tries to fetch a translator for the
     * main language as returned by QLocale::system().
     * On Qt 4.8 and higher, tries to fetch the first available translator
     * for the list returned by QLocale::uiLanguages().
     *
     * If no translator is available OR the best available translator
     * is for an English locale, returns false and doesn't touch
     * the passed translator object.
     * Otherwise returns true and sets the translator object to loaded
     * translation.
     */
    static bool get_translator (const QString file_name_start, QTranslator **translator);
    void getClientKeyboardConfig(QString& layout, QString& model, QString& variant);


protected:
    virtual void closeEvent ( QCloseEvent* event );
    virtual void hideEvent ( QHideEvent* event);

#ifndef Q_OS_WIN
    virtual void mouseReleaseEvent ( QMouseEvent * event );
#else
private slots:
    void slotSetWinServersReady();
    void startWinServers(key_types key_type = RSA_KEY_TYPE);
    void slotCheckXOrgLog();
    void slotCheckXOrgConnection();
#endif

#if defined (Q_OS_DARWIN) || defined (Q_OS_WIN)
private slots:
    void pulseManagerWrapper ();
#endif /* defined (Q_OS_DARWIN) || defined (Q_OS_WIN) */

private slots:
    void slotAppDialog();
    void slotShowPassForm();
    void displayUsers();
    void slotAppMenuTriggered ( QAction * action );
    void slotPassChanged(const QString& result);
    void slotResize ( const QSize sz );
    void slotUnameChanged ( const QString& text );
    void slotPassEnter();
    void slotChangeBrokerPass();
    void slotTestConnection();
    void slotCheckPortableDir();
    void readUsers();
    void slotSelectedFromList ( UserButton* user );
    void slotUnameEntered();
    void slotReadSessions();
    void slotManage();
    void displayToolBar ( bool );
    void showSessionStatus();
    void slotSshConnectionError ( QString message, QString lastSessionError );
    void slotSshServerAuthError ( int error, QString sshMessage, SshMasterConnection* connection );
    void slotSshServerAuthPassphrase ( SshMasterConnection* connection, SshMasterConnection::passphrase_types passphrase_type );
    void slotSshInteractionStart ( SshMasterConnection* connection, QString prompt );
    void slotSshInteractionUpdate ( SshMasterConnection* connection, QString output );
    void slotSshInteractionFinish ( SshMasterConnection* connection);
    void slotSshServerAuthChallengeResponse( SshMasterConnection* connection, QString Challenge );
    void slotCloseInteractionDialog();
    void slotSshUserAuthError ( QString error );
    void slotSshConnectionOk();
    void slotServSshConnectionOk(QString server);
    void slotChangeKbdLayout(const QString& layout);
    void slotSyncX();
    void slotShutdownThinClient();
    void slotBrokerLogoutButton ();
    void slotReadApplications(bool result, QString output, int pid );
    void slotResLoadRequestFinished( QNetworkReply*  reply );

public slots:
    void slotConfig();
    void slotNewSession();
    void slotEmbedControlAction();
    void slotDetachProxyWindow();
    void slotActivateWindow();
    void setFocus();
    void slotEnableBrokerLogoutButton ();
    void slotClosePass();
    void slotCloseSelectDlg();

private slots:
    void slotSendBrokerAlive();
    void slotShowPAMSGDialog(bool error, const QString& main_text, const QString& info_text, bool modal);
    void slotSnameChanged ( const QString& );
    void slotSelectedFromList ( SessionButton* session );
    void slotSessEnter();
    void slotActivated ( const QModelIndex& index );
    void slotResumeSess();
    void slotSuspendSess();
    void slotTermSessFromSt();
    void slotSuspendSessFromSt();
    void slotTermSess();
    void slotNewSess();
    void slotGetBrokerAuth();
    void slotGetBrokerSession();
    void slotCmdMessage ( bool result,QString output,
                          int );
    void slotListSessions ( bool result,QString output,
                            int );
    void slotRetSuspSess ( bool value,QString message,
                           int );
    void slotRetTermSess ( bool result,QString output,
                           int );
    void slotRetResumeSess ( bool result,QString output,
                             int );
    void slotTunnelFailed ( bool result,QString output,
                            int );
    void slotFsTunnelFailed ( bool result,QString output,
                              int );
    void slotSndTunnelFailed ( bool result,QString output,
                               int );
    void slotCopyKey ( bool result,QString output,int );
    void slotTunnelOk(int = 0);
    void slotFsTunnelOk(int );
    void slotProxyError ( QProcess::ProcessError err );
    void slotProxyFinished ( int result,QProcess::ExitStatus st );
    void slotProxyStderr();
    void slotProxyStdout();
    void slotResumeDoubleClick ( const QModelIndex& );
    void slotShowAdvancedStat();
    void slotRestartProxy();
    void slotTestSessionStatus();
    void SlotRunCommand(bool, QString output, int);
    void slotRetRunCommand ( bool result, QString output,
                             int );
    void slotGetServers ( bool result, QString output,
                          int );
    void slotListAllSessions ( bool result,QString output,
                               int );
    void slotRetExportDir ( bool result,QString output,
                            int );
    void slotResize();
    void slotExportDirectory();
    void slotExportTimer();
    void slotAboutQt();
    void slotAbout();
    void slotSupport();

    //trayIcon stuff
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void trayMessageClicked();
    void trayQuit();
    void trayIconInit();


private slots:
    void slotSetProxyWinFullscreen();
    void slotCheckPrintSpool();
    void slotRereadUsers();
    void slotExtTimer();
    void slotStartPGPAuth();
    void slotScDaemonStdOut();
    void slotScDaemonStdErr();
    void slotGpgFinished ( int exitCode,
                           QProcess::ExitStatus exitStatus );
    void slotScDaemonFinished ( int exitCode,
                                QProcess::ExitStatus exitStatus );
    void slotScDaemonError (QProcess::ProcessError error);
    void slotGpgError();
    void slotExecXmodmap();
    void slotCreateSessionIcon();
    void slotFindProxyWin();
    void slotConfigXinerama();
    void slotXineramaConfigured();
    void slotAttachProxyWindow();
    void slotEmbedIntoParentWindow();
    void slotEmbedWindow();
    void slotPCookieReady (	bool result,QString output,
                            int proc );
    void slotEmbedToolBar();
    void slotEmbedToolBarToolTip();
    void slotHideEmbedToolBarToolTip();
    void slotDesktopFilterChanged ( const QString& text ) ;
    void slotDesktopFilterCb ( int state ) ;
    void slotShadowViewSess();
    void slotShadowSess();
    void slotReconnectSession();
    void slotStartBroker();
    void slotStartNewBrokerSession ();
    void slotInitLibssh ();
#ifdef Q_OS_DARWIN
    void slotSetModMap();
    void handle_xmodmap_error (QProcess &proc);
private:
    QTimer* modMapTimer;
    QString kbMap;
#endif

private:
    void resizeProxyWinOnDisplay(int display);
#ifdef Q_OS_LINUX
    long X11FindWindow ( QString text, long rootWin=0 );
#endif
    void addToAppNames ( QString intName, QString transName );
    bool checkAgentProcess();
    bool isColorDepthOk ( int disp, int sess );
    void check_cmd_status();
    int startSshFsTunnel();
    void startX2goMount();
    void cleanPrintSpool();
    void cleanAskPass();
    void initWidgetsEmbed();
    void initWidgetsNormal();
    QString getCurrentUname();
    QString getCurrentPass();
    void processSessionConfig();
    void processCfgLine ( QString line );
    void initSelectSessDlg();
    void initStatusDlg();
    void initPassDlg();
    void printSshDError_startupFailure();
    void printSshDError_noHostPubKey();
    void printSshDError_noExportPubKey();
    void printSshDError_noAuthorizedKeysFile();
    void loadPulseModuleNativeProtocol();
    void initEmbedToolBar();
#ifdef Q_OS_LINUX
    void startDirectRDP();
#endif
    void filterDesktops ( const QString& filter,
                          bool strict=false );
    void generateEtcFiles();
    QString u3DataPath();
    void cleanPortable();
    void removeDir ( QString path );
#ifdef Q_OS_WIN
    void saveCygnusSettings();
    void restoreCygnusSettings();
#endif
#if defined  (Q_OS_WIN) || defined (Q_OS_DARWIN)
    QString getXDisplay();
#endif

    key_types check_key_type (key_types key_type);
    QString key_type_to_string (key_types key_type);
    std::size_t default_size_for_key_type (key_types key_type);
    QString generateKey (key_types key_type, bool host_key = false);
    QString createKeyBundle (key_types key_type = RSA_KEY_TYPE);
};

#ifdef Q_OS_WIN
#include <QThread>
#include <QMutex>
class ONMainWindow;
class WinServerStarter: public QThread
{
public:
    enum daemon {X,SSH};
    WinServerStarter ( daemon server, ONMainWindow * par );
    void run();
    void set_ssh_key_type (ONMainWindow::key_types key_type);
    ONMainWindow::key_types get_ssh_key_type ();
private:
    daemon mode;
    ONMainWindow* parent;
    ONMainWindow::key_types ssh_key_type_;
};
#endif

#endif
