#ifndef SCRIPTING_H
#define SCRIPTING_H

#include "mainwindow.h"
#include "block.h"

#include <QStringList>
#include <QJSEngine>

enum CallbackType {
    OnBlockChanged,
};

class Scripting
{
public:
    Scripting(MainWindow *mainWindow);
    QJSValue newBlockObject(Block block);
    static void init(MainWindow *mainWindow);
    static void cb_MetatileChanged(int x, int y, Block prevBlock, Block newBlock);

private:
    QJSEngine *engine;
    QStringList filepaths;
    QList<QJSValue> modules;

    void loadModules(QStringList moduleFiles);
    void invokeCallback(CallbackType type, QJSValueList args);
};

#endif // SCRIPTING_H
