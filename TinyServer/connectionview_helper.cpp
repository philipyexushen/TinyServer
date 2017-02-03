#include <QDebug>
#include <QTime>
#include <QTreeWidgetItem>
#include "tcpserverwindow.h"

namespace ConnectionViewHelper
{   
    //把所有的UI事件操作都投递到操作队列中，然后开一个消费者线程不断dequeue操作
    //在投递操作之前，必须加锁
    
    static QQueue<QSharedPointer<ItemsOpsBase>> &opQueue()
    {
        static QQueue<QSharedPointer<ItemsOpsBase>> queue;
        return queue;
    }
    
    static QSharedPointer<ItemsOpsBase> endOperation;

    static QMutex &opQueueLock()
    {
        static QMutex mutex;
        return mutex;
    }
    
    static QWaitCondition &opQueueIsAvailable()
    {
        static QWaitCondition flag;
        return flag;
    }
    
    void DeleteItem::doOperation(ConnectionView *view)
    {
        qRegisterMetaType<qintptr>("qintptr");
        qRegisterMetaType<TcpConnectionHandler *>("TcpConnectionHandler *");
        QMetaObject::invokeMethod(view, "deleteConnection",Qt::QueuedConnection, Q_ARG(qint32, _target));
    }
    
    void UpdatePulse::doOperation(ConnectionView *view)
    {
        qRegisterMetaType<qintptr>("qintptr");
        qRegisterMetaType<qint32>("qint32");
        
        QMetaObject::invokeMethod(view, "updateConnectionTime",Qt::BlockingQueuedConnection, Q_ARG(qint32,_target),Q_ARG(qint32,_currentTime));
    }
    
    void UpdateRemark::doOperation(ConnectionView *view)
    {
        qRegisterMetaType<qintptr>("qintptr");
        qRegisterMetaType<QString>("QString");
        
        QMetaObject::invokeMethod(view, "updateRemark",Qt::BlockingQueuedConnection, Q_ARG(qint32,_target),Q_ARG(QString, _remark));
    }
    
    void TestConnection::doOperation(ConnectionView *view)
    {
        QMetaObject::invokeMethod(view, "replyTestConnection",Qt::BlockingQueuedConnection);
    }
    
    void DisconnectTargets::doOperation(ConnectionView *view)
    {
        QMetaObject::invokeMethod(view, "replyDisconnectTargets",Qt::BlockingQueuedConnection);
    }
    
    void CopySelectedItemInform::doOperation(ConnectionView *view)
    {
        QMetaObject::invokeMethod(view, "replyCopySelectedInform",Qt::BlockingQueuedConnection);
    }
    
    void InsertItem::doOperation(ConnectionView *view)
    {
        qRegisterMetaType<ConnectionViewItem *>("ConnectionViewItem *");
        
        using INDEX = ConnectionView::HeaderNameIndex;
        
        ConnectionViewItem *item(new ConnectionViewItem);
        
        auto addString = _address.toString();
        item->setText(INDEX::ADDRESS,addString);
        item->setTextAlignment(INDEX::ADDRESS,Qt::AlignCenter);
        
        if (_socketType == QAbstractSocket::TcpSocket)
            item->setText(INDEX::PROTOCOL,QString::fromLocal8Bit("TCP"));
        else if (_socketType == QAbstractSocket::UdpSocket)
            item->setText(INDEX::PROTOCOL,QString::fromLocal8Bit("UDP"));
        else
            item->setText(INDEX::PROTOCOL,QString::fromLocal8Bit("Unknown"));
        item->setTextAlignment(INDEX::PROTOCOL,Qt::AlignCenter);
        
        QTime time(0,0,0);
        
        item->setText(INDEX::ONLINETIME,time.toString());
        item->setTextAlignment(INDEX::ONLINETIME,Qt::AlignCenter);
        
        item->setText(INDEX::DESCRIPTOR,QString::number(_target));
        item->setTextAlignment(INDEX::DESCRIPTOR,Qt::AlignCenter);
        
        item->setText(INDEX::THREADID,_threadId);
        item->setTextAlignment(INDEX::THREADID,Qt::AlignCenter);
        
        item->setText(INDEX::PEERPORT,QString::number(_port));
        item->setTextAlignment(INDEX::PEERPORT,Qt::AlignCenter);
        
        item->setText(INDEX::LISTENEDPORT,QString::number(_serverPort));
        item->setTextAlignment(INDEX::LISTENEDPORT,Qt::AlignCenter);
        
        item->setText(INDEX::REMARK, QString("(null)"));
        item->setTextAlignment(INDEX::REMARK,Qt::AlignCenter);
        
        QMetaObject::invokeMethod(view, "updateConnectionList",Qt::BlockingQueuedConnection, Q_ARG(ConnectionViewItem *,item));
    }
    
