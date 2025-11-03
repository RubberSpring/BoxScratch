#ifndef BX_SCT_BLOCKS_MOTION
#define BX_SCT_BLOCKS_MOTION

#include <boxScratch/project.h>
#include <string>

namespace BlockExecutors {
    std::string evalBlockInput(std::string input, Block block) {
        Block tempBlock = block.inputs.at(input);
        if (tempBlock.isShadow) {
            return tempBlock.shadowValue;
        } else {
            return "dummy text";
        }
    }
    Sprite pointInDirection(Block block, Sprite sprite) {
        sprite.direction = std::stod(evalBlockInput("DIRECTION", block));
        return sprite;
    }
    Sprite goToXY(Block block, Sprite sprite) {
        sprite.x = std::stod(evalBlockInput("X", block));
        sprite.y = std::stod(evalBlockInput("Y", block));
        return sprite;
    }
}

#endif