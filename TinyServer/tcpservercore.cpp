#include <QByteArray>
#include <QDebug>
#include <QTextCodec>
#include <QTimer>
#include <exception>
#include <QHostInfo>
#include <QThread>
#include <QMutex>
#include <QSemaphore>
#include <QReadWriteLock>
#include <QDateTime>
#include <QDataStream>
#include <QApplication>

#include "tcpservercore.h"
#include "connectionview_helper.h"

using ConnectionViewHelper::UpdatePulseProducer;
using ConnectionViewHelper::DeleteItemProducer;
using ConnectionViewHelper::InsertItemProducer;
using ConnectionViewHelper::UpdateRemarkProducer;

namespace TcpserverCore
{
    /// 用于转换handleError的错误
    QString getSocketErrorType(QAbstractSocket::SocketError error)
    {
        static QMap<QAbstractSocket::SocketError,QString> errorMap
        { 
            { QAbstractSocket::ConnectionRefusedError,             "ConnectionRefusedError"          }, 
            { QAbstractSocket::RemoteHostClosedError,              "RemoteHostClosedError"           },
            { QAbstractSocket::HostNotFoundError,                  "HostNotFoundError"               },
            { QAbstractSocket::SocketAccessError,                  "SocketAccessError"               },
            { QAbstractSocket::SocketResourceError,                "SocketResourceError"             },
            { QAbstractSocket::SocketTimeoutError,                 "SocketTimeoutError"              },
            { QAbstractSocket::DatagramTooLargeError,              "DatagramTooLargeError"           },
            { QAbstractSocket::NetworkError,                       "NetworkError"                    },
            { QAbstractSocket::AddressInUseError,                  "AddressInUseError"               },
            { QAbstractSocket::SocketAddressNotAvailableError,     "SocketAddressNotAvailableError"  },
            { QAbstractSocket::UnsupportedSocketOperationError,    "UnsupportedSocketOperationError" },
            { QAbstractSocket::ProxyAuthenticationRequiredError,   "ProxyAuthenticationRequiredError"},
            { QAbstractSocket::SslHandshakeFailedError,            "SslHandshakeFailedError"         },
            { QAbstractSocket::UnfinishedSocketOperationError,     "UnfinishedSocketOperationError"  },
            { QAbstractSocket::ProxyConnectionRefusedError,        "ProxyConnectionRefusedError"     },
            { QAbstractSocket::ProxyConnectionClosedError,         "ProxyConnectionClosedError"      },
            { QAbstractSocket::ProxyConnectionTimeoutError,        "ProxyConnectionTimeoutError"     },
            { QAbstractSocket::ProxyNotFoundError,                 "ProxyNotFoundError"              },
            { QAbstractSocket::ProxyProtocolError,                 "ProxyProtocolError"              },
            { QAbstractSocket::OperationError,                     "OperationError"                  },
            { QAbstractSocket::SslInternalError,                   "SslInternalError"                },
            { QAbstractSocket::SslInvalidUserDataError,            "SslInvalidUserDataError"         },
            { QAbstractSocket::TemporaryError ,                    "TemporaryError"                  },
            { QAbstractSocket::UnknownSocketError,                 "UnknownSocketError"              }
        };
        return errorMap[error];
    }
    
