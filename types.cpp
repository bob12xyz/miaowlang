#include "types.hpp"
#include "intrinsics.hpp"

// Global state definitions
std::unordered_map<std::string, MemObject> object_registry;
std::unordered_map<std::string, StructDef> struct_registry;
std::unordered_map<std::string, std::vector<std::string>> overload_registry;
std::unique_ptr<llvm::LLVMContext> TheContext;
std::unique_ptr<llvm::Module> TheModule;
std::unique_ptr<llvm::IRBuilder<>> Builder;

// Type mapping definitions
const std::unordered_map<std::string, std::string> NATIVE_TYPES = {
    {"Int", "i32"},
    {"Float", "float"},
    {"Bool", "i1"},
    {"Char", "i8"},
    {"Str", "i8"},
    {"Nil", "void"},
    {"Var", "ptr"}
}; 

// Removed instruction maps
const std::unordered_map<std::string, std::string> ARITH_INSTRUCTIONS = {
    {"+", "add"}, {"-", "sub"}, {"*", "mul"}, {"/", "sdiv"}, {"%", "srem"}
}; 
const std::unordered_map<std::string, std::string> FLOAT_ARITH_INSTRUCTIONS = {
    {"+", "fadd"}, {"-", "fsub"}, {"*", "fmul"}, {"/", "fdiv"}, {"%", "frem"}
}; 
const std::unordered_map<std::string, std::string> COMPARE_INSTRUCTIONS = {
    {"==", "icmp eq"}, {"!=", "icmp ne"}, {">", "icmp sgt"}, {">=", "icmp sge"}, {"<", "icmp slt"}, {"<=", "icmp sle"}
}; 
const std::unordered_map<std::string, std::string> FLOAT_COMPARE_INSTRUCTIONS = {
    {"==", "fcmp oeq"}, {"!=", "fcmp one"}, {">", "fcmp ogt"}, {">=", "fcmp oge"}, {"<", "fcmp olt"}, {"<=", "fcmp ole"}
}; 
const std::unordered_map<std::string, std::string> LOGIC_INSTRUCTIONS = {
    {"&&", "and"}, {"||", "or"}, {"^", "xor"}
}; 
const std::unordered_map<std::string, std::string> UNARY_STRING = {
    {"meow", "call i32 (ptr, ...) @printf("}
}; 


Atom::Atom(std::string i) : identifier(i), stored_in(nullptr), len(0) {
    if (!identifier.empty() && identifier[0] == '"') {
        len = identifier.size()-1;
        identifier.pop_back();

        identifier = identifier.substr(1);
        type = "Str";
    } else {
        // Check for Type:object syntax (e.g., Int:b, Char:33)
        auto colon = identifier.find(':');
        if (colon != std::string::npos && colon > 0 && std::isupper(identifier[0])) {
            type = identifier.substr(0, colon);
            identifier = identifier.substr(colon + 1);
        }
        
        // Check for member access syntax (e.g., bob>name)
        // Only if identifier starts with lowercase (variable name), not operators like ->
        auto arrow = identifier.find('>');
        if (arrow != std::string::npos && arrow > 0 && std::islower(identifier[0])) {
            member_access = identifier.substr(arrow + 1);
            identifier = identifier.substr(0, arrow);
        }
    }
}

void Atom::set_type() {
    type = get_type(true);
}

std::string Atom::get_type(bool native) {
    if (!type.empty()) {
        if (type == "Str") {
            return native ? "ptr" : "Str";
        }
        if (struct_registry.count(type)) {
            return native ? "ptr" : type;
        }
        if (NATIVE_TYPES.count(type)) {
            return native ? NATIVE_TYPES.at(type) : type;
        }
        return native ? "ptr" : type;
    }
    if (!identifier.empty() && (isdigit(identifier[0]) || (identifier[0] == '-' && identifier.size() > 1 && isdigit(identifier[1])))) {
        if (identifier.find('.') == std::string::npos) return (native) ? "i32" : "Int";
        else return (native) ? "float" : "Float";
    } else if (identifier == "true" || identifier == "false") {
        return (native) ? "i1" : "Bool";
    } else if (identifier == "nil") {
        return (native) ? "void" : "Nil";
    } else if (identifier[0] == '\"' || type == "Str") {
        return (native) ? "ptr" : "Str";
    } else if (object_registry.count(identifier)) {
        std::string registered_type = object_registry.at(identifier).type;
        if (struct_registry.count(registered_type)) {
            return native ? "ptr" : registered_type;
        }
        if (NATIVE_TYPES.count(registered_type)) {
            return native ? NATIVE_TYPES.at(registered_type) : registered_type;
        }
        return native ? "ptr" : registered_type;
    }
    return (native) ? "ptr" : "Var";
}

// Molecule implementation
Molecule::Molecule(List a, bool e) : atoms(a), stored_in(nullptr), eval(e) {}
Molecule::Molecule(bool e) : atoms(), stored_in(nullptr), eval(e) {}

Particle& Molecule::subject() {
    return atoms.front();
}

List Molecule::predicate() {
    if (atoms.size() <= 1) return {};
    else {
        auto start = atoms.begin() + 1;
        auto end = atoms.end();
        return List(start, end);
    }
}

void Molecule::add_atom(std::string identifier) {
    atoms.push_back(Particle(Atom(identifier)));
}

Molecule* Molecule::add_molecule(std::string identifier) {
    Molecule m({}, true);
    m.add_atom(identifier);
    atoms.push_back(Particle(m));
    return std::get_if<Molecule>(&atoms.back());
}

Molecule* Molecule::add_molecule(Molecule m) {
    atoms.push_back(Particle(m));
    return std::get_if<Molecule>(&atoms.back());
}

