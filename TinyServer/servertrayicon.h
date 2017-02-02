#ifndef SERVERTRAYICON_H
#define SERVERTRAYICON_H

#include <QAction>
#include <QSharedPointer>
#include <QMenu>
#include <QIcon>
#include <QSystemTrayIcon>

namespace ServerTrayIcon
{    
#define TOOLTIP "多线程Tcp服务器v1.30 -by Philip"
    
    class TrayIconManager :public QSystemTrayIcon
    {
        Q_OBJECT
    public:
        explicit TrayIconManager(QObject *parent = 0);
        TrayIconManager(const QIcon &icon, QObject *parent = 0);
    signals:
        void listenedPortRequest();
        void requestShowMainWindow();
        void requestExit();
        void requestOpenListenedPort();
        void requestResetPulse(quint64 pulseInterval);
        void showMainWindow();
    public slots:
        void replyMouseClicked(QSystemTrayIcon::ActivationReason reason);
    private:
        QAction *showWindowAct, *closeWindowAct, *listenedPortSettingAct;
        QSharedPointer<QMenu> contextMenu;
        
        void createMenu();
    };
}


#endif // SERVERTRAYICON_H
