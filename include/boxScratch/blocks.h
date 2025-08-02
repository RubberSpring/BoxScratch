#ifndef BLOCK_BXSCT_H
#define BLOCK_BXSCT_H
#include <string>
#include <vector>

class Block
{
public:
    std::string opcode;
    std::string id;
    std::string next;
    std::string parent;
    std::vector<std::string> inputs;

    void pretty();
};

#endif