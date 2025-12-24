#include "intrinsics.hpp"

std::unordered_map<std::string, Function> INTRINSICS;

IntrinsicResult Function::evaluate(Molecule& mol, const std::vector<llvm::Value*>& args) {
    return build(mol, args);
}

llvm::Value* load_value(llvm::Value* v, llvm::Type* type) {
    return Builder->CreateLoad(type, v);
}

static std::string get_array_element_type_str(const std::string& array_type_str) {
    // Expect: Array<T> or Str (which is Array<Char>)
    if (array_type_str == "Str") {
        return "Char";
    }
    if (array_type_str.size() >= 7 && array_type_str.rfind("Array<", 0) == 0 && array_type_str.back() == '>') {
        return array_type_str.substr(6, array_type_str.size() - 7);
    }
    return "Var";
}

IntrinsicResult build_arith(Molecule& mol, const std::vector<llvm::Value*>& args, const std::string& fn_name) {
    std::string particle_type = get_particle_type(mol.atoms[1]);
    llvm::Type* llvm_type = get_llvm_type(particle_type);
    
    llvm::Value* lhs = load_value(args[0], llvm_type);
    llvm::Value* rhs = load_value(args[1], llvm_type);
    
    llvm::Value* result = nullptr;
    
    if (particle_type == "Int") {
        if (fn_name == "+") result = Builder->CreateAdd(lhs, rhs);
        else if (fn_name == "-") result = Builder->CreateSub(lhs, rhs);
        else if (fn_name == "*") result = Builder->CreateMul(lhs, rhs);
        else if (fn_name == "/") result = Builder->CreateSDiv(lhs, rhs);
        else if (fn_name == "%") result = Builder->CreateSRem(lhs, rhs);
    } else if (particle_type == "Float") {
        if (fn_name == "+") result = Builder->CreateFAdd(lhs, rhs);
        else if (fn_name == "-") result = Builder->CreateFSub(lhs, rhs);
        else if (fn_name == "*") result = Builder->CreateFMul(lhs, rhs);
        else if (fn_name == "/") result = Builder->CreateFDiv(lhs, rhs);
        else if (fn_name == "%") result = Builder->CreateFRem(lhs, rhs);
    }
    
    llvm::AllocaInst* alloca = Builder->CreateAlloca(llvm_type);
    Builder->CreateStore(result, alloca);
    
    return {alloca};
}

IntrinsicResult build_compound_arith(Molecule& mol, const std::vector<llvm::Value*>& args, const std::string& fn_name) {
    std::string particle_type = get_particle_type(mol.atoms[1]);
    llvm::Type* llvm_type = get_llvm_type(particle_type);
    
    llvm::Value* result = nullptr;
    if (args.size() == 1) {
        llvm::Value* arg = load_value(args[0], llvm_type);
        
        
        if (particle_type == "Int") {
            if (fn_name == "++") result = Builder->CreateAdd(arg, Builder->getInt32(1));
            else if (fn_name == "--") result = Builder->CreateAdd(arg, Builder->getInt32(-1));
        } else if (particle_type == "Float") {
            if (fn_name == "++") result = Builder->CreateFAdd(arg, llvm::ConstantFP::get(Builder->getFloatTy(), 1.0));
            else if (fn_name == "--") result = Builder->CreateFAdd(arg, llvm::ConstantFP::get(Builder->getFloatTy(), -1.0));
        }
    } else if (args.size() == 2) {
        llvm::Value* lhs = load_value(args[0], llvm_type);
        llvm::Value* rhs = load_value(args[1], llvm_type);
        
        if (particle_type == "Int") {
            if (fn_name == "+") result = Builder->CreateAdd(lhs, rhs);
            else if (fn_name == "-") result = Builder->CreateSub(lhs, rhs);
            else if (fn_name == "*") result = Builder->CreateMul(lhs, rhs);
            else if (fn_name == "/") result = Builder->CreateSDiv(lhs, rhs);
            else if (fn_name == "%") result = Builder->CreateSRem(lhs, rhs);
        } else if (particle_type == "Float") {
            if (fn_name == "+") result = Builder->CreateFAdd(lhs, rhs);
            else if (fn_name == "-") result = Builder->CreateFSub(lhs, rhs);
            else if (fn_name == "*") result = Builder->CreateFMul(lhs, rhs);
            else if (fn_name == "/") result = Builder->CreateFDiv(lhs, rhs);
            else if (fn_name == "%") result = Builder->CreateFRem(lhs, rhs);
        }
    }
        
    llvm::AllocaInst* alloca = Builder->CreateAlloca(llvm_type);
    Builder->CreateStore(result, alloca);
    Builder->CreateStore(result, args[0]);
    return {alloca};
}

IntrinsicResult build_compare(Molecule& mol, const std::vector<llvm::Value*>& args, const std::string& fn_name) {
    std::string particle_type = get_particle_type(mol.atoms[1]);
    llvm::Type* llvm_type = get_llvm_type(particle_type);
    
    llvm::Value* lhs = load_value(args[0], llvm_type);
    llvm::Value* rhs = load_value(args[1], llvm_type);
    
    llvm::Value* result = nullptr;
    
    if (particle_type == "Int") {
        if (fn_name == "==") result = Builder->CreateICmpEQ(lhs, rhs);
        else if (fn_name == "!=") result = Builder->CreateICmpNE(lhs, rhs);
        else if (fn_name == ">") result = Builder->CreateICmpSGT(lhs, rhs);
        else if (fn_name == ">=") result = Builder->CreateICmpSGE(lhs, rhs);
        else if (fn_name == "<") result = Builder->CreateICmpSLT(lhs, rhs);
        else if (fn_name == "<=") result = Builder->CreateICmpSLE(lhs, rhs);
    } else if (particle_type == "Float") {
        if (fn_name == "==") result = Builder->CreateFCmpOEQ(lhs, rhs);
        else if (fn_name == "!=") result = Builder->CreateFCmpONE(lhs, rhs);
        else if (fn_name == ">") result = Builder->CreateFCmpOGT(lhs, rhs);
        else if (fn_name == ">=") result = Builder->CreateFCmpOGE(lhs, rhs);
        else if (fn_name == "<") result = Builder->CreateFCmpOLT(lhs, rhs);
        else if (fn_name == "<=") result = Builder->CreateFCmpOLE(lhs, rhs);
    }
    
    llvm::AllocaInst* alloca = Builder->CreateAlloca(llvm::Type::getInt1Ty(*TheContext));
    Builder->CreateStore(result, alloca);
    
    return {alloca};
}

