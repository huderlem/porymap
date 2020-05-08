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
    OnMapOpened,
};

class Scripting
{
public:
    Scripting(MainWindow *mainWindow);
    static QJSValue fromBlock(Block block);
    static QJSValue dimensions(int width, int height);
    static QJSEngine *getEngine();
    static void init(MainWindow *mainWindow);
    static void registerAction(QString functionName, QString actionName);
    static int numRegisteredActions();
    static void invokeAction(QString actionName);
    static void cb_ProjectOpened(QString projectPath);
    static void cb_ProjectClosed(QString projectPath);
    static void cb_MetatileChanged(int x, int y, Block prevBlock, Block newBlock);
    static void cb_MapOpened(QString mapName);

private:
    QJSEngine *engine;
    QStringList filepaths;
    QList<QJSValue> modules;
    QMap<QString, QString> registeredActions;

    void loadModules(QStringList moduleFiles);
    void invokeCallback(CallbackType type, QJSValueList args);
};

#endif // SCRIPTING_H
