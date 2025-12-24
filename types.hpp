#ifndef TYPES_HPP
#define TYPES_HPP

#include <iostream>
#include <vector>
#include <string>
#include <memory> 
#include <stdexcept>
#include <cctype>
#include <variant>
#include <string_view>
#include <sstream>
#include <unordered_map>
#include <functional>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Constants.h>

#define WHITESPACE "\n\t "

// Type mappings
extern const std::unordered_map<std::string, std::string> NATIVE_TYPES;
extern const std::unordered_map<std::string, std::string> ARITH_INSTRUCTIONS;
extern const std::unordered_map<std::string, std::string> FLOAT_ARITH_INSTRUCTIONS;
extern const std::unordered_map<std::string, std::string> COMPARE_INSTRUCTIONS;
extern const std::unordered_map<std::string, std::string> FLOAT_COMPARE_INSTRUCTIONS;
extern const std::unordered_map<std::string, std::string> LOGIC_INSTRUCTIONS;
extern const std::unordered_map<std::string, std::string> UNARY_STRING;

// Memory object for tracking variables
class MemObject {
public:
    std::string type;
    llvm::Value* value;

    MemObject() : type(), value(nullptr) {}
    MemObject(std::string t, llvm::Value* v) : type(t), value(v) {}
};

// Struct definition
struct StructDef {
    std::string name;
    std::vector<std::string> field_names;
    std::vector<std::string> field_types;
    llvm::StructType* llvm_type;
    bool is_extern = false;  // External C structs are passed by value
};

// Global state
extern std::unordered_map<std::string, MemObject> object_registry;
extern std::unordered_map<std::string, StructDef> struct_registry;
extern std::unordered_map<std::string, std::vector<std::string>> overload_registry;
extern std::unique_ptr<llvm::LLVMContext> TheContext;
extern std::unique_ptr<llvm::Module> TheModule;
extern std::unique_ptr<llvm::IRBuilder<>> Builder;

// Forward declarations
class Molecule;
typedef std::variant<class Atom, Molecule> Particle;
typedef std::vector<Particle> List;

// Atom class - represents a single token/value
class Atom {
public:
    std::string identifier;
    llvm::Value* stored_in;
    std::string type;
    std::string member_access;  // For x>field syntax
    int len;

    Atom(std::string i);
    void set_type();
    std::string get_type(bool native = true);
};

// Molecule class - represents an S-expression
class Molecule {
public:
    List atoms;
    llvm::Value* stored_in;
    std::string type;
    bool eval = true;

    Molecule(List a, bool e = true);
    Molecule(bool e = true);

    Particle& subject();
    List predicate();
    void add_atom(std::string identifier);
    Molecule* add_molecule(std::string identifier);
    Molecule* add_molecule(Molecule m);
    std::string indent(int n, int nc = 4, char c = ' ');
    void print_tree(int deep = 0);
    std::string get_type(bool native);
    std::string get_type();
};

// Utility functions
std::string get_particle_type(Particle p);
std::string get_particle_type(Particle p, bool native);
llvm::Value* get_stored_in(Particle p);

llvm::Type* get_llvm_type(const std::string& type_name);
llvm::Constant* get_llvm_constant(Atom& atom);

llvm::StructType* get_array_struct_type(llvm::Type* element_type);

#endif // TYPES_HPP
