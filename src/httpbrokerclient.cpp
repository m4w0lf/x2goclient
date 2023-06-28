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

#include "httpbrokerclient.h"
#include <QNetworkAccessManager>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkDiskCache>
#include <QNetworkReply>
#include <QUuid>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QSslSocket>
#include "x2gologdebug.h"
#include <QMessageBox>
#include <QDateTime>
#include "onmainwindow.h"
#include "x2gosettings.h"
#include <QDesktopWidget>
#include <QTimer>
#include "SVGFrame.h"
#include "onmainwindow.h"
#include <QTemporaryFile>
#include <QInputDialog>
#include "InteractionDialog.h"
#include <QStatusBar>
#include "version.h"


HttpBrokerClient::HttpBrokerClient ( ONMainWindow* wnd, ConfigFile* cfg )
{
    config=cfg;
    mainWindow=wnd;
    sshConnection=0;
    sessionsRequest=0l;
    selSessRequest=0l;
    chPassRequest=0l;
    testConRequest=0l;
    eventRequest=0l;
    QUrl lurl ( config->brokerurl );
    if(lurl.userName().length()>0)
        config->brokerUser=lurl.userName();
    nextAuthId=config->brokerUserId;

    if(config->brokerurl.indexOf("ssh://")==0)
    {
        sshBroker=true;
        x2goDebug<<"host:"<<lurl.host();
        x2goDebug<<"port:"<<lurl.port(22);
        x2goDebug<<"uname:"<<lurl.userName();
        x2goDebug<<"path:"<<lurl.path();
        config->sshBrokerBin=lurl.path();
    }
    else
    {
        sshBroker=false;

        if ((config->brokerCaCertFile.length() >0) && (QFile::exists(config->brokerCaCertFile))) {
            QSslSocket::addDefaultCaCertificates(config->brokerCaCertFile, QSsl::Pem);
            x2goDebug<<"Custom CA certificate file loaded into HTTPS broker client: "<<config->brokerCaCertFile;
        }

        QDir dr;
        dr.mkpath(mainWindow->getHomeDirectory()+"/.x2go");
        http=new QNetworkAccessManager ( this );
        QNetworkDiskCache *cache = new QNetworkDiskCache(this);
        cache->setCacheDirectory(mainWindow->getHomeDirectory()+"/.x2go/cache");
        http->setCache(cache);
        x2goDebug<<"Setting up connection to broker: "<<config->brokerurl;

        connect ( http, SIGNAL ( sslErrors ( QNetworkReply*, const QList<QSslError>& ) ),this,
                  SLOT ( slotSslErrors ( QNetworkReply*, const QList<QSslError>& ) ) );

        connect ( http,SIGNAL ( finished (QNetworkReply*) ),this,
                  SLOT ( slotRequestFinished (QNetworkReply*) ) );
    }
}


HttpBrokerClient::~HttpBrokerClient()
{
}

void HttpBrokerClient::createSshConnection()
{
    QUrl lurl ( config->brokerurl );
    sshConnection=new SshMasterConnection (this, lurl.host(), lurl.port(22),mainWindow->getAcceptRSA(),
                                           config->brokerUser, config->brokerPass,config->brokerSshKey,config->brokerAutologin,
                                           config->brokerKrbLogin, false);

    qRegisterMetaType<SshMasterConnection::passphrase_types> ("SshMasterConnection::passphrase_types");

    connect ( sshConnection, SIGNAL ( connectionOk(QString)), this, SLOT ( slotSshConnectionOk() ) );
    connect ( sshConnection, SIGNAL ( serverAuthError ( int,QString, SshMasterConnection* ) ),this,
              SLOT ( slotSshServerAuthError ( int,QString, SshMasterConnection* ) ) );
    connect ( sshConnection, SIGNAL ( needPassPhrase(SshMasterConnection*, SshMasterConnection::passphrase_types)),this,
              SLOT ( slotSshServerAuthPassphrase(SshMasterConnection*, SshMasterConnection::passphrase_types)) );
    connect ( sshConnection, SIGNAL ( userAuthError ( QString ) ),this,SLOT ( slotSshUserAuthError ( QString ) ) );
    connect ( sshConnection, SIGNAL ( connectionError(QString,QString)), this,
              SLOT ( slotSshConnectionError ( QString,QString ) ) );
    connect ( sshConnection, SIGNAL(ioErr(SshProcess*,QString,QString)), this,
              SLOT(slotSshIoErr(SshProcess*,QString,QString)));


    connect ( sshConnection, SIGNAL(startInteraction(SshMasterConnection*,QString)),mainWindow,
              SLOT(slotSshInteractionStart(SshMasterConnection*,QString)) );
    connect ( sshConnection, SIGNAL(updateInteraction(SshMasterConnection*,QString)),mainWindow,
              SLOT(slotSshInteractionUpdate(SshMasterConnection*,QString)) );
    connect ( sshConnection, SIGNAL(finishInteraction(SshMasterConnection*)),mainWindow,
              SLOT(slotSshInteractionFinish(SshMasterConnection*)));
    connect ( mainWindow->getInteractionDialog(), SIGNAL(textEntered(QString)), sshConnection,
              SLOT(interactionTextEnter(QString)));
    connect ( mainWindow->getInteractionDialog(), SIGNAL(interrupt()), sshConnection, SLOT(interactionInterruptSlot()));

    sshConnection->start();
}

