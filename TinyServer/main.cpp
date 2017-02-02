#include <QApplication>
#include <QSharedMemory>
#include <QMutex>
#include <QMessageBox>
#include <QMutexLocker>
#include <QTextCodec>
#include "tcpserverwindow.h"

static const char *serverName = "TinyServer";

bool serverIsRunning(const char *name)
{
    static QSharedMemory shm(name);
    
    bool accessShmSuccessed = false;
    
    shm.lock();
    accessShmSuccessed = shm.create(100);
    shm.unlock();
    
    return accessShmSuccessed;
}

int main(int argc, char *argv[])
{
    
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("GBK"));
    QApplication a(argc, argv);
    
    if (!serverIsRunning(serverName))
    {
        QMessageBox::information(NULL,QString::fromLocal8Bit("TinyServer"),QString::fromUtf8("服务器已经启动"));
        return -1;
    }
    
    MainWindows::TcpServerWindow *w = new MainWindows::TcpServerWindow;
    w->hide();
    return a.exec();
}
