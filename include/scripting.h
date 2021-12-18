#pragma once
#ifndef SCRIPTING_H
#define SCRIPTING_H

#include "mainwindow.h"
#include "block.h"

#include <QStringList>
#include <QJSEngine>

enum CallbackType {
    OnProjectOpened,
    OnProjectClosed,
    OnBlockChanged,
    OnBlockHoverChanged,
    OnBlockHoverCleared,
    OnMapOpened,
    OnMapResized,
    OnMapShifted,
    OnTilesetUpdated,
    OnMainTabChanged,
    OnMapViewTabChanged,
};

class Scripting
{
public:
    Scripting(MainWindow *mainWindow);
    static QJSValue fromBlock(Block block);
    static QJSValue fromTile(Tile tile);
    static Tile toTile(QJSValue obj);
    static QJSValue dimensions(int width, int height);
    static QJSValue position(int x, int y);
    static QJSEngine *getEngine();
    static QImage getImage(QString filepath);
    static void init(MainWindow *mainWindow);
    static void registerAction(QString functionName, QString actionName);
    static int numRegisteredActions();
    static void invokeAction(QString actionName);
    static void cb_ProjectOpened(QString projectPath);
    static void cb_ProjectClosed(QString projectPath);
    static void cb_MetatileChanged(int x, int y, Block prevBlock, Block newBlock);
    static void cb_BlockHoverChanged(int x, int y);
    static void cb_BlockHoverCleared();
    static void cb_MapOpened(QString mapName);
    static void cb_MapResized(int oldWidth, int oldHeight, int newWidth, int newHeight);
    static void cb_MapShifted(int xDelta, int yDelta);
    static void cb_TilesetUpdated(QString tilesetName);
    static void cb_MainTabChanged(int oldTab, int newTab);
    static void cb_MapViewTabChanged(int oldTab, int newTab);
    static bool tryErrorJS(QJSValue js);

private:
    QJSEngine *engine;
    QStringList filepaths;
    QList<QJSValue> modules;
    QMap<QString, QString> registeredActions;
    QMap<QString, const QImage*> imageCache;

    void loadModules(QStringList moduleFiles);
    void invokeCallback(CallbackType type, QJSValueList args);
};

#endif // SCRIPTING_H