void HttpBrokerClient::slotSshConnectionError(QString message, QString lastSessionError)
{
    if ( sshConnection )
    {
        sshConnection->wait();
        delete sshConnection;
        sshConnection=0l;
    }

    QMessageBox::critical ( 0l,message,lastSessionError,
                            QMessageBox::Ok,
                            QMessageBox::NoButton );
}

void HttpBrokerClient::slotSshConnectionOk()
{
    mainWindow->getInteractionDialog()->hide();
    getUserSessions();
}

void HttpBrokerClient::slotSshServerAuthError(int error, QString sshMessage, SshMasterConnection* connection)
{
    QString errMsg;
    switch ( error )
    {
    case SSH_SERVER_KNOWN_CHANGED:
        errMsg=tr ( "Host key for server changed.\nIt is now: " ) +sshMessage+"\n"+
               tr ( "For security reasons, the connection attempt will be aborted." );
        connection->writeKnownHosts(false);
        connection->wait();
        if(sshConnection && sshConnection !=connection)
        {
            sshConnection->wait();
            delete sshConnection;
        }
        sshConnection=0;
        slotSshUserAuthError ( errMsg );
        return;

    case SSH_SERVER_FOUND_OTHER:
        errMsg=tr ( "The host key for this server was not found but another "
                    "type of key exists. An attacker might have changed the default server key to "
                    "trick your client into thinking the key does not exist yet." );
        connection->writeKnownHosts(false);
        connection->wait();
        if(sshConnection && sshConnection !=connection)
        {
            sshConnection->wait();
            delete sshConnection;
        }
        sshConnection=0;
        slotSshUserAuthError ( errMsg );
        return ;

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
        return ;
    case SSH_SERVER_FILE_NOT_FOUND:
        errMsg=tr ( "Could not find known hosts file. "
                    "If you accept the host key here, the file will be automatically created." );
        break;

    case SSH_SERVER_NOT_KNOWN:
        errMsg=tr ( "The server is unknown. Do you trust the host key?\nPublic key hash: " ) +sshMessage;
        break;
    }

    if ( QMessageBox::warning ( 0, tr ( "Host key verification failed." ),errMsg,tr ( "Yes" ), tr ( "No" ) ) !=0 )
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
        return;
    }
    connection->writeKnownHosts(true);
    connection->wait();
    connection->start();

}

void HttpBrokerClient::slotSshServerAuthPassphrase(SshMasterConnection* connection, SshMasterConnection::passphrase_types passphrase_type)
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
                                                        x2goDebug << "Unknown passphrase type requested! Was: " << passphrase_type << endl;
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
    }
}

void HttpBrokerClient::closeSSHInteractionDialog()
{
    slotSshUserAuthError("NO_ERROR");
}


void HttpBrokerClient::slotSshUserAuthError(QString error)
{
    if ( sshConnection )
    {
        sshConnection->wait();
        delete sshConnection;
        sshConnection=0l;
    }
    mainWindow->getInteractionDialog()->hide();

    if(error!="NO_ERROR")

        QMessageBox::critical ( 0l,tr ( "Authentication failed." ),error,
                                QMessageBox::Ok,
                                QMessageBox::NoButton );
    emit authFailed();
    return;
}