    void DeleteItemProducer::run()
    {
        QSharedPointer<ItemsOpsBase> op = QSharedPointer<ItemsOpsBase>(new DeleteItem(_target));
        
        QMutexLocker locker(&opQueueLock());
        opQueue().enqueue(op);
        opQueueIsAvailable().wakeOne();
    }
    
    void CopySelectedItemInformProducer::run()
    {
        QSharedPointer<ItemsOpsBase> op = QSharedPointer<ItemsOpsBase>(new CopySelectedItemInform);
        
        QMutexLocker locker(&opQueueLock());
        opQueue().enqueue(op);
        opQueueIsAvailable().wakeOne();
    }
    
    void UpdatePulseProducer::run()
    {
        QSharedPointer<ItemsOpsBase> op = QSharedPointer<ItemsOpsBase>(new UpdatePulse(_index,_currentTime));
        
        QMutexLocker locker(&opQueueLock());
        opQueue().enqueue(op);
        opQueueIsAvailable().wakeOne();
    }
    
    void TestConnectionProducer::run()
    {
        QSharedPointer<ItemsOpsBase> op = QSharedPointer<ItemsOpsBase>(new TestConnection);
        
        QMutexLocker locker(&opQueueLock());
        opQueue().enqueue(op);
        opQueueIsAvailable().wakeOne();
    }
    
    void DisconnectTargetsProducer::run()
    {
        QSharedPointer<ItemsOpsBase> op = QSharedPointer<ItemsOpsBase>(new DisconnectTargets);
        
        QMutexLocker locker(&opQueueLock());
        opQueue().enqueue(op);
        opQueueIsAvailable().wakeOne();
    }
    
    void InsertItemProducer::run()
    {
        QSharedPointer<ItemsOpsBase> op
                 = QSharedPointer<ItemsOpsBase>(new InsertItem(_targetIndex,_address, _socketType,_threadId,_port,_serverPort));
        
        QMutexLocker locker(&opQueueLock());
        opQueue().enqueue(op);
        opQueueIsAvailable().wakeOne();
    }
    
    void UpdateRemarkProducer::run()
    {
        QSharedPointer<ItemsOpsBase> op = QSharedPointer<ItemsOpsBase>(new UpdateRemark(_index,_remark));
        
        QMutexLocker locker(&opQueueLock());
        opQueue().enqueue(op);
        opQueueIsAvailable().wakeOne();
    }
    
    void ConsumerHelper::run()
    {
        forever
        {
            QSharedPointer<ItemsOpsBase> opPointer;
            
            {
                QMutexLocker locker(&opQueueLock());
                
                if (opQueue().isEmpty())
                    opQueueIsAvailable().wait(&opQueueLock());
                opPointer = opQueue().dequeue();
                
                if (opPointer == endOperation)
                    break;
            }
            {
                if(!opPointer.isNull())
                    opPointer->doOperation(_view);
            }
        }
    } 

    ConsumerHelper::~ConsumerHelper()
    {
        {
            QMutexLocker locker(&opQueueLock());
            while(!opQueue().isEmpty())
                opQueue().dequeue();
            
            opQueue().enqueue(endOperation);
            opQueueIsAvailable().wakeOne();
        }
        
        wait();//注意这里是wait在次线程上的
    }
}























