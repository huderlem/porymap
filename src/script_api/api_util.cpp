#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "scripting.h"
#include "config.h"

void MainWindow::registerAction(QString functionName, QString actionName, QString shortcut) {
    if (!this->ui || !this->ui->menuTools)
        return;

    Scripting::registerAction(functionName, actionName);
    if (Scripting::numRegisteredActions() == 1) {
        QAction *section = this->ui->menuTools->addSection("Custom Actions");
        this->registeredActions.append(section);
    }
    QAction *action = this->ui->menuTools->addAction(actionName, [actionName](){
       Scripting::invokeAction(actionName);
    });
    if (!shortcut.isEmpty()) {
        action->setShortcut(QKeySequence(shortcut));
    }
    this->registeredActions.append(action);
}

void MainWindow::setTimeout(QJSValue callback, int milliseconds) {
  if (!callback.isCallable() || milliseconds < 0)
      return;

    QTimer *timer = new QTimer(0);
    connect(timer, &QTimer::timeout, [=](){
        this->invokeCallback(callback);
    });
    connect(timer, &QTimer::timeout, timer, &QTimer::deleteLater);
    timer->setSingleShot(true);
    timer->start(milliseconds);
}

void MainWindow::invokeCallback(QJSValue callback) {
    Scripting::tryErrorJS(callback.call());
}

void MainWindow::log(QString message) {
    logInfo(message);
}

void MainWindow::warn(QString message) {
    logWarn(message);
}

void MainWindow::error(QString message) {
    logError(message);
}

void MainWindow::runMessageBox(QString text, QString informativeText, QString detailedText, QMessageBox::Icon icon) {
    QMessageBox messageBox(this);
    messageBox.setText(text);
    messageBox.setInformativeText(informativeText);
    messageBox.setDetailedText(detailedText);
    messageBox.setIcon(icon);
    messageBox.exec();
}

void MainWindow::showMessage(QString text, QString informativeText, QString detailedText) {
    this->runMessageBox(text, informativeText, detailedText, QMessageBox::Information);
}

void MainWindow::showWarning(QString text, QString informativeText, QString detailedText) {
    this->runMessageBox(text, informativeText, detailedText, QMessageBox::Warning);
}

void MainWindow::showError(QString text, QString informativeText, QString detailedText) {
    this->runMessageBox(text, informativeText, detailedText, QMessageBox::Critical);
}

bool MainWindow::showQuestion(QString text, QString informativeText, QString detailedText) {
    QMessageBox messageBox(this);
    messageBox.setText(text);
    messageBox.setInformativeText(informativeText);
    messageBox.setDetailedText(detailedText);
    messageBox.setIcon(QMessageBox::Question);
    messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    return messageBox.exec() == QMessageBox::Yes;
}

QJSValue MainWindow::getInputText(QString title, QString label, QString defaultValue) {
    bool ok;
    QString input = QInputDialog::getText(this, title, label, QLineEdit::Normal, defaultValue, &ok);
    return Scripting::dialogInput(input, ok);
}

QJSValue MainWindow::getInputNumber(QString title, QString label, double defaultValue, double min, double max, int decimals, double step) {
    bool ok;
    double input = QInputDialog::getDouble(this, title, label, defaultValue, min, max, decimals, &ok, Qt::WindowFlags(), step);
    return Scripting::dialogInput(input, ok);
}

QJSValue MainWindow::getInputItem(QString title, QString label, QStringList items, int defaultValue, bool editable) {
    bool ok;
    QString input = QInputDialog::getItem(this, title, label, items, defaultValue, editable, &ok);
    return Scripting::dialogInput(input, ok);
}

int MainWindow::getMainTab() {
    if (!this->ui || !this->ui->mainTabBar)
        return -1;
    return this->ui->mainTabBar->currentIndex();
}

void MainWindow::setMainTab(int index) {
    if (!this->ui || !this->ui->mainTabBar || index < 0 || index >= this->ui->mainTabBar->count())
        return;
    // Can't select Wild Encounters tab if it's disabled
    if (index == 4 && !projectConfig.getEncounterJsonActive())
        return;
    this->on_mainTabBar_tabBarClicked(index);
}

int MainWindow::getMapViewTab() {
    if (!this->ui || !this->ui->mapViewTab)
        return -1;
    return this->ui->mapViewTab->currentIndex();
}

void MainWindow::setMapViewTab(int index) {
    if (this->getMainTab() != 0 || !this->ui->mapViewTab || index < 0 || index >= this->ui->mapViewTab->count())
        return;
    this->on_mapViewTab_tabBarClicked(index);
}

void MainWindow::setGridVisibility(bool visible) {
    this->ui->checkBox_ToggleGrid->setChecked(visible);
}

bool MainWindow::getGridVisibility() {
    return this->ui->checkBox_ToggleGrid->isChecked();
}

void MainWindow::setBorderVisibility(bool visible) {
    this->editor->toggleBorderVisibility(visible, false);
}

bool MainWindow::getBorderVisibility() {
    return this->ui->checkBox_ToggleBorder->isChecked();
}

void MainWindow::setSmartPathsEnabled(bool visible) {
    this->ui->checkBox_smartPaths->setChecked(visible);
}

bool MainWindow::getSmartPathsEnabled() {
    return this->ui->checkBox_smartPaths->isChecked();
}

QList<int> MainWindow::getMetatileLayerOrder() {
    if (!this->editor || !this->editor->map)
        return QList<int>();
    return this->editor->map->metatileLayerOrder;
}

void MainWindow::setMetatileLayerOrder(QList<int> order) {
    if (!this->editor || !this->editor->map)
        return;

    const int numLayers = 3;
    int size = order.size();
    if (size < numLayers) {
        logError(QString("Metatile layer order has insufficient elements (%1), needs at least %2.").arg(size).arg(numLayers));
        return;
    }
    bool invalid = false;
    for (int i = 0; i < numLayers; i++) {
        int layer = order.at(i);
        if (layer < 0 || layer >= numLayers) {
            logError(QString("'%1' is not a valid metatile layer order value, must be in range 0-%2.").arg(layer).arg(numLayers - 1));
            invalid = true;
        }
    }
    if (invalid) return;

    this->editor->map->metatileLayerOrder = order;
    this->refreshAfterPalettePreviewChange();
}

QList<float> MainWindow::getMetatileLayerOpacity() {
    if (!this->editor || !this->editor->map)
        return QList<float>();
    return this->editor->map->metatileLayerOpacity;
}

void MainWindow::setMetatileLayerOpacity(QList<float> order) {
    if (!this->editor || !this->editor->map)
        return;
    this->editor->map->metatileLayerOpacity = order;
    this->refreshAfterPalettePreviewChange();
}

bool MainWindow::isPrimaryTileset(QString tilesetName) {
    if (!this->editor || !this->editor->project)
        return false;
    return this->editor->project->tilesetLabels["primary"].contains(tilesetName);
}

bool MainWindow::isSecondaryTileset(QString tilesetName) {
    if (!this->editor || !this->editor->project)
        return false;
    return this->editor->project->tilesetLabels["secondary"].contains(tilesetName);
}

QList<QString> MainWindow::getCustomScripts() {
    return projectConfig.getCustomScripts();
}

