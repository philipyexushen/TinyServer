#ifndef CONNECTIONVIEW_H
#define CONNECTIONVIEW_H

#include <QTreeWidget>
#include <QString>
#include <QList>
#include <QSharedPointer>
#include <QScopedPointer>

#include "windows_editors.h"
#include "connectionview_helper.h"

using namespace Editors;
using namespace ConnectionViewHelper;

namespace MainWindows
{
    class ConnectionViewItem : public QTreeWidgetItem
    {
        friend class ConnectionViewHelper::InsertItem;
    public:
        ConnectionViewItem(): QTreeWidgetItem(){ }
    };
    
    class ConnectionView : public QTreeWidget
    {
        Q_OBJECT
    signals:
        void requestDisconnection(quint16 port, qint32 target);
        void requestTestConnection(quint16 port, qint32 target, const QString msg);
        void newConnectionComming(const QString &address);
    public slots:
        void replyDisconnectTargetsAct();
        void replyDisconnectTargets();
        void replyTestConnectionAct();
        void replyTestConnection();
        void replyCopySelectedInformAct();
        void replyCopySelectedInform();
        
        void replySendMsgWanted(const QString &msg);
        void updateConnectionTime(qint32 index, qint32 currentTime);
        void deleteConnection(qint32 descriptor);
        void updateConnectionList(ConnectionViewItem *item);
        void updateRemark(qint32 descriptor, const QString &username);
    public:
        using DesciptorIndexMap = QMap<qint32,ConnectionViewItem *>;
        
        explicit ConnectionView(QWidget *parent = 0);
        ~ConnectionView() = default;
        
        const QList<int>&getHeaderLengthList()const { return headerSizeList; }
        
        DesciptorIndexMap &getIndexMap(){ return itemIndexMap; }
        const DesciptorIndexMap &getIndexMap()const { return itemIndexMap; }
        
        int getItemCount()const { return topLevelItemCount(); }
        
        void startConnectionViewWatcher();
    
        enum HeaderNameIndex      
        { ADDRESS,         PROTOCOL,      DESCRIPTOR,   THREADID,   ONLINETIME,    PEERPORT,      LISTENEDPORT , REMARK  };
        QStringList headerNameList
        { QString::fromLocal8Bit("链接地址"),QString::fromLocal8Bit("协议类型"),QString::fromLocal8Bit("描述符"),QString::fromLocal8Bit("线程ID"),QString::fromLocal8Bit("已连接时间"),QString::fromLocal8Bit("请求端口号"),QString::fromLocal8Bit("监听端口"),QString::fromLocal8Bit("备注")};
        QList<int> headerSizeList 
        {       200,          100,           100,           100,         100,        100,              80 ,         100  };
    protected:
        void mousePressEvent(QMouseEvent *event)override;
        void mouseMoveEvent(QMouseEvent *event)override;
        void mouseReleaseEvent(QMouseEvent *event)override;
    private:
        DesciptorIndexMap itemIndexMap;
        
        QRubberBand *rubberBand = nullptr;
        QSharedPointer<QMenu> contextMenu;
        QAction *deleteConnectionAct, *testConnectionAct, *copyInformAct;
        
        QScopedPointer<ConsumerHelper> _consumerHelper;
        
        QList<QPair<quint16,qint32>> selectedPortsandDescriptors;
        QSharedPointer<MsgSendEditor> msgSendingEditor;
        QPoint origin;
        
        void initConnectionListHeader();
        void initContextMenu();
    private slots:
        void createContextMenu(const QPoint &pos);
    };
}


#endif // CONNECTIONVIEW_H
