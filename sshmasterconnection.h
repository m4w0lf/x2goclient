
/***************************************************************************
 *   Copyright (C) 2005-2012 by Oleksandr Shneyder   *
 *   oleksandr.shneyder@obviously-nice.de   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef SSHMASTERCONNECTION_H
#define SSHMASTERCONNECTION_H

#include <libssh/libssh.h>
#include <QString>
#include <QList>
#include <QMutex>
#include <QThread>
#include <QStringList>
#include <QTcpSocket>
#include <QNetworkProxy>

class ONMainWindow;
class SshProcess;
struct ChannelConnection
{
    ssh_channel channel;
    int sock;
    SshProcess* creator;
    int forwardPort;
    int localPort;
    QString forwardHost;
    QString localHost;
    QString command;
    bool operator==(ChannelConnection& t)
    {
        return (channel==t.channel);
    }
};

struct CopyRequest
{
    SshProcess* creator;
    QString src;
    QString dst;
};

class SshMasterConnection: public QThread
{
    Q_OBJECT
public:
    void run();
    SshMasterConnection(QString host, int port, bool acceptUnknownServers, QString user,
                        QString pass, QString key, bool autologin, bool krblogin,
                        bool useproxy, QString proxyserver, quint16 proxyport,
                        QString proxylogin, QString proxypassword,
                        QObject* parent = 0);
    ~SshMasterConnection();
    static void finalizeLibSsh();
    void addChannelConnection(SshProcess* creator, int sock, QString forwardHost,
                              int forwardPort, QString localHost, int localPort, void* channel=0l);
    void addChannelConnection(SshProcess* creator, QString cmd);
    void addCopyRequest(SshProcess* creator, QString src, QString dst);
    void disconnectSession();
    void setAcceptUnknownServers(bool accept)
    {
        acceptUnknownServers=accept;
    }
    SshMasterConnection* reverseTunnelConnection(SshProcess* creator, int remotePort,
            QString localHost, int localPort);
    QString getHost()
    {
        return host;
    }
    QString getUser()
    {
        return user;
    }
    int getPort()
    {
        return port;
    }
    bool useKerberos()
    {
        return kerberos;
    };

private:
    SshMasterConnection(QString host, int port, bool acceptUnknownServers,
                        QString user, QString pass, QString key,bool autologin,
                        int remotePort, QString localHost, int localPort, SshProcess* creator,
                        bool useproxy, QString proxyserver, quint16 proxyport,
                        QString proxylogin, QString proxypassword,
                        QObject* parent, ONMainWindow* parWnd);
    bool sshConnect();
    bool userAuthWithPass();
    bool userAuthAuto();
    bool userAuthWithKey();
    bool userAuth();
    void channelLoop();
    void finalize(int arg1);
    void copy();
    int serverAuth(QString& errorMsg);
#ifdef Q_OS_WIN
    void parseKnownHosts();
#endif

private:
    ssh_session my_ssh_session;
    QList<ChannelConnection> channelConnections;
    QList<CopyRequest> copyRequests;
    QList<SshMasterConnection*> reverseTunnelConnections;
    QMutex channelConnectionsMutex;
    QMutex copyRequestMutex;
    QMutex disconnectFlagMutex;
    QMutex reverseTunnelConnectionsMutex;
    QString host;
    int port;
    QString user;
    QString pass;
    QString key;
    bool useproxy;
    QString proxyserver;
    quint16 proxyport;
    QString proxylogin;
    QString proxypassword;
    QStringList authErrors;
    bool autologin;
    bool disconnectSessionFlag;
    bool reverseTunnel;
    int reverseTunnelRemotePort;
    int reverseTunnelLocalPort;
    bool acceptUnknownServers;
    QString reverseTunnelLocalHost;
    SshProcess* reverseTunnelCreator;
    ONMainWindow* mainWnd;
    bool kerberos;
    QString sshProcErrString;
    QTcpSocket *tcpProxySocket = NULL;
    QNetworkProxy *tcpNetworkProxy = NULL;

signals:
    void stdErr(SshProcess* caller, QByteArray data);
    void stdOut(SshProcess* caller, QByteArray data);
    void ioErr(SshProcess* caller, QString error, QString lastSessionError);
    void copyErr(SshProcess* caller, QString error, QString lastSessionError);
    void copyOk(SshProcess* caller);
    void channelClosed(SshProcess* caller);

    void connectionError(QString message, QString lastSessionError);
    void serverAuthError(int errCode, QString lastSessionError);
    void userAuthError(QString error);

    void newReverceTunnelConnection(SshProcess* creator, void* newChannel);
    void reverseListenOk(SshProcess* creator);
    void connectionOk( QString host);
};


#endif // SSHMASTERCONNECTION_H


