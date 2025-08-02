#ifndef SCRATCHPJCT_BXSCT_H
#define SCRATCHPJCT_BXSCT_H
#include <boxScratch/stack.h>
#include <map>

class Project
{
public:
    std::map<std::string, Block*> blockMap;
    std::vector<Stack> stacks;
};

#endif