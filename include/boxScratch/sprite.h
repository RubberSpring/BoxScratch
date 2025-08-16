#ifndef SPRITE_BXSCT_H
#define SPRITE_BXSCT_H
#include <boxScratch/stack.h>
#include <boxScratch/values.h>

class Costume {
public:
    std::string name;
    std::string dataFormat;
    std::string dataName;
    Image cpuImage;
    Image ogImage;
    Texture2D gpuImage;
    float width;
    float height;
    int number;
    int id;
};

class ScratchSound {
public:
    std::string name;
    int number;
};

class Sprite {
public:
    std::string name;
    std::vector<Stack> stacks;
    std::vector<Costume> costumes;
    std::vector<ScratchSound> sounds;
    std::vector<Variable> variables;
    std::vector<List> lists;
    std::vector<Broadcast> broadcasts;
    double x;
    double y;
    bool visible;
    double size;
    double direction;
    bool isStage;
    int costumeIndex;
};

#endif