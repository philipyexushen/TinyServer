#ifndef TCPWINDOWSHELPER_H
#define TCPWINDOWSHELPER_H

#include <QThread>
#include <QQueue>
#include <QMutex>
#include <QHostAddress>
#include <QWaitCondition>
#include <QSharedPointer>
#include <QMutexLocker>
#include <QDebug>

namespace MainWindows
{
    class ConnectionView;
    class ConnectionViewItem;
}

using MainWindows::ConnectionView;
using MainWindows::ConnectionViewItem;

namespace ConnectionViewHelper
{   
    class ItemsOpsBase
    {
    public:
        virtual void doOperation(ConnectionView *view) = 0;
        virtual ~ItemsOpsBase() = default;
    };
    
    class DeleteItem : public ItemsOpsBase
    {
    public:
        DeleteItem(qint32 target)
            :ItemsOpsBase(), _target(target){ }
        
        void doOperation(ConnectionView *view)override;
        ~DeleteItem() = default;
    private:
        qint32 _target;
    };
    
    class UpdatePulse :public ItemsOpsBase
    {
    public:
        UpdatePulse(qint32 target,qint32 currentTime)
            :ItemsOpsBase(), _target(target),_currentTime(currentTime){  }
        
        void doOperation(ConnectionView *view)override;
        ~UpdatePulse() = default;
    private:
        qint32 _target;
        qint32 _currentTime;
    };
    
    class InsertItem :public ItemsOpsBase
    {
    public:
        InsertItem(qint32 target,QHostAddress address, QAbstractSocket::SocketType socketType, 
                   QString threadId, quint16 port, quint16 serverPort)
            :ItemsOpsBase()
            , _target(target)
            ,_address(address)
            ,_socketType(socketType)
            ,_threadId(threadId)
            , _port(port)
            ,_serverPort(serverPort)
        { }
        
        void doOperation(ConnectionView *view)override;
        ~InsertItem() = default;
    private:
        qint32  _target;
        QHostAddress _address;
        QAbstractSocket::SocketType _socketType;
        QString _threadId;
        quint16 _port;
        quint16 _serverPort;
    };
    
    class UpdateRemark : public ItemsOpsBase
    {
    public:
        UpdateRemark(qint32 target, const QString &remark)
            : ItemsOpsBase(),_remark(remark),_target(target){  }
        
        void doOperation(ConnectionView *view)override;
        ~UpdateRemark() = default;
    private:
        QString _remark;
        qint32 _target;
    };
    
    class TestConnection : public ItemsOpsBase
    {
    public:
        void doOperation(ConnectionView *view)override;
    };
    
    class CopySelectedItemInform :  public ItemsOpsBase
    {
    public:
        void doOperation(ConnectionView *view)override;
    };
    
    class DisconnectTargets : public ItemsOpsBase
    {
    public:
        void doOperation(ConnectionView *view)override;
    };
    
    class TestConnectionProducer : public QThread
    {
    public:
        void run()override;
    };
    
    class CopySelectedItemInformProducer :  public QThread
    {
    public:
        void run()override;
    };
    
    class DisconnectTargetsProducer : public QThread
    {
    public:
        void run()override;
    };
    
    class DeleteItemProducer :public QThread
    {
    public:
        DeleteItemProducer(qint32 target)
            : QThread(),_target(target) { }
        void run()override;
    private:
        qint32 _target;
    };
    
    class UpdatePulseProducer :public QThread
    {
    public:
        UpdatePulseProducer(qint32 index, qint32 currentTime)
            :QThread(),_index(index),_currentTime(currentTime){  }
    protected:  
        void run()override;
    private:
        qint32 _index;
        qint32 _currentTime;
    };
    
    class UpdateRemarkProducer : public QThread
    {
    public:
        UpdateRemarkProducer(qint32 index, const QString &remark)
            :QThread(),_remark(remark),_index(index){ }
    protected:   
        void run()override;
    private:
        QString _remark;
        qint32 _index;
    };
    
    class InsertItemProducer :public QThread
    {
    public:
        InsertItemProducer(qint32 targetIndex,QHostAddress address, QAbstractSocket::SocketType socketType, 
                           QString threadId, quint16 port, quint16 serverPort)
            :QThread()
            ,_targetIndex(targetIndex)
            ,_address(address)
            ,_socketType(socketType)
            ,_threadId(threadId)
            ,_port(port)
            ,_serverPort(serverPort)
        { }
    protected:    
        void run()override;
    private:
        qint32 _targetIndex;
        QHostAddress _address;
        QAbstractSocket::SocketType _socketType;
        QString _threadId;
        quint16 _port;
        quint16 _serverPort;
    };
    
    class ConsumerHelper :public QThread
    {
    public:
        ConsumerHelper(ConnectionView *view)
            :QThread(),_view(view){ }
        ~ConsumerHelper();
    protected:
        void run() override;
    private:
        ConnectionView *_view;
        
        ConsumerHelper(const ConsumerHelper &other) = delete;
        ConsumerHelper(const ConsumerHelper &&other) = delete;
        ConsumerHelper &operator=(const ConsumerHelper &other) = delete;
    };
}

#endif // TCPWINDOWSHELPER_H
