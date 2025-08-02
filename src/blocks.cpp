#include <blocks.h>
#include <iostream>

void Block::pretty() {
    std::vector<std::string> blockInputs = this->inputs;
    std::string str{blockInputs.begin(), blockInputs.end()};
    std::cout << this->opcode
              << this->id
              << this->next
              << str
              << std::endl;
}