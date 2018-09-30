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


SOURCES += core/block.cpp \
    core/blockdata.cpp \
    core/event.cpp \
    core/heallocation.cpp \
    core/historyitem.cpp \
    core/map.cpp \
    core/maplayout.cpp \
    core/metatile.cpp \
    core/parseutil.cpp \
    core/tile.cpp \
    core/tileset.cpp \
    ui/bordermetatilespixmapitem.cpp \
    ui/collisionpixmapitem.cpp \
    ui/connectionpixmapitem.cpp \
    ui/currentselectedmetatilespixmapitem.cpp \
    ui/eventpropertiesframe.cpp \
    ui/graphicsview.cpp \
    ui/imageproviders.cpp \
    ui/mappixmapitem.cpp \
    ui/metatileselector.cpp \
    ui/movementpermissionsselector.cpp \
    ui/neweventtoolbutton.cpp \
    ui/noscrollcombobox.cpp \
    ui/noscrollspinbox.cpp \
    ui/selectablepixmapitem.cpp \
    editor.cpp \
    main.cpp \
    mainwindow.cpp \
    project.cpp \
    settings.cpp \
    ui/mapsceneeventfilter.cpp \
    ui/tileseteditor.cpp

HEADERS  += core/block.h \
    core/blockdata.h \
    core/event.h \
    core/heallocation.h \
    core/history.h \
    core/historyitem.h \
    core/map.h \
    core/mapconnection.h \
    core/maplayout.h \
    core/metatile.h \
    core/parseutil.h \
    core/tile.h \
    core/tileset.h \
    ui/bordermetatilespixmapitem.h \
    ui/collisionpixmapitem.h \
    ui/connectionpixmapitem.h \
    ui/currentselectedmetatilespixmapitem.h \
    ui/eventpropertiesframe.h \
    ui/graphicsview.h \
    ui/imageproviders.h \
    ui/mappixmapitem.h \
    ui/metatileselector.h \
    ui/movementpermissionsselector.h \
    ui/neweventtoolbutton.h \
    ui/noscrollcombobox.h \
    ui/noscrollspinbox.h \
    ui/selectablepixmapitem.h \
    editor.h \
    mainwindow.h \
    project.h \
    settings.h \
    ui/mapsceneeventfilter.h \
    ui/tileseteditor.h

FORMS    += mainwindow.ui \
    eventpropertiesframe.ui \
    tileseteditor.ui

RESOURCES += \
    resources/images.qrc

INCLUDEPATH += core
INCLUDEPATH += ui
