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
QMAKE_CXXFLAGS += -std=c++11

SOURCES += src/core/block.cpp \
    src/core/blockdata.cpp \
    src/core/event.cpp \
    src/core/heallocation.cpp \
    src/core/historyitem.cpp \
    src/core/map.cpp \
    src/core/maplayout.cpp \
    src/core/metatile.cpp \
    src/core/parseutil.cpp \
    src/core/settings.cpp \
    src/core/tile.cpp \
    src/core/tileset.cpp \
    src/ui/bordermetatilespixmapitem.cpp \
    src/ui/collisionpixmapitem.cpp \
    src/ui/connectionpixmapitem.cpp \
    src/ui/currentselectedmetatilespixmapitem.cpp \
    src/ui/eventpropertiesframe.cpp \
    src/ui/filterchildrenproxymodel.cpp \
    src/ui/graphicsview.cpp \
    src/ui/imageproviders.cpp \
    src/ui/mappixmapitem.cpp \
    src/ui/mapsceneeventfilter.cpp \
    src/ui/metatilelayersitem.cpp \
    src/ui/metatileselector.cpp \
    src/ui/movementpermissionsselector.cpp \
    src/ui/neweventtoolbutton.cpp \
    src/ui/noscrollcombobox.cpp \
    src/ui/noscrollspinbox.cpp \
    src/ui/paletteeditor.cpp \
    src/ui/selectablepixmapitem.cpp \
    src/ui/tileseteditor.cpp \
    src/ui/tileseteditormetatileselector.cpp \
    src/ui/tileseteditortileselector.cpp \
    src/editor.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/project.cpp \

HEADERS  += include/core/block.h \
    include/core/blockdata.h \
    include/core/event.h \
    include/core/heallocation.h \
    include/core/history.h \
    include/core/historyitem.h \
    include/core/map.h \
    include/core/mapconnection.h \
    include/core/maplayout.h \
    include/core/metatile.h \
    include/core/parseutil.h \
    include/core/settings.h \
    include/core/tile.h \
    include/core/tileset.h \
    include/ui/bordermetatilespixmapitem.h \
    include/ui/collisionpixmapitem.h \
    include/ui/connectionpixmapitem.h \
    include/ui/currentselectedmetatilespixmapitem.h \
    include/ui/eventpropertiesframe.h \
    include/ui/filterchildrenproxymodel.h \
    include/ui/graphicsview.h \
    include/ui/imageproviders.h \
    include/ui/mappixmapitem.h \
    include/ui/mapsceneeventfilter.h \
    include/ui/metatilelayersitem.h \
    include/ui/metatileselector.h \
    include/ui/movementpermissionsselector.h \
    include/ui/neweventtoolbutton.h \
    include/ui/noscrollcombobox.h \
    include/ui/noscrollspinbox.h \
    include/ui/paletteeditor.h \
    include/ui/selectablepixmapitem.h \
    include/ui/tileseteditor.h \
    include/ui/tileseteditormetatileselector.h \
    include/ui/tileseteditortileselector.h \
    include/editor.h \
    include/mainwindow.h \
    include/project.h \

FORMS    += forms/mainwindow.ui \
    forms/eventpropertiesframe.ui \
    forms/tileseteditor.ui \
    forms/paletteeditor.ui

RESOURCES += \
    resources/images.qrc

INCLUDEPATH += include
INCLUDEPATH += include/core
INCLUDEPATH += include/ui
