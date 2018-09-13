#-------------------------------------------------
#
# Project created by QtCreator 2016-08-31T15:19:13
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = porymap
TEMPLATE = app
RC_ICONS = resources/icons/porymap-icon-1.ico
ICON = resources/icons/porymap-icon-1.ico


SOURCES += main.cpp\
        mainwindow.cpp \
    project.cpp \
    map.cpp \
    blockdata.cpp \
    block.cpp \
    tileset.cpp \
    tile.cpp \
    event.cpp \
    editor.cpp \
    objectpropertiesframe.cpp \
    graphicsview.cpp \
    parseutil.cpp \
    neweventtoolbutton.cpp \
    noscrollcombobox.cpp \
    noscrollspinbox.cpp

HEADERS  += mainwindow.h \
    project.h \
    map.h \
    blockdata.h \
    block.h \
    tileset.h \
    tile.h \
    event.h \
    editor.h \
    objectpropertiesframe.h \
    graphicsview.h \
    parseutil.h \
    neweventtoolbutton.h \
    noscrollcombobox.h \
    noscrollspinbox.h

FORMS    += mainwindow.ui \
    objectpropertiesframe.ui

RESOURCES += \
    resources/images.qrc
