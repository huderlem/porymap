#include "mapheader.h"

MapHeader::MapHeader(const MapHeader& other) : MapHeader() {
    m_song = other.m_song;
    m_location = other.m_location;
    m_requiresFlash = other.m_requiresFlash;
    m_weather = other.m_weather;
    m_type = other.m_type;
    m_showsLocationName = other.m_showsLocationName;
    m_allowsRunning = other.m_allowsRunning;
    m_allowsBiking = other.m_allowsBiking;
    m_allowsEscaping = other.m_allowsEscaping;
    m_floorNumber = other.m_floorNumber;
    m_battleScene = other.m_battleScene;
}

MapHeader &MapHeader::operator=(const MapHeader &other) {
    // We want to call each set function here to ensure any fieldChanged signals
    // are sent as necessary. This does also mean the modified signal can be sent
    // repeatedly (but for now at least that's not a big issue).
    setSong(other.m_song);
    setLocation(other.m_location);
    setRequiresFlash(other.m_requiresFlash);
    setWeather(other.m_weather);
    setType(other.m_type);
    setShowsLocationName(other.m_showsLocationName);
    setAllowsRunning(other.m_allowsRunning);
    setAllowsBiking(other.m_allowsBiking);
    setAllowsEscaping(other.m_allowsEscaping);
    setFloorNumber(other.m_floorNumber);
    setBattleScene(other.m_battleScene);
    return *this;
}

void MapHeader::setSong(const QString &song) {
    if (m_song == song)
        return;
    m_song = song;
    emit songChanged(m_song);
    emit modified();
}

void MapHeader::setLocation(const QString &location) {
    if (m_location == location)
        return;
    m_location = location;
    emit locationChanged(m_location);
    emit modified();
}

void MapHeader::setRequiresFlash(bool requiresFlash) {
    if (m_requiresFlash == requiresFlash)
        return;
    m_requiresFlash = requiresFlash;
    emit requiresFlashChanged(m_requiresFlash);
    emit modified();
}

void MapHeader::setWeather(const QString &weather) {
    if (m_weather == weather)
        return;
    m_weather = weather;
    emit weatherChanged(m_weather);
    emit modified();
}

void MapHeader::setType(const QString &type) {
    if (m_type == type)
        return;
    m_type = type;
    emit typeChanged(m_type);
    emit modified();
}

void MapHeader::setShowsLocationName(bool showsLocationName) {
    if (m_showsLocationName == showsLocationName)
        return;
    m_showsLocationName = showsLocationName;
    emit showsLocationNameChanged(m_showsLocationName);
    emit modified();
}

void MapHeader::setAllowsRunning(bool allowsRunning) {
    if (m_allowsRunning == allowsRunning)
        return;
    m_allowsRunning = allowsRunning;
    emit allowsRunningChanged(m_allowsRunning);
    emit modified();
}

void MapHeader::setAllowsBiking(bool allowsBiking) {
    if (m_allowsBiking == allowsBiking)
        return;
    m_allowsBiking = allowsBiking;
    emit allowsBikingChanged(m_allowsBiking);
    emit modified();
}

void MapHeader::setAllowsEscaping(bool allowsEscaping) {
    if (m_allowsEscaping == allowsEscaping)
        return;
    m_allowsEscaping = allowsEscaping;
    emit allowsEscapingChanged(m_allowsEscaping);
    emit modified();
}

void MapHeader::setFloorNumber(int floorNumber) {
    if (m_floorNumber == floorNumber)
        return;
    m_floorNumber = floorNumber;
    emit floorNumberChanged(m_floorNumber);
    emit modified();
}

void MapHeader::setBattleScene(const QString &battleScene) {
    if (m_battleScene == battleScene)
        return;
    m_battleScene = battleScene;
    emit battleSceneChanged(m_battleScene);
    emit modified();
}
