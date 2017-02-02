#include <QString>
#include <QRegExp>
#include <QHostInfo>
#include <QPrinter>
#include <QFileDialog>
#include <QTextDocument>
#include <QAction>
#include <QTreeWidgetItem>
#include <QTextCodec>
#include <QTextCursor>
#include <QReadWriteLock>
#include <QMutex>
#include <QIcon>
#include <QImage>
#include <QPixmap>
#include <QSettings>
#include <QRegExp>
#include <QApplication>
#include <QMessageBox>
#include <QSettings>
#include <QFile>
#include <QDataStream>
#include <QNetworkInterface>
#include <QPair>
#include <QClipboard>
#include <QProgressDialog>
#include <QDateTime>
#include <QTime>
#include <algorithm>

#include "tcpserverwindow.h"
#include "servertrayicon.h"
#include "headerframe_helper.h"

namespace MainWindows
{
    TcpServerWindow::TcpServerWindow(QWidget *parent)
        : QMainWindow(parent)
        , trayIcon(new TrayIconManager(QIcon(QString::fromLocal8Bit(":/images/icon.png")),this))
    {
        setupUi(this);
        setAttribute(Qt::WA_DeleteOnClose);
        setWindowIcon(QIcon(QString::fromLocal8Bit(":/images/icon.png")));
        setWindowTitle(QString::fromLocal8Bit(TOOLTIP));
        resize(defWidth,defHeight);
        
        serverWindow::msgView->setReadOnly(true);
        
        setLocalAddress();
        initConnectionList();
        
        //安装事件处理器
        msgView->installEventFilter(this);
        connectionsView->installEventFilter(this);
        
        createConnections();
    }
    
    bool TcpServerWindow::eventFilter(QObject *target, QEvent *event)
    {
        if(target == connectionsView && event->type() == QEvent::KeyPress)
        {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if(keyEvent->key() == Qt::Key_C && keyEvent->modifiers() == Qt::ControlModifier)
                connectionsView->replyCopySelectedInformAct();
            return true;
            //不能再让他响应其他eventFilter，不然会响应treeWideget自己的eventFilter，复制会失败
        }
        return QObject::eventFilter(target,event);
    }
    
    void TcpServerWindow::setLocalAddress()
    {
        localAddressEdit->setReadOnly(true);
        QHostInfo host = QHostInfo::fromName(QHostInfo::localHostName());
        
        for(const auto &i:host.addresses())
           if(i.protocol() == QAbstractSocket::IPv4Protocol)
           {
               localAddressEdit->setText(i.toString());
               break;
           }
    }
    
    void TcpServerWindow::moveNewCursorPostion()
    {
        QTextCursor cursor = msgView->textCursor();
        cursor.movePosition(QTextCursor::End);
        msgView->setTextCursor(cursor);
    }
    
    void TcpServerWindow::createConnections()
    {
        connect(createNewServerAction, &QAction::triggered, this,&TcpServerWindow::openPortEditor);
        connect(saveAsPdfAction, &QAction::triggered, this,&TcpServerWindow::saveAsPdfFile);
        connect(saveMsgBtn, &QPushButton::clicked, this,&TcpServerWindow::saveAsPdfFile);
        connect(clearScreenBtn,&QPushButton::clicked, this,&TcpServerWindow::clearScreen);
        connect(msgView, &QTextEdit::textChanged, this,&TcpServerWindow::moveNewCursorPostion);
        connect(allowTaryAct, &QAction::triggered, this,&TcpServerWindow::replyIsAllowSystemTray);
        connect(allowMinimizeToTrayAct, &QAction::triggered, this,&TcpServerWindow::replyIsAllowMinimizeToTray);
        connect(closeAct,&QAction::triggered, this,&TcpServerWindow::close);
        connect(exitServerAct,&QAction::triggered, this,&TcpServerWindow::replyExit);
        connect(pulseEditAct,&QAction::triggered, this,&TcpServerWindow::replypulseSettingTriggered);
        connect(saveCurrentSettingAct,&QAction::triggered, this,&TcpServerWindow::writeSettings);
        connect(readAndStartAct,&QAction::triggered, this,&TcpServerWindow::readSettings);
        connect(autoStartAct,&QAction::triggered, this,&TcpServerWindow::replyAutoStartActTriggered);
        connect(openPortListBtn,&QPushButton::clicked, this,&TcpServerWindow::openPortList);
        connect(aboutMeAct,&QAction::triggered, this,&TcpServerWindow::aboutMeDialog);
        connect(this, &TcpServerWindow::allServerCleared, this,&TcpServerWindow::createSettingsPort);
        connect(broadcastOpenAct,&QAction::triggered, this,&TcpServerWindow::replyAllowBroadcastTriggered);
        
        connect(trayIcon,&TrayIconManager::requestOpenListenedPort,this,&TcpServerWindow::openPortEditor);
        connect(trayIcon,&TrayIconManager::requestShowMainWindow,this,&TcpServerWindow::show);
        connect(trayIcon,&TrayIconManager::requestExit,this,&TcpServerWindow::replyExit);
        connect(trayIcon,&TrayIconManager::showMainWindow, this,&TcpServerWindow::show);
        
        //把部件的这两个信号接出来，用来解耦
        connect(connectionsView,&ConnectionView::requestDisconnection, this, &TcpServerWindow::replyDisconnection);
        connect(connectionsView,&ConnectionView::requestTestConnection, this, &TcpServerWindow::replyTestConnection);
        connect(connectionsView, &ConnectionView::newConnectionComming,this, &TcpServerWindow::updateConnectionList);
    }
    