void HttpBrokerClient::getUserSessions()
{
    QString brokerUser=config->brokerUser;
    // Otherwise, after logout from the session, we will be connected by a previous user without a password by authid.
    if (config->brokerAutologoff) {
        nextAuthId=config->brokerUserId;
    }
    x2goDebug<<"Called getUserSessions: brokeruser: "<<brokerUser<<" authid: "<<nextAuthId;
    if(mainWindow->getUsePGPCard())
        brokerUser=mainWindow->getCardLogin();
    config->sessiondata=QString::null;
    if(!sshBroker)
    {
        QString req;
        QTextStream ( &req ) <<
                             "task=listsessions&"<<
                             "user="<<QUrl::toPercentEncoding(brokerUser)<<"&"<<
                             "password="<<QUrl::toPercentEncoding(config->brokerPass)<<"&"<<
                             "authid="<<nextAuthId;

        x2goDebug << "sending request: "<< scramblePwd(req.toUtf8());
        QNetworkRequest request(QUrl(config->brokerurl));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        sessionsRequest=http->post (request, req.toUtf8() );
    }
    else
    {
        if(!sshConnection)
        {
            createSshConnection();
            return;
        }
        if (nextAuthId.length() > 0) {
            sshConnection->executeCommand ( config->sshBrokerBin+" --user "+ brokerUser +" --authid "+nextAuthId+ " --task listsessions",
                                            this, SLOT ( slotListSessions ( bool, QString,int ) ));
        } else {
            sshConnection->executeCommand ( config->sshBrokerBin+" --user "+ brokerUser +" --task listsessions",
                                            this, SLOT ( slotListSessions ( bool, QString,int ) ));
        }
    }
}

void HttpBrokerClient::selectUserSession(const QString& session, const QString& loginName)
{
    x2goDebug<<"Called selectUserSession for session "<<session<<", "<<"loginName "<<loginName;
    QString brokerUser=config->brokerUser;
    if(mainWindow->getUsePGPCard())
        brokerUser=mainWindow->getCardLogin();

    if(!sshBroker)
    {
        QString req;
        QTextStream ( &req ) <<
                             "task=selectsession&"<<
                             "sid="<<session<<"&"<<
                             "user="<<QUrl::toPercentEncoding(brokerUser)<<"&"<<
                             "password="<<QUrl::toPercentEncoding(config->brokerPass)<<"&"<<
                             "authid="<<nextAuthId;
        if(loginName.length()>0)
        {
            QTextStream ( &req ) <<"&login="<<QUrl::toPercentEncoding(loginName);
        }
        x2goDebug << "sending request: "<< scramblePwd(req.toUtf8());
        QNetworkRequest request(QUrl(config->brokerurl));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        selSessRequest=http->post (request, req.toUtf8() );

    }
    else
    {
        QString sshCmd=config->sshBrokerBin+" --user "+ brokerUser + " --task selectsession --sid \""+session+"\"";
        if(nextAuthId.length() > 0)
        {
            sshCmd+=" --authid "+nextAuthId;
        }
        if(loginName.length() > 0)
        {
            sshCmd+=" --login " + loginName;
        }
        sshConnection->executeCommand (sshCmd, this,SLOT ( slotSelectSession(bool,QString,int)));
    }

}

void HttpBrokerClient::sendEvent(const QString& ev, const QString& id, const QString& server, const QString& client,
                                 const QString& login, const QString& cmd,
                                 const QString& display, const QString& start, uint connectionTime)
{
    x2goDebug<<"Called sendEvent.";
    QString brokerUser=config->brokerUser;
    if(mainWindow->getUsePGPCard())
        brokerUser=mainWindow->getCardLogin();

    QString os="linux";
#ifdef Q_OS_WIN
    os="windows";
#endif
#ifdef Q_OS_DARWIN
    os="mac";
#endif
    if(!sshBroker)
    {
        QString req;
        QTextStream ( &req ) <<
                             "task=clientevent&"<<
                             "user="<<QUrl::toPercentEncoding(brokerUser)<<"&"<<
                             "password="<<QUrl::toPercentEncoding(config->brokerPass)<<"&"<<
                             "sid="<<id<<"&"<<
                             "event="<<ev<<"&"<<
                             "server="<<QUrl::toPercentEncoding(server)<<"&"<<
                             "client="<<QUrl::toPercentEncoding(client)<<"&"<<
                             "login="<<QUrl::toPercentEncoding(login)<<"&"<<
                             "cmd="<<QUrl::toPercentEncoding(cmd)<<"&"<<
                             "display="<<QUrl::toPercentEncoding(display)<<"&"<<
                             "start="<<QUrl::toPercentEncoding(start)<<"&"<<
                             "elapsed="<<QString::number(connectionTime)<<"&"<<
                             "version="<<QUrl::toPercentEncoding(VERSION)<<"&"<<
                             "os="<<os<<"&"<<
                             "authid="<<nextAuthId;
        x2goDebug << "sending request: "<< scramblePwd(req.toUtf8());
        QNetworkRequest request(QUrl(config->brokerurl));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        eventRequest=http->post (request, req.toUtf8() );

    }
    else
    {
        if (nextAuthId.length() > 0) {
            sshConnection->executeCommand ( config->sshBrokerBin+" --user "+ brokerUser +" --authid "+nextAuthId+
            " --task clientevent --sid \""+id+"\" --event "+ev+" --server \""+server+"\" --client \""+client+"\" --login "+"\""+
            login+"\" --cmd \""+cmd+"\" --display \""+display+"\" --start \""+start+"\" --elapsed "+QString::number(connectionTime)+" --version \""+VERSION+"\" --os "+os,
            this,SLOT ( slotEventSent(bool,QString,int)));
        } else {
            sshConnection->executeCommand ( config->sshBrokerBin+" --user "+ brokerUser +
            " --task clientevent --sid \""+id+"\" --event "+ev+" --server \""+server+"\" --client \""+client+"\" --login "+"\""+
            login+"\" --cmd \""+cmd+"\" --display \""+display+"\" --start \""+start+"\" --elapsed "+QString::number(connectionTime)+" --version \""+VERSION+"\" --os "+os,
            this,SLOT ( slotEventSent(bool,QString,int)));
        }
    }
}


