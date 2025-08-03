#ifndef VALUES_BXSCT_H
#define VALUES_BXSCT_H
#include <string>
#include <vector>

class Variable {
public:
    std::string name;
    std::string value;
};

class List {
public:
    std::string name;
    std::vector<std::string> value;
};

class Broadcast {
public:
    std::string name;
};

#endif