    void TcpServerWindow::writeSettings()
    {
        QSettings appBaseSettings(QString::fromLocal8Bit(SETTING_FILEPATH),QSettings::IniFormat);
        
        appBaseSettings.setValue("/ifAutoRuning",bAutoStart);
        appBaseSettings.setValue("/ifbAllowMinimizeToTray",bAllowMinimizeToTray);
        appBaseSettings.setValue("/ifbSystemTrayAccepted",bSystemTrayAccepted);
        appBaseSettings.setValue("/ifbAllowBroadcast",bAllowBroadCast);
        appBaseSettings.setValue("/serverMapcount",serverMap.count());
        
        auto portList(serverMap.keys());
        
        for(int i = 0; i!= portList.count();i++)
        {
            appBaseSettings.setValue(QString::fromLocal8Bit("/ServerPort%1:").arg(i),portList[i]);
            appBaseSettings.setValue(QString::fromLocal8Bit("/ServerPulse%1:").arg(i),serverMap[portList[i]]->pulseInterval());
        }
        conectionStatusBar->showMessage(QString::fromLocal8Bit("写入设置成功!"), 3000);
    }
    
    void TcpServerWindow::readSettingsPrivate()
    {
        QSettings appBaseSettings(QString::fromLocal8Bit(SETTING_FILEPATH),QSettings::IniFormat);
        
        bAutoStart = appBaseSettings.value("/ifAutoRuning").toBool();
        bAllowMinimizeToTray = appBaseSettings.value("/ifbAllowMinimizeToTray").toBool();
        bSystemTrayAccepted = appBaseSettings.value("/ifbSystemTrayAccepted").toBool();
        bAllowBroadCast = appBaseSettings.value("/ifbAllowBroadcast").toBool();
        
        settingsSumOfPort = appBaseSettings.value("/serverMapcount").toInt();
        
        for(qint32 i = 0; i!= settingsSumOfPort;i++)
        {
            settingsPortList.append(appBaseSettings.value(QString::fromLocal8Bit("/ServerPort%1:").arg(i)).toInt());
            settingsPulseList.append(appBaseSettings.value(QString::fromLocal8Bit("/ServerPulse%1:").arg(i)).toInt());
        }
        clearServerMap();
    }
    
    void TcpServerWindow::readSettings()
    {
        QFile file(QString::fromLocal8Bit(SETTING_FILEPATH));
        if(!file.open(QIODevice::ReadOnly))
        {
            QMessageBox::warning(this,QString::fromLocal8Bit("错误"),QString::fromLocal8Bit("配置文件%1不存在").arg(SETTING_FILEPATH));
            return;
        }
        readSettingsPrivate();
    }
    
    void TcpServerWindow::createSettingsPort()
    //这个函数相当于把所有的server都关闭了的回调函数
    {
        QProgressDialog progressDialog(this);
        progressDialog.setLabelText(QString::fromLocal8Bit("正在打开端口..."));
        progressDialog.setWindowTitle(QString::fromLocal8Bit("请稍后..."));
        progressDialog.setRange(0,settingsSumOfPort);
        progressDialog.setModal(true);
        progressDialog.setWindowIcon(QIcon(QString::fromLocal8Bit(":/images/icon.png")));
        
        conectionStatusBar->showMessage(QString::fromLocal8Bit("正在打开端口..."), INT_MAX);
        
        for(int i = 0; i!= settingsSumOfPort;i++)
        {
            progressDialog.setValue(i);
            qApp->processEvents();
            if(progressDialog.wasCanceled())
            {
                QMessageBox::information(this,QString::fromLocal8Bit("打开端口"),QString::fromLocal8Bit("无法打开设置中全部端口"));
                conectionStatusBar->showMessage(QString::fromLocal8Bit("无法打开设置中全部端口"), 3000);
            }
            
            quint16 port = settingsPortList.at(i);
            if (createNewServer(port))
                QMetaObject::invokeMethod(serverMap[port],"setPulseInterval",
                                          Qt::BlockingQueuedConnection, Q_ARG(int, settingsPulseList.at(i)));
        }
        conectionStatusBar->showMessage(QString::fromLocal8Bit("打开设置成功"), 3000);
        bDeletingAllServer = 0;
        
        settingsPortList.clear();
        settingsPulseList.clear();
        settingsSumOfPort = 0;
    }
    