// ! - boolean not operator
IntrinsicResult build_not(Molecule& mol, const std::vector<llvm::Value*>& args) {
    (void)mol; // unused
    llvm::Value* val = load_value(args[0], llvm::Type::getInt1Ty(*TheContext));
    llvm::Value* result = Builder->CreateNot(val, "not");
    llvm::AllocaInst* alloca = Builder->CreateAlloca(llvm::Type::getInt1Ty(*TheContext));
    Builder->CreateStore(result, alloca);
    return {alloca};
}

// && - logical AND
IntrinsicResult build_and(Molecule& mol, const std::vector<llvm::Value*>& args) {
    (void)mol;
    llvm::Value* lhs = load_value(args[0], llvm::Type::getInt1Ty(*TheContext));
    llvm::Value* rhs = load_value(args[1], llvm::Type::getInt1Ty(*TheContext));
    llvm::Value* result = Builder->CreateAnd(lhs, rhs, "and");
    llvm::AllocaInst* alloca = Builder->CreateAlloca(llvm::Type::getInt1Ty(*TheContext));
    Builder->CreateStore(result, alloca);
    return {alloca};
}

// || - logical OR
IntrinsicResult build_or(Molecule& mol, const std::vector<llvm::Value*>& args) {
    (void)mol;
    llvm::Value* lhs = load_value(args[0], llvm::Type::getInt1Ty(*TheContext));
    llvm::Value* rhs = load_value(args[1], llvm::Type::getInt1Ty(*TheContext));
    llvm::Value* result = Builder->CreateOr(lhs, rhs, "or");
    llvm::AllocaInst* alloca = Builder->CreateAlloca(llvm::Type::getInt1Ty(*TheContext));
    Builder->CreateStore(result, alloca);
    return {alloca};
}

// def - declaration with REQUIRED type annotation, optional initial value
IntrinsicResult build_def(Molecule& mol) {
    if (mol.atoms.size() >= 2) {
        std::string var_name;
        std::string explicit_type;
        
        if (std::holds_alternative<Atom>(mol.atoms[1])) {
            Atom& var_atom = std::get<Atom>(mol.atoms[1]);
            var_name = var_atom.identifier;
            explicit_type = var_atom.type;
        }
        
        // Type annotation is required for def
        if (explicit_type.empty()) {
            std::cerr << "Error: def requires type annotation (e.g., def Int:x or def Int:x 5)" << std::endl;
            return {nullptr};
        }
        
        // Check if this is an extern struct type
        bool is_extern_struct = struct_registry.count(explicit_type) && struct_registry[explicit_type].is_extern;
        
        llvm::Type* llvm_type;
        if (is_extern_struct) {
            // For extern structs, allocate the actual struct type (not a pointer)
            llvm_type = struct_registry[explicit_type].llvm_type;
        } else {
            llvm_type = get_llvm_type(explicit_type);
        }
        
        // Use hoisted alloca if exists, otherwise create new one
        llvm::Value* var_ptr = nullptr;
        if (object_registry.count(var_name) && object_registry[var_name].value != nullptr) {
            var_ptr = object_registry[var_name].value;
        } else {
            llvm::AllocaInst* alloca = Builder->CreateAlloca(llvm_type);
            object_registry[var_name] = MemObject(explicit_type, alloca);
            var_ptr = alloca;
        }
        
        // If initial value provided, store it
        if (mol.atoms.size() >= 3) {
            llvm::Value* val_ptr = get_stored_in(mol.atoms[2]);
            if (val_ptr) {
                if (is_extern_struct) {
                    // For extern structs, copy the struct value directly
                    llvm::Value* val = Builder->CreateLoad(llvm_type, val_ptr);
                    Builder->CreateStore(val, var_ptr);
                } else {
                    llvm::Value* val = load_value(val_ptr, llvm_type);
                    Builder->CreateStore(val, var_ptr);
                }
            }
        }
        
        return {var_ptr};
    }
    return {nullptr};
}

// = - reassignment of existing variable only
IntrinsicResult build_reassign(Molecule& mol) {
    if (mol.atoms.size() >= 3) {
        std::string var_name;
        
        if (std::holds_alternative<Atom>(mol.atoms[1])) {
            Atom& var_atom = std::get<Atom>(mol.atoms[1]);
            var_name = var_atom.identifier;
        }
        
        // Variable must already exist
        if (!object_registry.count(var_name) || object_registry[var_name].value == nullptr) {
            std::cerr << "Error: variable '" << var_name << "' not defined. Use def to declare." << std::endl;
            return {nullptr};
        }
        
        std::string var_type = object_registry[var_name].type;
        llvm::Type* llvm_type = get_llvm_type(var_type);
        llvm::Value* val_ptr = get_stored_in(mol.atoms[2]);
        if (!val_ptr) return {nullptr};

        llvm::Value* val = load_value(val_ptr, llvm_type);
        llvm::Value* var_ptr = object_registry[var_name].value;
        Builder->CreateStore(val, var_ptr);
        return {var_ptr};
    }
    return {nullptr};
}

