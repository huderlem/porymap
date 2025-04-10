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
#include "project.h"

namespace Ui {
class MapHeaderForm;
}

class MapHeaderForm : public QWidget
{
    Q_OBJECT

public:
    explicit MapHeaderForm(QWidget *parent = nullptr);
    ~MapHeaderForm();

    void setProject(Project * project, bool allowProjectChanges = true);
    void clear();

    void setHeader(MapHeader *header);
    void setHeaderData(const MapHeader &header);
    MapHeader headerData() const;

    void setSong(const QString &song);
    void setLocation(const QString &location);
    void setLocationName(const QString &locationName);
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
    QString locationName() const;
    bool requiresFlash() const;
    QString weather() const;
    QString type() const;
    QString battleScene() const;
    bool showsLocationName() const;
    bool allowsRunning() const;
    bool allowsBiking() const;
    bool allowsEscaping() const;
    int floorNumber() const;

private:
    Ui::MapHeaderForm *ui;
    QPointer<MapHeader> m_header = nullptr;
    QPointer<Project> m_project = nullptr;
    bool m_allowProjectChanges = true;

    void setLocations(const QStringList &locations);
    void updateLocationName();

    void onSongUpdated(const QString &song);
    void onLocationChanged(const QString &location);
    void onLocationNameChanged(const QString &locationName);
    void onWeatherChanged(const QString &weather);
    void onTypeChanged(const QString &type);
    void onBattleSceneChanged(const QString &battleScene);
    void onRequiresFlashChanged(CheckState selected);
    void onShowLocationNameChanged(CheckState selected);
    void onAllowRunningChanged(CheckState selected);
    void onAllowBikingChanged(CheckState selected);
    void onAllowEscapingChanged(CheckState selected);
    void onFloorNumberChanged(int offset);
};

#endif // MAPHEADERFORM_H
