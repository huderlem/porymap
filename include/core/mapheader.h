#ifndef MAPHEADER_H
#define MAPHEADER_H

#include <QObject>

class MapHeader : public QObject
{
    Q_OBJECT
public:
    MapHeader(QObject *parent = nullptr) : QObject(parent) {};
    ~MapHeader() {};
    MapHeader(const MapHeader& other);
    MapHeader& operator=(const MapHeader& other);
    bool operator==(const MapHeader& other) const {
        return m_song == other.m_song
        && m_location == other.m_location
        && m_region == other.m_region
        && m_requiresFlash == other.m_requiresFlash
        && m_weather == other.m_weather
        && m_type == other.m_type
        && m_showsLocationName == other.m_showsLocationName
        && m_allowsRunning == other.m_allowsRunning
        && m_allowsBiking == other.m_allowsBiking
        && m_allowsEscaping == other.m_allowsEscaping
        && m_floorNumber == other.m_floorNumber
        && m_battleScene == other.m_battleScene;
    }
    bool operator!=(const MapHeader& other) const {
        return !(operator==(other));
    }

    void setSong(const QString &song);
    void setLocation(const QString &location);
    void setRegion(const QString &region);
    void setUsesMultiRegion(bool usesMultiRegion);
    void setRequiresFlash(bool requiresFlash);
    void setWeather(const QString &weather);
    void setType(const QString &type);
    void setShowsLocationName(bool showsLocationName);
    void setAllowsRunning(bool allowsRunning);
    void setAllowsBiking(bool allowsBiking);
    void setAllowsEscaping(bool allowsEscaping);
    void setFloorNumber(int floorNumber);
    void setBattleScene(const QString &battleScene);

    QString song() const { return m_song; }
    QString location() const { return m_location; }
    QString region() const { return m_region; }
    bool usesMultiRegion() const { return m_usesMultiRegion; }
    bool requiresFlash() const { return m_requiresFlash; }
    QString weather() const { return m_weather; }
    QString type() const { return m_type; }
    bool showsLocationName() const { return m_showsLocationName; }
    bool allowsRunning() const { return m_allowsRunning; }
    bool allowsBiking() const { return m_allowsBiking; }
    bool allowsEscaping() const { return m_allowsEscaping; }
    int floorNumber() const { return m_floorNumber; }
    QString battleScene() const { return m_battleScene; }

signals:
    void songChanged(QString);
    void locationChanged(QString);
    void regionChanged(QString);
    void usesMultiRegionChanged(bool);
    void requiresFlashChanged(bool);
    void weatherChanged(QString);
    void typeChanged(QString);
    void showsLocationNameChanged(bool);
    void allowsRunningChanged(bool);
    void allowsBikingChanged(bool);
    void allowsEscapingChanged(bool);
    void floorNumberChanged(int);
    void battleSceneChanged(QString);
    void modified();

private:
    QString m_song;
    QString m_location;
    QString m_region;
    bool m_usesMultiRegion = false;
    bool m_requiresFlash = false;
    QString m_weather;
    QString m_type;
    bool m_showsLocationName = false;
    bool m_allowsRunning = false;
    bool m_allowsBiking = false;
    bool m_allowsEscaping = false;
    int m_floorNumber = 0;
    QString m_battleScene;
};

#endif // MAPHEADER_H
