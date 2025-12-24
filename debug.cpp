#include "debug.hpp"

void print_debug(std::string statement, bool debug) {
    if (debug) {
        std::cerr << "debug statement: " << statement << std::endl;
    }
};