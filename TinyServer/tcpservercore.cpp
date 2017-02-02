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
    qint32 getRawIndex()
    {
        static bool bSrandSet = false;
        if (!bSrandSet)
        {
            QTime time = QTime::currentTime();
            qsrand(time.msec()+time.second()*1000);
            bSrandSet = true;
        }
        
        return qrand() % 65536;
    }
    
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
    
    /****************************************************************************
     *IPV6点分十进制转化方法
     ****************************************************************************/
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
    
    TcpServerListendCore::TcpServerListendCore(QObject *parent)
        :QTcpServer(parent)
    {
        
    }
    
    /****************************************************************************
     *当一个socket读到信号时，将把信息从端口广播到所有的socket
     ****************************************************************************/
    void TcpServerListendCore::replyBoardcastMessage(TcpHeaderFrameHelper::MessageType messageType, qint32 indexExcepted, const QByteArray &bytes)
    {
        //发送信息更新客户端的view,可能会失败，如果失败那就在UI线程里面再更新一次
        if(messageType != TcpHeaderFrameHelper::MessageType::DeviceLogIn)
        {
            QString userName = getItemName(&connectionMap,indexExcepted);
            emit updateServer(messageType, bytes,std::move(userName), port);
        }
        emit requestSendData(messageType, indexExcepted, bytes);
    }
    
    /****************************************************************************
     *当存在一个新连接时引发此槽，如果可以正确设置socket的描述符，那么*异步*地将handler存入端口的connectionMap中
     ****************************************************************************/
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
        
        connect(this, &TcpServerListendCore::requestGetConnectionInform, connection, &TcpConnectionHandler::replyGetConnectionInform);
        connect(connection, &TcpConnectionHandler::sendConnectionInform, this, &TcpServerListendCore::replyUpdateTcpInform);
        connect(countClock, &SecondCountClock::timeOutInvoke, connection, &TcpConnectionHandler::replyClockTimeOut);
        connect(this, &TcpServerListendCore::requestResetPulse,connection, &TcpConnectionHandler::replyResetPulse);
        
        connection->moveToThread(workerThread);
        workerThread->start();
        QMetaObject::invokeMethod(connection,"initConnection",Qt::QueuedConnection, Q_ARG(qintptr,socketDescriptor));
        
        addItem(&connectionMap,index,connection);
    }
    
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
    
    void TcpServerListendCore::replyResetPulse(qint32 pulseInterval)
    {
        timeInterval = pulseInterval;   
        emit requestResetPulse(pulseInterval);
    }
    
    void TcpServerListendCore::replyUpdateTcpInform(const qint32 index, 
                                                    const QHostAddress &address,
                                                    const QAbstractSocket::SocketType type, 
                                                    const QString &threadId, 
                                                    const quint16 peerPort)
    {
        InsertItemProducer *insertItemProducer 
                = new InsertItemProducer(index,std::move(address),std::move(type),std::move(threadId), peerPort, port);
        
        connect(insertItemProducer, &QThread::finished, insertItemProducer,&QThread::deleteLater);
        insertItemProducer->start();
    }
    
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
    
    bool TcpServerListendCore::startListening(quint16 targetPort)
    {
        if(!listen(QHostAddress::Any, targetPort))
        {
            QString errorStr = this->errorString();
            emit updateServer(errorStr);
            
            return false;
        }
        
        port = targetPort;
        countClock = new SecondCountClock(this);
        return true;
    }
    
    void TcpServerListendCore::deleteConnection(const qint32 targetIndex)
    {
        removeItem(&connectionMap,targetIndex);
        replyRemoveTarget(targetIndex);
    }
    
    bool TcpServerListendCore::replyTestConnection(qintptr target,const QString &message)
    {
        return testConnection(&connectionMap,target,message);
    }
    
    TcpServerSocketCore::TcpServerSocketCore(QObject *parent)
        :QTcpSocket(parent)
    {
        connect(this,&TcpServerSocketCore::readyRead,  this, &TcpServerSocketCore::dataReceiver);
    }
    
    bool TcpServerSocketCore::setSocketDescriptor(qintptr descriptor, SocketState state, OpenMode openMode)
    {
        socketDescriptor = descriptor;
        return QTcpSocket::setSocketDescriptor(socketDescriptor,state,openMode);
    }
    
    void TcpServerSocketCore::dataReceiver()noexcept
    {
        QByteArray bytes, realDataBytes, bytesTmp;
        bytes.resize(MAXBUFSIZE);
        TcpHeaderFrameHelper::TcpHeaderFrame headerFrame;
        
        bytes.resize(0);
        while (bytesAvailable() != 0)
        {
            auto newBufferSize = bytesAvailable() < MAXBUFSIZE
                    ? static_cast<int>(bytesAvailable()): MAXBUFSIZE;
            bytesTmp.resize(newBufferSize);
            read(bytesTmp.data(),bytesTmp.size());
            
            bytes += bytesTmp;
        }
        
        TcpHeaderFrameHelper::praseHeaderAndDatagram(bytes,headerFrame,realDataBytes);
        
        if (headerFrame.messageType == (int)TcpHeaderFrameHelper::MessageType::DeviceLogIn)
        {        
            QTextCodec *codec = QTextCodec::codecForName("GB18030");
            QTextDecoder *decoder = codec->makeDecoder();
            
            //把登陆信息直接设定为用户名记录
            userName = decoder->toUnicode(realDataBytes);
            emit loginChecked();
            emit clientRemarkUpdate(userName);
            emit socketMessageBoardcast(TcpHeaderFrameHelper::MessageType::DeviceLogIn, realDataBytes);
            
            delete decoder;
        }
        else if(headerFrame.messageType == (int)TcpHeaderFrameHelper::MessageType::PulseFacility)
        {
            
        }
        else if(headerFrame.messageType == (int)TcpHeaderFrameHelper::MessageType::PlainMessage
                ||headerFrame.messageType == (int)TcpHeaderFrameHelper::MessageType::ServerTest)
        {
            emit socketMessageBoardcast(TcpHeaderFrameHelper::MessageType::PlainMessage, realDataBytes);
            //一个客户端的套接字的信息发生改变，立马广播到服务器和其他客户端
        }
        else if(headerFrame.messageType == (int)TcpHeaderFrameHelper::MessageType::Coordinate)
        {
            emit socketMessageBoardcast(TcpHeaderFrameHelper::MessageType::Coordinate, realDataBytes);
        }
    }
    
    qint64 TcpServerSocketCore::replySendData(
            TcpHeaderFrameHelper::MessageType messageType, qint32 featureCode, qint32 sourceFeatureCode,  const QByteArray &bytes)
    {
        qint64 ret = -1;
        
        TcpHeaderFrameHelper::TcpHeaderFrame header;
        header.messageType = (int)messageType;
        header.featureCode = featureCode;
        header.sourceFeatureCode = sourceFeatureCode;
        header.messageLength = bytes.size();      
        
        QByteArray packingBytes =TcpHeaderFrameHelper::bindHeaderAndDatagram(header,bytes);
        
         //网络IO本身在系统层就是异步的，不要锁                                          
        ret = write(packingBytes,packingBytes.size());
        flush();
        return ret;
    }
    
    qint64 TcpServerSocketCore::replySendData(
            TcpHeaderFrameHelper::MessageType messageType, qint32 featureCode,qint32 sourceFeatureCode,  const QString &msg)
    {
        QTextCodec *codec = QTextCodec::codecForName("GB18030");
        auto bytes(codec->fromUnicode(msg));
        
        return replySendData(messageType,featureCode,sourceFeatureCode, bytes);
    }
    
    qint64 TcpConnectionHandler::sendMessage(TcpHeaderFrameHelper::MessageType messageType, qint32 sourceFeatureCode, const QByteArray &bytes)
    {
        if (sourceFeatureCode == getAllocateIndex())
            return 0;
        return socket->replySendData(messageType, getAllocateIndex(),sourceFeatureCode, bytes);
    }
    
    qint64 TcpConnectionHandler::sendMessage(TcpHeaderFrameHelper::MessageType messageType, qint32 sourceFeatureCode, const QString &msg)
    {
        if (sourceFeatureCode == getAllocateIndex())
            return 0;
        return socket->replySendData(messageType, getAllocateIndex(),sourceFeatureCode, msg);
    }
    
    void TcpConnectionHandler::initConnection(qintptr target)
    {
        qRegisterMetaType<qintptr>("qintptr");
        qRegisterMetaType<QHostAddress>("QHostAddress");
        qRegisterMetaType<QAbstractSocket::SocketType>("QAbstractSocket::SocketType");
        qRegisterMetaType<TcpHeaderFrameHelper::MessageType>("TcpHeaderFrameHelper::MessageType");
        
        pulseTimer.reset(new QTimer);
        socket.reset(new TcpServerSocketCore, TCPSocketDeleter());
        
        //实现广播
        connect(socket.data(),SIGNAL(socketMessageBoardcast(TcpHeaderFrameHelper::MessageType,const QByteArray &)),
                this, SLOT(replyBoardcastMessage(TcpHeaderFrameHelper::MessageType,  const QByteArray &)));
        connect(socket.data(),SIGNAL(error(QAbstractSocket::SocketError)), this,SLOT(handleErrors(QAbstractSocket::SocketError)));
        connect(socket.data(),&TcpServerSocketCore::clientRemarkUpdate,this, &TcpConnectionHandler::replyclientRemarkUpdate);
        
        if(!socket->setSocketDescriptor(target))
            handleErrors(socket->error());
        setReturnInform();
        
        pulseTimer.reset(new QTimer);
        loginCheckTimer.reset(new QTimer);
        connect(pulseTimer.data(), &QTimer::timeout, this, &TcpConnectionHandler::replyPulseTimeOut);
        connect(socket.data(),&TcpServerSocketCore::loginChecked, this, &TcpConnectionHandler::handleLoginChecked);
        
        pulseTimer->start(PULSEINTERVAL);       
        loginCheckTimer->singleShot(LOGINCHECK_TIMEOUT, this, SLOT(handleLoginTimeout()));
    }
    
    void TcpConnectionHandler::replyBoardcastMessage(TcpHeaderFrameHelper::MessageType messageType, const QByteArray &bytes)
    {
        qint32 index = getAllocateIndex();
        emit requestBoardcastMessage(messageType, index ,bytes);
    }
    void TcpConnectionHandler::replyClockTimeOut(qint32 interval)
    {
        connectionTime += interval / 1000;
        
        //更新时间
        UpdatePulseProducer *updatePulseProducer = new UpdatePulseProducer(getAllocateIndex(),connectionTime);
        connect(updatePulseProducer,&QThread::finished, updatePulseProducer, &QThread::deleteLater);
        updatePulseProducer->start();
    }
    
    void TcpConnectionHandler::replyclientRemarkUpdate(const QString &remark)
    {
        UpdateRemarkProducer *updateRemarkProducer = new UpdateRemarkProducer(getAllocateIndex(), remark);
        connect(updateRemarkProducer, &QThread::finished, updateRemarkProducer,&QThread::deleteLater);
        updateRemarkProducer->start();
    }
    
    void TcpConnectionHandler::handleLoginTimeout()noexcept
    {
        if(!bLoginCheckIn)
        {
            loginCheckTimer->stop();
            loginCheckTimer.reset();
            emit socketDisconneted(getAllocateIndex(), _thisAllocateIndex);//超过登陆确认间隔，关闭套接字
        }
    }
    
    void TcpConnectionHandler::handleLoginChecked()
    {
        bLoginCheckIn = true;
        loginCheckTimer->stop();
        loginCheckTimer.reset();
    }
    
    void TcpConnectionHandler::replyGetConnectionInform()
    {
        auto peerAddress(socket->peerAddress());
        auto connectioType(socket->socketType());
        auto port(socket->peerPort());
        auto threadId(QString::number((qintptr)QThread::currentThreadId()));
        
        emit sendConnectionInform(_thisAllocateIndex,std::move(peerAddress), connectioType, std::move(threadId), port);
    }
    
    void TcpConnectionHandler::replyResetPulse(qint32 pulseInterval)
    {
        pulseTimer->stop();
        pulseTimer->setInterval(pulseInterval*1000);
        
        if(pulseInterval)
            pulseTimer->start();
    }
    
    void TcpConnectionHandler::replyPulseTimeOut()noexcept
    {
        //qDebug() << "Connection: in thread " <<  QThread::currentThreadId();
        auto time(QDateTime::currentDateTime());
        auto ret(time.toString());
        QString replyMsg(QString::fromLocal8Bit("@Server - keep-alive: ") + ret + "\n");
        
        socket->replySendData(TcpHeaderFrameHelper::MessageType::PulseFacility, getAllocateIndex(), getAllocateIndex(), replyMsg);
    }
    
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
    
    void TcpConnectionHandler::setReturnInform()
    {
        //回显消息
        auto peerAddress(socket->peerAddress());
        QString addString(peerAddress.toString());
        QString msg(QString::fromLocal8Bit("####系统消息：产生了一个新连接,来自：%1 端口: %2\n")
                    .arg(addString).arg(QString::number(socket->peerPort())));
        
        //特征码就是Descriptor算了
        socket->replySendData(TcpHeaderFrameHelper::MessageType::PlainMessage,getAllocateIndex(),getAllocateIndex(), msg);
        QTextCodec *codec = QTextCodec::codecForName("GB18030");
        QByteArray datagram(codec->fromUnicode(msg));
        
        emit requestBoardcastMessage(TcpHeaderFrameHelper::MessageType::PlainMessage,getAllocateIndex(),datagram);
        replyGetConnectionInform();
    }
    
    TcpConnectionHandler::~TcpConnectionHandler()
    {
        //1. 析构对象(可以先把对象从组里面挖掉以免对象被析构之前被获取)
        //2. 线程quit
    }
    
    SecondCountClock::SecondCountClock(QObject *parent)
        :QObject (parent)
        ,clockTimer(new QTimer)
    {
        clockTimer->start(timeInterval);
        connect(clockTimer.data(), &QTimer::timeout, this, &SecondCountClock::replyTimeout);
    }
    
    void SecondCountClock::replyTimeout()
    {
        emit timeOutInvoke(timeInterval);
    }
    

}