IntrinsicResult build_meow(Molecule& mol, const std::vector<llvm::Value*>& args) {
    std::string type = get_particle_type(mol.atoms[1]);
    
    if (type == "Str") {
        // String is now a struct, load the data pointer from it
        llvm::Type* char_type = llvm::Type::getInt8Ty(*TheContext);
        llvm::StructType* str_struct_type = get_array_struct_type(char_type);
        
        llvm::Value* str_ptr_ptr = args[0];
        llvm::Value* str_ptr = Builder->CreateLoad(llvm::PointerType::getUnqual(*TheContext), str_ptr_ptr, "str_ptr");
        
        llvm::Value* data_ptr_ptr = Builder->CreateStructGEP(str_struct_type, str_ptr, 2, "data_ptr_ptr");
        llvm::Value* data_ptr = Builder->CreateLoad(llvm::PointerType::getUnqual(*TheContext), data_ptr_ptr, "data_ptr");
        
        std::vector<llvm::Type*> puts_args;
        puts_args.push_back(llvm::PointerType::getUnqual(*TheContext));
        llvm::FunctionType* puts_type = llvm::FunctionType::get(llvm::Type::getInt32Ty(*TheContext), puts_args, false);
        llvm::FunctionCallee puts = TheModule->getOrInsertFunction("puts", puts_type);
        Builder->CreateCall(puts, data_ptr);
    } 
    
    return {nullptr};
}

IntrinsicResult build_return(Molecule& mol, const std::vector<llvm::Value*>& args) {
    if (args.empty()) {
        Builder->CreateRetVoid();
    } else {
        std::string type = get_particle_type(mol.atoms[1]);
        llvm::Type* llvm_type = get_llvm_type(type);
        llvm::Value* val = load_value(args[0], llvm_type);
        Builder->CreateRet(val);
    }
    return {nullptr};
}


