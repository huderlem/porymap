#include "regionmapgenerator.h"



RegionMapGenerator::RegionMapGenerator(Project *project_) {
    this->project = project_;
}

QDebug operator<<(QDebug debug, const GeneratorEntry &entry)
{
    debug.nospace() << "Entry_" << entry.name 
                    << " (" << entry.x << ", " << entry.y << ")"
                    << " [" << entry.width << " x " << entry.height << "]"
    ;
    return debug;
}

// 
void RegionMapGenerator::generate(QString mapName) {
    //
    int i = project->mapNames->indexOf(mapName);
    //Map *map = project->loadMap(mapName);
    qDebug() << "generating region map from:" << mapName;
    search(i);

    populateSquares();
}

// use a progress bar because this is rather slow (because loadMap)
// TODO: use custom functions to load only necessary data from maps?
//       need connections and dimensions
// maybe use gMapGroup0.numMaps as hint for size of progress
void RegionMapGenerator::bfs(int start) {
    //
    int size = project->mapNames->size();

    //*

    QVector<bool> visited(size, false);
    QList<int> queue;

    visited[start] = true;
    queue.append(start);

    while (!queue.isEmpty()) {
        start = queue.first();

        qDebug() << project->mapNames->at(start);
        Map *map = project->loadMap(project->mapNames->at(start));

        if (!map) break;

        this->entries.insert(map->location, {
            map->name, 0, 0, map->getWidth() / square_block_width_, map->getHeight() / square_block_height_
        });

        queue.removeFirst();

        // get all connected map indexes
        // if not visited, mark it visited and insert into queue
        for (auto c : map->connections) {
            int i = project->mapNames->indexOf(c->map_name);
            if (!visited[i] && c->direction != "dive") {
                visited[i] = true;
                queue.append(i);
            }
        }
        //delete map;
    }
    //*/
    qDebug() << "search complete";// << entries.keys();
    //return;
}

// 
void RegionMapGenerator::dfs(int start, QVector<bool> &visited) {
    //
    visited[start] = true;

    Map *map = project->loadMap(project->mapNames->at(start));
    //qDebug() << map->name;
    this->entries.insert(map->location, {
        map->name, 0, 0, map->getWidth() / square_block_width_, map->getHeight() / square_block_height_
    });

    // place map on the grid?

    for (auto c : map->connections) {
        int i = project->mapNames->indexOf(c->map_name);
        if (!visited[i] && c->direction != "dive") {
            dfs(i, visited);
        }
    }
}

void RegionMapGenerator::search(int start) {
    //
    int size = project->mapNames->size();
    QVector<bool> visited(size, false);

    dfs(start, visited);
}

void RegionMapGenerator::populateSquares() {
    //
    for (auto entry : entries.values()) {
        qDebug() << entry;//.name << entry.width << "x" << entry.height;
    }
}







































