#-------------------------------------------------
#
# Project created by QtCreator 2016-08-31T15:19:13
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = pretmap
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    project.cpp \
    asm.cpp \
    map.cpp \
    blockdata.cpp \
    block.cpp \
    tileset.cpp \
    metatile.cpp \
    tile.cpp \
    event.cpp \
    editor.cpp

HEADERS  += mainwindow.h \
    project.h \
    asm.h \
    map.h \
    blockdata.h \
    block.h \
    tileset.h \
    metatile.h \
    tile.h \
    event.h \
    editor.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources/images.qrc