IntrinsicResult build_conv(Molecule& mol, const std::vector<llvm::Value*>& args, const std::string& out_type) {
    std::string type = get_particle_type(mol.atoms[1]);

    llvm::Type* llvm_type = get_llvm_type(type);
    
    llvm::Value* val = load_value(args[0], llvm_type);
    
    if (out_type == "Str") {
        if (type == "Char") {
            // For Char, create a 2-byte string (char + null terminator)
            llvm::Type* char_type = llvm::Type::getInt8Ty(*TheContext);
            int buffer_size = 2;
            llvm::ArrayType* buffer_type = llvm::ArrayType::get(char_type, buffer_size);
            llvm::AllocaInst* buffer = Builder->CreateAlloca(buffer_type);
            
            // Store the character
            std::vector<llvm::Value*> indices0 = {
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 0),
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 0)
            };
            llvm::Value* char_ptr = Builder->CreateInBoundsGEP(buffer_type, buffer, indices0);
            Builder->CreateStore(val, char_ptr);
            
            // Store null terminator
            std::vector<llvm::Value*> indices1 = {
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 0),
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 1)
            };
            llvm::Value* null_ptr = Builder->CreateInBoundsGEP(buffer_type, buffer, indices1);
            Builder->CreateStore(llvm::ConstantInt::get(char_type, 0), null_ptr);
            
            // Build string struct
            llvm::StructType* str_struct_type = get_array_struct_type(char_type);
            llvm::AllocaInst* str_alloc = Builder->CreateAlloca(str_struct_type, nullptr, "conv_str_struct");
            
            llvm::Value* size_ptr = Builder->CreateStructGEP(str_struct_type, str_alloc, 0, "size_ptr");
            Builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 1), size_ptr);
            
            llvm::Value* cap_ptr = Builder->CreateStructGEP(str_struct_type, str_alloc, 1, "cap_ptr");
            Builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), buffer_size), cap_ptr);
            
            llvm::Value* data_ptr_ptr = Builder->CreateStructGEP(str_struct_type, str_alloc, 2, "data_ptr_ptr");
            Builder->CreateStore(buffer, data_ptr_ptr);
            
            llvm::AllocaInst* result_ptr = Builder->CreateAlloca(llvm::PointerType::getUnqual(*TheContext), nullptr, "str_ref");
            Builder->CreateStore(str_alloc, result_ptr);

            return {result_ptr};
        } else if (type == "Int") {
            llvm::Type* char_type = llvm::Type::getInt8Ty(*TheContext);
            int buffer_size = 12;
            llvm::ArrayType* buffer_type = llvm::ArrayType::get(char_type, buffer_size);
            llvm::AllocaInst* buffer = Builder->CreateAlloca(buffer_type);

            llvm::Value* format_str = Builder->CreateGlobalString("%d");

            std::vector<llvm::Type*> sprintf_args = {
                llvm::PointerType::getUnqual(*TheContext),
                llvm::PointerType::getUnqual(*TheContext)
            };

            llvm::FunctionType* sprintf_type = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(*TheContext), 
                sprintf_args, 
                true
            );
            
            llvm::FunctionCallee sprintf_func = TheModule->getOrInsertFunction("sprintf", sprintf_type);
            llvm::Value* written = Builder->CreateCall(sprintf_func, {buffer, format_str, val});

            // Build string struct like string literals
            llvm::StructType* str_struct_type = get_array_struct_type(char_type);
            llvm::AllocaInst* str_alloc = Builder->CreateAlloca(str_struct_type, nullptr, "conv_str_struct");
            
            llvm::Value* size_ptr = Builder->CreateStructGEP(str_struct_type, str_alloc, 0, "size_ptr");
            Builder->CreateStore(written, size_ptr);
            
            llvm::Value* cap_ptr = Builder->CreateStructGEP(str_struct_type, str_alloc, 1, "cap_ptr");
            Builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), buffer_size), cap_ptr);
            
            llvm::Value* data_ptr_ptr = Builder->CreateStructGEP(str_struct_type, str_alloc, 2, "data_ptr_ptr");
            Builder->CreateStore(buffer, data_ptr_ptr);
            
            llvm::AllocaInst* result_ptr = Builder->CreateAlloca(llvm::PointerType::getUnqual(*TheContext), nullptr, "str_ref");
            Builder->CreateStore(str_alloc, result_ptr);

            return {result_ptr};
        } else if (type == "Float") {
            llvm::Type* char_type = llvm::Type::getInt8Ty(*TheContext);
            int buffer_size = 32;
            llvm::ArrayType* buffer_type = llvm::ArrayType::get(char_type, buffer_size);
            llvm::AllocaInst* buffer = Builder->CreateAlloca(buffer_type);

            llvm::Value* format_str = Builder->CreateGlobalString("%f");

            std::vector<llvm::Type*> sprintf_args = {
                llvm::PointerType::getUnqual(*TheContext),
                llvm::PointerType::getUnqual(*TheContext)
            };

            llvm::FunctionType* sprintf_type = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(*TheContext), 
                sprintf_args, 
                true
            );
            
            llvm::FunctionCallee sprintf_func = TheModule->getOrInsertFunction("sprintf", sprintf_type);

            llvm::Value* double_val = Builder->CreateFPExt(val, llvm::Type::getDoubleTy(*TheContext));
            llvm::Value* written = Builder->CreateCall(sprintf_func, {buffer, format_str, double_val});

            // Build string struct like string literals
            llvm::StructType* str_struct_type = get_array_struct_type(char_type);
            llvm::AllocaInst* str_alloc = Builder->CreateAlloca(str_struct_type, nullptr, "conv_str_struct");
            
            llvm::Value* size_ptr = Builder->CreateStructGEP(str_struct_type, str_alloc, 0, "size_ptr");
            Builder->CreateStore(written, size_ptr);
            
            llvm::Value* cap_ptr = Builder->CreateStructGEP(str_struct_type, str_alloc, 1, "cap_ptr");
            Builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), buffer_size), cap_ptr);
            
            llvm::Value* data_ptr_ptr = Builder->CreateStructGEP(str_struct_type, str_alloc, 2, "data_ptr_ptr");
            Builder->CreateStore(buffer, data_ptr_ptr);
            
            llvm::AllocaInst* result_ptr = Builder->CreateAlloca(llvm::PointerType::getUnqual(*TheContext), nullptr, "str_ref");
            Builder->CreateStore(str_alloc, result_ptr);

            return {result_ptr};
        } else if (type == "Bool") {
            // Bool to Str: "true" or "false"
            llvm::Type* char_type = llvm::Type::getInt8Ty(*TheContext);
            
            // Create global strings for "true" and "false"
            llvm::Value* true_str = Builder->CreateGlobalString("true");
            llvm::Value* false_str = Builder->CreateGlobalString("false");
            
            // Select based on boolean value
            llvm::Value* selected_str = Builder->CreateSelect(val, true_str, false_str, "bool_str");
            llvm::Value* selected_len = Builder->CreateSelect(val, 
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 4),
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 5),
                "bool_len");
            
            // Build string struct
            llvm::StructType* str_struct_type = get_array_struct_type(char_type);
            llvm::AllocaInst* str_alloc = Builder->CreateAlloca(str_struct_type, nullptr, "conv_str_struct");
            
            llvm::Value* size_ptr = Builder->CreateStructGEP(str_struct_type, str_alloc, 0, "size_ptr");
            Builder->CreateStore(selected_len, size_ptr);
            
            llvm::Value* cap_ptr = Builder->CreateStructGEP(str_struct_type, str_alloc, 1, "cap_ptr");
            Builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 6), cap_ptr);
            
            llvm::Value* data_ptr_ptr = Builder->CreateStructGEP(str_struct_type, str_alloc, 2, "data_ptr_ptr");
            Builder->CreateStore(selected_str, data_ptr_ptr);
            
            llvm::AllocaInst* result_ptr = Builder->CreateAlloca(llvm::PointerType::getUnqual(*TheContext), nullptr, "str_ref");
            Builder->CreateStore(str_alloc, result_ptr);

            return {result_ptr};
        }
    } else if (out_type == "Int") {
        if (type == "Str") {
            // Str is now a struct, need to extract data pointer first
            llvm::Type* char_type = llvm::Type::getInt8Ty(*TheContext);
            llvm::StructType* str_struct_type = get_array_struct_type(char_type);
            
            llvm::Value* str_ptr = Builder->CreateLoad(llvm::PointerType::getUnqual(*TheContext), args[0], "str_ptr");
            llvm::Value* data_ptr_ptr = Builder->CreateStructGEP(str_struct_type, str_ptr, 2, "data_ptr_ptr");
            llvm::Value* data_ptr = Builder->CreateLoad(llvm::PointerType::getUnqual(*TheContext), data_ptr_ptr, "data_ptr");

            llvm::Value* format_str = Builder->CreateGlobalString("%d");

            std::vector<llvm::Type*> sscanf_args = {
                llvm::PointerType::getUnqual(*TheContext),
                llvm::PointerType::getUnqual(*TheContext)
            };

            llvm::FunctionType* sscanf_type = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(*TheContext), 
                sscanf_args, 
                true
            );
            
            llvm::FunctionCallee sscanf_func = TheModule->getOrInsertFunction("sscanf", sscanf_type);
            
            llvm::AllocaInst* result_int = Builder->CreateAlloca(llvm::Type::getInt32Ty(*TheContext));
            
            Builder->CreateCall(sscanf_func, {data_ptr, format_str, result_int});

            return {result_int};
        } 
    }
    

    return {nullptr};
}

