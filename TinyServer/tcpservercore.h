#ifndef TCPSERVERCORE_H
#define TCPSERVERCORE_H

#include <QTcpSocket>
#include <QTcpServer>
#include <QList>
#include <QSharedPointer>
#include <QTimer>
#include <QThread>
#include <QMap>
#include <QMutex>
#include <QMutexLocker>
#include <QByteArray>
#include <QPair>
#include <iostream>

#include "headerframe_helper.h"
#include "connectionmap_helper.h"

namespace TcpserverCore
{
    class TcpHeaderFrameHelper;
    class TcpConnectionHandler;
    class SecondCountClock;
    
    qint32 getRawIndex();
    QString exchangeIPV6ToDottedDecimal(const Q_IPV6ADDR &ipv6);
    QString getSocketErrorType(QAbstractSocket::SocketError error);
    
    class TcpServerListendCore : public QTcpServer
    {
        Q_OBJECT
    public:  
        enum { ALLOCATEINDEX_MAX = INT32_MAX };
        
        explicit TcpServerListendCore(QObject *parent = 0);
        ~TcpServerListendCore(){ close(); }
    signals:
        void updateServer(TcpHeaderFrameHelper::MessageType messageType,const QByteArray &byteArray, const QString &userName, quint16 port);
        void updateServer(const QString &message);
        void serverReplyFinished();
        void requestSendData(TcpHeaderFrameHelper::MessageType messageType,qint32 indexExcepted, const QByteArray &bytes);
        void requestSendData(TcpHeaderFrameHelper::MessageType messageType,qint32 indexExcepted, const QString &bytes);
        void requestGetConnectionInform();
        void secondClockOut();
        void requestResetPulse(qint32 pulseInterval);
        void serverStarted(const QString &protocol, quint16 port);
        void allDisConnected(quint16 port);
    public slots:
        void replyBoardcastMessage(TcpHeaderFrameHelper::MessageType messageType, qint32 socketDescriptor, const QByteArray &bytes);
        void deleteConnection(const qint32 index);
        void replyResetPulse(qint32 pulseInterval);
        void replyUpdateTcpInform(const qint32 index, 
                                  const QHostAddress &address,
                                  const QAbstractSocket::SocketType type, 
                                  const QString &threadId, 
                                  const quint16 port);
        void stopServer(){ pauseAccepting(); bBlocking = 1; }
        void wakeUpServer(){ resumeAccepting(); bBlocking = 0; }
        void endAllConnection();
        bool replyTestConnection(qintptr target,const QString &message);
        
        bool startListening(quint16 port);
        qint32 pulseInterval() const{ return timeInterval; }
        void setPulseInterval(qint32 interval) { timeInterval = interval; }
        bool getServerBlockingState()const { return !bBlocking; }
        quint16 getCurrentPort()const{ return port; }
        QPair<QString,quint16> getServerType()const { return { QString::fromLocal8Bit("TCP"), port } ;} 
        void stopListening(){ close(); }
    protected:
        void incomingConnection(qintptr socketDescriptor)override final;
    private:
        SecondCountClock *countClock;
        
        ConnectionMap connectionMap;
        qint32 timeInterval = 1000;
        quint16 port;
        
        qint32 _allocateIndex = getRawIndex();
        bool bBlocking = 0;
        bool bInClosing = 0;
        
        qint32 allocateIndex()
        { 
            _allocateIndex = _allocateIndex + 50 < ALLOCATEINDEX_MAX - 50 ?  _allocateIndex + 50 : _allocateIndex + 50 -  ALLOCATEINDEX_MAX; 
            return _allocateIndex; 
        }
        void replyRemoveTarget(const qint32 targetIndex);
        
        TcpServerListendCore(const TcpServerListendCore &other)                = delete;
        TcpServerListendCore(TcpServerListendCore &&other)                     = delete;
        TcpServerListendCore &operator=(const TcpServerListendCore &other)     = delete;
    };
    
    class TcpServerSocketCore : public QTcpSocket
    {
        Q_OBJECT
    public:
        enum { MAXBUFSIZE = 8096 };
        
