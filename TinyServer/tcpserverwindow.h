#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QMainWindow>
#include <QDialog>
#include <QTcpServer>
#include <QLineEdit>
#include <QList>
#include <QHostAddress>
#include <QTreeWidget>
#include <QRubberBand>
#include <QSharedPointer>
#include <QMenu>
#include <QPoint>
#include <QAction>
#include <QCloseEvent>
#include <QSystemTrayIcon>
#include <QScopedPointer>
#include <QRegExpValidator>
#include <QEvent>
#include <QSettings>
#include <QPair>
#include <QQueue>

#include "ui_serverMainWindow.h"
#include "ui_portEditDialog.h"
#include "ui_msgTestEditor.h"
#include "ui_pulseEditdialog.h"
#include "ui_portListDialog.h"
#include "tcpservercore.h"
#include "servertrayicon.h"
#include "connectionview_helper.h"
#include "windows_editors.h"
#include "connectionview.h"

using namespace TcpserverCore;
using namespace ServerTrayIcon;
using namespace Editors;
using namespace ConnectionViewHelper;

namespace MainWindows
{
#define SETTING_FILEPATH ".\\serverSettings\\settings.ini"
#define REG_RUN "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define DEFWIDTH 1100
#define DEFHEIGHT 780
    
    class ConnectionView;
    
    class ListendPortDialog :public QDialog, public Ui::PortListDialog
    {
        Q_OBJECT
    public:
        ListendPortDialog(QWidget *parent = 0);
        enum HeaderNameIndex      
        { PROTOCOL,    PORT,     STATE}; 
        QStringList headerNameList
        { tr("监听端口类型"),tr("端口号"),tr("状态")};
        QList<int> headerSizeList 
        {       100,          80,           80,  };
    signals:
        void closeListened(quint16 port);
        void pauseListened(quint16 port);
        void wakeUpListened(quint16 port);
    public slots:
        void replySendedServerList(const QList<TcpServerListendCore *> &list);
        void deleteClosedServer(quint16 port);
        void remarkPausedServer(quint16 port);
        void remarkWakeUpServer(quint16 port);
        void updateSumOfListening(int offset);
        void appendNewServerToList(const QString &protocol, quint16 port);
    private slots:
        void replyContextMenuRequest(const QPoint &pos);
        
        void replyCloseListenedClicked();
        void replyPauseListenedClicked();
        void replyWakeUpListenedClicked();
    private:
        QSharedPointer<QMenu> contextMenu;
        QMap<quint16, QTreeWidgetItem *> itemsMap;
        QAction *pauseListenedAct, *closeListenedAct, *wakeUpListenedAct;
        
        void initContextMenu();
        void inivlizePortList();
    };
    
    class TcpServerWindow : public QMainWindow, private Ui::serverWindow
    {
        Q_OBJECT
    public:
        explicit TcpServerWindow(QWidget *parent = 0);
    signals:
        void requestResetPulse(qint32 pulseInterval);
        void sendServerList(const QList<TcpServerListendCore *> &list);
        void serverClosed(quint16 port);
        void serverPaused(quint16 port);
        void serverWakeUp(quint16 port);
        void newServerStarted(const QString &protocol, quint16 port);
        void requestDisconnection(qint32 target,qint32 connectionIndex);
        void allServerCleared();
    public slots:
        void openPortEditor();
        bool createNewServer(const unsigned short port = 1088);
        void updateView(TcpHeaderFrameHelper::MessageType messageType,const QByteArray &bytes, const QString &userName, quint16 port, qint32 index);
        void updateView(const QString &msg);
        void saveAsPdfFile();
        void clearScreen();
        void replyIsAllowSystemTray();
        void replyIsAllowMinimizeToTray();
        void replypulseSettingTriggered();
        void replyAllowBroadcastTriggered();
        void replyDisconnection(quint16 port, qint32 target);
        void replyTestConnection(quint16 port, qint32 target,const QString &msg);
        void updateConnectionList(const QString &address);
        void replyExit(){ exit(0); }
        void writeSettings();
        void readSettings();
        void replyAutoStartActTriggered();
        void replyCloseListened(quint16 port);
        void replyCloseListened(TcpServerListendCore *pointer);
        void replyPauseListened(quint16 port);
        void replyWakeUpListened(quint16 port);
        void openPortList();
        void replyAllDisConnected(quint16 port);
        void aboutMeDialog();
        void createSettingsPort();
    protected:
        void closeEvent(QCloseEvent *event)override;
        bool eventFilter(QObject *target, QEvent *event)override;
    private slots:
        void moveNewCursorPostion();
    private:
        TrayIconManager *trayIcon;
        
        QMap<quint16, TcpServerListendCore *> serverMap;
        ConnectionView *connectionsView;
        QSharedPointer<ListendPortDialog> listenedPortListView;
        
        QList<quint16> settingsPortList;
        QList<qint32> settingsPulseList;
        qint32 settingsSumOfPort;
        
        int defWidth = DEFWIDTH, defHeight = DEFHEIGHT;
        
        bool bSystemTrayAccepted = 1;
        bool bAllowMinimizeToTray = 1;
        bool bAllowBroadCast = 1;
        bool bAutoStart = 0;
        
        bool bDeletingServer = 0;
        bool bDeletingAllServer = 0;
        
        void setLocalAddress();
        
        void createConnections();
        void initConnectionList();
        void initSystemTrayIcon();
        bool clearServerMap();
        void readSettingsPrivate();
    };
}

#endif // TCPSERVER_H