IntrinsicResult build_array(Molecule& mol, const std::vector<llvm::Value*>& args) {
    if (args.empty()) {
        return {}; 
    }

    std::string element_type_str = get_particle_type(mol.atoms[1]);
    llvm::Type* element_type = get_llvm_type(element_type_str);
    
    int size = args.size();
    int capacity = std::pow(2, std::ceil(std::log2(size)));

    llvm::StructType* array_type = get_array_struct_type(element_type);
    llvm::AllocaInst* array_alloc = Builder->CreateAlloca(array_type, nullptr, "array_struct");

    llvm::Value* size_ptr = Builder->CreateStructGEP(array_type, array_alloc, 0, "size_ptr");
    Builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), size), size_ptr);

    llvm::Value* cap_ptr = Builder->CreateStructGEP(array_type, array_alloc, 1, "cap_ptr");
    Builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), capacity), cap_ptr);

    llvm::ArrayType* data_array_type = llvm::ArrayType::get(element_type, size);
    llvm::AllocaInst* data_alloc = Builder->CreateAlloca(data_array_type, nullptr, "data_arr");
    
    for (int i = 0; i < size; ++i) {
        llvm::Value* val = load_value(args[i], element_type);
        std::vector<llvm::Value*> indices = {
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 0),
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), i)
        };
        llvm::Value* ptr = Builder->CreateInBoundsGEP(data_array_type, data_alloc, indices, "elem_ptr");
        Builder->CreateStore(val, ptr);
    }

    llvm::Value* data_ptr_ptr = Builder->CreateStructGEP(array_type, array_alloc, 2, "data_ptr_ptr");
    llvm::Value* data_ptr = Builder->CreateBitCast(data_alloc, llvm::PointerType::getUnqual(*TheContext));
    Builder->CreateStore(data_ptr, data_ptr_ptr);

    llvm::AllocaInst* result_ptr = Builder->CreateAlloca(llvm::PointerType::getUnqual(*TheContext), nullptr, "array_ref");
    Builder->CreateStore(array_alloc, result_ptr);

    return {result_ptr};
}

IntrinsicResult build_array_element(Molecule& mol, const std::vector<llvm::Value*>& args, std::string name) {
    if (args.empty()) {
        return {}; 
    }

    std::string array_type_str = get_particle_type(mol.atoms[1]);
    std::string element_type_str = get_array_element_type_str(array_type_str);
    llvm::Type* element_type = get_llvm_type(element_type_str);

    if (name == "get" && args.size() < 2) return {nullptr};
    if (name == "set" && args.size() < 3) return {nullptr};

    llvm::Value* index_ptr = args[1];
    llvm::Value* array_ptr_ptr = args[0];

    llvm::Value* array_ptr = Builder->CreateLoad(
        llvm::PointerType::getUnqual(*TheContext), 
        array_ptr_ptr, 
        "array_ptr"
    );

    llvm::StructType* array_struct_type = get_array_struct_type(element_type);
    
    llvm::Value* data_ptr_ptr = Builder->CreateStructGEP(array_struct_type, array_ptr, 2, "data_ptr_ptr");
    llvm::Value* data_ptr = Builder->CreateLoad(
        llvm::PointerType::getUnqual(*TheContext), 
        data_ptr_ptr, 
        "data_ptr"
    );

    llvm::Value* index = Builder->CreateLoad(
        llvm::Type::getInt32Ty(*TheContext), 
        index_ptr, 
        "index"
    );
    
    if (name == "get") {
        llvm::Value* element_ptr = Builder->CreateInBoundsGEP(element_type, data_ptr, index, "elem_ptr");
        llvm::Value* element_val = Builder->CreateLoad(element_type, element_ptr, "elem_val");

        llvm::AllocaInst* result_alloca = Builder->CreateAlloca(element_type);
        Builder->CreateStore(element_val, result_alloca);
        return {result_alloca};
    } else if (name == "set") {
        llvm::Value* element_ptr = Builder->CreateInBoundsGEP(element_type, data_ptr, index, "elem_ptr");

        llvm::Value* value = load_value(args[2], element_type);
        Builder->CreateStore(value, element_ptr);

        return {nullptr};
    }

    return {nullptr};
}

IntrinsicResult build_array_size(Molecule& mol, const std::vector<llvm::Value*>& args) {
    if (args.empty()) {
        return {}; 
    }

    std::string array_type_str = get_particle_type(mol.atoms[1]);
    std::string element_type_str = get_array_element_type_str(array_type_str);
    llvm::Type* element_type = get_llvm_type(element_type_str);

    llvm::Value* array_ptr_ptr = args[0];

    llvm::Value* array_ptr = Builder->CreateLoad(
        llvm::PointerType::getUnqual(*TheContext), 
        array_ptr_ptr, 
        "array_ptr"
    );

    llvm::StructType* array_struct_type = get_array_struct_type(element_type);
    
    llvm::Value* size_ptr = Builder->CreateStructGEP(array_struct_type, array_ptr, 0, "size_ptr");
    llvm::Value* size = Builder->CreateLoad(
        llvm::Type::getInt32Ty(*TheContext), 
        size_ptr, 
        "size"
    );

    llvm::AllocaInst* result_alloca = Builder->CreateAlloca(llvm::Type::getInt32Ty(*TheContext));
    Builder->CreateStore(size, result_alloca);
    return {result_alloca};
}

