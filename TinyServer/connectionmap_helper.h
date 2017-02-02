#ifndef CONNECTIONMAP_HELPER_H
#define CONNECTIONMAP_HELPER_H

#include <QObject>
#include <QMap>
#include <QThread>
#include <QList>
#include <QReadWriteLock>

namespace TcpserverCore
{
    class TcpConnectionHandler;
    class TcpServerListendCore;
    
    class ConnectionMap
    {
        friend class AddConnectionItemOp;
        friend class DeleteConnectionItemOp;
        friend class ConnectionReader;
    public:
        ConnectionMap() = default;
    private:
        QReadWriteLock _lock;
        QMap<qint32,TcpConnectionHandler *> _connections;
    };
    
    class AddConnectionItemOp : private QThread
    {
        friend void addItem(ConnectionMap *map, qint32 socketIndex,TcpConnectionHandler *connection);
    public:
        AddConnectionItemOp(ConnectionMap *map,qint32 socketIndex,TcpConnectionHandler *connection)
            :QThread(),_map(map),_socketIndex(socketIndex),_connection(connection) { }
    protected:
        void run()override;
    private:
        ConnectionMap *_map;
        qint32 _socketIndex;
        TcpConnectionHandler *_connection;
    };
    
    class DeleteConnectionItemOp : private QThread
    {
        friend void removeItem(ConnectionMap *map, qint32 socketIndex);
    public:
        DeleteConnectionItemOp(ConnectionMap *map,qint32 socketIndex)
            :QThread(),_map(map),  _socketIndex(socketIndex) { }
    protected:
        void run()override;
    private:
        ConnectionMap *_map;
        qint32 _socketIndex;
    };
    
    class ConnectionReader : public QObject
    {
        Q_OBJECT
        friend qint32 getItemIndex(ConnectionMap *map, qint32 index);
        friend QList<qint32> getDesciptors(ConnectionMap *map);
        friend bool isEmpty(ConnectionMap *map);
        friend QString getItemName(ConnectionMap *map,qint32 index);
        friend bool testConnection(ConnectionMap *map,qint32 index, const QString &message);
    public:
        ConnectionReader(ConnectionMap *map)
            :QObject(),_map(map){ }
        ~ConnectionReader() = default;
    private slots:
        bool isEmpty();
        qint32 getItemIndex(qint32 descriptor);
        QString getItemName(qint32 descriptor);
        QList<qint32> getIndexes();
        bool testConnection(qint32 descriptor, const QString &message);
    private:
        ConnectionMap *_map;
    };
    
    extern qint32 getItemIndex(ConnectionMap *map, qint32 index);//永远不要尝试去取TcpConnectionHandler的指针，很危险，要什么内容直接用线程去取
    extern QString getItemName(ConnectionMap *map, qint32 index);
    extern bool testConnection(ConnectionMap *map, qint32 index, const QString &message);
    extern QList<qint32> getIndexes(ConnectionMap *map);
    extern bool isEmpty(ConnectionMap *map);
    
    extern void addItem(ConnectionMap *map, qint32 socketIndex,TcpConnectionHandler *connection);
    extern void removeItem(ConnectionMap *map, qint32 socketIndex);
}





#endif // CONNECTIONMAP_HELPER_H