void HttpBrokerClient::slotEventSent(bool success, QString answer, int)
{
    if(!success)
    {
        x2goDebug<<answer;
        mainWindow->setBrokerStatus(tr("Disconnected from broker: ")+answer, true);
        QMessageBox::critical(0,tr("Error"),answer);
        emit fatalHttpError();
        return;
    }
    if(!checkAccess(answer))
        return;
    mainWindow->setBrokerStatus(tr("Connected to broker"));
    x2goDebug<<"event sent:"<<answer;
    if(answer.indexOf("SUSPEND")!=-1)
    {
        QString sid=answer.split("SUSPEND ")[1].simplified();
        x2goDebug<<"broker asks to suspend "<<sid;
        mainWindow->suspendFromBroker(sid);
    }
    if(answer.indexOf("TERMINATE")!=-1)
    {
        QString sid=answer.split("TERMINATE ")[1].simplified();
        x2goDebug<<"broker asks to terminate "<<sid;
        mainWindow->terminateFromBroker(sid);
    }
}


void HttpBrokerClient::changePassword(QString newPass)
{
    newBrokerPass=newPass;
    QString brokerUser=config->brokerUser;
    if(mainWindow->getUsePGPCard())
        brokerUser=mainWindow->getCardLogin();

    if(!sshBroker)
    {
        QString req;
        QTextStream ( &req ) <<
                             "task=setpass&"<<
                             "newpass="<<QUrl::toPercentEncoding(newPass)<<"&"<<
                             "user="<<QUrl::toPercentEncoding(brokerUser)<<"&"<<
                             "password="<<QUrl::toPercentEncoding(config->brokerPass)<<"&"<<
                             "authid="<<nextAuthId;
        x2goDebug << "sending request: "<< scramblePwd(req.toUtf8());
        QNetworkRequest request(QUrl(config->brokerurl));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        chPassRequest=http->post (request, req.toUtf8() );
    }
    else
    {
        if (nextAuthId.length() > 0) {
            sshConnection->executeCommand ( config->sshBrokerBin+" --user "+ brokerUser +" --authid "+nextAuthId+ " --task setpass --newpass "+newPass, this,
                                            SLOT ( slotPassChanged(bool,QString,int)));
        } else {
            sshConnection->executeCommand ( config->sshBrokerBin+" --user "+ brokerUser +" --task setpass --newpass "+newPass, this,
                                            SLOT ( slotPassChanged(bool,QString,int)));
        }
    }
}

void HttpBrokerClient::testConnection()
{
    x2goDebug<<"Called testConnection.";
    if(!sshBroker)
    {
        QString req;
        QTextStream ( &req ) <<
                             "task=testcon";
        x2goDebug << "sending request: "<< scramblePwd(req.toUtf8());
        QNetworkRequest request(QUrl(config->brokerurl));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        testConRequest=http->post (request, req.toUtf8() );
    }
    else
    {
        if (nextAuthId.length() > 0) {
            sshConnection->executeCommand(config->sshBrokerBin+" --authid "+nextAuthId+ " --task testcon",
                                          this, SLOT ( slotSelectSession(bool,QString,int)));
        } else {
            sshConnection->executeCommand(config->sshBrokerBin+" --task testcon",
                                          this, SLOT ( slotSelectSession(bool,QString,int)));
        }
    }
}

