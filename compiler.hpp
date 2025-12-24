#ifndef COMPILER_HPP
#define COMPILER_HPP

#include "types.hpp"
#include "intrinsics.hpp"
#include "debug.hpp"
#include <cmath>

// Target flag (defined in miaow.cpp)
extern bool target_wasm;

llvm::Value* evaluate(Atom& atom);

llvm::Value* evaluate(Molecule& mol);

void collect_struct_declarations(Particle& p);

void collect_variables(Particle& p, std::unordered_map<std::string, std::string>& vars);

void compile(Particle& p);


#endif // COMPILER_HPP
