#ifdef QT_QML_LIB
#include "scriptutility.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "scripting.h"
#include "config.h"

ScriptUtility::~ScriptUtility() {
    if (window && window->ui && window->ui->menuTools) {
        for (auto action : this->registeredActions) {
            window->ui->menuTools->removeAction(action);
        }
    }
    for (auto timer : this->activeTimers) {
        timer->stop();
        delete timer;
    }
}

bool ScriptUtility::registerAction(QString functionName, QString actionName, QString shortcut) {
    if (!window || !window->ui || !window->ui->menuTools)
        return false;

    if (functionName.isEmpty() || actionName.isEmpty()) {
        logError("Failed to register script action. 'functionName' and 'actionName' must be non-empty.");
        return false;
    }

    if (this->registeredActions.size() == 0) {
        QAction *section = window->ui->menuTools->addSection("Custom Actions");
        this->registeredActions.append(section);
    }

    const int actionIndex = this->registeredActions.size();
    QAction *action = window->ui->menuTools->addAction(actionName, [actionIndex](){
       Scripting::invokeAction(actionIndex);
    });

    if (!shortcut.isEmpty()) {
        action->setShortcut(QKeySequence(shortcut));
    }

    this->actionMap.insert(actionIndex, functionName);
    this->registeredActions.append(action);
    return true;
}

bool ScriptUtility::registerToggleAction(QString functionName, QString actionName, QString shortcut, bool checked) {
    if (!registerAction(functionName, actionName, shortcut))
        return false;
    QAction *action = this->registeredActions.last();
    action->setCheckable(true);
    action->setChecked(checked);
    return true;
}

QString ScriptUtility::getActionFunctionName(int actionIndex) {
    return this->actionMap.value(actionIndex);
}

void ScriptUtility::setTimeout(QJSValue callback, int milliseconds) {
  if (!callback.isCallable() || milliseconds < 0)
      return;

    QTimer *timer = new QTimer();
    connect(timer, &QTimer::timeout, [=](){
        if (this->activeTimers.remove(timer)) {
            this->callTimeoutFunction(callback);
            timer->deleteLater();
        }
    });

    this->activeTimers.insert(timer);
    timer->setSingleShot(true);
    timer->start(milliseconds);
}

void ScriptUtility::callTimeoutFunction(QJSValue callback) {
    Scripting::tryErrorJS(callback.call());
}

void ScriptUtility::log(QString message) {
    logInfo(message);
}

void ScriptUtility::warn(QString message) {
    logWarn(message);
}

void ScriptUtility::error(QString message) {
    logError(message);
}

void ScriptUtility::runMessageBox(QString text, QString informativeText, QString detailedText, QMessageBox::Icon icon) {
    QMessageBox messageBox(window);
    messageBox.setText(text);
    messageBox.setInformativeText(informativeText);
    messageBox.setDetailedText(detailedText);
    messageBox.setIcon(icon);
    messageBox.exec();
}

void ScriptUtility::showMessage(QString text, QString informativeText, QString detailedText) {
    this->runMessageBox(text, informativeText, detailedText, QMessageBox::Information);
}

void ScriptUtility::showWarning(QString text, QString informativeText, QString detailedText) {
    this->runMessageBox(text, informativeText, detailedText, QMessageBox::Warning);
}

void ScriptUtility::showError(QString text, QString informativeText, QString detailedText) {
    this->runMessageBox(text, informativeText, detailedText, QMessageBox::Critical);
}

bool ScriptUtility::showQuestion(QString text, QString informativeText, QString detailedText) {
    QMessageBox messageBox(window);
    messageBox.setText(text);
    messageBox.setInformativeText(informativeText);
    messageBox.setDetailedText(detailedText);
    messageBox.setIcon(QMessageBox::Question);
    messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    return messageBox.exec() == QMessageBox::Yes;
}

QJSValue ScriptUtility::getInputText(QString title, QString label, QString defaultValue) {
    bool ok;
    QString input = QInputDialog::getText(window, title, label, QLineEdit::Normal, defaultValue, &ok);
    return Scripting::dialogInput(input, ok);
}

QJSValue ScriptUtility::getInputNumber(QString title, QString label, double defaultValue, double min, double max, int decimals, double step) {
    bool ok;
    double input = QInputDialog::getDouble(window, title, label, defaultValue, min, max, decimals, &ok, Qt::WindowFlags(), step);
    return Scripting::dialogInput(input, ok);
}

QJSValue ScriptUtility::getInputItem(QString title, QString label, QStringList items, int defaultValue, bool editable) {
    bool ok;
    QString input = QInputDialog::getItem(window, title, label, items, defaultValue, editable, &ok);
    return Scripting::dialogInput(input, ok);
}

int ScriptUtility::getMainTab() {
    if (!window || !window->ui || !window->ui->mainTabBar)
        return -1;
    return window->ui->mainTabBar->currentIndex();
}

void ScriptUtility::setMainTab(int index) {
    if (!window || !window->ui || !window->ui->mainTabBar || index < 0 || index >= window->ui->mainTabBar->count())
        return;
    // Can't select tab if it's disabled
    if (!window->ui->mainTabBar->isTabEnabled(index))
        return;
    window->on_mainTabBar_tabBarClicked(index);
}

int ScriptUtility::getMapViewTab() {
    if (!window || !window->ui || !window->ui->mapViewTab)
        return -1;
    return window->ui->mapViewTab->currentIndex();
}

