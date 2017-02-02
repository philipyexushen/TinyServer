#include "servertrayicon.h"

namespace ServerTrayIcon 
{
    TrayIconManager::TrayIconManager(QObject *parent)
        :QSystemTrayIcon (parent)
    {
        
    }
    
    TrayIconManager::TrayIconManager(const QIcon &icon, QObject *parent )
        :QSystemTrayIcon (icon, parent)
    {
        setToolTip(QString::fromLocal8Bit(TOOLTIP));
        createMenu();
        
        connect(this, &TrayIconManager::activated,this,&TrayIconManager::replyMouseClicked);
    #ifdef WIN32
        show();//显示在托盘上
    #endif
    }
    
    void TrayIconManager::replyMouseClicked(QSystemTrayIcon::ActivationReason reason)
    {
        switch(reason)
        {
        case QSystemTrayIcon::Trigger:
            emit requestShowMainWindow();
            break;
        case QSystemTrayIcon::DoubleClick:
            emit requestShowMainWindow();
            break;
        case QSystemTrayIcon::Context:
            contextMenu->show();
            break;
        case QSystemTrayIcon::Unknown :case QSystemTrayIcon::MiddleClick:
            break;
        }
    }
    
    void TrayIconManager::createMenu()
    {
        contextMenu.reset(new QMenu);
        
        listenedPortSettingAct = new QAction(QString::fromLocal8Bit("设定监听端口"),this);
        
        showWindowAct = new QAction(QString::fromLocal8Bit("打开主程序"),this);
        closeWindowAct = new QAction(QString::fromLocal8Bit("关闭程序"),this);
        
        contextMenu->addAction(listenedPortSettingAct);
        contextMenu->addAction(showWindowAct);
        contextMenu->addSeparator();
        contextMenu->addAction(closeWindowAct);
        
        connect(listenedPortSettingAct,&QAction::triggered,this,&TrayIconManager::requestOpenListenedPort);
        connect(listenedPortSettingAct,&QAction::triggered,this,&TrayIconManager::showMainWindow);
        connect(showWindowAct,&QAction::triggered,this,&TrayIconManager::requestShowMainWindow);
        connect(closeWindowAct,&QAction::triggered,this,&TrayIconManager::requestExit);
        
        setContextMenu(contextMenu.data());
    }
}
