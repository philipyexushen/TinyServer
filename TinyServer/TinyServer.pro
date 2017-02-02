#-------------------------------------------------
#
# Project created by QtCreator 2017-01-17T02:47:33
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = server
TEMPLATE = app


SOURCES += main.cpp\
    tcpservercore.cpp \
    tcpserverwindow.cpp \
    servertrayicon.cpp \
    windows_editors.cpp \
    connectionview_helper.cpp \
    headerframe_helper.cpp \
    connectionmap_helper.cpp

HEADERS  += \
    tcpservercore.h \
    tcpserverwindow.h \
    servertrayicon.h \
    connectionview_helper.h \
    windows_editors.h \
    connectionview.h \
    headerframe_helper.h \
    connectionmap_helper.h

FORMS += \
    serverMainWindow.ui \
    msgTestEditor.ui \
    pulseEditdialog.ui \
    portListDialog.ui \
    portEditDialog.ui

QT += network
QT += printsupport

DISTFILES += \
    icon.rc
    
RC_FILE += \
    icon.rc

RESOURCES += \
    icon.qrc
