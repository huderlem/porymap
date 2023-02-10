#pragma once
#ifndef SCRIPTUTILITY_H
#define SCRIPTUTILITY_H

#include "mainwindow.h"

class ScriptUtility : public QObject
{
    Q_OBJECT

public:
    ScriptUtility(MainWindow *mainWindow);
    void clearActions();
    QString getActionFunctionName(int actionIndex);
    Q_INVOKABLE bool registerAction(QString functionName, QString actionName, QString shortcut = "");
    Q_INVOKABLE bool registerToggleAction(QString functionName, QString actionName, QString shortcut = "", bool checked = false);
    Q_INVOKABLE void setTimeout(QJSValue callback, int milliseconds);
    Q_INVOKABLE void log(QString message);
    Q_INVOKABLE void warn(QString message);
    Q_INVOKABLE void error(QString message);
    Q_INVOKABLE void showMessage(QString text, QString informativeText = "", QString detailedText = "");
    Q_INVOKABLE void showWarning(QString text, QString informativeText = "", QString detailedText = "");
    Q_INVOKABLE void showError(QString text, QString informativeText = "", QString detailedText = "");
    Q_INVOKABLE bool showQuestion(QString text, QString informativeText = "", QString detailedText = "");
    Q_INVOKABLE QJSValue getInputText(QString title, QString label, QString defaultValue = "");
    Q_INVOKABLE QJSValue getInputNumber(QString title, QString label, double defaultValue = 0, double min = INT_MIN, double max = INT_MAX, int decimals = 0, double step = 1);
    Q_INVOKABLE QJSValue getInputItem(QString title, QString label, QStringList items, int defaultValue = 0, bool editable = false);
    Q_INVOKABLE int getMainTab();
    Q_INVOKABLE void setMainTab(int index);
    Q_INVOKABLE int getMapViewTab();
    Q_INVOKABLE void setMapViewTab(int index);
    Q_INVOKABLE void setGridVisibility(bool visible);
    Q_INVOKABLE bool getGridVisibility();
    Q_INVOKABLE void setBorderVisibility(bool visible);
    Q_INVOKABLE bool getBorderVisibility();
    Q_INVOKABLE void setSmartPathsEnabled(bool visible);
    Q_INVOKABLE bool getSmartPathsEnabled();
    Q_INVOKABLE QList<QString> getCustomScripts();
    Q_INVOKABLE QList<int> getMetatileLayerOrder();
    Q_INVOKABLE void setMetatileLayerOrder(QList<int> order);
    Q_INVOKABLE QList<float> getMetatileLayerOpacity();
    Q_INVOKABLE void setMetatileLayerOpacity(QList<float> order);
    Q_INVOKABLE QList<QString> getMapNames();
    Q_INVOKABLE QList<QString> getTilesetNames();
    Q_INVOKABLE QList<QString> getPrimaryTilesetNames();
    Q_INVOKABLE QList<QString> getSecondaryTilesetNames();
    Q_INVOKABLE QList<QString> getMetatileBehaviorNames();
    Q_INVOKABLE QList<QString> getSongNames();
    Q_INVOKABLE QList<QString> getLocationNames();
    Q_INVOKABLE QList<QString> getWeatherNames();
    Q_INVOKABLE QList<QString> getMapTypeNames();
    Q_INVOKABLE QList<QString> getBattleSceneNames();
    Q_INVOKABLE bool isPrimaryTileset(QString tilesetName);
    Q_INVOKABLE bool isSecondaryTileset(QString tilesetName);

private:
    void callTimeoutFunction(QJSValue callback);
    void runMessageBox(QString text, QString informativeText, QString detailedText, QMessageBox::Icon icon);

    MainWindow *window;
    QList<QAction *> registeredActions;
    QHash<int, QString> actionMap;
};

#endif // SCRIPTUTILITY_H