void HttpBrokerClient::processClientConfig(const QString& raw_content)
{
    X2goSettings st(raw_content, QSettings::IniFormat);
    mainWindow->config.brokerEvents=st.setting()->value("events",false).toBool();
    mainWindow->config.brokerLiveEventsTimeout=st.setting()->value("liveevent",0).toUInt();
    mainWindow->config.brokerSyncTimeout=st.setting()->value("syncinterval",0).toUInt();
    if(mainWindow->config.brokerEvents)
    {
        x2goDebug<<"sending client events to broker";
        if(mainWindow->config.brokerLiveEventsTimeout)
        {
            x2goDebug<<"sending alive events to broker every "<<mainWindow->config.brokerLiveEventsTimeout<<" seconds";
        }
    }
    if(mainWindow->config.brokerSyncTimeout)
    {
        x2goDebug<<"synchronizing with broker every "<<mainWindow->config.brokerSyncTimeout<<" seconds";
    }
}


void HttpBrokerClient::createIniFile(const QString& raw_content)
{
    QString content;
    content = raw_content;
    content.replace("<br>","\n");
//     x2goDebug<<"Inifile content: "<<content<<endl;
    QString cont;
    QStringList lines=content.split("START_USER_SESSIONS\n");
    if (lines.count()>1)
    {
        cont=lines[1];
        cont=cont.split("END_USER_SESSIONS\n")[0];
    }

    if(!sshBroker)
    {
        QStringList iniLines=cont.split("\n");
        cont.clear();
        clearResRequests();
        //check if some of the icon or icon_foldername values are URLs and need to be downloaded
        foreach(QString ln, iniLines)
        {
            ln=ln.trimmed();
            if(ln.indexOf("icon")==0)
            {
                QStringList lnParts=ln.split("=");
                QString iconUrl=lnParts.last().trimmed();
                if((iconUrl.indexOf("http://")!=-1)||(iconUrl.indexOf("https://")!=-1))
                {
                    ln=lnParts[0]+"=file://"+getResource(iconUrl);
                }
            }
            cont+=ln+"\n";
        }
        mainWindow->config.iniFile=cont;
        if(resReplies.count()==0)
        {
            //we didn't request any resources, all session info is loaded
            emit sessionsLoaded();
        }
    }
    else
    {
        mainWindow->config.iniFile=cont;
        emit sessionsLoaded();
    }
    lines=content.split("START_CLIENT_CONFIG\n");
    if (lines.count()>1)
    {
        cont=lines[1];
        cont=cont.split("END_CLIENT_CONFIG\n")[0];
        processClientConfig(cont);
    }
    else
    {
        x2goDebug<<"no client config from broker";
    }
}


//get the fname on disk from resource URL
QString HttpBrokerClient::resourceFname(const QUrl& url)
{
    QString fname=url.toString();
    fname.replace("http://","");
    fname.replace("https://","");
    fname.replace("/","_");
    QDir dr;
    dr.mkdir(mainWindow->getHomeDirectory()+"/.x2go/res");
    return mainWindow->getHomeDirectory()+"/.x2go/res/"+fname;
}

//return true if the URL is already requested
bool HttpBrokerClient::isResRequested(const QUrl& url)
{
    foreach(QNetworkReply* reply, resReplies)
    {
        if(reply->url().toString()==url.toString())
        {
            return true;
        }
    }
    return false;
}

//download resource if needed and return the name on the disk
QString HttpBrokerClient::getResource(QString resURL)
{
    QUrl url(resURL);
    if(!isResRequested(url))
    {
        resReplies << http->get(QNetworkRequest(url));
    }
    return resourceFname(url);
}

//clear list of res requests
void HttpBrokerClient::clearResRequests()
{
    foreach(QNetworkReply* reply, resReplies)
    {
        reply->deleteLater();
    }
    resReplies.clear();
}

bool HttpBrokerClient::checkAccess(QString answer )
{
//     x2goDebug<<"Called checkAccess - answer was: "<<answer;
    if (answer.indexOf("Access granted")==-1)
    {
        QMessageBox::critical (
            0,tr ( "Error" ),
            tr ( "Login failed!<br>"
                 "Please try again." ) );
        emit authFailed();
        return false;
    }
    config->brokerAuthenticated=true;
    emit (enableBrokerLogoutButton ());
    int authBegin=answer.indexOf("AUTHID:");
    if (authBegin!=-1)
    {
        nextAuthId=answer.mid(authBegin+7, answer.indexOf("\n",authBegin)-authBegin-7);
    }
    return true;
}