    bool TcpServerWindow::clearServerMap()
    {
        bDeletingAllServer = 1;
        auto sumOfCurPort(serverMap.count());
        
        QProgressDialog progressDialog(this);
        progressDialog.setLabelText(QString::fromLocal8Bit("正在关闭端口..."));
        progressDialog.setRange(0,sumOfCurPort);
        progressDialog.setModal(true);
        progressDialog.setWindowTitle(QString::fromLocal8Bit("请稍后..."));
        progressDialog.setWindowIcon(QIcon(QString::fromLocal8Bit(":/images/icon.png")));
        
        int i = 0;
        auto serverList = serverMap.values();
        
        if(serverList.isEmpty())
            createSettingsPort();
        else
        {
            for (const auto &server : serverList)
            {
                unsigned short port;
                QMetaObject::invokeMethod(server, "getCurrentPort", Qt::BlockingQueuedConnection, Q_RETURN_ARG(unsigned short,port));
                
                replyCloseListened(server);
                progressDialog.setValue(++i);
                if(progressDialog.wasCanceled())
                {
                    conectionStatusBar->showMessage(QString::fromLocal8Bit("用户取消操作"), 3000);
                    return false;
                }
                conectionStatusBar->showMessage(QString::fromLocal8Bit("正在关闭端口..."), INT_MAX);
                qApp->processEvents();
            }
        }
        
        conectionStatusBar->clearMessage();
        return true;
    }
    
    void TcpServerWindow::replyAutoStartActTriggered()
    {
        bAutoStart = !bAutoStart;
        autoStartAct->setChecked(bAutoStart);
        
        QString appName = QApplication::applicationName();
        QSettings regSettings(REG_RUN,QSettings::NativeFormat);
        if(!bAutoStart)
            regSettings.remove(appName);
        else
        {
            QString appPath = QApplication::applicationFilePath();
            regSettings.setValue(appName,appPath.replace("/","\\"));
        }
    }
    
    void TcpServerWindow::replyAllowBroadcastTriggered()
    {
        bAllowBroadCast = !bAllowBroadCast;
        broadcastOpenAct->setChecked(bAllowBroadCast);
    }
    
    void TcpServerWindow::initConnectionList()
    {
        connectionsView = new ConnectionView(this);
        connectionsView->startConnectionViewWatcher();
        splitter->insertWidget(1,connectionsView);
    }
    
    void TcpServerWindow::clearScreen()
    {
        msgView->clear();
    }
    
    void TcpServerWindow::aboutMeDialog()
    {
        QMessageBox::about(this,QString::fromLocal8Bit(TOOLTIP),
                           QString::fromLocal8Bit("<html>"
                              "<head>"
                                  "<title>Lists</title>"
                              "</head>"
                                  "<h2>Philip</h2>"
                                  "<p>如程序中有Bug，欢迎来骚扰OwO</p>"
                                  "<ul>"
                                      "<li>email :2173454740@qq.com</li>"
                                      "<li>QQ: 2173454740</li>"
                                  "</ul>"
                              "</body>"
                         "</html>"));
    }
    
    void TcpServerWindow::openPortEditor()
    {
        QSharedPointer<OpenPortEditor> portInpuDialog(new OpenPortEditor);
        
        if(portInpuDialog->exec())
        {
            auto port(portInpuDialog->portEdit->text().toUShort());
            
            if(serverMap.find(port) != serverMap.end())
                QMessageBox::information(this,QString::fromLocal8Bit("服务器"),QString::fromLocal8Bit("端口已经监听"));
            else
                createNewServer(port);
        }
    }
    
