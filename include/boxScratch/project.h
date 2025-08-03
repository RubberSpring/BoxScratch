#ifndef SCRATCHPJCT_BXSCT_H
#define SCRATCHPJCT_BXSCT_H
#include <boxScratch/sprite.h>
#include <map>

class Project {
public:
    std::map<std::string, Block> blockMap;
    std::map<std::string, Variable> variableMap;
    std::map<std::string, List> listMap;
    std::map<std::string, Broadcast> broadcastMap;
    std::vector<Sprite> sprites;
};

#endif