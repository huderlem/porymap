#include "scripting.h"
#include "log.h"

QMap<CallbackType, QString> callbackFunctions = {
    {OnProjectOpened, "onProjectOpened"},
    {OnProjectClosed, "onProjectClosed"},
    {OnBlockChanged, "onBlockChanged"},
    {OnMapOpened, "onMapOpened"},
};

Scripting *instance = nullptr;

void Scripting::init(MainWindow *mainWindow) {
    if (instance) {
        instance->engine->setInterrupted(true);
        delete instance;
    }
    instance = new Scripting(mainWindow);
}

Scripting::Scripting(MainWindow *mainWindow) {
    this->engine = new QJSEngine(mainWindow);
    this->engine->installExtensions(QJSEngine::ConsoleExtension);
    this->engine->globalObject().setProperty("map", this->engine->newQObject(mainWindow));
    for (QString script : projectConfig.getCustomScripts()) {
        this->filepaths.append(script);
    }
    this->loadModules(this->filepaths);
}

void Scripting::loadModules(QStringList moduleFiles) {
    for (QString filepath : moduleFiles) {
        QJSValue module = this->engine->importModule(filepath);
        if (module.isError()) {
            QString relativePath = QDir::cleanPath(projectConfig.getProjectDir() + QDir::separator() + filepath);
            module = this->engine->importModule(relativePath);
            if (module.isError()) {
                logError(QString("Failed to load custom script file '%1'\nName: %2\nMessage: %3\nFile: %4\nLine Number: %5\nStack: %6")
                         .arg(filepath)
                         .arg(module.property("name").toString())
                         .arg(module.property("message").toString())
                         .arg(module.property("fileName").toString())
                         .arg(module.property("lineNumber").toString())
                         .arg(module.property("stack").toString()));
                continue;
            }
        }

        logInfo(QString("Successfully loaded custom script file '%1'").arg(filepath));
        this->modules.append(module);
    }
}

void Scripting::invokeCallback(CallbackType type, QJSValueList args) {
    for (QJSValue module : this->modules) {
        QString functionName = callbackFunctions[type];
        QJSValue callbackFunction = module.property(functionName);
        if (callbackFunction.isError()) {
            continue;
        }

        QJSValue result = callbackFunction.call(args);
        if (result.isError()) {
            logError(QString("Module %1 encountered an error when calling '%2'").arg(module.toString()).arg(functionName));
            continue;
        }
    }
}

void Scripting::registerAction(QString functionName, QString actionName) {
    if (!instance) return;
    instance->registeredActions.insert(actionName, functionName);
}

int Scripting::numRegisteredActions() {
    if (!instance) return 0;
    return instance->registeredActions.size();
}

void Scripting::invokeAction(QString actionName) {
    if (!instance) return;
    if (!instance->registeredActions.contains(actionName)) return;

    QString functionName = instance->registeredActions.value(actionName);
    for (QJSValue module : instance->modules) {
        QJSValue callbackFunction = module.property(functionName);
        if (callbackFunction.isError()) {
            continue;
        }

        QJSValue result = callbackFunction.call(QJSValueList());
        if (result.isError()) {
            logError(QString("Module %1 encountered an error when calling '%2'").arg(module.toString()).arg(functionName));
            continue;
        }
    }
}

void Scripting::cb_ProjectOpened(QString projectPath) {
    if (!instance) return;

    QJSValueList args {
        projectPath,
    };
    instance->invokeCallback(OnProjectOpened, args);
}

void Scripting::cb_ProjectClosed(QString projectPath) {
    if (!instance) return;

    QJSValueList args {
        projectPath,
    };
    instance->invokeCallback(OnProjectClosed, args);
}

void Scripting::cb_MetatileChanged(int x, int y, Block prevBlock, Block newBlock) {
    if (!instance) return;

    QJSValueList args {
        x,
        y,
        instance->fromBlock(prevBlock),
        instance->fromBlock(newBlock),
    };
    instance->invokeCallback(OnBlockChanged, args);
}

void Scripting::cb_MapOpened(QString mapName) {
    if (!instance) return;

    QJSValueList args {
        mapName,
    };
    instance->invokeCallback(OnMapOpened, args);
}

QJSValue Scripting::fromBlock(Block block) {
    QJSValue obj = instance->engine->newObject();
    obj.setProperty("metatileId", block.tile);
    obj.setProperty("collision", block.collision);
    obj.setProperty("elevation", block.elevation);
    obj.setProperty("rawValue", block.rawValue());
    return obj;
}

QJSValue Scripting::dimensions(int width, int height) {
    QJSValue obj = instance->engine->newObject();
    obj.setProperty("width", width);
    obj.setProperty("height", height);
    return obj;
}

QJSEngine *Scripting::getEngine() {
    return instance->engine;
}
