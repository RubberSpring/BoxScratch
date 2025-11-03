#ifndef BLOCK_BXSCT_H
#define BLOCK_BXSCT_H

#include <string>
#include <map>

class Block {
public:
    std::string opcode;
    std::string id;
    std::string next;
    std::string parent;
    std::string owner;
    std::map<std::string, Block> inputs;
    bool topLevel;
    int stackId;
    bool isShadow;
    std::string shadowValue;
    int shadowType;

    void pretty();
};

#endif