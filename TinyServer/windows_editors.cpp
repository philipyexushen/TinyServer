#include <QClipboard>
#include <QApplication>
#include <memory>
#include "windows_editors.h"

namespace Editors
{
    OpenPortEditor::OpenPortEditor(QWidget *parent)
        :QDialog (parent)
    {
        setupUi(this);
        connect(portEdit,&QLineEdit::textChanged,this, &OpenPortEditor::checkLineEditIsNotEmpty);
        okButton->setEnabled(false);
        
        QRegExp pulseRegExp("[1-9]\\d*");
        editValidator = new QRegExpValidator(pulseRegExp,this);
        portEdit->setValidator(editValidator);
    }
    
    void OpenPortEditor::checkLineEditIsNotEmpty(const QString &)
    {
        auto flag = portEdit->text().isEmpty();
        okButton->setEnabled(!flag);
    }
    
    MsgSendEditor::MsgSendEditor(QWidget *parent)
        :QDialog (parent)
        ,contextMenu(new QMenu)
    {
        setupUi(this);
        sendBtn->setEnabled(false);
        connect(msgEditor,&QTextEdit::textChanged,this,&MsgSendEditor::enableSendBtn);
        connect(sendBtn, &QPushButton::clicked,this, &MsgSendEditor::sendBtnClicked);
        connect(closeBtn,&QPushButton::clicked,this, &MsgSendEditor::close);
        
        initContextMenu();
    }
    
    void MsgSendEditor::sendBtnClicked()
    {
        emit requestMsgSending(msgEditor->document()->toPlainText());
    }
    
    void MsgSendEditor::enableSendBtn()
    {
        sendBtn->setEnabled(!msgEditor->document()->isEmpty());
    }
    
    void MsgSendEditor::initContextMenu()
    {
        msgEditor->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(msgEditor, &QTextEdit::customContextMenuRequested, this, &MsgSendEditor::createContextMenu);
        
        copy = new QAction(QString::fromLocal8Bit("复制"));
        cut = new QAction(QString::fromLocal8Bit("剪切"));
        addText = new QAction(QString::fromLocal8Bit("粘贴"));
        addPlainText = new QAction(QString::fromLocal8Bit("粘贴为纯文本"));
        selectAll = new QAction(QString::fromLocal8Bit("全选"));
        undo = new QAction(QString::fromLocal8Bit("撤销"));
        redo = new QAction(QString::fromLocal8Bit("重做"));
        
        contextMenu->addAction(cut);
        contextMenu->addAction(copy);
        contextMenu->addSeparator();
        contextMenu->addAction(addText);
        contextMenu->addAction(addPlainText);
        contextMenu->addSeparator();
        contextMenu->addAction(undo);
        contextMenu->addAction(redo);
        contextMenu->addSeparator();
        contextMenu->addAction(selectAll);
        
        connect(copy, &QAction::triggered,msgEditor, &QTextEdit::copy);
        connect(cut, &QAction::triggered,msgEditor, &QTextEdit::cut);
        connect(addText, &QAction::triggered,this, &MsgSendEditor::replyAddText);
        connect(addPlainText, &QAction::triggered, this, &MsgSendEditor::replyAddPlainText);   
        connect(selectAll, &QAction::triggered,msgEditor, &QTextEdit::selectAll);
        connect(undo, &QAction::triggered,msgEditor, &QTextEdit::undo);
        connect(redo, &QAction::triggered,msgEditor, &QTextEdit::redo);
    }
    
    void MsgSendEditor::replyAddText()
    {
        QClipboard *clipboard =  QApplication::clipboard();
        QString text = clipboard->text();
        
        msgEditor->insertHtml(text);
    }

    void MsgSendEditor::replyAddPlainText()
    {
        QClipboard *clipboard =  QApplication::clipboard();
        QString text = clipboard->text();
        
        msgEditor->insertPlainText(text);
    }
    
    void MsgSendEditor::createContextMenu()
    {
        contextMenu->exec(QCursor::pos());
    }
    
    SettingPulseEditor::SettingPulseEditor(QWidget *parent)
        :QDialog (parent)
    {
        setupUi(this);
        okButton->setEnabled(false);
        
        QRegExp pulseRegExp("[1-9]\\d*");
        editValidator = new QRegExpValidator(pulseRegExp,this);
        pulseEdit->setValidator(editValidator);
        
        connect(pulseEdit, &QLineEdit::textChanged,
                this, &SettingPulseEditor::checkLineEditIsNotEmpty);
    }
    
    void SettingPulseEditor::checkLineEditIsNotEmpty(const QString &)
    {
        okButton->setEnabled(!pulseEdit->text().isEmpty());
    }
}

