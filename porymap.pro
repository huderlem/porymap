#-------------------------------------------------
#
# Project created by QtCreator 2016-08-31T15:19:13
#
#-------------------------------------------------

QT       += core gui qml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = porymap
TEMPLATE = app
RC_ICONS = resources/icons/porymap-icon-2.ico
ICON = resources/icons/porymap.icns
QMAKE_CXXFLAGS += -std=c++17 -Wall
QMAKE_TARGET_BUNDLE_PREFIX = com.pret

SOURCES += src/core/block.cpp \
    src/core/bitpacker.cpp \
    src/core/blockdata.cpp \
    src/core/events.cpp \
    src/core/heallocation.cpp \
    src/core/imageexport.cpp \
    src/core/map.cpp \
    src/core/maplayout.cpp \
    src/core/mapparser.cpp \
    src/core/metatile.cpp \
    src/core/metatileparser.cpp \
    src/core/paletteutil.cpp \
    src/core/parseutil.cpp \
    src/core/tile.cpp \
    src/core/tileset.cpp \
    src/core/regionmap.cpp \
    src/core/wildmoninfo.cpp \
    src/core/editcommands.cpp \
    src/lib/fex/lexer.cpp \
    src/lib/fex/parser.cpp \
    src/lib/fex/parser_util.cpp \
    src/lib/orderedjson.cpp \
    src/core/regionmapeditcommands.cpp \
    src/scriptapi/apimap.cpp \
    src/scriptapi/apioverlay.cpp \
    src/scriptapi/apiutility.cpp \
    src/scriptapi/scripting.cpp \
    src/ui/aboutporymap.cpp \
    src/ui/customscriptseditor.cpp \
    src/ui/customscriptslistitem.cpp \
    src/ui/draggablepixmapitem.cpp \
    src/ui/bordermetatilespixmapitem.cpp \
    src/ui/collisionpixmapitem.cpp \
    src/ui/connectionpixmapitem.cpp \
    src/ui/currentselectedmetatilespixmapitem.cpp \
    src/ui/overlay.cpp \
    src/ui/prefab.cpp \
    src/ui/projectsettingseditor.cpp \
    src/ui/regionmaplayoutpixmapitem.cpp \
    src/ui/regionmapentriespixmapitem.cpp \
    src/ui/cursortilerect.cpp \
    src/ui/customattributestable.cpp \
    src/ui/eventframes.cpp \
    src/ui/filterchildrenproxymodel.cpp \
    src/ui/graphicsview.cpp \
    src/ui/imageproviders.cpp \
    src/ui/mappixmapitem.cpp \
    src/ui/prefabcreationdialog.cpp \
    src/ui/regionmappixmapitem.cpp \
    src/ui/citymappixmapitem.cpp \
    src/ui/mapsceneeventfilter.cpp \
    src/ui/metatilelayersitem.cpp \
    src/ui/metatileselector.cpp \
    src/ui/movablerect.cpp \
    src/ui/movementpermissionsselector.cpp \
    src/ui/neweventtoolbutton.cpp \
    src/ui/noscrollcombobox.cpp \
    src/ui/noscrollspinbox.cpp \
    src/ui/montabwidget.cpp \
    src/ui/encountertablemodel.cpp \
    src/ui/encountertabledelegates.cpp \
    src/ui/paletteeditor.cpp \
    src/ui/selectablepixmapitem.cpp \
    src/ui/tileseteditor.cpp \
    src/ui/tileseteditormetatileselector.cpp \
    src/ui/tileseteditortileselector.cpp \
    src/ui/tilemaptileselector.cpp \
    src/ui/regionmapeditor.cpp \
    src/ui/newmappopup.cpp \
    src/ui/mapimageexporter.cpp \
    src/ui/newtilesetdialog.cpp \
    src/ui/flowlayout.cpp \
    src/ui/mapruler.cpp \
    src/ui/shortcut.cpp \
    src/ui/shortcutseditor.cpp \
    src/ui/multikeyedit.cpp \
    src/ui/prefabframe.cpp \
    src/ui/preferenceeditor.cpp \
    src/ui/regionmappropertiesdialog.cpp \
    src/ui/colorpicker.cpp \
    src/config.cpp \
    src/editor.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/project.cpp \
    src/settings.cpp \
    src/log.cpp \
    src/ui/uintspinbox.cpp

