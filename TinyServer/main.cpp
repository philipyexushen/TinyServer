#include <QApplication>
#include <QSharedMemory>
#include <QMutex>
#include <QMessageBox>
#include <QMutexLocker>
#include <QTextCodec>
#include "tcpserverwindow.h"

static const char *serverName = "TinyServer";

///创建一个共享内存，保证当前只有一个实例
bool serverIsRunning(const char *name)
{
    static QSharedMemory shm(name);
    
    bool accessShmSuccessed = false;
    
    shm.lock();
    accessShmSuccessed = shm.create(100);
    shm.unlock();
    
    return accessShmSuccessed;
}

///注意如果使用MSVC64编译器，打中文用QString::fromLocal8Bit(...)显示，如果用MinGW编译器，则用QObject::tr(...)
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