void ScriptUtility::setMapViewTab(int index) {
    if (this->getMainTab() != MainTab::Map || !window->ui->mapViewTab || index < 0 || index >= window->ui->mapViewTab->count())
        return;
    // Can't select tab if it's disabled
    if (!window->ui->mapViewTab->isTabEnabled(index))
        return;
    window->on_mapViewTab_tabBarClicked(index);
}

void ScriptUtility::setGridVisibility(bool visible) {
    window->ui->checkBox_ToggleGrid->setChecked(visible);
}

bool ScriptUtility::getGridVisibility() {
    return window->ui->checkBox_ToggleGrid->isChecked();
}

void ScriptUtility::setBorderVisibility(bool visible) {
    window->ui->checkBox_ToggleBorder->setChecked(visible);
}

bool ScriptUtility::getBorderVisibility() {
    return window->ui->checkBox_ToggleBorder->isChecked();
}

void ScriptUtility::setSmartPathsEnabled(bool visible) {
    window->ui->checkBox_smartPaths->setChecked(visible);
}

bool ScriptUtility::getSmartPathsEnabled() {
    return window->ui->checkBox_smartPaths->isChecked();
}

QList<QString> ScriptUtility::getCustomScripts() {
    return userConfig.getCustomScriptPaths();
}

QList<int> ScriptUtility::getMetatileLayerOrder() {
    return Layout::globalMetatileLayerOrder();
}

bool ScriptUtility::validateMetatileLayerOrder(const QList<int> &order) {
    const int numLayers = 3;
    bool valid = true;
    for (int i = 0; i < qMin(order.length(), numLayers); i++) {
        int layer = order.at(i);
        if (layer < 0 || layer >= numLayers) {
            logError(QString("'%1' is not a valid metatile layer order value, must be in range 0-%2.").arg(layer).arg(numLayers - 1));
            valid = false;
        }
    }
    return valid;
}

void ScriptUtility::setMetatileLayerOrder(const QList<int> &order) {
    if (!validateMetatileLayerOrder(order))
        return;
    Layout::setGlobalMetatileLayerOrder(order);
    if (window) window->refreshAfterPalettePreviewChange();
}

QList<float> ScriptUtility::getMetatileLayerOpacity() {
    return Layout::globalMetatileLayerOpacity();
}

void ScriptUtility::setMetatileLayerOpacity(const QList<float> &opacities) {
    Layout::setGlobalMetatileLayerOpacity(opacities);
    if (window) window->refreshAfterPalettePreviewChange();
}

QList<QString> ScriptUtility::getMapNames() {
    if (!window || !window->editor || !window->editor->project)
        return QList<QString>();
    return window->editor->project->mapNames();
}

QList<QString> ScriptUtility::getMapConstants() {
    if (!window || !window->editor || !window->editor->project)
        return QList<QString>();
    return window->editor->project->mapConstantsToMapNames.keys();
}

QList<QString> ScriptUtility::getLayoutNames() {
    if (!window || !window->editor || !window->editor->project)
        return {};
    return window->editor->project->getLayoutNames();
}

QList<QString> ScriptUtility::getLayoutConstants() {
    if (!window || !window->editor || !window->editor->project)
        return QList<QString>();
    return window->editor->project->layoutIds();
}

QList<QString> ScriptUtility::getTilesetNames() {
    if (!window || !window->editor || !window->editor->project)
        return QList<QString>();
    return window->editor->project->tilesetLabelsOrdered;
}

QList<QString> ScriptUtility::getPrimaryTilesetNames() {
    if (!window || !window->editor || !window->editor->project)
        return QList<QString>();
    return window->editor->project->primaryTilesetLabels;
}

QList<QString> ScriptUtility::getSecondaryTilesetNames() {
    if (!window || !window->editor || !window->editor->project)
        return QList<QString>();
    return window->editor->project->secondaryTilesetLabels;
}

QList<QString> ScriptUtility::getMetatileBehaviorNames() {
    if (!window || !window->editor || !window->editor->project)
        return QList<QString>();
    return window->editor->project->metatileBehaviorMap.keys();
}

QList<QString> ScriptUtility::getSongNames() {
    if (!window || !window->editor || !window->editor->project)
        return QList<QString>();
    return window->editor->project->songNames;
}

QList<QString> ScriptUtility::getLocationNames() {
    if (!window || !window->editor || !window->editor->project)
        return QList<QString>();
    return window->editor->project->locationNames();
}

QList<QString> ScriptUtility::getWeatherNames() {
    if (!window || !window->editor || !window->editor->project)
        return QList<QString>();
    return window->editor->project->weatherNames;
}

QList<QString> ScriptUtility::getMapTypeNames() {
    if (!window || !window->editor || !window->editor->project)
        return QList<QString>();
    return window->editor->project->mapTypes;
}

QList<QString> ScriptUtility::getBattleSceneNames() {
    if (!window || !window->editor || !window->editor->project)
        return QList<QString>();
    return window->editor->project->mapBattleScenes;
}

bool ScriptUtility::isPrimaryTileset(QString tilesetName) {
    return getPrimaryTilesetNames().contains(tilesetName);
}

bool ScriptUtility::isSecondaryTileset(QString tilesetName) {
    return getSecondaryTilesetNames().contains(tilesetName);
}

#endif // QT_QML_LIB
