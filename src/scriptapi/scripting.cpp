#include "scripting.h"
#include "log.h"
#include "config.h"
#include "aboutporymap.h"

QMap<CallbackType, QString> callbackFunctions = {
    {OnProjectOpened, "onProjectOpened"},
    {OnProjectClosed, "onProjectClosed"},
    {OnBlockChanged, "onBlockChanged"},
    {OnBorderMetatileChanged, "onBorderMetatileChanged"},
    {OnBlockHoverChanged, "onBlockHoverChanged"},
    {OnBlockHoverCleared, "onBlockHoverCleared"},
    {OnMapOpened, "onMapOpened"},
    {OnMapResized, "onMapResized"},
    {OnBorderResized, "onBorderResized"},
    {OnMapShifted, "onMapShifted"},
    {OnTilesetUpdated, "onTilesetUpdated"},
    {OnMainTabChanged, "onMainTabChanged"},
    {OnMapViewTabChanged, "onMapViewTabChanged"},
    {OnBorderVisibilityToggled, "onBorderVisibilityToggled"},
};

Scripting *instance = nullptr;

void Scripting::init(MainWindow *mainWindow) {
    if (instance) {
        instance->engine->setInterrupted(true);
        instance->scriptUtility->clearActions();
        qDeleteAll(instance->imageCache);
        delete instance;
    }
    instance = new Scripting(mainWindow);
}

Scripting::Scripting(MainWindow *mainWindow) {
    this->mainWindow = mainWindow;
    this->engine = new QJSEngine(mainWindow);
    this->engine->installExtensions(QJSEngine::ConsoleExtension);
    const QStringList paths = userConfig.getCustomScriptPaths();
    const QList<bool> enabled = userConfig.getCustomScriptsEnabled();
    for (int i = 0; i < paths.length(); i++) {
        if (enabled.at(i))
            this->filepaths.append(paths.at(i));
    }
    this->loadModules(this->filepaths);
    this->scriptUtility = new ScriptUtility(mainWindow);
}

void Scripting::loadModules(QStringList moduleFiles) {
    for (QString filepath : moduleFiles) {
        QString validPath = Project::getExistingFilepath(filepath);
        if (!validPath.isEmpty()) filepath = validPath; // Otherwise allow it to fail with the original path

        QJSValue module = this->engine->importModule(filepath);
        if (tryErrorJS(module)) {
            QMessageBox messageBox(this->mainWindow);
            messageBox.setText("Failed to load script");
            messageBox.setInformativeText(QString("An error occurred while loading custom script file '%1'").arg(filepath));
            messageBox.setDetailedText(getMostRecentError());
            messageBox.setIcon(QMessageBox::Warning);
            messageBox.addButton(QMessageBox::Ok);
            messageBox.exec();
            continue;
        }
        logInfo(QString("Successfully loaded custom script file '%1'").arg(filepath));
        this->modules.append(module);
    }
}

void Scripting::populateGlobalObject(MainWindow *mainWindow) {
    if (!instance || !instance->engine) return;

    instance->engine->globalObject().setProperty("map", instance->engine->newQObject(mainWindow));
    instance->engine->globalObject().setProperty("overlay", instance->engine->newQObject(mainWindow->ui->graphicsView_Map));
    instance->engine->globalObject().setProperty("utility", instance->engine->newQObject(instance->scriptUtility));

    QJSValue constants = instance->engine->newObject();

    // Invisibly create an "About" window to read Porymap version
    AboutPorymap *about = new AboutPorymap(mainWindow);
    if (about) {
        QJSValue version = Scripting::version(about->getVersionNumbers());
        constants.setProperty("version", version);
        delete about;
    } else {
        logError("Failed to read Porymap version for API");
    }

    // Get basic tileset information
    int numTilesPrimary = Project::getNumTilesPrimary();
    int numMetatilesPrimary = Project::getNumMetatilesPrimary();
    int numPalettesPrimary = Project::getNumPalettesPrimary();
    constants.setProperty("max_primary_tiles", numTilesPrimary);
    constants.setProperty("max_secondary_tiles", Project::getNumTilesTotal() - numTilesPrimary);
    constants.setProperty("max_primary_metatiles", numMetatilesPrimary);
    constants.setProperty("max_secondary_metatiles", Project::getNumMetatilesTotal() - numMetatilesPrimary);
    constants.setProperty("num_primary_palettes", numPalettesPrimary);
    constants.setProperty("num_secondary_palettes", Project::getNumPalettesTotal() - numPalettesPrimary);
    constants.setProperty("layers_per_metatile", projectConfig.getNumLayersInMetatile());
    constants.setProperty("tiles_per_metatile", projectConfig.getNumTilesInMetatile());

    constants.setProperty("base_game_version", projectConfig.getBaseGameVersionString());

    // Read out behavior values into constants object
    QJSValue behaviorsArray = instance->engine->newObject();
    const QMap<QString, uint32_t> * map = &mainWindow->editor->project->metatileBehaviorMap;
    for (auto i = map->cbegin(), end = map->cend(); i != end; i++)
        behaviorsArray.setProperty(i.key(), i.value());
    constants.setProperty("metatile_behaviors", behaviorsArray);

    instance->engine->globalObject().setProperty("constants", constants);

    // Prevent changes to the constants object
    instance->engine->evaluate("Object.freeze(constants.metatile_behaviors);");
    instance->engine->evaluate("Object.freeze(constants.version);");
    instance->engine->evaluate("Object.freeze(constants);");
}