void HttpBrokerClient::slotConnectionTest(bool success, QString answer, int)
{
    x2goDebug<<"Called slotConnectionTest.";
    if(!success)
    {
        x2goDebug<<answer;
        QMessageBox::critical(0,tr("Error"),answer);
        emit fatalHttpError();
        return;
    }
    if(!checkAccess(answer))
        return;
    if(!sshBroker)
    {
        x2goDebug<<"Elapsed: "<<requestTime.elapsed()<<"; received:"<<answer.size()<<endl;
        emit connectionTime(requestTime.elapsed(),answer.size());
    }
    return;

}

void HttpBrokerClient::slotListSessions(bool success, QString answer, int)
{
    if(!success)
    {
        x2goDebug<<answer;
        QMessageBox::critical(0,tr("Error"),answer);
        emit fatalHttpError();
        return;
    }

    if(!checkAccess(answer))
        return;

    mainWindow->setBrokerStatus(tr("Connected to broker"));

    createIniFile(answer);
}

void HttpBrokerClient::slotPassChanged(bool success, QString answer, int)
{
    if(!success)
    {
        x2goDebug<<answer;
        QMessageBox::critical(0,tr("Error"),answer);
        emit fatalHttpError();
        return;
    }
    if(!checkAccess(answer))
        return;

}

void HttpBrokerClient::slotSelectSession(bool success, QString answer, int)
{
    if(!success)
    {
        x2goDebug<<answer;
        QMessageBox::critical(0,tr("Error"),answer);
        emit fatalHttpError();
        return;
    }
    if(!checkAccess(answer))
        return;
    x2goDebug<<"parsing "<<answer;
    parseSession(answer);
}


void HttpBrokerClient::slotRequestFinished ( QNetworkReply*  reply )
{
    if(resReplies.indexOf(reply) != -1)
    {
        if(reply->error() != QNetworkReply::NoError)
        {
            x2goDebug<<"Download request for "<<reply->url().toString()<< " failed with error: "<<reply->errorString();
        }
        else
        {
//             x2goDebug<<"saving to "<<resourceFname(reply->url().toString())<<" from cache: "<<reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool();
            QFile file(resourceFname(reply->url().toString()));
            file.open(QIODevice::WriteOnly);
            file.write(reply->readAll());
            file.close();
        }
        foreach(QNetworkReply* r, resReplies)
        {
            if(!r->isFinished())
            {
                return;
            }
        }
        emit sessionsLoaded();
        return;
    }
    if(reply->error() != QNetworkReply::NoError)
    {
        x2goDebug<<"Broker HTTP request failed with error: "<<reply->errorString();
        mainWindow->setBrokerStatus(tr("Disconnected from broker: ")+reply->errorString(), true);
        if(reply == eventRequest || reply == sessionsRequest)
        {
            reply->deleteLater();
            if(reply == eventRequest)
                eventRequest=0l;
            else
                sessionsRequest=0l;
            //do not exit, just return the function
            return;
        }
        QMessageBox::critical(0,tr("Error"),reply->errorString());
        emit fatalHttpError();
        return;
    }

    QString answer ( reply->readAll() );
//     x2goDebug<<"A http request returned. Result was: "<<answer;
    if (reply == testConRequest)
    {
        testConRequest=0l;
        slotConnectionTest(true,answer,0);
    }
    else if (reply == sessionsRequest)
    {
        sessionsRequest=0l;
        slotListSessions(true, answer,0);
    }
    else if (reply == selSessRequest)
    {
        selSessRequest=0l;
        slotSelectSession(true,answer,0);
    }
    else if (reply == chPassRequest)
    {
        chPassRequest=0l;
        slotPassChanged(true,answer,0);
    }
    else if (reply == eventRequest)
    {
        eventRequest=0l;
        slotEventSent(true,answer,0);
    }


    // We receive ownership of the reply object
    // and therefore need to handle deletion.
    reply->deleteLater();
}

