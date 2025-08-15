#pragma once
#ifndef MAPCONNECTION_H
#define MAPCONNECTION_H

#include <QString>
#include <QObject>
#include <QMap>
#include <QJsonObject>

class Project;
class Map;

class MapConnection : public QObject
{
    Q_OBJECT
public:
    MapConnection(const QString &targetMapName, const QString &direction, int offset = 0);

    Map* parentMap() const { return m_parentMap; }
    QString parentMapName() const;
    void setParentMap(Map* map, bool mirror = true);

    Map* targetMap() const;
    QString targetMapName() const { return m_targetMapName; }
    void setTargetMapName(const QString &targetMapName, bool mirror = true);

    QString direction() const { return m_direction; }
    void setDirection(const QString &direction, bool mirror = true);

    bool isCardinal() const { return isCardinal(m_direction); }
    bool isHorizontal() const { return isHorizontal(m_direction); }
    bool isVertical() const { return isVertical(m_direction); }
    bool isDiving() const { return isDiving(m_direction); }

    int offset() const { return m_offset; }
    void setOffset(int offset, bool mirror = true);

    QJsonObject customData() const { return m_customData; }
    void setCustomData(const QJsonObject &customData) { m_customData = customData; }

    MapConnection* findMirror();
    MapConnection* createMirror();

    QPixmap render() const;
    QImage renderImage() const;
    QPoint relativePixelPos(bool clipped = false) const;

    static QPointer<Project> project;
    static const QMap<QString, QString> oppositeDirections;
    static const QStringList cardinalDirections;
    static bool isCardinal(const QString &direction);
    static bool isHorizontal(const QString &direction);
    static bool isVertical(const QString &direction);
    static bool isDiving(const QString &direction);
    static QString oppositeDirection(const QString &direction) { return oppositeDirections.value(direction, direction); }
    static bool areMirrored(const MapConnection*, const MapConnection*);

private:
    Map* m_parentMap;
    QString m_targetMapName;
    QString m_direction;
    int m_offset;
    QJsonObject m_customData;

    void markMapEdited();
    Map* getMap(const QString& mapName) const;

signals:
    void parentMapChanged(Map* before, Map* after);
    void targetMapNameChanged(QString before, QString after);
    void directionChanged(QString before, QString after);
    void offsetChanged(int before, int after);
};

#endif // MAPCONNECTION_H
