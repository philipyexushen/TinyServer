#include "tcpservercore.h"
#include "headerframe_helper.h"

namespace TcpserverCore
{
    void addItem(ConnectionMap *map, qint32 socketIndex,TcpConnectionHandler *connection)
    {
        AddConnectionItemOp *addConnectionItemOp = new AddConnectionItemOp(map, socketIndex,connection);
        QObject::connect(addConnectionItemOp, &QThread::finished,addConnectionItemOp,&QThread::deleteLater);
        addConnectionItemOp->start();
    }
    
    void removeItem(ConnectionMap *map, qint32 socketIndex)
    {
        DeleteConnectionItemOp *deleteConnectionItemOp = new DeleteConnectionItemOp(map, socketIndex);
        QObject::connect(deleteConnectionItemOp, &QThread::finished,deleteConnectionItemOp,&QThread::deleteLater);
        deleteConnectionItemOp->start();
    }
    
    bool isEmpty(ConnectionMap *map)
    {
        QThread *workerThread = new QThread;
        ConnectionReader *reader = new ConnectionReader(map);
        
        QObject::connect(reader,&QObject::destroyed,  workerThread,&QThread::quit);
        QObject::connect(workerThread,&QThread::finished, workerThread,&QThread::deleteLater);
        
        reader->moveToThread(workerThread);
        workerThread->start();
        
        bool reply;
        QMetaObject::invokeMethod(reader, "isEmpty",Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool,reply));
        reader->deleteLater();
        