IntrinsicResult build_array_memshift(Molecule& mol, const std::vector<llvm::Value*>& args, std::string name) {
    if (args.empty()) return {};

    llvm::Value* array_ptr_ptr = args[0];
    std::string array_type_str = get_particle_type(mol.atoms[1]);
    std::string element_type_str = get_array_element_type_str(array_type_str);
    llvm::Type* element_type = get_llvm_type(element_type_str);
    llvm::StructType* array_struct_type = get_array_struct_type(element_type);

    llvm::Value* array_ptr = Builder->CreateLoad(llvm::PointerType::getUnqual(*TheContext), array_ptr_ptr, "array_ptr");
    
    llvm::Value* size_ptr = Builder->CreateStructGEP(array_struct_type, array_ptr, 0, "size_ptr");
    llvm::Value* size = Builder->CreateLoad(llvm::Type::getInt32Ty(*TheContext), size_ptr, "size");
    
    llvm::Value* cap_ptr = Builder->CreateStructGEP(array_struct_type, array_ptr, 1, "cap_ptr");
    llvm::Value* capacity = Builder->CreateLoad(llvm::Type::getInt32Ty(*TheContext), cap_ptr, "capacity");
    
    llvm::Value* data_ptr_ptr = Builder->CreateStructGEP(array_struct_type, array_ptr, 2, "data_ptr_ptr");
    llvm::Value* data_ptr = Builder->CreateLoad(llvm::PointerType::getUnqual(*TheContext), data_ptr_ptr, "data_ptr");

    llvm::Value* size_of_elem = Builder->CreatePtrToInt(
        Builder->CreateInBoundsGEP(element_type, llvm::Constant::getNullValue(llvm::PointerType::getUnqual(*TheContext)), llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 1)),
        llvm::Type::getInt64Ty(*TheContext)
    );

    

    if (name == "append" || name == "insert") {
        // get more memory if needed
        // For strings, we need room for null terminator, so grow when size+1 >= capacity
        llvm::Value* cond;
        if (array_type_str == "Str") {
            llvm::Value* size_plus_one = Builder->CreateAdd(size, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 1));
            cond = Builder->CreateICmpUGE(size_plus_one, capacity);
        } else {
            cond = Builder->CreateICmpEQ(size, capacity);
        }
        llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(*TheContext, "grow", Builder->GetInsertBlock()->getParent());
        llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(*TheContext, "cont", Builder->GetInsertBlock()->getParent());
        
        Builder->CreateCondBr(cond, thenBB, mergeBB);
        Builder->SetInsertPoint(thenBB);

        llvm::Value* cap_is_zero = Builder->CreateICmpEQ(capacity, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 0));
        llvm::Value* double_cap = Builder->CreateMul(capacity, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 2));
        llvm::Value* new_cap = Builder->CreateSelect(cap_is_zero, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 1), double_cap);

        llvm::Value* total_size = Builder->CreateMul(Builder->CreateZExt(new_cap, llvm::Type::getInt64Ty(*TheContext)), size_of_elem);

        llvm::FunctionCallee malloc_func = TheModule->getOrInsertFunction("malloc", 
            llvm::FunctionType::get(llvm::PointerType::getUnqual(*TheContext), {llvm::Type::getInt64Ty(*TheContext)}, false));
        llvm::Value* new_data = Builder->CreateBitCast(Builder->CreateCall(malloc_func, {total_size}), llvm::PointerType::getUnqual(*TheContext));
        
        llvm::Value* current_bytes = Builder->CreateMul(Builder->CreateZExt(size, llvm::Type::getInt64Ty(*TheContext)), size_of_elem);
        llvm::FunctionCallee memcpy_func = TheModule->getOrInsertFunction("memcpy",
            llvm::FunctionType::get(llvm::PointerType::getUnqual(*TheContext), 
            {llvm::PointerType::getUnqual(*TheContext), llvm::PointerType::getUnqual(*TheContext), llvm::Type::getInt64Ty(*TheContext)}, false));
        
        Builder->CreateCall(memcpy_func, {
            Builder->CreateBitCast(new_data, llvm::PointerType::getUnqual(*TheContext)),
            Builder->CreateBitCast(data_ptr, llvm::PointerType::getUnqual(*TheContext)),
            current_bytes
        });
        
        Builder->CreateStore(new_cap, cap_ptr);
        Builder->CreateStore(new_data, data_ptr_ptr);
        Builder->CreateBr(mergeBB);
        Builder->SetInsertPoint(mergeBB);
        
        data_ptr = Builder->CreateLoad(llvm::PointerType::getUnqual(*TheContext), data_ptr_ptr, "data_ptr_reloaded");
        
        llvm::Value* idx = (name == "append") ? size : Builder->CreateLoad(llvm::Type::getInt32Ty(*TheContext), args[1]);
        llvm::Value* val = load_value((name == "append") ? args[1] : args[2], element_type);

        if (name == "insert") {
            llvm::Value* move_size = Builder->CreateSub(size, idx);
            llvm::Value* move_bytes = Builder->CreateMul(Builder->CreateZExt(move_size, llvm::Type::getInt64Ty(*TheContext)), size_of_elem);
            
            llvm::Value* src_ptr = Builder->CreateInBoundsGEP(element_type, data_ptr, idx);
            llvm::Value* dst_ptr = Builder->CreateInBoundsGEP(element_type, data_ptr, Builder->CreateAdd(idx, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 1)));
            
            llvm::FunctionCallee memmove_func = TheModule->getOrInsertFunction("memmove",
                llvm::FunctionType::get(llvm::PointerType::getUnqual(*TheContext), 
                {llvm::PointerType::getUnqual(*TheContext), llvm::PointerType::getUnqual(*TheContext), llvm::Type::getInt64Ty(*TheContext)}, false));
            
            Builder->CreateCall(memmove_func, {
                Builder->CreateBitCast(dst_ptr, llvm::PointerType::getUnqual(*TheContext)),
                Builder->CreateBitCast(src_ptr, llvm::PointerType::getUnqual(*TheContext)),
                move_bytes
            });
        }

        Builder->CreateStore(val, Builder->CreateInBoundsGEP(element_type, data_ptr, idx));
        llvm::Value* new_size = Builder->CreateAdd(size, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 1));
        Builder->CreateStore(new_size, size_ptr);
        
        // Add null terminator for strings
        if (array_type_str == "Str") {
            llvm::Value* null_ptr = Builder->CreateInBoundsGEP(element_type, data_ptr, new_size);
            Builder->CreateStore(llvm::ConstantInt::get(element_type, 0), null_ptr);
        }
        
        return {array_ptr_ptr};
    } 
    else if (name == "remove") {
        llvm::Value* idx = Builder->CreateLoad(llvm::Type::getInt32Ty(*TheContext), args[1]);
        llvm::Value* move_size = Builder->CreateSub(Builder->CreateSub(size, idx), llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 1));
        llvm::Value* move_bytes = Builder->CreateMul(Builder->CreateZExt(move_size, llvm::Type::getInt64Ty(*TheContext)), size_of_elem);
        
        llvm::Value* dst_ptr = Builder->CreateInBoundsGEP(element_type, data_ptr, idx);
        llvm::Value* src_ptr = Builder->CreateInBoundsGEP(element_type, data_ptr, Builder->CreateAdd(idx, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 1)));
        
        llvm::FunctionCallee memmove_func = TheModule->getOrInsertFunction("memmove",
            llvm::FunctionType::get(llvm::PointerType::getUnqual(*TheContext), 
            {llvm::PointerType::getUnqual(*TheContext), llvm::PointerType::getUnqual(*TheContext), llvm::Type::getInt64Ty(*TheContext)}, false));
            
        Builder->CreateCall(memmove_func, {
            Builder->CreateBitCast(dst_ptr, llvm::PointerType::getUnqual(*TheContext)),
            Builder->CreateBitCast(src_ptr, llvm::PointerType::getUnqual(*TheContext)),
            move_bytes
        });
        
        llvm::Value* new_size = Builder->CreateSub(size, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 1));
        Builder->CreateStore(new_size, size_ptr);
        
        // Add null terminator for strings
        if (array_type_str == "Str") {
            llvm::Value* null_ptr = Builder->CreateInBoundsGEP(element_type, data_ptr, new_size);
            Builder->CreateStore(llvm::ConstantInt::get(element_type, 0), null_ptr);
        }
        
        return {array_ptr_ptr};
    }
    else if (name == "pop_back") {
        llvm::Value* new_size = Builder->CreateSub(size, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 1));
        Builder->CreateStore(new_size, size_ptr);
        
        llvm::Value* val = Builder->CreateLoad(element_type, Builder->CreateInBoundsGEP(element_type, data_ptr, new_size));
        
        // Add null terminator for strings
        if (array_type_str == "Str") {
            llvm::Value* null_ptr = Builder->CreateInBoundsGEP(element_type, data_ptr, new_size);
            Builder->CreateStore(llvm::ConstantInt::get(element_type, 0), null_ptr);
        }
        
        llvm::AllocaInst* result_alloca = Builder->CreateAlloca(element_type);
        Builder->CreateStore(val, result_alloca);
        return {result_alloca};
    }

    return {nullptr};
}

