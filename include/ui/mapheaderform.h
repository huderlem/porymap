#ifndef MAPHEADERFORM_H
#define MAPHEADERFORM_H

/*
    This is the UI class used to edit the fields in a map's header.
    It's intended to be used anywhere the UI needs to present an editor for a map's header,
    e.g. for the current map in the main editor or in the new map dialog.
*/

#include <QWidget>
#include <QPointer>
#include "mapheader.h"

class Project;

namespace Ui {
class MapHeaderForm;
}

class MapHeaderForm : public QWidget
{
    Q_OBJECT

public:
    explicit MapHeaderForm(QWidget *parent = nullptr);
    ~MapHeaderForm();

    void init(const Project * project);
    void clear();

    void setHeader(MapHeader *header);
    void setHeaderData(const MapHeader &header);
    MapHeader headerData() const;

    void setSong(const QString &song);
    void setLocation(const QString &location);
    void setRequiresFlash(bool requiresFlash);
    void setWeather(const QString &weather);
    void setType(const QString &type);
    void setBattleScene(const QString &battleScene);
    void setShowsLocationName(bool showsLocationName);
    void setAllowsRunning(bool allowsRunning);
    void setAllowsBiking(bool allowsBiking);
    void setAllowsEscaping(bool allowsEscaping);
    void setFloorNumber(int floorNumber);

    QString song() const;
    QString location() const;
    bool requiresFlash() const;
    QString weather() const;
    QString type() const;
    QString battleScene() const;
    bool showsLocationName() const;
    bool allowsRunning() const;
    bool allowsBiking() const;
    bool allowsEscaping() const;
    int floorNumber() const;

    void setLocations(QStringList locations);

private:
    Ui::MapHeaderForm *ui;
    QPointer<MapHeader> m_header = nullptr;

    void onSongUpdated(const QString &song);
    void onLocationChanged(const QString &location);
    void onWeatherChanged(const QString &weather);
    void onTypeChanged(const QString &type);
    void onBattleSceneChanged(const QString &battleScene);
    void onRequiresFlashChanged(int selected);
    void onShowLocationNameChanged(int selected);
    void onAllowRunningChanged(int selected);
    void onAllowBikingChanged(int selected);
    void onAllowEscapingChanged(int selected);
    void onFloorNumberChanged(int offset);
};

#endif // MAPHEADERFORM_H