        explicit TcpServerSocketCore(QObject *parent = 0);
        bool setSocketDescriptor(qintptr socketDescriptor, SocketState state = ConnectedState,
                                 OpenMode openMode = ReadWrite) override;
        qintptr getSocketDescriptor()const{ return socketDescriptor; }
        QString getUserName(){ return userName; }
    signals:
        void socketMessageBoardcast(TcpHeaderFrameHelper::MessageType messageType,const QByteArray &bytes);
        void sendPulseToSocket();
        void loginChecked();
        void clientRemarkUpdate(const QString &msg);
    public slots:
        qint64 replySendData(TcpHeaderFrameHelper::MessageType messageType, qint32 featureCode, qint32 sourceFeatureCode, const QByteArray &bytes);
        qint64 replySendData(TcpHeaderFrameHelper::MessageType messageType, qint32 featureCode, qint32 sourceFeatureCode, const QString &msg);
    private slots:
        void dataReceiver()noexcept;
    private:
        QString userName;
        qintptr socketDescriptor = -1;//保留这个SocketDescriptor拿来删除socket的，不然没办法delete sokcet
        
        TcpServerSocketCore(const TcpServerSocketCore &other) = delete;
        TcpServerSocketCore(TcpServerSocketCore &&) = delete;
        TcpServerSocketCore &operator=(const TcpServerSocketCore &other) = delete;
    };
    
    class TCPSocketDeleter
    {
    public:
        void operator()(TcpServerSocketCore *socket){ socket->deleteLater();}
    };
    
    class TcpConnectionHandler :public QObject
    {
        Q_OBJECT
        friend class TcpServerListendCore;
    public:
        enum { PULSEINTERVAL = 10000, LOGINCHECK_TIMEOUT = 8000 };
        using SocketCorePointer = QSharedPointer<TcpServerSocketCore>;
    signals:
        void finishedSendingPulse();
        void requestBoardcastMessage(TcpHeaderFrameHelper::MessageType messageType, qint32 index, const QByteArray &bytes);
        void socketDisconneted(qintptr target, qint32 connetionIndex);
        void sendConnectionInform(const qint32 index, 
                                  const QHostAddress &address, 
                                  const QAbstractSocket::SocketType type,
                                  const QString threadId, 
                                  const quint16 peerPort);
        void updateOnLineTime(qintptr descriptor, qint32 interval);
    public slots:
        qint64 sendMessage(TcpHeaderFrameHelper::MessageType messageType, qint32 sourceFeatureCode, const QByteArray &bytes);
        qint64 sendMessage(TcpHeaderFrameHelper::MessageType messageType, qint32 sourceFeatureCode, const QString &msg);
        QString getUserName(){ return std::move(socket->getUserName()); }
        qint32 getAllocateIndex(){ return _thisAllocateIndex; } 
        qintptr getSocketDescriptor()const{ return socket->getSocketDescriptor(); }
        
        void initConnection(qintptr target);
        void replyGetConnectionInform();
        void replyBoardcastMessage(TcpHeaderFrameHelper::MessageType messageType, const QByteArray &bytes);
        void replyClockTimeOut(qint32 interval);
        void replyclientRemarkUpdate(const QString &msg);
        void replyResetPulse(qint32 pulseInterval);
        void handleLoginChecked();
        void setReturnInform();
    private slots:
        void replyPulseTimeOut()noexcept;
        void handleErrors(QAbstractSocket::SocketError error);
        void handleLoginTimeout()noexcept;
    private:
        explicit TcpConnectionHandler(qint32 allocateIndex)
            :QObject(), _thisAllocateIndex(allocateIndex) { }
        ~TcpConnectionHandler();
        
        SocketCorePointer socket;
        QSharedPointer<QTimer> pulseTimer;
        QSharedPointer<QTimer> loginCheckTimer;
        int connectionTime = 0;
        
        bool bLoginCheckIn = false;
        qint32 _thisAllocateIndex = -1; 
        
        TcpConnectionHandler()                                        = delete;
        TcpConnectionHandler(const TcpConnectionHandler &)            = delete;
        TcpConnectionHandler(TcpConnectionHandler &&)                 = delete;
        TcpConnectionHandler &operator=(const TcpConnectionHandler &) = delete;
    };
    
    class SecondCountClock :public QObject
    {
        Q_OBJECT
    public:
        explicit SecondCountClock(QObject *parent = 0);
        ~SecondCountClock(){ clockTimer->stop(); }
        
        void setInterval(qint32 interval){ timeInterval = interval; }
    signals:
        void timeOutInvoke(qint32 interval);
    public slots:
        void replyTimeout();
    private:
        QSharedPointer<QTimer> clockTimer;
        qint32 timeInterval = 1000;
    };
}

#endif // TCPSERVERCORE_H