void HttpBrokerClient::parseSession(QString sinfo)
{
    config->sessiondata="";
    suspendedSession.clear();
    x2goDebug<<"Starting parser.";
    QStringList lst=sinfo.split("SERVER:",QString::SkipEmptyParts);
    int keyStartPos=sinfo.indexOf("-----BEGIN DSA PRIVATE KEY-----");
    if(keyStartPos==-1)
        keyStartPos=sinfo.indexOf("-----BEGIN RSA PRIVATE KEY-----");
    if(keyStartPos==-1)
        keyStartPos=sinfo.indexOf("-----BEGIN OPENSSH PRIVATE KEY-----");
    QString endStr="-----END DSA PRIVATE KEY-----";
    int keyEndPos=sinfo.indexOf(endStr);
    if(keyEndPos==-1)
    {
        endStr="-----END RSA PRIVATE KEY-----";
        keyEndPos=sinfo.indexOf(endStr);
    }
    if(keyEndPos==-1)
    {
        endStr="-----END OPENSSH PRIVATE KEY-----";
        keyEndPos=sinfo.indexOf(endStr);
    }
    if (! (keyEndPos == -1 || keyStartPos == -1 || lst.size()==0))
    {
        x2goDebug<<"Found key for authentication";
        config->key=sinfo.mid(keyStartPos, keyEndPos+endStr.length()-keyStartPos);
    }
    QString serverLine=(lst[1].split("\n"))[0];
    QStringList words=serverLine.split(":",QString::SkipEmptyParts);
    config->serverIp=words[0];
    if (words.count()>1)
        config->sshport=words[1];
    x2goDebug<<"Server IP address: "<<config->serverIp;
    x2goDebug<<"Server port: "<<config->sshport;
    if (sinfo.indexOf("SESSION_INFO")!=-1)
    {
        QStringList lst=sinfo.split("SESSION_INFO:",QString::SkipEmptyParts);
        //config->sessiondata=lst[1];
        x2goDebug<<"Session data: "<<lst[1]<<"\n";
        suspendedSession=lst[1].trimmed().split ( '\n', QString::SkipEmptyParts );
        mainWindow->selectSession(suspendedSession);
    }
    else
    {
        emit sessionSelected();
    }
    x2goDebug<<"Parsing has finished.";
}

void HttpBrokerClient::resumeSession(const QString& id, const QString& server)
{
    x2goDebug<<"Resuming session with id:"<<id<<"on:"<<server;
    foreach (QString sline, suspendedSession)
    {
        if(sline.indexOf(id)!=-1)
        {
            config->sessiondata=sline;
            config->serverIp=server;
            emit sessionSelected();
            break;
        }
    }
}