    /// IPV6点分十进制转化方法
    QString exchangeIPV6ToDottedDecimal(const Q_IPV6ADDR &ipv6)
    {
        static QChar numToWordMap[]{'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };
        QString result;
        
        for(int i = 0;i < 16; i+=2 )
        {
            quint8 tmp = ipv6[i];
            result += numToWordMap[(tmp >> 4)&0x0f];
            result += numToWordMap[tmp&0x0f];
            
            tmp = ipv6[i + 1];
            result += numToWordMap[(tmp >> 4)&0x0f];
            result += numToWordMap[tmp&0x0f];
            
            result += ":";
        }
        return result;
    }
    
    /// 创建一个TcpServerListendCore实例
    TcpServerListendCore::TcpServerListendCore(QObject *parent)
        :QTcpServer(parent)
    {
        
    }
    
    /// 获取一个index
    /// 模仿UNIX FIFO的设计，随机给定一个index，然后每次分配一个新的socket值递增50
    qint32 TcpServerListendCore::getRawIndex()
    {
        static QMutex mutex;
        QMutexLocker locker(&mutex);
        enum { ALLOCATEINDEX_MAX = INT32_MAX };
        
        static bool bSrandSet = false;
        static qint32 indexBase;
        if (!bSrandSet)
        {
            QTime time = QTime::currentTime();
            qsrand(time.msec()+time.second()*1000);
            bSrandSet = true;
            indexBase =  qrand() % 65536;
        }
        indexBase = indexBase + 50 < ALLOCATEINDEX_MAX - 50 ?  indexBase + 50 : indexBase + 50 -  indexBase;
        return indexBase;
    }
    
    /// 当一个socket读到信号时，将把信息从端口广播到所有的socket
    void TcpServerListendCore::replyBoardcastMessage(TcpHeaderFrameHelper::MessageType messageType, qint32 indexExcepted, const QByteArray &bytes)
    {
        //发送信息更新客户端的view,可能会失败，如果失败那就在UI线程里面再更新一次
        QString userName = getItemName(&connectionMap,indexExcepted);
        emit updateServer(messageType, bytes,std::move(userName), port, indexExcepted);
        emit requestSendData(messageType, indexExcepted, bytes);
    }
    
    /// 当存在一个新连接时引发此槽，如果可以正确设置socket的描述符，那么*异步*地将handler存入端口的connectionMap中
    void TcpServerListendCore::incomingConnection(qintptr socketDescriptor)
    {
        qRegisterMetaType<qintptr>("qintptr");
        qRegisterMetaType<QHostAddress>("QHostAddress");
        qRegisterMetaType<QAbstractSocket::SocketType>("QAbstractSocket::SocketType");
        
        //注意这里获取index是线程安全的，因为incomingConnection本身是队列操作，这个函数执行期间只会是单一线程
        qint32 index = allocateIndex();
        TcpConnectionHandler *connection = new TcpConnectionHandler(index);
        QThread *workerThread = new QThread;
        
        connect(this, &TcpServerListendCore::serverReplyFinished, connection,&TcpConnectionHandler::setReturnInform);
        
        connect(connection, SIGNAL(requestBoardcastMessage(TcpHeaderFrameHelper::MessageType, qint32, const QByteArray &)), 
                this, SLOT(replyBoardcastMessage(TcpHeaderFrameHelper::MessageType, qint32, const QByteArray &)));
        
        connect(this, SIGNAL(requestSendData(TcpHeaderFrameHelper::MessageType, qint32, const QString &)), 
                connection,SLOT(sendMessage(TcpHeaderFrameHelper::MessageType, qint32,const QString &)));
        
        connect(this, SIGNAL(requestSendData(TcpHeaderFrameHelper::MessageType,qint32,const QByteArray &)),
                connection,SLOT(sendMessage(TcpHeaderFrameHelper::MessageType, qint32, const QByteArray &)));
        
        connect(connection,&TcpConnectionHandler::socketDisconneted, this, &TcpServerListendCore::deleteConnection);
        
        connect(connection, &QObject::destroyed, workerThread, &QThread::quit);
        connect(workerThread, &QThread::finished, workerThread, &QThread::deleteLater);
        
        connect(connection, &TcpConnectionHandler::sendConnectionInform, this, &TcpServerListendCore::replyUpdateTcpInform);
        connect(this, &TcpServerListendCore::requestResetPulse,connection, &TcpConnectionHandler::replyResetPulse);
        
        connection->moveToThread(workerThread);
        workerThread->start();
        QMetaObject::invokeMethod(connection,"initConnection",Qt::QueuedConnection, Q_ARG(qintptr,socketDescriptor));
        
        addItem(&connectionMap,index,connection);
    }
    
    /// 异步清除对应index的在view中的表项
    void TcpServerListendCore::replyRemoveTarget(const qint32 targetIndex)
    {
        DeleteItemProducer *deleteItemProducer = new DeleteItemProducer(targetIndex);
        connect(deleteItemProducer, &QThread::finished, deleteItemProducer, &QThread::deleteLater);
        deleteItemProducer->start();
        
        if (bInClosing && isEmpty(&connectionMap))//在这里异步发送关闭服务信号
        {
            emit allDisConnected(port);
            deleteLater();
        }
    }
    
    /// 变更当前心跳包的间隔
    void TcpServerListendCore::replyResetPulse(qint32 pulseInterval)
    {
        timeInterval = pulseInterval;   
        emit requestResetPulse(pulseInterval);
    }
    
    /// 异步添加一个TcpConnectionHandler
    void TcpServerListendCore::replyUpdateTcpInform(const qint32 index, const QHostAddress &address, const QAbstractSocket::SocketType type, 
                                                    const QString &threadId, const quint16 peerPort)
    {
        InsertItemProducer *insertItemProducer 
                = new InsertItemProducer(index,std::move(address),std::move(type),std::move(threadId), peerPort, port);
        
        connect(insertItemProducer, &QThread::finished, insertItemProducer,&QThread::deleteLater);
        insertItemProducer->start();
    }
    
    /// 关闭所有连接
    void TcpServerListendCore::endAllConnection()
    {
        auto Indexes = getIndexes(&connectionMap);
        if (Indexes.isEmpty())//如果直接是空的，删除就好了
        {
            emit allDisConnected(port);
            deleteLater();
            return;
        }
        
        bInClosing = 1;//表示需要关闭Server，必须通过Handle异步通知
        
        for (const auto &index: Indexes)
            deleteConnection(index);
    }
    
    /// 开启对应端口监听监听
    bool TcpServerListendCore::startListening(quint16 targetPort)
    {
        if(!listen(QHostAddress::Any, targetPort))
        {
            QString errorStr = this->errorString();
            emit updateServer(errorStr);
            
            return false;
        }
        
        port = targetPort;
        return true;
    }
    
    /// 关闭连接，此方法会调用removeItem异步删除连接，并且调用replyRemoveTarget
    void TcpServerListendCore::deleteConnection(const qint32 targetIndex)
    {
        removeItem(&connectionMap,targetIndex);
        replyRemoveTarget(targetIndex);
    }
    
    /// 响应view的测试连接操作
    bool TcpServerListendCore::replyTestConnection(qint32 target,const QString &message)
    {
        return testConnection(&connectionMap,target,message);
    }
    
    /// 创建一个新的TcpServerSocketCore实例
    TcpServerSocketCore::TcpServerSocketCore(QObject *parent)
        :QTcpSocket(parent)
    {
        connect(this,&TcpServerSocketCore::readyRead,  this, &TcpServerSocketCore::dataReceiver);
    }
    
    /// 重载QTcpSocket的setSocketDescriptor方法，让TcpServerSocketCore自身拥有descriptor的副本
    /// 方便socket以descriptor的方式存储于表中
    bool TcpServerSocketCore::setSocketDescriptor(qintptr descriptor, SocketState state, OpenMode openMode)
    {
        socketDescriptor = descriptor;
        return QTcpSocket::setSocketDescriptor(socketDescriptor,state,openMode);
    }
    
    /// 响应socket的异步读取信号readyRead
    /// 注意在Qt中，Socket||Server类都是异步组件，bytesAvailable()的值并不能保证在同一个方法中不同时刻相同
    /// 当你调用read()函数时，它们仅仅返回已可用的数据；当你调用write()函数时，它们仅仅将写入列入计划列表稍后执行。只有返回事件循环的时候，真正的读写才会执行
    /// readyRead信号一旦发出，并不代表可以全部读取其他进程write到socket的全部信息（根本根本不知道write进行了几次）
    /// 相反，readyRead信号可能多次触发，bytesAvailable()随着调用次数的改变会不同，意味着IO缓冲区在不断变化
    /// 比如读取25755长度的信息，socket，如果执行下面这个槽函数将会产生如下结果：
    /// nAvailable:  1440
    /// headerFrame length:  35479
    /// nAvailable:  7164
    /// nAvailable:  36
    /// nAvailable:  2880
    /// nAvailable:  1368
    /// nAvailable:  72
    /// nAvailable:  1440
    /// nAvailable:  1428
    /// nAvailable:  6678
    /// nAvailable:  3270
    /// nAvailable:  826
    /// nAvailable:  2030
    /// nAvailable:  18
    /// nAvailable:  2048
    /// nAvailable:  778
    /// nAvailable:  2844
    /// nAvailable:  474
    /// nAvailable:  701
    /// _currentRead:  35495
    /// 所以传输信息时必须在头帧上添加数据长度信息，在数据长度信息准确的情况下，读取全部信息
    /// 并且注意QIODevice允许的读取最大长度是64位的，而QByteArray最多只能到uint
    qint64 TcpServerSocketCore::dataReceiver()
    {
        qint32 nRead = 0;
        qint64 readReturn;
        QByteArray bytes;

        _currentRead = bytesAvailable();
        //qDebug () << "nAvailable: " << _currentRead;
        
        if (_currentRead < TcpHeaderFrameHelper::sizeofHeaderFrame())
            return 0;
        
        if (!_waitingForWholeData)
        {
            bytes.resize(TcpHeaderFrameHelper::sizeofHeaderFrame());
            readReturn = read(bytes.data(), TcpHeaderFrameHelper::sizeofHeaderFrame());
            
            if (readReturn == -1)
                return -1;
            nRead += readReturn;
            
            TcpHeaderFrameHelper::praseHeader(bytes, _headerFrame);
            _targetLength = _headerFrame.messageLength + TcpHeaderFrameHelper::sizeofHeaderFrame();
            //qDebug () << "headerFrame length: " << _headerFrame.messageLength;
        }
        
        if (_currentRead >= _targetLength)
        {
            qint32 length = _targetLength - TcpHeaderFrameHelper::sizeofHeaderFrame();
            bytes.resize(length);
            readReturn = read(bytes.data(), length);
            
            if (readReturn == -1)
                return -1;
            nRead += readReturn;
            
            if (_headerFrame.messageType == static_cast<qint32>(TcpHeaderFrameHelper::MessageType::DeviceLogIn))
            {        
                QTextCodec *codec = QTextCodec::codecForName("GB18030");
                QTextDecoder *decoder = codec->makeDecoder();
                
                //把登陆信息直接设定为用户名记录
                userName = decoder->toUnicode(bytes);
                emit loginChecked();
                emit clientRemarkUpdate(userName);
                emit socketMessageBoardcast(TcpHeaderFrameHelper::MessageType::DeviceLogIn, bytes);
                
                delete decoder;
            }
            else if(_headerFrame.messageType == static_cast<qint32>(TcpHeaderFrameHelper::MessageType::PulseFacility))
            {
                
            }
            else if(_headerFrame.messageType == static_cast<qint32>(TcpHeaderFrameHelper::MessageType::PlainMessage)
                    ||_headerFrame.messageType == static_cast<qint32>(TcpHeaderFrameHelper::MessageType::ServerTest))
            {
                emit socketMessageBoardcast(TcpHeaderFrameHelper::MessageType::PlainMessage, bytes);
                //一个客户端的套接字的信息发生改变，立马广播到服务器和其他客户端
            }
            else if(_headerFrame.messageType == static_cast<qint32>(TcpHeaderFrameHelper::MessageType::Coordinate))
            {
                emit socketMessageBoardcast(TcpHeaderFrameHelper::MessageType::Coordinate, bytes);
            }
            //qDebug () << "_currentRead: " << _currentRead;
            
            _waitingForWholeData = false;
            _currentRead -= _targetLength;
        }
        //如果不等于headerFrame.messageLength，说明还没读完，继续读取
        else
        {
            _waitingForWholeData = true;
        }
        return nRead;
    }
    
    /// 往socket中异步写入数据，注意socket是异步API，不要用锁，也不要flush，因为当控制权返回IO时，write会被立即执行
    qint64 TcpServerSocketCore::replySendData(
            TcpHeaderFrameHelper::MessageType messageType, qint32 featureCode, qint32 sourceFeatureCode,  const QByteArray &bytes)
    {
        qint64 ret = -1;
        
        TcpHeaderFrameHelper::TcpHeaderFrame header;
        header.messageType = static_cast<qint32>(messageType);
        header.featureCode = featureCode;
        header.sourceFeatureCode = sourceFeatureCode;
        header.messageLength = bytes.size();      
        
        QByteArray packingBytes =TcpHeaderFrameHelper::bindHeaderAndDatagram(header,bytes);
                                               
        ret = write(packingBytes,packingBytes.size());
        return ret;
    }
    
    /// （重载）往socket中异步写入数据
    qint64 TcpServerSocketCore::replySendData(
            TcpHeaderFrameHelper::MessageType messageType, qint32 featureCode,qint32 sourceFeatureCode,  const QString &msg)
    {
        QTextCodec *codec = QTextCodec::codecForName("GB18030");
        auto bytes(codec->fromUnicode(msg));
        
        return replySendData(messageType,featureCode,sourceFeatureCode, bytes);
    }
    
    /// 发送数据的槽函数，sourceFeatureCode为源的index
    qint64 TcpConnectionHandler::sendMessage(TcpHeaderFrameHelper::MessageType messageType, qint32 sourceFeatureCode, const QByteArray &bytes)
    {
        // 防止广播信息到自身
        if (sourceFeatureCode == getAllocateIndex())
            return 0;
        return socket->replySendData(messageType, getAllocateIndex(),sourceFeatureCode, bytes);
    }
    
     /// （重载）发送数据的槽函数，sourceFeatureCode为源的index
    qint64 TcpConnectionHandler::sendMessage(TcpHeaderFrameHelper::MessageType messageType, qint32 sourceFeatureCode, const QString &msg)
    {
        // 防止广播信息到自身
        if (sourceFeatureCode == getAllocateIndex())
            return 0;
        return socket->replySendData(messageType, getAllocateIndex(),sourceFeatureCode, msg);
    }
    
    /// 初始化TcpConnectionHandler的所有部件，注意此方法被调用时应该在非server线程上
    void TcpConnectionHandler::initConnection(qintptr target)
    {
        qRegisterMetaType<qintptr>("qintptr");
        qRegisterMetaType<QHostAddress>("QHostAddress");
        qRegisterMetaType<QAbstractSocket::SocketType>("QAbstractSocket::SocketType");
        qRegisterMetaType<TcpHeaderFrameHelper::MessageType>("TcpHeaderFrameHelper::MessageType");
        socket.reset(new TcpServerSocketCore, TCPSocketDeleter());
        //实现广播
        connect(socket.data(),SIGNAL(socketMessageBoardcast(TcpHeaderFrameHelper::MessageType,const QByteArray &)),
                this, SLOT(replyBoardcastMessage(TcpHeaderFrameHelper::MessageType,  const QByteArray &)));
        connect(socket.data(),SIGNAL(error(QAbstractSocket::SocketError)), this,SLOT(handleErrors(QAbstractSocket::SocketError)));
        connect(socket.data(),&TcpServerSocketCore::clientRemarkUpdate,this, &TcpConnectionHandler::replyclientRemarkUpdate);
        connect(socket.data(),&TcpServerSocketCore::loginChecked, this, &TcpConnectionHandler::handleLoginChecked);
        
        if(!socket->setSocketDescriptor(target))
            handleErrors(socket->error());
        setReturnInform();
        
        pulseTimer.reset(new QTimer);
        connect(pulseTimer.data(), &QTimer::timeout, this, &TcpConnectionHandler::replyPulseTimeOut);
        pulseTimer->start(PULSEINTERVAL);
        
        loginCheckTimer.reset(new QTimer);
        loginCheckTimer->singleShot(LOGINCHECK_TIMEOUT, this, SLOT(handleLoginTimeout()));
               
        _secondsCounter.reset(new QTimer);
        _interval = 1000;
        connect(_secondsCounter.data(),&QTimer::timeout, this, &TcpConnectionHandler::on_secondsCounter_timeout);
        _secondsCounter->start(_interval);
    }
    
    /// 响应socket自身的秒钟超时
    void TcpConnectionHandler::on_secondsCounter_timeout()
    {
        connectionTime += _interval / 1000;
        
        //更新时间
        UpdatePulseProducer *updatePulseProducer = new UpdatePulseProducer(getAllocateIndex(),connectionTime);
        connect(updatePulseProducer,&QThread::finished, updatePulseProducer, &QThread::deleteLater);
        updatePulseProducer->start();
    }
    
    /// 封装socket发出的广播信号
    void TcpConnectionHandler::replyBoardcastMessage(TcpHeaderFrameHelper::MessageType messageType, const QByteArray &bytes)
    {
        qint32 index = getAllocateIndex();
        emit requestBoardcastMessage(messageType, index ,bytes);
    }
    
    /// 异步更新目标连接的签名
    //TODO: 这里可以改用json
    void TcpConnectionHandler::replyclientRemarkUpdate(const QString &remark)
    {
        UpdateRemarkProducer *updateRemarkProducer = new UpdateRemarkProducer(getAllocateIndex(), remark);
        connect(updateRemarkProducer, &QThread::finished, updateRemarkProducer,&QThread::deleteLater);
        updateRemarkProducer->start();
    }
    
    /// 目标连接的登陆超时
    void TcpConnectionHandler::handleLoginTimeout()noexcept
    {
        if(!bLoginCheckIn)
        {
            loginCheckTimer->stop();
            loginCheckTimer.reset();
            emit socketDisconneted(getAllocateIndex(), _thisAllocateIndex);//超过登陆确认间隔，关闭套接字
        }
    }
    
    /// 目标连接的登陆按时响应
    void TcpConnectionHandler::handleLoginChecked()
    {
        bLoginCheckIn = true;
        loginCheckTimer->stop();
        loginCheckTimer.reset();
    }
    
    /// 响应心跳包的更改
    void TcpConnectionHandler::replyResetPulse(qint32 pulseInterval)
    {
        pulseTimer->stop();
        pulseTimer->setInterval(pulseInterval*1000);
        
        if(pulseInterval)
            pulseTimer->start();
    }
    
    /// 响应心跳包超时
    void TcpConnectionHandler::replyPulseTimeOut()noexcept
    {
        //qDebug() << "Connection: in thread " <<  QThread::currentThreadId();
        auto time(QDateTime::currentDateTime());
        auto ret(time.toString());
        QString replyMsg(QString::fromLocal8Bit("@Server - keep-alive: ") + ret + "\n");
        
        socket->replySendData(TcpHeaderFrameHelper::MessageType::PulseFacility, getAllocateIndex(), getAllocateIndex(), replyMsg);
    }
    
    /// 处理连接错误
    void TcpConnectionHandler::handleErrors(QAbstractSocket::SocketError error)
    {
        QString errorString = QString::fromLocal8Bit("####SocketError:") + getSocketErrorType(error) + QString::fromLocal8Bit("\n");
        QTextCodec *codec = QTextCodec::codecForName("GB18030");
        QByteArray datagram(codec->fromUnicode(errorString));
        
        //注意广播是下线信息
        emit requestBoardcastMessage(TcpHeaderFrameHelper::MessageType::DeviceLogOut,getAllocateIndex(),std::move(datagram));
        
        //断开连接
        pulseTimer->stop();
        emit socketDisconneted(getAllocateIndex(),_thisAllocateIndex);
    }
    
    /// 返回登陆信息
    //TODO: 可以与登陆响应合并
    void TcpConnectionHandler::setReturnInform()
    {
        //回显消息
        auto peerAddress(socket->peerAddress());
        auto connectioType(socket->socketType());
        auto port(socket->peerPort());
        auto threadId(QString::number((qintptr)QThread::currentThreadId()));
        
        QString addString(peerAddress.toString());
        QString msg(QString::fromLocal8Bit("####系统消息：产生了一个新连接,来自：%1 端口: %2\n")
                    .arg(addString).arg(QString::number(socket->peerPort())));
        
        //特征码就是index
        socket->replySendData(TcpHeaderFrameHelper::MessageType::PlainMessage,getAllocateIndex(),getAllocateIndex(), msg);
        QTextCodec *codec = QTextCodec::codecForName("GB18030");
        QByteArray datagram(codec->fromUnicode(msg));
        
        emit requestBoardcastMessage(TcpHeaderFrameHelper::MessageType::PlainMessage,getAllocateIndex(),datagram);
        emit sendConnectionInform(_thisAllocateIndex,std::move(peerAddress), connectioType, std::move(threadId), port);
    }    
}








