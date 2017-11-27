#-------------------------------------------------
#
# Project created by QtCreator 2017-09-09T15:46:50
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SQLiteExplorer
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += ../sqlite3


SOURCES += \
        main.cpp \
        mainwindow.cpp \
        qhexwindow.cpp \
    SQLite3DB.cpp \
    utils.cpp \
    qsqlitetableview.cpp \
    qsqlitequerywindow.cpp

HEADERS += \
        mainwindow.h \
        qhexwindow.h \
    SQLite3DB.h \
    utils.h \
    qsqlitetableview.h \
    qsqlitequerywindow.h

CONFIG += c++11

FORMS += \
        mainwindow.ui \
        qhexwindow.ui \
    qsqlitequerywindow.ui

DESTDIR  = $$PWD/../bin

LIBS = -L$$DESTDIR -lsqlite3

include(QHexEdit/QHexEdit.pri)
