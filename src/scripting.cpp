#include "scripting.h"
#include "log.h"

QMap<CallbackType, QString> callbackFunctions = {
    {OnProjectOpened, "onProjectOpened"},
    {OnProjectClosed, "onProjectClosed"},
    {OnBlockChanged, "onBlockChanged"},
    {OnBlockHoverChanged, "onBlockHoverChanged"},
    {OnBlockHoverCleared, "onBlockHoverCleared"},
    {OnMapOpened, "onMapOpened"},
    {OnMapResized, "onMapResized"},
    {OnMapShifted, "onMapShifted"},
    {OnTilesetUpdated, "onTilesetUpdated"},
    {OnMainTabChanged, "onMainTabChanged"},
    {OnMapViewTabChanged, "onMapViewTabChanged"},
};

Scripting *instance = nullptr;

void Scripting::init(MainWindow *mainWindow) {
    if (instance) {
        instance->engine->setInterrupted(true);
        qDeleteAll(instance->imageCache);
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
            if (tryErrorJS(module)) continue;
        }

        logInfo(QString("Successfully loaded custom script file '%1'").arg(filepath));
        this->modules.append(module);
    }
}

bool Scripting::tryErrorJS(QJSValue js) {
    if (!js.isError()) return false;

    // Get properties of the error
    QFileInfo file(js.property("fileName").toString());
    QString fileName = file.fileName();
    QString lineNumber = js.property("lineNumber").toString();

    // Convert properties to message strings
    QString fileErrStr = fileName == "undefined" ? "" : QString(" '%1'").arg(fileName);
    QString lineErrStr = lineNumber == "undefined" ? "" : QString(" at line %1").arg(lineNumber);

    logError(QString("Error in custom script%1%2: '%3'")
             .arg(fileErrStr)
             .arg(lineErrStr)
             .arg(js.toString()));
    return true;
}

void Scripting::invokeCallback(CallbackType type, QJSValueList args) {
    for (QJSValue module : this->modules) {
        QString functionName = callbackFunctions[type];
        QJSValue callbackFunction = module.property(functionName);
        if (tryErrorJS(callbackFunction)) continue;

        QJSValue result = callbackFunction.call(args);
        if (tryErrorJS(result)) continue;
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

    bool foundFunction = false;
    QString functionName = instance->registeredActions.value(actionName);
    for (QJSValue module : instance->modules) {
        QJSValue callbackFunction = module.property(functionName);
        if (callbackFunction.isUndefined() || !callbackFunction.isCallable())
            continue;
        foundFunction = true;
        if (tryErrorJS(callbackFunction)) continue;

        QJSValue result = callbackFunction.call(QJSValueList());
        if (tryErrorJS(result)) continue;
    }
    if (!foundFunction)
        logError(QString("Unknown custom script function '%1'").arg(functionName));
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

void Scripting::cb_BlockHoverChanged(int x, int y) {
    if (!instance) return;

    QJSValueList args {
        x,
        y,
    };
    instance->invokeCallback(OnBlockHoverChanged, args);
}

void Scripting::cb_BlockHoverCleared() {
    if (!instance) return;
    instance->invokeCallback(OnBlockHoverCleared, QJSValueList());
}

void Scripting::cb_MapOpened(QString mapName) {
    if (!instance) return;

    QJSValueList args {
        mapName,
    };
    instance->invokeCallback(OnMapOpened, args);
}

void Scripting::cb_MapResized(int oldWidth, int oldHeight, int newWidth, int newHeight) {
    if (!instance) return;

    QJSValueList args {
        oldWidth,
        oldHeight,
        newWidth,
        newHeight,
    };
    instance->invokeCallback(OnMapResized, args);
}

void Scripting::cb_MapShifted(int xDelta, int yDelta) {
    if (!instance) return;

    QJSValueList args {
        xDelta,
        yDelta,
    };
    instance->invokeCallback(OnMapShifted, args);
}

void Scripting::cb_TilesetUpdated(QString tilesetName) {
    if (!instance) return;

    QJSValueList args {
        tilesetName,
    };
    instance->invokeCallback(OnTilesetUpdated, args);
}

void Scripting::cb_MainTabChanged(int oldTab, int newTab) {
    if (!instance) return;

    QJSValueList args {
        oldTab,
        newTab,
    };
    instance->invokeCallback(OnMainTabChanged, args);
}

void Scripting::cb_MapViewTabChanged(int oldTab, int newTab) {
    if (!instance) return;

    QJSValueList args {
        oldTab,
        newTab,
    };
    instance->invokeCallback(OnMapViewTabChanged, args);
}

QJSValue Scripting::fromBlock(Block block) {
    QJSValue obj = instance->engine->newObject();
    obj.setProperty("metatileId", block.metatileId);
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

QJSValue Scripting::position(int x, int y) {
    QJSValue obj = instance->engine->newObject();
    obj.setProperty("x", x);
    obj.setProperty("y", y);
    return obj;
}

Tile Scripting::toTile(QJSValue obj) {
    if (!obj.hasProperty("tileId")
     || !obj.hasProperty("xflip")
     || !obj.hasProperty("yflip")
     || !obj.hasProperty("palette")) {
        return Tile();
    }
    Tile tile = Tile();
    tile.tileId = obj.property("tileId").toInt();
    tile.xflip = obj.property("xflip").toBool();
    tile.yflip = obj.property("yflip").toBool();
    tile.palette = obj.property("palette").toInt();
    return tile;
}

QJSValue Scripting::fromTile(Tile tile) {
    QJSValue obj = instance->engine->newObject();
    obj.setProperty("tileId", tile.tileId);
    obj.setProperty("xflip", tile.xflip);
    obj.setProperty("yflip", tile.yflip);
    obj.setProperty("palette", tile.palette);
    return obj;
}

QJSEngine *Scripting::getEngine() {
    return instance->engine;
}

QImage Scripting::getImage(QString filepath) {
    const QImage * image = instance->imageCache.value(filepath, nullptr);
    if (!image) {
        image = new QImage(filepath);
        instance->imageCache.insert(filepath, image);
    }
    return QImage(*image);
}
