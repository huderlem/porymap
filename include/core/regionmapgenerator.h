#ifndef GUARD_REGIONMAPGENERATOR_H
#define GUARD_REGIONMAPGENERATOR_H

#include "project.h"
#include "regionmap.h"
#include "map.h"

#include <QDebug>
#include <QString>
#include <QVector>
#include <QStack>
#include <QPair>
#include <QSet>

class GeneratorEntry
{
    GeneratorEntry(QString name_, int x_, int y_, int width_, int height_) {
        this->name   = name_;
        this->x      = x_;
        this->y      = y_;
        this->width  = width_;
        this->height = height_;
    }
    int x;
    int y;
    int width;
    int height;
    QString name;
    friend class RegionMapGenerator;
public:
    friend QDebug operator<<(QDebug, const GeneratorEntry &);
};

class RegionMapGenerator
{
    //
public:
    //
    RegionMapGenerator() = default;
    RegionMapGenerator(Project *);
    ~RegionMapGenerator() {};

    Project *project = nullptr;

    QVector<RegionMapSquare> map_squares;

    QStack<QPair<QString, int>> forks;//?
    QSet<MapConnection> connections;//
    // <MapConnection>

    void generate(QString);

    void center();// center the map squares so they arent hanging over

private:
    //
    int square_block_width_  = 20;
    int square_block_height_ = 20;

    int width_;
    int height_;

    //QPoint 

    QList<struct RegionMapEntry> entries_;
    QMap<QString, GeneratorEntry> entries;

    void bfs(int);// breadth first search of a graph
    void search(int);
    void dfs(int, QVector<bool> &);// depth first search
    void sort();
    void ts();// topological sort
    void populateSquares();

};

#endif // GUARD_REGIONMAPGENERATOR_H