void init_intrinsics() {
    // arithmetic type: int+int=int, float+float=float
    auto arithmetic_type = [](const std::vector<Particle>& args) -> std::string {
        if (args.size() >= 2) {
            std::string t1 = get_particle_type(args[0]);
            std::string t2 = get_particle_type(args[1]);
            if (t1 == "Int" && t2 == "Int") return "Int";
            if (t1 == "Float" && t2 == "Float") return "Float";
            if (t1 == "Var" || t2 == "Var") return "Var";
        }
        return "Nil";
    };

    // comparison always returns bool
    auto comparison_type = [](const std::vector<Particle>&) -> std::string {
        return "Bool";
    };

    // def - declaration requires type annotation
    auto def_type = [](const std::vector<Particle>& args) -> std::string {
        if (args.size() >= 1) {
            if (std::holds_alternative<Atom>(args[0])) {
                const Atom& var_atom = std::get<Atom>(args[0]);
                std::string explicit_type = var_atom.type;
                
                if (explicit_type.empty()) {
                    std::cerr << "Error: def requires type annotation (e.g., def Int:x)" << std::endl;
                    return "Nil";
                }
                
                std::string var_name = var_atom.identifier;
                if (!object_registry.count(var_name)) {
                    object_registry[var_name] = MemObject(explicit_type, nullptr);
                }
                return explicit_type;
            }
        }
        return "Nil";
    };
    
    // = - reassignment uses existing variable type
    auto reassign_type = [](const std::vector<Particle>& args) -> std::string {
        if (args.size() >= 2) {
            if (std::holds_alternative<Atom>(args[0])) {
                std::string var_name = std::get<Atom>(args[0]).identifier;
                if (object_registry.count(var_name)) {
                    return object_registry[var_name].type;
                }
            }
        }
        return "Nil";
    };
    
    // get the first argument's type
    auto infer_first_type = [](const std::vector<Particle>& args) -> std::string {
        if (args.empty()) return "Nil";
        return get_particle_type(args[0]);
    };

    // return Str
    auto str_type = [](const std::vector<Particle>&) -> std::string {
        return "Str";
    };

    // return Int
    auto int_type = [](const std::vector<Particle>&) -> std::string {
        return "Int";
    };

    // return nothing
    auto nil_type = [](const std::vector<Particle>&) -> std::string {
        return "Nil";
    };

    // infer the type of the array element (for get operation)
    auto infer_array_type = [](const std::vector<Particle>& args) -> std::string {
        std::string particle_type = get_particle_type(args[0]);
        if (particle_type == "Str") return "Char";
        return get_array_element_type_str(particle_type);
    };

    // infer the type of an element of the array
    auto infer_element_type = [](const std::vector<Particle>& args) -> std::string {
        if (args.empty()) return "Nil";
        std::string particle_type = get_particle_type(args[0]);
        if (particle_type == "Str") return "Char";
        return get_array_element_type_str(particle_type);
    };

    // returns first arg type (for append/insert/remove on arrays and strings)
    auto infer_array_self_type = [](const std::vector<Particle>& args) -> std::string {
        if (args.empty()) return "Nil";
        return get_particle_type(args[0]);
    };
    

    // arithmetic
    INTRINSICS["+"] = Function("+", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_arith(mol, args, "+"); }, arithmetic_type);
    INTRINSICS["-"] = Function("-", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_arith(mol, args, "-"); }, arithmetic_type);
    INTRINSICS["*"] = Function("*", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_arith(mol, args, "*"); }, arithmetic_type);
    INTRINSICS["/"] = Function("/", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_arith(mol, args, "/"); }, arithmetic_type);
    INTRINSICS["%"] = Function("%", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_arith(mol, args, "%"); }, arithmetic_type);
    
    // modifying arithmetic
    INTRINSICS["++"] = Function("++", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_compound_arith(mol, args, "++"); }, infer_first_type);
    INTRINSICS["eat"] = Function("eat", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_compound_arith(mol, args, "++"); }, infer_first_type);
    INTRINSICS["--"] = Function("--", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_compound_arith(mol, args, "--"); }, infer_first_type);
    INTRINSICS["exercise"] = Function("exercise", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_compound_arith(mol, args, "++"); }, infer_first_type);
    INTRINSICS["+="] = Function("+=", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_compound_arith(mol, args, "+"); }, infer_first_type);
    INTRINSICS["-="] = Function("-=", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_compound_arith(mol, args, "-"); }, infer_first_type);
    
    // comparison
    INTRINSICS["=="] = Function("==", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_compare(mol, args, "=="); }, comparison_type);
    INTRINSICS["!="] = Function("!=", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_compare(mol, args, "!="); }, comparison_type);
    INTRINSICS[">"] = Function(">", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_compare(mol, args, ">"); }, comparison_type);
    INTRINSICS[">="] = Function(">=", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_compare(mol, args, ">="); }, comparison_type);
    INTRINSICS["<"] = Function("<", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_compare(mol, args, "<"); }, comparison_type);
    INTRINSICS["<="] = Function("<=", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_compare(mol, args, "<="); }, comparison_type);
    
    // boolean not
    INTRINSICS["!"] = Function("!", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_not(mol, args); }, comparison_type);
    
    // boolean and/or
    INTRINSICS["&&"] = Function("&&", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_and(mol, args); }, comparison_type);
    INTRINSICS["||"] = Function("||", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_or(mol, args); }, comparison_type);
    
    // def = declaration (requires type annotation)
    INTRINSICS["def"] = Function("def", [](Molecule& mol, const std::vector<llvm::Value*>&) { return build_def(mol); }, def_type);
    // = = reassignment only (variable must exist)
    INTRINSICS["="] = Function("=", [](Molecule& mol, const std::vector<llvm::Value*>&) { return build_reassign(mol); }, reassign_type);
    
    // meow (always str) to overload later
    INTRINSICS["meow"] = Function("meow", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_meow(mol, args); }, nil_type);
    
    // return
    INTRINSICS["return"] = Function("return", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_return(mol, args); }, infer_first_type);
    
    // typecasts
    INTRINSICS["->S"] = Function("->S", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_conv(mol, args, "Str"); }, str_type);
    INTRINSICS["->I"] = Function("->I", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_conv(mol, args, "Int"); }, int_type);

    auto array_type = [](const std::vector<Particle>& args) -> std::string {
        if (args.empty()) return "Array<Nil>";
        return "Array<" + get_particle_type(args[0]) + ">";
    };
    INTRINSICS["array"] = Function("array", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_array(mol, args); }, array_type);
    INTRINSICS["len"] = Function("len", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_array_size(mol, args); }, int_type);
    INTRINSICS["get"] = Function("get", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_array_element(mol, args, "get"); }, infer_array_type);
    INTRINSICS["set"] = Function("set", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_array_element(mol, args, "set"); }, nil_type);

    INTRINSICS["append"] = Function("append", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_array_memshift(mol, args, "append"); }, infer_array_self_type);
    INTRINSICS["insert"] = Function("insert", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_array_memshift(mol, args, "insert"); }, infer_array_self_type);
    INTRINSICS["remove"] = Function("remove", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_array_memshift(mol, args, "remove"); }, infer_array_self_type);
    INTRINSICS["pop_back"] = Function("pop_back", [](Molecule& mol, const std::vector<llvm::Value*>& args) { return build_array_memshift(mol, args, "pop_back"); }, infer_element_type);
}
