#pragma once
#ifndef MAPCONNECTION_H
#define MAPCONNECTION_H

#include <QString>
#include <QObject>
#include <QMap>

class Project;

class MapConnection : public QObject
{
    Q_OBJECT
public:
    MapConnection(const QString &direction, const QString &hostMapName, const QString &targetMapName, int offset = 0);

    QString direction() const { return m_direction; }
    void setDirection(const QString &direction);

    QString hostMapName() const { return m_hostMapName; }
    void setHostMapName(const QString &hostMapName);

    QString targetMapName() const { return m_targetMapName; }
    void setTargetMapName(const QString &targetMapName);

    int offset() const { return m_offset; }
    void setOffset(int offset);

    bool isMirror(const MapConnection*);
    MapConnection* findMirror();
    MapConnection* createMirror();

    QPixmap getPixmap();

    static QPointer<Project> project;
    static const QMap<QString, QString> oppositeDirections;
    static const QStringList cardinalDirections;
    static bool isCardinal(const QString &direction);
    static bool isHorizontal(const QString &direction);
    static bool isVertical(const QString &direction);
    static QString oppositeDirection(const QString &direction) { return oppositeDirections.value(direction, direction); }

private:
    QString m_direction;
    QString m_hostMapName;
    QString m_targetMapName;
    int m_offset;
    bool m_ignoreMirror;

    void mirrorDirection(const QString &direction);
    void mirrorHostMapName(const QString &hostMapName);
    void mirrorTargetMapName(const QString &targetMapName);
    void mirrorOffset(int offset);
    void markMapEdited();

signals:
    void directionChanged(const QString &before, const QString &after);
    void targetMapNameChanged(const QString &before, const QString &after);
    void hostMapNameChanged(const QString &before, const QString &after);
    void offsetChanged(int before, int after);
};

#endif // MAPCONNECTION_H
