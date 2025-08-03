#ifndef SPRITE_BXSCT_H
#define SPRITE_BXSCT_H
#include <boxScratch/stack.h>
#include <boxScratch/values.h>

class Costume {
public:
    std::string name;
    std::string image;
    int number;
};

class ScratchSound {
public:
    std::string name;
    std::string sound;
    int number;
};

class Sprite {
public:
    std::vector<Stack> stacks;
    std::vector<Costume> costumes;
    std::vector<ScratchSound> sounds;
    std::string name;
    std::vector<Variable> variables;
    std::vector<List> lists;
    double x;
    double y;
    bool visible;
    double size;
    double direction;
    bool isStage;
};

#endif