HEADERS  += include/core/block.h \
    include/core/bitpacker.h \
    include/core/blockdata.h \
    include/core/events.h \
    include/core/heallocation.h \
    include/core/history.h \
    include/core/imageexport.h \
    include/core/map.h \
    include/core/mapconnection.h \
    include/core/maplayout.h \
    include/core/mapparser.h \
    include/core/metatile.h \
    include/core/metatileparser.h \
    include/core/paletteutil.h \
    include/core/parseutil.h \
    include/core/tile.h \
    include/core/tileset.h \
    include/core/regionmap.h \
    include/core/wildmoninfo.h \
    include/core/editcommands.h \
    include/core/regionmapeditcommands.h \
    include/lib/fex/array.h \
    include/lib/fex/array_value.h \
    include/lib/fex/define_statement.h \
    include/lib/fex/lexer.h \
    include/lib/fex/parser.h \
    include/lib/fex/parser_util.h \
    include/lib/orderedmap.h \
    include/lib/orderedjson.h \
    include/ui/aboutporymap.h \
    include/ui/customscriptseditor.h \
    include/ui/customscriptslistitem.h \
    include/ui/draggablepixmapitem.h \
    include/ui/bordermetatilespixmapitem.h \
    include/ui/collisionpixmapitem.h \
    include/ui/connectionpixmapitem.h \
    include/ui/currentselectedmetatilespixmapitem.h \
    include/ui/prefabframe.h \
    include/ui/projectsettingseditor.h \
    include/ui/regionmaplayoutpixmapitem.h \
    include/ui/regionmapentriespixmapitem.h \
    include/ui/cursortilerect.h \
    include/ui/customattributestable.h \
    include/ui/eventframes.h \
    include/ui/filterchildrenproxymodel.h \
    include/ui/graphicsview.h \
    include/ui/imageproviders.h \
    include/ui/mappixmapitem.h \
    include/ui/mapview.h \
    include/ui/prefabcreationdialog.h \
    include/ui/regionmappixmapitem.h \
    include/ui/citymappixmapitem.h \
    include/ui/mapsceneeventfilter.h \
    include/ui/metatilelayersitem.h \
    include/ui/metatileselector.h \
    include/ui/movablerect.h \
    include/ui/movementpermissionsselector.h \
    include/ui/neweventtoolbutton.h \
    include/ui/noscrollcombobox.h \
    include/ui/noscrollspinbox.h \
    include/ui/montabwidget.h \
    include/ui/encountertablemodel.h \
    include/ui/encountertabledelegates.h \
    include/ui/adjustingstackedwidget.h \
    include/ui/paletteeditor.h \
    include/ui/selectablepixmapitem.h \
    include/ui/tileseteditor.h \
    include/ui/tileseteditormetatileselector.h \
    include/ui/tileseteditortileselector.h \
    include/ui/tilemaptileselector.h \
    include/ui/regionmapeditor.h \
    include/ui/newmappopup.h \
    include/ui/mapimageexporter.h \
    include/ui/newtilesetdialog.h \
    include/ui/overlay.h \
    include/ui/flowlayout.h \
    include/ui/mapruler.h \
    include/ui/shortcut.h \
    include/ui/shortcutseditor.h \
    include/ui/multikeyedit.h \
    include/ui/prefab.h \
    include/ui/preferenceeditor.h \
    include/ui/regionmappropertiesdialog.h \
    include/ui/colorpicker.h \
    include/config.h \
    include/editor.h \
    include/mainwindow.h \
    include/project.h \
    include/scripting.h \
    include/scriptutility.h \
    include/settings.h \
    include/log.h \
    include/ui/uintspinbox.h

FORMS    += forms/mainwindow.ui \
    forms/prefabcreationdialog.ui \
    forms/prefabframe.ui \
    forms/tileseteditor.ui \
    forms/paletteeditor.ui \
    forms/regionmapeditor.ui \
    forms/newmappopup.ui \
    forms/aboutporymap.ui \
    forms/newtilesetdialog.ui \
    forms/mapimageexporter.ui \
    forms/shortcutseditor.ui \
    forms/preferenceeditor.ui \
    forms/regionmappropertiesdialog.ui \
    forms/colorpicker.ui \
    forms/projectsettingseditor.ui \
    forms/customscriptseditor.ui \
    forms/customscriptslistitem.ui

RESOURCES += \
    resources/images.qrc \
    resources/themes.qrc \
    resources/text.qrc

INCLUDEPATH += include
INCLUDEPATH += include/core
INCLUDEPATH += include/ui
INCLUDEPATH += include/lib
INCLUDEPATH += forms

include(src/vendor/QtGifImage/gifimage/qtgifimage.pri)
