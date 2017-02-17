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
    
    QString exchangeIPV6ToDottedDecimal(const Q_IPV6ADDR &ipv6);
    
    class TcpServerListendCore : public QTcpServer
    {
        Q_OBJECT
    public:  
        explicit TcpServerListendCore(QObject *parent = 0);
        ~TcpServerListendCore(){ close(); }
    signals:
        void updateServer(TcpHeaderFrameHelper::MessageType messageType,const QByteArray &byteArray, const QString &userName, quint16 port,qint32 idnex);
        void updateServer(const QString &message);
        void serverReplyFinished();
        void requestSendData(TcpHeaderFrameHelper::MessageType messageType,qint32 indexExcepted, const QByteArray &bytes);
        void requestSendData(TcpHeaderFrameHelper::MessageType messageType,qint32 indexExcepted, const QString &bytes);
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
        bool replyTestConnection(qint32 target,const QString &message);
        
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
        ConnectionMap connectionMap;
        qint32 timeInterval = 1000;
        quint16 port;
        
        bool bBlocking = 0;
        bool bInClosing = 0;
        
        static qint32 getRawIndex();
        
        qint32 allocateIndex(){ return getRawIndex();}
        void replyRemoveTarget(const qint32 targetIndex);
        
        TcpServerListendCore(const TcpServerListendCore &other)                = delete;
        TcpServerListendCore(TcpServerListendCore &&other)                     = delete;
        TcpServerListendCore &operator=(const TcpServerListendCore &other)     = delete;
    };
    
    class TcpServerSocketCore : public QTcpSocket
    {
        Q_OBJECT
    public:
        enum { MAXBUFSIZE = INT32_MAX };
        
        explicit TcpServerSocketCore(QObject *parent = 0);
        bool setSocketDescriptor(qintptr socketDescriptor, SocketState state = ConnectedState,
                                 OpenMode openMode = ReadWrite) override;
        qintptr getSocketDescriptor()const{ return socketDescriptor; }
        QString getUserName(){ return userName; }
    signals:
        void socketMessageBoardcast(TcpHeaderFrameHelper::MessageType messageType,const QByteArray &bytes);
        void loginChecked();
        void clientRemarkUpdate(const QString &msg);
    public slots:
        qint64 replySendData(TcpHeaderFrameHelper::MessageType messageType, qint32 featureCode, qint32 sourceFeatureCode, const QByteArray &bytes);
        qint64 replySendData(TcpHeaderFrameHelper::MessageType messageType, qint32 featureCode, qint32 sourceFeatureCode, const QString &msg);
    private slots:
        qint64 dataReceiver();
    private:
        QString userName;
        qintptr socketDescriptor = -1;//保留这个SocketDescriptor拿来删除socket的，不然没办法delete sokcet
        
        bool _isConnected = false;
        bool _waitingForWholeData = false;
        bool _isReading = false;
        qint64 _currentRead = 0; 
        qint32 _targetLength = 0;
        TcpHeaderFrameHelper::TcpHeaderFrame _headerFrame;
        
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
        QString getUserName(){ return socket->getUserName(); }
        qint32 getAllocateIndex(){ return _thisAllocateIndex; } 
        qintptr getSocketDescriptor()const{ return socket->getSocketDescriptor(); }
        
        void initConnection(qintptr target);
        void replyBoardcastMessage(TcpHeaderFrameHelper::MessageType messageType, const QByteArray &bytes);
        void replyclientRemarkUpdate(const QString &msg);
        void replyResetPulse(qint32 pulseInterval);
        void handleLoginChecked();
        void setReturnInform();
    private slots:
        void replyPulseTimeOut()noexcept;
        void handleErrors(QAbstractSocket::SocketError error);
        void handleLoginTimeout()noexcept;
        void on_secondsCounter_timeout();
    private:
        explicit TcpConnectionHandler(qint32 allocateIndex)
            :QObject(), _thisAllocateIndex(allocateIndex) { }
        ~TcpConnectionHandler() = default;
        
        SocketCorePointer socket;
        QSharedPointer<QTimer> pulseTimer;
        QSharedPointer<QTimer> _secondsCounter;
        int connectionTime = 0;
        
        bool bLoginCheckIn = false;
        qint32 _thisAllocateIndex = -1; 
        qint32 _interval;
        
        TcpConnectionHandler()                                        = delete;
        TcpConnectionHandler(const TcpConnectionHandler &)            = delete;
        TcpConnectionHandler(TcpConnectionHandler &&)                 = delete;
        TcpConnectionHandler &operator=(const TcpConnectionHandler &) = delete;
    };
}

#endif // TCPSERVERCORE_H
