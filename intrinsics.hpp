#ifndef INTRINSICS_HPP
#define INTRINSICS_HPP

#include "types.hpp"
#include "debug.hpp"
#include <functional>
#include <unordered_map>
#include <cmath>

// Result structure for intrinsic operations
struct IntrinsicResult {
    llvm::Value* value;
};

typedef std::function<IntrinsicResult(Molecule&, const std::vector<llvm::Value*>&)> IntrinsicBuilder;

// Function class - wraps an intrinsic operation
class Function {
public: 
    std::string identifier;
    std::string return_type;
    std::vector<std::string> param_types;  // For overload matching
    IntrinsicBuilder build;
    std::function<std::string(const std::vector<Particle>&)> type_inference;

    Function() : identifier(), build(), type_inference() {}
    Function(std::string id, std::string r, IntrinsicBuilder b) 
     : identifier(id), return_type(r), build(b), type_inference([r](const std::vector<Particle>&){ return r; }) {}
    
    Function(std::string id, IntrinsicBuilder b, std::function<std::string(const std::vector<Particle>&)> t)
     : identifier(id), return_type(), build(b), type_inference(t) {}

    IntrinsicResult evaluate(Molecule& mol, const std::vector<llvm::Value*>& args);
};

// Intrinsic builders
IntrinsicResult build_arith(Molecule& mol, const std::vector<llvm::Value*>& args, const std::string& fn_name);
IntrinsicResult build_compare(Molecule& mol, const std::vector<llvm::Value*>& args, const std::string& fn_name);
IntrinsicResult build_def(Molecule& mol);
IntrinsicResult build_reassign(Molecule& mol);
IntrinsicResult build_meow(Molecule& mol, const std::vector<llvm::Value*>& args);
IntrinsicResult build_conv(Molecule& mol, const std::vector<llvm::Value*>& args, const std::string& out_type);
IntrinsicResult build_array(Molecule& mol, const std::vector<llvm::Value*>& args);
IntrinsicResult build_return(Molecule& mol, const std::vector<llvm::Value*>& args);

// Global intrinsics map
extern std::unordered_map<std::string, Function> INTRINSICS;

// Initialize intrinsics map
void init_intrinsics();

#endif // INTRINSICS_HPP