std::string Molecule::indent(int n, int nc, char c) {
    return std::string(nc*n, c);
}

void Molecule::print_tree(int deep) {
    if (atoms.empty()) return;
    
    if (!eval) {
        std::cout << indent(deep) << "no eval\n";
    }
    
    auto& current_subject = subject();
    std::cout << indent(deep) << "subj: \n";
    if (std::holds_alternative<Atom>(current_subject)) {
        std::cout << indent(deep+1) << std::get<Atom>(current_subject).identifier << "\n";
    } else if (std::holds_alternative<Molecule>(current_subject)) {
        std::cout << indent(deep+1) << "mol: \n";
        std::get<Molecule>(current_subject).print_tree(deep + 2);
    }
    
    auto rest = predicate();
    if (rest.empty()) return;
    std::cout << indent(deep) << "pred: \n";
    for (auto& a: rest) {
        if (std::holds_alternative<Atom>(a)) {
            std::cout << indent(deep+1) << "atom: " << std::get<Atom>(a).identifier << "\n";
        } else if (std::holds_alternative<Molecule>(a)) {
            auto current_molecule = std::get<Molecule>(a); 
            if (current_molecule.eval){
                std::cout << indent(deep+1)<< "mol: \n";
                current_molecule.print_tree(deep + 2);
            } else {
                std::cout << indent(deep+1) << "mol: \n";
                current_molecule.print_tree(deep + 2);
            }
        }
    }
}

std::string Molecule::get_type(bool native) {
    std::string t = type.empty() ? get_type() : type;
    if (native) {
        if (struct_registry.count(t)) return "ptr";
        if (NATIVE_TYPES.count(t)) return NATIVE_TYPES.at(t);
        return "ptr";
    }
    return t;
} 

std::string Molecule::get_type() {
    if (!type.empty()) return type;
    
    std::string identifier = std::get<Atom>(atoms.front()).identifier;
    
    if (INTRINSICS.count(identifier)) {
        std::vector<Particle> args;
        if (atoms.size() > 1) {
            args.insert(args.end(), atoms.begin() + 1, atoms.end());
        }
        type = INTRINSICS[identifier].type_inference(args);
    } else {
        type = "Nil";
    }

    return type;
}

// Utility function implementations
std::string get_particle_type(Particle p) {
    std::string type;
    std::visit([&type](auto&& particle) {
        type = particle.get_type(false);
    }, p);
    return type;
}

std::string get_particle_type(Particle p, bool native) {
    std::string type;
    std::visit([&type, &native](auto&& particle) {
        type = particle.get_type(native);
    }, p);
    return type;
}

llvm::Value* get_stored_in(Particle p) {
    llvm::Value* val = nullptr;
    std::visit([&val](auto&& particle) {
        val = particle.stored_in;
    }, p);
    return val;
}

llvm::Type* get_llvm_type(const std::string& type_name) {
    if (type_name == "Int") return llvm::Type::getInt32Ty(*TheContext);
    if (type_name == "Float") return llvm::Type::getFloatTy(*TheContext);
    if (type_name == "Bool") return llvm::Type::getInt1Ty(*TheContext);
    if (type_name == "Char") return llvm::Type::getInt8Ty(*TheContext);
    if (type_name == "Nil") return llvm::Type::getVoidTy(*TheContext);
    if (type_name == "Str") return llvm::PointerType::getUnqual(*TheContext);
    if (struct_registry.count(type_name)) return llvm::PointerType::getUnqual(*TheContext);
    return llvm::PointerType::getUnqual(*TheContext);
}

llvm::Constant* get_llvm_constant(Atom& atom) {
    if (atom.type.empty()) {
        atom.type = atom.get_type(false);
    }

    if (atom.type == "Int") {
        // Only parse if it's actually a numeric literal
        if (!atom.identifier.empty() && (isdigit(atom.identifier[0]) || (atom.identifier[0] == '-' && atom.identifier.size() > 1 && isdigit(atom.identifier[1])))) {
            return llvm::ConstantInt::get(*TheContext, llvm::APInt(32, std::stoi(atom.identifier)));
        }
        return nullptr;  // Variable reference, not a literal
    }
    if (atom.type == "Float") {
        // Only parse if it's actually a numeric literal
        if (!atom.identifier.empty() && (isdigit(atom.identifier[0]) || (atom.identifier[0] == '-' && atom.identifier.size() > 1 && isdigit(atom.identifier[1])))) {
            return llvm::ConstantFP::get(*TheContext, llvm::APFloat(std::stof(atom.identifier)));
        }
        return nullptr;  // Variable reference, not a literal
    }
    if (atom.type == "Char") {
        // Char:33 means character with ASCII code 33, or Char:'!' for literal char
        if (!atom.identifier.empty() && isdigit(atom.identifier[0])) {
            return llvm::ConstantInt::get(*TheContext, llvm::APInt(8, std::stoi(atom.identifier)));
        }
        return nullptr;
    }
    if (atom.type == "Bool") {
        return llvm::ConstantInt::get(*TheContext, llvm::APInt(1, atom.identifier == "true" ? 1 : 0));
    }
    // Str is now handled specially in evaluate() to build array struct
    return nullptr;
}

llvm::StructType* get_array_struct_type(llvm::Type* element_type) {
    std::vector<llvm::Type*> members;
    members.push_back(llvm::Type::getInt32Ty(*TheContext)); // size
    members.push_back(llvm::Type::getInt32Ty(*TheContext)); // capacity
    members.push_back(llvm::PointerType::getUnqual(element_type)); // data pointer
    return llvm::StructType::get(*TheContext, members);
}
