#include <fstream>
#include <sstream>
#include <cstring>
#include "types.hpp"
#include "intrinsics.hpp"
#include "parser.hpp"
#include "compiler.hpp"
#include "preprocessor.hpp"



#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/TargetParser/Triple.h>

// Global target flag
bool target_wasm = false;

int main(int argc, char** argv) {
    // llvm globals
    TheContext = std::make_unique<llvm::LLVMContext>();
    TheModule = std::make_unique<llvm::Module>("miaow_module", *TheContext);
    Builder = std::make_unique<llvm::IRBuilder<>>(*TheContext);

    // build miaow intrinsic functions
    init_intrinsics();
    
    std::string filename = "hello.inf";
    std::string output_file = "hello.ll";
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--wasm") == 0 || strcmp(argv[i], "-w") == 0) {
            target_wasm = true;
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (argv[i][0] != '-') {
            filename = argv[i];
        }
    }
    
    // Set target triple and data layout for WASM
    if (target_wasm) {
        TheModule->setTargetTriple(llvm::Triple("wasm32-unknown-emscripten"));
        TheModule->setDataLayout("e-m:e-p:32:32-p10:8:8-p20:8:8-i64:64-n32:64-S128-ni:1:10:20");
    }
    
    std::ifstream sourcefile(filename);

    // read source
    std::stringstream buffer;
    buffer << sourcefile.rdbuf();
    std::string source = buffer.str();
    
    // preprocess source (remove comments)
    source = preprocess(source);
    
    std::string_view source_view(source);
    
    // parse source
    Molecule root = lexparse(source_view);
    Particle root_particle = Particle(root); // capture the overarching curly braces

    // Pass 1: type checking
    for (auto& cmd : root.atoms) {
        get_particle_type(cmd);
    }

    // Pass 1.5: collect struct declarations first (needed for correct variable hoisting)
    collect_struct_declarations(root_particle);

    std::unordered_map<std::string, std::string> all_vars;
    
    // Pass 2: variable hoisting
    collect_variables(root_particle, all_vars);

    // main this is where the curly braces go
    llvm::FunctionType* FT = llvm::FunctionType::get(llvm::Type::getInt32Ty(*TheContext), false);
    llvm::Function* MainFunc = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "main", TheModule.get());
    llvm::BasicBlock* EntryBB = llvm::BasicBlock::Create(*TheContext, "entry", MainFunc);
    Builder->SetInsertPoint(EntryBB);

    // allocate the hosted variables
    for (auto& [var_name, var_type] : all_vars) {
        llvm::Type* llvm_type;
        // For extern structs, use the actual struct type (not pointer)
        if (struct_registry.count(var_type) && struct_registry[var_type].is_extern) {
            llvm_type = struct_registry[var_type].llvm_type;
        } else {
            llvm_type = get_llvm_type(var_type);
        }
        llvm::AllocaInst* alloca = Builder->CreateAlloca(llvm_type, nullptr, var_name);
        object_registry[var_name] = MemObject(var_type, alloca);
    }
    
    // Pass 3: compilation
    Molecule& root_mol = std::get<Molecule>(root_particle);
    
    for (size_t i = 1; i < root_mol.atoms.size(); i++) {
        compile(root_mol.atoms[i]);
    }

    // Return 0
    Builder->CreateRet(llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 0)));

    // Verify module
    if (llvm::verifyModule(*TheModule, &llvm::errs())) {
        std::cerr << "Error: Module verification failed\n";
        return 1;
    }

    // Print to file
    std::error_code EC;
    llvm::raw_fd_ostream dest(output_file, EC, llvm::sys::fs::OF_None);
    if (EC) {
        std::cerr << "Could not open file: " << EC.message();
        return 1;
    }
    TheModule->print(dest, nullptr);
    
    return 0;
}