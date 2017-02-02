#ifndef WINDOWSEDITORS_H
#define WINDOWSEDITORS_H

#include <QDialog>
#include <QRegExpValidator>
#include <QString>
#include <QMap>
#include <QMenu>
#include <QAction>
#include <QSharedPointer>

#include "ui_portEditDialog.h"
#include "ui_msgTestEditor.h"
#include "ui_pulseEditdialog.h"
#include "ui_portListDialog.h"

namespace Editors 
{
    class OpenPortEditor : public QDialog, public Ui::portInputDialog
    {
        Q_OBJECT
    public:
        explicit OpenPortEditor(QWidget *parent = 0);
    public slots:
        void checkLineEditIsNotEmpty(const QString &);
    private:
        QRegExpValidator *editValidator;
    };
    
    class SettingPulseEditor :  public QDialog, public Ui::PulseInputDialog
    {
        Q_OBJECT
    public:
        explicit SettingPulseEditor(QWidget *parent = 0);
    public slots:
        void checkLineEditIsNotEmpty(const QString &);
    private:
        QRegExpValidator *editValidator;
    };
    
    class MsgSendEditor : public QDialog, public Ui::MsgEditorDialog
    {
        Q_OBJECT
    public:
        MsgSendEditor(QWidget *parent = 0);
    signals:
        void requestMsgSending(const QString msg);
    public slots:
        void sendBtnClicked();
        
        void replyAddText();
        void replyAddPlainText();
    private slots:
        void enableSendBtn();
        void createContextMenu();
    private:
        QScopedPointer<QMenu> contextMenu;
        QAction *addText, *addPlainText, *copy, *selectAll, *cut, *undo, *redo;
        
        void initContextMenu();
    };
    
    class MsgSendEditorDeletor
    {
    public:
        void operator()(MsgSendEditor *editor){ editor->QObject::deleteLater(); }
    };
}


#endif // WINDOWSEDITORS_H
