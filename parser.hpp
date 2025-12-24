#ifndef PARSER_HPP
#define PARSER_HPP

#include "types.hpp"

// Parse source code into a Molecule AST
Molecule lexparse(std::string_view view);

#endif // PARSER_HPP
