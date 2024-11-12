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
    auto before = m_song;
    m_song = song;
    emit songChanged(before, m_song);
    emit modified();
}

void MapHeader::setLocation(const QString &location) {
    if (m_location == location)
        return;
    auto before = m_location;
    m_location = location;
    emit locationChanged(before, m_location);
    emit modified();
}

void MapHeader::setRequiresFlash(bool requiresFlash) {
    if (m_requiresFlash == requiresFlash)
        return;
    auto before = m_requiresFlash;
    m_requiresFlash = requiresFlash;
    emit requiresFlashChanged(before, m_requiresFlash);
    emit modified();
}

void MapHeader::setWeather(const QString &weather) {
    if (m_weather == weather)
        return;
    auto before = m_weather;
    m_weather = weather;
    emit weatherChanged(before, m_weather);
    emit modified();
}

void MapHeader::setType(const QString &type) {
    if (m_type == type)
        return;
    auto before = m_type;
    m_type = type;
    emit typeChanged(before, m_type);
    emit modified();
}

void MapHeader::setShowsLocationName(bool showsLocationName) {
    if (m_showsLocationName == showsLocationName)
        return;
    auto before = m_showsLocationName;
    m_showsLocationName = showsLocationName;
    emit showsLocationNameChanged(before, m_showsLocationName);
    emit modified();
}

void MapHeader::setAllowsRunning(bool allowsRunning) {
    if (m_allowsRunning == allowsRunning)
        return;
    auto before = m_allowsRunning;
    m_allowsRunning = allowsRunning;
    emit allowsRunningChanged(before, m_allowsRunning);
    emit modified();
}

void MapHeader::setAllowsBiking(bool allowsBiking) {
    if (m_allowsBiking == allowsBiking)
        return;
    auto before = m_allowsBiking;
    m_allowsBiking = allowsBiking;
    emit allowsBikingChanged(before, m_allowsBiking);
    emit modified();
}

void MapHeader::setAllowsEscaping(bool allowsEscaping) {
    if (m_allowsEscaping == allowsEscaping)
        return;
    auto before = m_allowsEscaping;
    m_allowsEscaping = allowsEscaping;
    emit allowsEscapingChanged(before, m_allowsEscaping);
    emit modified();
}

void MapHeader::setFloorNumber(int floorNumber) {
    if (m_floorNumber == floorNumber)
        return;
    auto before = m_floorNumber;
    m_floorNumber = floorNumber;
    emit floorNumberChanged(before, m_floorNumber);
    emit modified();
}

void MapHeader::setBattleScene(const QString &battleScene) {
    if (m_battleScene == battleScene)
        return;
    auto before = m_battleScene;
    m_battleScene = battleScene;
    emit battleSceneChanged(before, m_battleScene);
    emit modified();
}