void HttpBrokerClient::slotSslErrors ( QNetworkReply* netReply, const QList<QSslError> & errors )
{
    QStringList err;
    QSslCertificate cert;
    for ( int i=0; i<errors.count(); ++i )
    {
        x2goDebug<<"sslError, code:"<<errors[i].error() <<":";
        err<<errors[i].errorString();
        if ( !errors[i].certificate().isNull() )
            cert=errors[i].certificate();
    }


    QString md5=getHexVal ( cert.digest() );
    QString fname=md5;
    fname=fname.replace(":","_");
    QUrl lurl ( config->brokerurl );
    QString homeDir=mainWindow->getHomeDirectory();
    if ( QFile::exists ( homeDir+"/.x2go/ssl/exceptions/"+
                         lurl.host() +"/"+fname ) )
    {
        QFile fl ( homeDir+"/.x2go/ssl/exceptions/"+
                   lurl.host() +"/"+fname );
        fl.open ( QIODevice::ReadOnly | QIODevice::Text );
        QSslCertificate mcert ( &fl );
        if ( mcert==cert )
        {
            netReply->ignoreSslErrors();
            requestTime.restart();
            return;
        }
    }

    QString text=tr ( "<br><b>Server uses an invalid "
                      "security certificate.</b><br><br>" );
    text+=err.join ( "<br>" );
    text+=tr ( "<p style='background:#FFFFDC;'>"
               "You should not add an exception "
               "if you are using an internet connection "
               "that you do not trust completely or if you are "
               "not used to seeing a warning for this server.</p>" );
    QMessageBox mb ( QMessageBox::Warning,tr ( "Secure connection failed." ),
                     text );
    text=QString::null;
    QTextStream ( &text ) <<err.join ( "\n" ) <<"\n"<<
                          "------------\n"<<
                          tr ( "Issued to:\n" ) <<
                          tr ( "Common Name(CN)\t" ) <<
#if QT_VERSION >= 0x050000
                          cert.issuerInfo ( QSslCertificate::CommonName ).join("; ")
#else
                          cert.issuerInfo ( QSslCertificate::CommonName )
#endif
                          <<endl<<
                          tr ( "Organization(O)\t" ) <<
#if QT_VERSION >= 0x050000
                          cert.issuerInfo ( QSslCertificate::Organization ).join("; ")
#else
                          cert.issuerInfo ( QSslCertificate::Organization )
#endif
                          <<endl<<
                          tr ( "Organizational Unit(OU)\t" ) <<
#if QT_VERSION >= 0x050000
                          cert.issuerInfo ( QSslCertificate::OrganizationalUnitName ).join("; ")
#else
                          cert.issuerInfo ( QSslCertificate::OrganizationalUnitName )
#endif
                          <<endl<<
                          tr ( "Serial Number\t" ) <<getHexVal ( cert.serialNumber() )
                          <<endl<<endl<<
                          tr ( "Issued by:\n" ) <<
                          tr ( "Common Name(CN)\t" ) <<
#if QT_VERSION >= 0x050000
                          cert.subjectInfo ( QSslCertificate::CommonName ).join("; ")
#else
                          cert.subjectInfo ( QSslCertificate::CommonName )
#endif
                          <<endl<<
                          tr ( "Organization(O)\t" ) <<
#if QT_VERSION >= 0x050000
                          cert.subjectInfo ( QSslCertificate::Organization ).join("; ")
#else
                          cert.subjectInfo ( QSslCertificate::Organization )
#endif
                          <<endl<<
                          tr ( "Organizational Unit(OU)\t" ) <<
#if QT_VERSION >= 0x050000
                          cert.subjectInfo ( QSslCertificate::OrganizationalUnitName ).join("; ")
#else
                          cert.subjectInfo ( QSslCertificate::OrganizationalUnitName )
#endif
                          <<endl<<endl<<

                          tr ( "Validity:\n" ) <<
                          tr ( "Issued on\t" ) <<cert.effectiveDate().toString() <<endl<<
                          tr ( "expires on\t" ) <<cert.expiryDate().toString() <<endl<<endl<<
                          tr ( "Fingerprints:\n" ) <<
                          tr ( "SHA1\t" ) <<
                          getHexVal ( cert.digest ( QCryptographicHash::Sha1 ) ) <<endl<<
                          tr ( "MD5\t" ) <<md5;



    mb.setDetailedText ( text );
    mb.setEscapeButton (
        ( QAbstractButton* ) mb.addButton ( tr ( "Exit X2Go Client" ),
                                            QMessageBox::RejectRole ) );
    QPushButton *okButton=mb.addButton ( tr ( "Add exception" ),
                                         QMessageBox::AcceptRole );
    mb.setDefaultButton ( okButton );

    mb.exec();
    if ( mb.clickedButton() == ( QAbstractButton* ) okButton )
    {
        x2goDebug<<"User accepted certificate.";
        QDir dr;
        dr.mkpath ( homeDir+"/.x2go/ssl/exceptions/"+lurl.host() +"/" );
        QFile fl ( homeDir+"/.x2go/ssl/exceptions/"+
                   lurl.host() +"/"+fname );
        fl.open ( QIODevice::WriteOnly | QIODevice::Text );
        QTextStream ( &fl ) <<cert.toPem();
        fl.close();
        netReply->ignoreSslErrors();
        x2goDebug<<"Storing certificate in "<<homeDir+"/.x2go/ssl/exceptions/"+
                 lurl.host() +"/"+fname;
        requestTime.restart();
    }
    else
        emit fatalHttpError();
}


QString HttpBrokerClient::getHexVal ( const QByteArray& ba )
{
    QStringList val;
    for ( int i=0; i<ba.size(); ++i )
    {
        QString bt;
        bt.sprintf ( "%02X", ( unsigned char ) ba[i] );
        val<<bt;
    }
    return val.join ( ":" );
}

void HttpBrokerClient::slotSshIoErr(SshProcess* caller, QString error, QString lastSessionError)
{
    x2goDebug<<"Brocker SSH Connection IO Error, reconnect session\n";
    if ( sshConnection )
    {
        delete sshConnection;
        sshConnection=0l;
    }
    createSshConnection();
}

QString HttpBrokerClient::scramblePwd(const QString& req)
{
    QString scrambled=req;
    int startPos=scrambled.indexOf("password=");
    if(startPos!=-1)
    {
        startPos+=9;
        int endPos=scrambled.indexOf("&",startPos);
        int plength;
        if(endPos==-1)
        {
            plength=scrambled.length()-startPos;
        }
        else
        {
            plength=endPos-startPos;
        }
        scrambled.remove(startPos, plength);
        // Hardcode a value of 8 here - the length of the string "password".
        scrambled.insert(startPos, QString ('*').repeated (8));
    }
    return scrambled;
}