bool Scripting::tryErrorJS(QJSValue js) {
    if (!js.isError())
        return false;

    // Get properties of the error
    QFileInfo file(js.property("fileName").toString());
    QString fileName = file.fileName();
    QString lineNumber = js.property("lineNumber").toString();
    QString errStr = js.toString();

    // The script engine is interrupted during project reopen, during which
    // all script modules intentionally return as error objects.
    // We don't need to report these "errors" to the user.
    if (errStr == "Error: Interrupted")
        return false;

    // Convert properties to message strings
    QString fileErrStr = fileName == "undefined" ? "" : QString(" '%1'").arg(fileName);
    QString lineErrStr = lineNumber == "undefined" ? "" : QString(" at line %1").arg(lineNumber);

    logError(QString("Error in custom script%1%2: '%3'")
             .arg(fileErrStr)
             .arg(lineErrStr)
             .arg(errStr));
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

void Scripting::invokeAction(int actionIndex) {
    if (!instance || !instance->scriptUtility) return;
    QString functionName = instance->scriptUtility->getActionFunctionName(actionIndex);
    if (functionName.isEmpty()) return;

    bool foundFunction = false;
    for (QJSValue module : instance->modules) {
        QJSValue callbackFunction = module.property(functionName);
        if (callbackFunction.isUndefined() || !callbackFunction.isCallable())
            continue;
        foundFunction = true;
        if (tryErrorJS(callbackFunction)) continue;

        QJSValue result = callbackFunction.call(QJSValueList());
        if (tryErrorJS(result)) continue;
    }
    if (!foundFunction) {
        logError(QString("Unknown custom script function '%1'").arg(functionName));
        QMessageBox messageBox(instance->mainWindow);
        messageBox.setText("Failed to run custom action");
        messageBox.setInformativeText(getMostRecentError());
        messageBox.setIcon(QMessageBox::Warning);
        messageBox.addButton(QMessageBox::Ok);
        messageBox.exec();
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

void Scripting::cb_BorderMetatileChanged(int x, int y, uint16_t prevMetatileId, uint16_t newMetatileId) {
    if (!instance) return;

    QJSValueList args {
        x,
        y,
        prevMetatileId,
        newMetatileId,
    };
    instance->invokeCallback(OnBorderMetatileChanged, args);
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

void Scripting::cb_BorderResized(int oldWidth, int oldHeight, int newWidth, int newHeight) {
    if (!instance) return;

    QJSValueList args {
        oldWidth,
        oldHeight,
        newWidth,
        newHeight,
    };
    instance->invokeCallback(OnBorderResized, args);
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

void Scripting::cb_BorderVisibilityToggled(bool visible) {
    if (!instance) return;

    QJSValueList args {
        visible,
    };
    instance->invokeCallback(OnBorderVisibilityToggled, args);
}

QJSValue Scripting::fromBlock(Block block) {
    QJSValue obj = instance->engine->newObject();
    obj.setProperty("metatileId", block.metatileId());
    obj.setProperty("collision", block.collision());
    obj.setProperty("elevation", block.elevation());
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

QJSValue Scripting::version(QList<int> versionNums) {
    QJSValue obj = instance->engine->newObject();
    obj.setProperty("major", versionNums.at(0));
    obj.setProperty("minor", versionNums.at(1));
    obj.setProperty("patch", versionNums.at(2));
    return obj;
}

Tile Scripting::toTile(QJSValue obj) {
    Tile tile = Tile();

    if (obj.hasProperty("tileId"))
        tile.tileId = obj.property("tileId").toInt();
    if (obj.hasProperty("xflip"))
        tile.xflip = obj.property("xflip").toBool();
    if (obj.hasProperty("yflip"))
        tile.yflip = obj.property("yflip").toBool();
    if (obj.hasProperty("palette"))
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

QJSValue Scripting::dialogInput(QJSValue input, bool selectedOk) {
    QJSValue obj = instance->engine->newObject();
    obj.setProperty("input", input);
    obj.setProperty("ok", selectedOk);
    return obj;
}

QJSEngine *Scripting::getEngine() {
    return instance->engine;
}

const QImage * Scripting::getImage(const QString &inputFilepath, bool useCache) {
    if (inputFilepath.isEmpty())
        return nullptr;

    const QImage * image;
    if (useCache) {
        // Try to retrieve image from the cache
        image = instance->imageCache.value(inputFilepath, nullptr);
        if (image) return image;
    }

    const QString filepath = Project::getExistingFilepath(inputFilepath);
    if (filepath.isEmpty())
        return nullptr;

    image = new QImage(filepath);
    instance->imageCache.insert(inputFilepath, image);
    return image;
}