    void TcpServerWindow::updateView(TcpHeaderFrameHelper::MessageType messageType, const QByteArray &bytes,const QString &userName, quint16 port)
    {
        TcpServerListendCore *source = qobject_cast<TcpServerListendCore *>(sender());
        
        QTextCodec *codec = QTextCodec::codecForName("GB18030");
        QTextDecoder *decoder = codec->makeDecoder();
        
        QString msg;
        
        msg += QTime::currentTime().toString() + QString::fromLocal8Bit("  监听端口: %1\n").arg(port);
        if (!userName.isEmpty())
            msg += userName + ": ";
        msg += decoder->toUnicode(bytes) + "\n";
        
        updateView(msg);
        delete decoder;
        
        qRegisterMetaType<TcpHeaderFrameHelper::MessageType>("TcpHeaderFrameHelper::MessageType");
        qRegisterMetaType<QByteArray>("QByteArray");
        
        if (bAllowBroadCast)
        {
            auto serverList = serverMap.values();
            for(const auto &server : serverList)
            {
                if ( server == source)
                    continue;
                QMetaObject::invokeMethod(server, "requestSendData",Qt::QueuedConnection,
                                          Q_ARG(TcpHeaderFrameHelper::MessageType,messageType),
                                          Q_ARG(qint32,-1),
                                          Q_ARG(QByteArray,bytes));
            }
        }
    }
    
    void TcpServerWindow::updateView(const QString &msg)
    {
        msgView->insertPlainText(msg);
        msgView->moveCursor(QTextCursor::End);
        msgView->update();
    }
    
    bool TcpServerWindow::createNewServer(const unsigned short port)
    {
        qRegisterMetaType<QHostAddress>("QHostAddress");
        qRegisterMetaType<QAbstractSocket::SocketType>("QAbstractSocket::SocketType");
        qRegisterMetaType<quint16>("quint16");
        QThread *workerThread = new QThread;
        
        connect(workerThread, &QThread::finished, workerThread, &QThread::quit);
        connect(workerThread, &QThread::finished, workerThread, &QThread::deleteLater);
        
        TcpServerListendCore *newServer = new TcpServerListendCore;
        
        connect(newServer,SIGNAL(updateServer(TcpHeaderFrameHelper::MessageType, const QByteArray &,const QString &, quint16)),
                this,SLOT(updateView(TcpHeaderFrameHelper::MessageType, const QByteArray &,const QString&,quint16)));
        connect(newServer,SIGNAL(updateServer(const QString &)),
                this,SLOT(updateView(const QString&)));
        
        connect(this, &TcpServerWindow::requestDisconnection, newServer,&TcpServerListendCore::deleteConnection);
        connect(this, &TcpServerWindow::requestResetPulse,newServer,&TcpServerListendCore::replyResetPulse);
        connect(newServer,&TcpServerListendCore::allDisConnected,this,&TcpServerWindow::replyAllDisConnected);
        
        newServer->moveToThread(workerThread);
        workerThread->start();
        bool isListenedOk = false;
        QMetaObject::invokeMethod(newServer,"startListening",Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, isListenedOk), Q_ARG(quint16,port));
        