        return reply;
    }
    
    bool testConnection(ConnectionMap *map,qint32 index,const QString &message)
    {
        QThread *workerThread = new QThread;
        ConnectionReader *reader = new ConnectionReader(map);
        
        QObject::connect(reader,&QObject::destroyed, workerThread,&QThread::quit);
        QObject::connect(workerThread,&QThread::finished, workerThread,&QThread::deleteLater);
        
        reader->moveToThread(workerThread);
        workerThread->start();
        
        bool reply;
        QMetaObject::invokeMethod(reader, "testConnection",Qt::BlockingQueuedConnection, 
                                  Q_RETURN_ARG(bool,reply),
                                  Q_ARG(qint32, index),
                                  Q_ARG(const QString &,message));
        reader->deleteLater();
        
        return reply;
    }
    
    qint32 getItemIndex(ConnectionMap *map, qint32 index)
    {
        qRegisterMetaType<qint32>("qint32");
        qRegisterMetaType<qintptr>("qintptr");
        
        QThread *workerThread = new QThread;
        ConnectionReader *reader = new ConnectionReader(map);
        
        QObject::connect(reader,&QObject::destroyed, workerThread,&QThread::quit);
        QObject::connect(workerThread,&QThread::finished, workerThread,&QThread::deleteLater);
        
        reader->moveToThread(workerThread);
        workerThread->start();
        
        qint32 reply;
        QMetaObject::invokeMethod(reader, "getItemIndex",Qt::BlockingQueuedConnection, 
                                  Q_RETURN_ARG(qint32,reply),
                                  Q_ARG(qint32,index));
        reader->deleteLater();
        
        return reply;
    }
    
    QString getItemName(ConnectionMap *map,qint32 index)
    {
        qRegisterMetaType<TcpConnectionHandler *>("TcpConnectionHandler *");
        qRegisterMetaType<qintptr>("qintptr");
        
        QThread *workerThread = new QThread;
        ConnectionReader *reader = new ConnectionReader(map);
        
        QObject::connect(reader,&QObject::destroyed, workerThread,&QThread::quit);
        QObject::connect(workerThread,&QThread::finished, workerThread,&QThread::deleteLater);
        
        reader->moveToThread(workerThread);
        workerThread->start();
        
        QString reply;
        QMetaObject::invokeMethod(reader, "getItemName",Qt::BlockingQueuedConnection, 
                                  Q_RETURN_ARG(QString,reply),
                                  Q_ARG(qint32,index));
        reader->deleteLater();
        
        return reply;
    }
    
    QList<qint32> getIndexes(ConnectionMap *map)
    {
        qRegisterMetaType<QList<qint32>>("QList<qintptr>");
        
        QThread *workerThread = new QThread;
        ConnectionReader *reader = new ConnectionReader(map);
        
        QObject::connect(reader,&QObject::destroyed, workerThread,&QThread::quit);
        QObject::connect(workerThread,&QThread::finished, workerThread,&QThread::deleteLater);
        
        reader->moveToThread(workerThread);
        workerThread->start();
        
        QList<qint32> reply;
        QMetaObject::invokeMethod(reader, "getIndexes",Qt::BlockingQueuedConnection, 
                                  Q_RETURN_ARG(QList<qint32>,reply));
        reader->deleteLater();
        
        return reply;
    }
    
    bool ConnectionReader::isEmpty()
    {
        bool reply;
        _map->_lock.lockForRead();
        reply = _map->_connections.isEmpty();
        _map->_lock.unlock();
        return reply;
    }
    
    bool ConnectionReader::testConnection(qint32 index,const QString &message)
    {
        qRegisterMetaType<TcpHeaderFrameHelper::MessageType>("TcpHeaderFrameHelper::MessageType");
        qRegisterMetaType<qintptr>("qintptr");
        qRegisterMetaType<qint64>("qint64");
        
        qint64 reply;
        _map->_lock.lockForRead();
        
        auto item = _map->_connections.find(index);
        if (item == _map->_connections.end())
            reply = false;
        else 
        {
            QMetaObject::invokeMethod(item.value(),"sendMessage",Qt::BlockingQueuedConnection,
                                      Q_RETURN_ARG(qint64, reply),
                                      Q_ARG(TcpHeaderFrameHelper::MessageType,TcpHeaderFrameHelper::MessageType::ServerTest),
                                      Q_ARG(qint32,-1),
                                      Q_ARG(const QString &, message));
        }
        
        _map->_lock.unlock();
        return reply == 0? false : true;
    }

    qint32 ConnectionReader::getItemIndex(qint32 index)
    {
        qint32 reply;
        _map->_lock.lockForRead();
        
        auto item = _map->_connections.find(index);
        if (item == _map->_connections.end())
            reply = -1;
        else 
            QMetaObject::invokeMethod(item.value(), "getAllocateIndex",Qt::BlockingQueuedConnection, Q_RETURN_ARG(qint32, reply));
        
        _map->_lock.unlock();
        return reply;
    }
    
    QString ConnectionReader::getItemName(qint32 index)
    {
        QString reply;
        _map->_lock.lockForRead();
        
        auto item = _map->_connections.find(index);
        if (item == _map->_connections.end())
            reply = QString();
        else 
            QMetaObject::invokeMethod(item.value(),"getUserName", Qt::BlockingQueuedConnection,Q_RETURN_ARG(QString, reply));
        
        _map->_lock.unlock();
        return reply;
    }

    QList<qint32> ConnectionReader::getIndexes()
    {
        QList<qint32> indexes;
        _map->_lock.lockForRead();
        indexes = _map->_connections.keys();
        _map->_lock.unlock();
        
        return indexes;
    }
    
    void AddConnectionItemOp::run()
    {        
        _map->_lock.lockForWrite();
        _map->_connections.insert(_socketIndex,_connection);
        _map->_lock.unlock();
    }
    
    void DeleteConnectionItemOp::run()
    {
        _map->_lock.lockForWrite();
        auto iter = _map->_connections.find(_socketIndex);
        if (iter != _map->_connections.end())
        {
            TcpConnectionHandler *handler = iter.value();
            _map->_connections.remove(_socketIndex);
            QMetaObject::invokeMethod(handler, "deleteLater",Qt::QueuedConnection);
        }
        _map->_lock.unlock();
    }
}