        if (isListenedOk)
        {
            QString msg(QString::fromLocal8Bit("####终端: 服务器正在监听端口%1\n").arg(port));
            updateView(msg);
            serverMap.insert(port,newServer);
            
            emit newServerStarted(QString::fromLocal8Bit("TCP"),port);
        }
        return isListenedOk;
    }
    
    void TcpServerWindow::saveAsPdfFile()
    {
        QString filePath(QFileDialog::getSaveFileName(this,
                                                      QString::fromLocal8Bit("另存为"),
                                                      QString::fromLocal8Bit("/"),
                                                      QString::fromLocal8Bit("pdf File(*.pdf)")));
        if(filePath.isEmpty())
            return ;
        
        QPrinter printer;
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(filePath);
        
        auto docunment(msgView->document());
        docunment->print(&printer);
    }
    
    void TcpServerWindow::updateConnectionList(const QString &address)
    {
        conectionStatusBar->showMessage(QString::fromLocal8Bit("新连接") + address , 3000);
    }
    
    void TcpServerWindow::closeEvent(QCloseEvent *event)
    {
        if (bSystemTrayAccepted && bAllowMinimizeToTray)
        {
            hide();
            event->ignore();
        }
        else
            event->accept();
    }
    
    void TcpServerWindow::replyIsAllowSystemTray()
    {
        bSystemTrayAccepted = !bSystemTrayAccepted;
        allowTaryAct->setChecked(bSystemTrayAccepted);
        
        if(bSystemTrayAccepted)
            trayIcon->show();
        else
            trayIcon->hide();
    }
    
    void TcpServerWindow::replyIsAllowMinimizeToTray()
    {
        bAllowMinimizeToTray = !bAllowMinimizeToTray;
        allowMinimizeToTrayAct->setChecked(bAllowMinimizeToTray);
    }
    
    void TcpServerWindow::replypulseSettingTriggered()
    {
        QScopedPointer<SettingPulseEditor> editor(new SettingPulseEditor);
        
        if(editor->exec())
        {
            qint32 newInterval = editor->pulseEdit->text().toInt();
            emit requestResetPulse(newInterval);
        }
    }
    
    void TcpServerWindow::replyDisconnection(quint16 port, qint32 target)
    {
        qRegisterMetaType<qintptr>("qintptr");
        qRegisterMetaType<TcpConnectionHandler *>("TcpConnectionHandler *"); 
                
        if (serverMap.contains(port))
        {
            QMetaObject::invokeMethod(serverMap[port],"deleteConnection",Qt::QueuedConnection,
                                      Q_ARG(qint32,target));
        }
    }
    
    void TcpServerWindow::replyTestConnection(quint16 port, qintptr target, const QString &message)
    {
        qRegisterMetaType<quint16>("quint16");
        qRegisterMetaType<qintptr>("qintptr");
        qRegisterMetaType<TcpConnectionHandler *>("TcpConnectionHandler *"); 
        
        if (serverMap.contains(port))
        {
            bool testOk = false;
            QMetaObject::invokeMethod(serverMap[port],"replyTestConnection",Qt::BlockingQueuedConnection,
                                      Q_RETURN_ARG(bool, testOk),
                                      Q_ARG(qintptr,target),
                                      Q_ARG(const QString &,message));
            if (!testOk)
                QMessageBox::warning(this,QString::fromLocal8Bit("服务器测试"),QString::fromLocal8Bit("给指定链接发送信息失败"));
        }
    }
    
    void TcpServerWindow::openPortList()
    {
        if(bDeletingServer)
            return;
        bDeletingServer = 1;//防止点击过快引发的程序崩溃问题！
        if(listenedPortListView)
            listenedPortListView->close();
        listenedPortListView.reset();
        listenedPortListView.reset(new ListendPortDialog);
        
        connect(this, &TcpServerWindow::sendServerList,
                listenedPortListView.data(), &ListendPortDialog::replySendedServerList);
        
        connect(listenedPortListView.data(), SIGNAL(closeListened(quint16)),
                this,SLOT(replyCloseListened(quint16)));
        connect(listenedPortListView.data(), &ListendPortDialog::pauseListened,
                this,&TcpServerWindow::replyPauseListened);
        connect(listenedPortListView.data(), &ListendPortDialog::wakeUpListened,
                this,&TcpServerWindow::replyWakeUpListened);
        
        connect(this, &TcpServerWindow::serverClosed,
                listenedPortListView.data(), &ListendPortDialog::deleteClosedServer);
        connect(this, &TcpServerWindow::serverPaused,
                listenedPortListView.data(), &ListendPortDialog::remarkPausedServer);
        connect(this, &TcpServerWindow::serverWakeUp,
                listenedPortListView.data(), &ListendPortDialog::remarkWakeUpServer);
        
        connect(this, &TcpServerWindow::newServerStarted,
                listenedPortListView.data(), &ListendPortDialog::appendNewServerToList);
        
        listenedPortListView->show();
        emit sendServerList(serverMap.values());
        
        bDeletingServer = 0;
    }
    
    void TcpServerWindow::replyCloseListened(quint16 port)
    {
        QString msg(QString::fromLocal8Bit("####终端：端口%1关闭\n").arg(port));
        TcpServerListendCore *targetServer = serverMap.find(port).value();
        
        //必须关，不能再让server收信号了，否则会严重错误
        QMetaObject::invokeMethod(targetServer,"stopListening",Qt::BlockingQueuedConnection);
        QMetaObject::invokeMethod(targetServer,"endAllConnection",Qt::QueuedConnection);
        
        emit serverClosed(port);//dialog的内容一定会被立即删除，但是主窗口的那一堆连接删除有延迟
        
        updateView(msg);
    }
    
    void TcpServerWindow::replyCloseListened(TcpServerListendCore *server)
    {
        unsigned short port;
        qDebug() << QMetaObject::invokeMethod(server, "getCurrentPort", Qt::BlockingQueuedConnection, Q_RETURN_ARG(unsigned short,port));
        QString msg(QString::fromLocal8Bit("####终端：端口%1关闭\n").arg(port));
        emit serverClosed(port);
        
        QMetaObject::invokeMethod(server,"stopListening",Qt::BlockingQueuedConnection);
        QMetaObject::invokeMethod(server,"endAllConnection",Qt::QueuedConnection);
        
        updateView(msg);
    }
    
    void TcpServerWindow::replyAllDisConnected(quint16 port)
    {
        serverMap.remove(port);
        if (serverMap.isEmpty() && bDeletingAllServer)
            emit allServerCleared();
    }
    
    void TcpServerWindow::replyPauseListened(quint16 port)
    {
        QString msg(QString::fromLocal8Bit("####终端：端口%1暂停监听\n").arg(port));
        auto targetServer(serverMap.find(port));
                
        targetServer.value()->stopServer();
        emit serverPaused(port);
        emit updateView(msg);
    }
    
    void TcpServerWindow::replyWakeUpListened(quint16 port)
    {
        QString msg(QString::fromLocal8Bit("####终端：端口%1恢复监听\n").arg(port));
        auto targetServer(serverMap.find(port));
                
        targetServer.value()->wakeUpServer();
        emit serverWakeUp(port);
        emit updateView(msg);
    }

    ConnectionView::ConnectionView(QWidget *parent)
        :QTreeWidget(parent)
    {
        setAnimated(false);
        setAllColumnsShowFocus(true);
        setAlternatingRowColors(true);
        
        setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        setSelectionBehavior(QAbstractItemView::SelectRows);
        setSelectionMode(QAbstractItemView::ExtendedSelection);
        
        initConnectionListHeader();
        initContextMenu();
    }
    
    void ConnectionView::createContextMenu(const QPoint &pos)
    {
        if(!indexAt(pos).isValid())
            return;
        contextMenu->exec(QCursor::pos());                  
    }
    
    void ConnectionView::startConnectionViewWatcher()
    {
        _consumerHelper.reset(new ConsumerHelper(this));
        _consumerHelper->start();
    }
    
    void ConnectionView::initContextMenu()
    {
        setContextMenuPolicy(Qt::CustomContextMenu);//自定义右键菜单
        contextMenu.reset(new QMenu);
        connect(this,&ConnectionView::customContextMenuRequested, 
                this,&ConnectionView::createContextMenu);
        
        testConnectionAct = new QAction(QString::fromLocal8Bit("测试所选链接"),this);
        copyInformAct = new QAction(QString::fromLocal8Bit("复制链接信息"),this);
        deleteConnectionAct = new QAction(QString::fromLocal8Bit("断开此链接"),this);
        
        testConnectionAct->setIcon(QIcon(QString::fromLocal8Bit(":/images/test.png")));
        copyInformAct->setIcon(QIcon(QString::fromLocal8Bit(":/images/clipboard.png")));
        deleteConnectionAct->setIcon(QIcon(QString::fromLocal8Bit(":/images/closeBlack.png")));
        
        contextMenu->addAction(testConnectionAct);
        contextMenu->addAction(copyInformAct);
        contextMenu->addSeparator();
        contextMenu->addAction(deleteConnectionAct);
        
        connect(deleteConnectionAct,&QAction::triggered,this,&ConnectionView::replyDisconnectTargetsAct);
        connect(testConnectionAct,&QAction::triggered,this,&ConnectionView::replyTestConnectionAct);
        connect(copyInformAct, &QAction::triggered,this,&ConnectionView::replyCopySelectedInformAct);
    }
    
    void ConnectionView::initConnectionListHeader()
    {
        QTreeWidgetItem *header = new QTreeWidgetItem;
        for(int i = 0; i < headerNameList.count();i++)
        {
            header->setText(i, headerNameList.at(i));
            header->setTextAlignment(i,Qt::AlignCenter);
        }
        setHeaderItem(header);
    }
    
    void ConnectionView::updateConnectionTime(qint32 index, qint32 connectionTime)
    {
        //qDebug() <<"updating time :" << descriptor;
        auto item = getIndexMap().find(index);
        if(item == getIndexMap().end())
        {
            qDebug() <<"updating time wrong:" << index;
            return;
        }
        
        QTime time(0,0,0);
        time = time.addSecs(connectionTime);
        
        if(item.value())
            item.value()->setText(ONLINETIME,time.toString());
        //qDebug() <<"updating time end: " << descriptor;
    }
    
    void ConnectionView::deleteConnection(qint32 index)
    {
        qDebug() <<"deleteConnection :" << index;
        auto item = getIndexMap().find(index);
        if(item != getIndexMap().end())
        {
            takeTopLevelItem(indexOfTopLevelItem(item.value()));
            delete getIndexMap().take(index);
        }
        
        qDebug() <<"deleteConnection end: " << index;
    }
    
    void ConnectionView::updateRemark(qint32 index, const QString &username)
    {
        //qDebug() <<"updateRemark :" << descriptor;
        auto item = getIndexMap().find(index);
        if(item == getIndexMap().end())
        {
            qDebug() <<"updateRemark wrong:" << index;
            
            //重新把动作投递到队列中
            UpdateRemarkProducer *updateRemarkProducer = new UpdateRemarkProducer(index, username);
            connect(updateRemarkProducer, &QThread::finished, updateRemarkProducer,&QThread::deleteLater);
            updateRemarkProducer->start();
            return;
        }
        
        if(item.value())
            item.value()->setText(REMARK, username);
        //qDebug() <<"updateRemark end: " << descriptor;
    }
    
    void ConnectionView::updateConnectionList(ConnectionViewItem *item)
    {
        using INDEX = ConnectionView::HeaderNameIndex;
        qDebug() <<"addConnection :" << item->text(INDEX::DESCRIPTOR).toInt();
        
        qint32 index = item->text(INDEX::DESCRIPTOR).toInt();
        
        addTopLevelItem(item);
        getIndexMap().insert(index, item);
        
        auto headerList(getHeaderLengthList());
        
        for (int i = 0; i < headerList.length(); i++)
            setColumnWidth(i,headerList[i]);
        
        emit newConnectionComming(item->text(INDEX::ADDRESS));
        qDebug() <<"addConnection end: " << item->text(INDEX::DESCRIPTOR).toInt();
    }
    
    void ConnectionView::replyDisconnectTargets()
    {
        auto selectedItemList(selectedItems());
        
        for(const auto &item : selectedItemList)
        {
            qint32 index = item->text(DESCRIPTOR).toInt();
            quint16 port = item->text(LISTENEDPORT).toUShort();
            
            emit requestDisconnection(port, index);
        }
    }
    
    void ConnectionView::replyDisconnectTargetsAct()
    {
        DisconnectTargetsProducer *producer = new DisconnectTargetsProducer;
        connect(producer, &QThread::finished, producer ,&QThread::deleteLater);
        producer->start();
    }
    
    void ConnectionView::replyTestConnectionAct()
    {
        TestConnectionProducer *producer = new TestConnectionProducer;
        connect(producer, &QThread::finished, producer ,&QThread::deleteLater);
        producer->start();
    }
    
    void ConnectionView::replyTestConnection()
    {
        for(const auto &item : selectedItems())
            selectedPortsandDescriptors.append({item->text(LISTENEDPORT).toUShort(),item->text(DESCRIPTOR).toInt()});
        
        msgSendingEditor.reset(new MsgSendEditor(this),MsgSendEditorDeletor());
        connect(msgSendingEditor.data(), &MsgSendEditor::requestMsgSending, this, &ConnectionView::replySendMsgWanted);
        msgSendingEditor->show();
    }
    
    void ConnectionView::replySendMsgWanted(const QString &msg)
    {
        for(const auto &item :selectedPortsandDescriptors)
            emit requestTestConnection(item.first,item.second, msg);
    }
    
    void ConnectionView::mousePressEvent(QMouseEvent *event)
    {
        QTreeWidget::mousePressEvent(event);
        origin = event->pos() + QPoint(0,header()->height());//要加上表头的高度
        if (!rubberBand)
            rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
        rubberBand->setGeometry(QRect(origin, QSize()));
        rubberBand->show();
    }
    
    void ConnectionView::mouseMoveEvent(QMouseEvent *event)
    {
        QTreeWidget::mouseMoveEvent(event);
        if(rubberBand)
            rubberBand->setGeometry(QRect(origin, event->pos() + QPoint(0,header()->height())).normalized());
        //要加上表头的高度
    }
    
    void ConnectionView::mouseReleaseEvent(QMouseEvent *event)
    {
        QTreeWidget::mouseReleaseEvent(event);
        rubberBand->hide();
    }
    
    void ConnectionView::replyCopySelectedInformAct()
    {
        CopySelectedItemInformProducer *producer = new CopySelectedItemInformProducer;
        connect(producer, &QThread::finished, producer ,&QThread::deleteLater);
        producer->start();
    }
    
    void ConnectionView::replyCopySelectedInform()
    {
        auto selectedItemList(selectedItems());
        QString content;
        
        for(const auto &item :selectedItemList)
        {
            for(int i = 0;i != headerNameList.count();i++)
                content += item->text(i) + "\t";
            content += "\n";
        }
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(content);
    }
    
    void ListendPortDialog::appendNewServerToList(const QString &protocol, quint16 port)
    {
        QTreeWidgetItem *item = new QTreeWidgetItem;
        portView->addTopLevelItem(item);
        itemsMap.insert(port,item);
        
        item->setText(PROTOCOL,protocol);
        item->setTextAlignment(PROTOCOL,Qt::AlignCenter);
        
        item->setText(PORT,QString::number(port));
        item->setTextAlignment(PORT,Qt::AlignCenter);
        
        item->setText(STATE,QString::fromLocal8Bit("运行"));
        item->setTextAlignment(STATE,Qt::AlignCenter);
        
        for(int i = 0; i<headerSizeList.count();i++)
            portView->setColumnWidth(i,headerSizeList[i]);
    }
    
    void ListendPortDialog::replyContextMenuRequest(const QPoint &)
    {
        contextMenu->exec(QCursor::pos());       
    }
    
    ListendPortDialog::ListendPortDialog(QWidget *parent)
        :QDialog (parent)
    {   
        setupUi(this);
        initContextMenu();
        inivlizePortList();
    }
    
    void ListendPortDialog::inivlizePortList()
    {
        portView->setAnimated(true);
        portView->setAllColumnsShowFocus(true);
        portView->setAlternatingRowColors(true);
        
        portView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        portView->setSelectionBehavior(QAbstractItemView::SelectRows);
        portView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    }
    
    void ListendPortDialog::initContextMenu()
    {
        setContextMenuPolicy(Qt::CustomContextMenu);
        connect(this, &ListendPortDialog::customContextMenuRequested,
                this, &ListendPortDialog::replyContextMenuRequest);
        
        contextMenu.reset(new QMenu);
        
        pauseListenedAct = new QAction(QString::fromLocal8Bit("暂停所选监听"),this);
        wakeUpListenedAct = new QAction(QString::fromLocal8Bit("唤醒所选监听"),this);
        closeListenedAct = new QAction(QString::fromLocal8Bit("关闭所选监听"),this);
        
        pauseListenedAct->setIcon(QIcon(QString::fromLocal8Bit(":/images/pause.png")));
        wakeUpListenedAct->setIcon(QIcon(QString::fromLocal8Bit(":/images/restart.png")));
        closeListenedAct->setIcon(QIcon(QString::fromLocal8Bit(":/images/stop.png")));
                
        contextMenu->addAction(pauseListenedAct);
        contextMenu->addAction(wakeUpListenedAct);
        contextMenu->addSeparator();
        contextMenu->addAction(closeListenedAct);
        
        connect(pauseListenedAct,&QAction::triggered,this,&ListendPortDialog::replyPauseListenedClicked);
        connect(closeListenedAct,&QAction::triggered,this,&ListendPortDialog::replyCloseListenedClicked);
        connect(wakeUpListenedAct,&QAction::triggered,this,&ListendPortDialog::replyWakeUpListenedClicked);
    }
    
    void ListendPortDialog::replyCloseListenedClicked()
    {
        auto selectedList(portView->selectedItems());
        for(const auto &server : selectedList)
        {
            emit closeListened(server->text(PORT).toUInt());
        }
    }
    
    void ListendPortDialog::replyPauseListenedClicked()
    {
        auto selectedList(portView->selectedItems());
        for(const auto &server : selectedList)
            emit pauseListened(server->text(PORT).toUInt());
    }
    
    void ListendPortDialog::replyWakeUpListenedClicked()
    {
        auto selectedList(portView->selectedItems());
        for(const auto &server : selectedList)
            emit wakeUpListened(server->text(PORT).toUInt());
    }
    
    void ListendPortDialog::remarkWakeUpServer(quint16 port)
    {
        auto item(itemsMap.find(port).value());
        if(item)
            item->setText(STATE,QString::fromLocal8Bit("运行"));
    }
    
    void ListendPortDialog::remarkPausedServer(quint16 port)
    {
        auto item(itemsMap.find(port).value());
        if(item)
            item->setText(STATE,QString::fromLocal8Bit("暂停"));
    }
    
    void ListendPortDialog::deleteClosedServer(quint16 port)
    {
        auto item(itemsMap.find(port).value());
        if(item)
        {
            item = portView->takeTopLevelItem(portView->indexOfTopLevelItem(item));
            delete item;
        }
        updateSumOfListening(-1);
    }
    
    void ListendPortDialog::updateSumOfListening(int offset)
    {
        auto sum(sumOfPortsLabel->text().toInt());
        sum += offset;
        sumOfPortsLabel->setText(QString::number(sum));
    }
    
    void ListendPortDialog::replySendedServerList(const QList<TcpServerListendCore *> &list)
    {
        for(const auto &server:list)
        {
            auto typePair(server->getServerType());
            
            QTreeWidgetItem *item = new QTreeWidgetItem;
            portView->addTopLevelItem(item);
            itemsMap.insert(typePair.second,item);
            
            item->setText(PROTOCOL,typePair.first);
            item->setTextAlignment(PROTOCOL,Qt::AlignCenter);
            
            item->setText(PORT,QString::number(typePair.second));
            item->setTextAlignment(PORT,Qt::AlignCenter);
            
            item->setText(STATE,server->getServerBlockingState()?QString::fromLocal8Bit("运行"):QString::fromLocal8Bit("暂停"));
            item->setTextAlignment(STATE,Qt::AlignCenter);
            
            for(int i = 0; i<headerSizeList.count();i++)
                portView->setColumnWidth(i,headerSizeList[i]);
        }
        sumOfPortsLabel->setText(QString::number(list.count()));
        sumOfPortsLabel->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    }
}









