#include "compiler.hpp"

// Helper: Extract char* data pointer from a Str (for passing to C functions)
static llvm::Value* extract_cstring(llvm::Value* str_ptr_ptr) {
    llvm::Type* ptr_type = llvm::PointerType::getUnqual(*TheContext);
    llvm::Type* char_type = llvm::Type::getInt8Ty(*TheContext);
    llvm::StructType* str_struct_type = get_array_struct_type(char_type);
    
    // Load the struct pointer from the pointer-to-pointer
    llvm::Value* str_ptr = Builder->CreateLoad(ptr_type, str_ptr_ptr, "str_ptr");
    // Get the data pointer field (index 2)
    llvm::Value* data_ptr_ptr = Builder->CreateStructGEP(str_struct_type, str_ptr, 2, "data_ptr_ptr");
    // Load the actual char* data pointer
    llvm::Value* data_ptr = Builder->CreateLoad(ptr_type, data_ptr_ptr, "data_ptr");
    return data_ptr;
}


llvm::Value* evaluate(Atom& atom) {
    // Handle member access (e.g., bob>name)
    if (!atom.member_access.empty() && object_registry.count(atom.identifier)) {
        std::string var_type = object_registry[atom.identifier].type;
        if (struct_registry.count(var_type)) {
            StructDef& def = struct_registry[var_type];
            
            // Find field index
            int field_idx = -1;
            for (size_t i = 0; i < def.field_names.size(); i++) {
                if (def.field_names[i] == atom.member_access) {
                    field_idx = i;
                    break;
                }
            }
            
            if (field_idx >= 0) {
                // Load struct pointer
                llvm::Value* struct_ptr_ptr = object_registry[atom.identifier].value;
                llvm::Value* struct_ptr = Builder->CreateLoad(llvm::PointerType::getUnqual(*TheContext), struct_ptr_ptr);
                
                // GEP to field
                llvm::Value* field_ptr = Builder->CreateStructGEP(def.llvm_type, struct_ptr, field_idx);
                
                // For Str/struct fields (pointer types), load and wrap in another alloca
                std::string field_type = def.field_types[field_idx];
                if (field_type == "Str" || struct_registry.count(field_type)) {
                    llvm::Value* field_val = Builder->CreateLoad(llvm::PointerType::getUnqual(*TheContext), field_ptr);
                    llvm::AllocaInst* result = Builder->CreateAlloca(llvm::PointerType::getUnqual(*TheContext));
                    Builder->CreateStore(field_val, result);
                    atom.stored_in = result;
                    atom.type = field_type;
                    return result;
                }
                
                atom.stored_in = field_ptr;
                atom.type = field_type;
                return field_ptr;
            }
        }
    }
    
    // Handle string literals BEFORE variable lookup
    // (string literal "bob" becomes identifier "bob" with type "Str")
    if (atom.type == "Str") {
        llvm::Type* char_type = llvm::Type::getInt8Ty(*TheContext);
        int size = atom.identifier.size();
        int capacity = std::pow(2, std::ceil(std::log2(size > 0 ? size + 1 : 1)));
        
        llvm::StructType* str_struct_type = get_array_struct_type(char_type);
        llvm::AllocaInst* str_alloc = Builder->CreateAlloca(str_struct_type, nullptr, "str_struct");
        
        llvm::Value* size_ptr = Builder->CreateStructGEP(str_struct_type, str_alloc, 0, "size_ptr");
        Builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), size), size_ptr);
        
        llvm::Value* cap_ptr = Builder->CreateStructGEP(str_struct_type, str_alloc, 1, "cap_ptr");
        Builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), capacity), cap_ptr);
        
        llvm::ArrayType* data_array_type = llvm::ArrayType::get(char_type, size + 1);
        llvm::AllocaInst* data_alloc = Builder->CreateAlloca(data_array_type, nullptr, "str_data");
        
        for (int i = 0; i < size; ++i) {
            std::vector<llvm::Value*> indices = {
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 0),
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), i)
            };
            llvm::Value* ptr = Builder->CreateInBoundsGEP(data_array_type, data_alloc, indices, "char_ptr");
            Builder->CreateStore(llvm::ConstantInt::get(char_type, (unsigned char)atom.identifier[i]), ptr);
        }
        
        std::vector<llvm::Value*> null_indices = {
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 0),
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), size)
        };
        llvm::Value* null_ptr = Builder->CreateInBoundsGEP(data_array_type, data_alloc, null_indices, "null_ptr");
        Builder->CreateStore(llvm::ConstantInt::get(char_type, 0), null_ptr);
        
        llvm::Value* data_ptr_ptr = Builder->CreateStructGEP(str_struct_type, str_alloc, 2, "data_ptr_ptr");
        llvm::Value* data_ptr = Builder->CreateBitCast(data_alloc, llvm::PointerType::getUnqual(*TheContext));
        Builder->CreateStore(data_ptr, data_ptr_ptr);
        
        llvm::AllocaInst* result_ptr = Builder->CreateAlloca(llvm::PointerType::getUnqual(*TheContext), nullptr, "str_ref");
        Builder->CreateStore(str_alloc, result_ptr);
        
        atom.stored_in = result_ptr;
        return result_ptr;
    }
    
    if (object_registry.count(atom.identifier)) {
        llvm::Value* val = object_registry[atom.identifier].value;
        if (val) {
            atom.stored_in = val;
            return val;
        }
    }
    
    llvm::Constant* const_val = get_llvm_constant(atom);
    if (const_val) {
        llvm::Type* type = const_val->getType();
        
        llvm::AllocaInst* alloca = Builder->CreateAlloca(type);
        Builder->CreateStore(const_val, alloca);
        atom.stored_in = alloca;
        return alloca;
    }
    
    return nullptr;
}

llvm::Value* evaluate(Molecule& mol) {
    if (std::holds_alternative<Atom>(mol.subject())) {
        Atom subj = std::get<Atom>(mol.subject());
        
        std::vector<llvm::Value*> args;
        
        for (size_t i = 1; i < mol.atoms.size(); i++) {
            auto& arg = mol.atoms[i];
            llvm::Value* val = get_stored_in(arg);
            if (val) {
                args.push_back(val);
            }
        }
    
        std::string fn_name = subj.identifier;

        // Check overloads first
        if (overload_registry.count(fn_name)) {
            // Get argument types
            std::vector<std::string> arg_types;
            for (size_t i = 1; i < mol.atoms.size(); i++) {
                arg_types.push_back(get_particle_type(mol.atoms[i]));
            }
            
            // Find matching overload
            for (const std::string& candidate : overload_registry[fn_name]) {
                if (INTRINSICS.count(candidate)) {
                    Function& fn = INTRINSICS[candidate];
                    if (fn.param_types.size() == arg_types.size()) {
                        bool match = true;
                        for (size_t i = 0; i < arg_types.size(); i++) {
                            if (fn.param_types[i] != arg_types[i]) {
                                match = false;
                                break;
                            }
                        }
                        if (match) {
                            IntrinsicResult result = fn.evaluate(mol, args);
                            mol.stored_in = result.value;
                            // Set the molecule's type from the matched function's type inference
                            std::vector<Particle> type_args(mol.atoms.begin() + 1, mol.atoms.end());
                            mol.type = fn.type_inference(type_args);
                            return result.value;
                        }
                    }
                }
            }
        }

        // Fall back to original intrinsic
        if (INTRINSICS.count(fn_name)) {
            IntrinsicResult result = INTRINSICS.at(fn_name).evaluate(mol, args);
            mol.stored_in = result.value;
            return result.value;
        }
    }
    return nullptr;
}

// Pass 1.5: Collect struct declarations before variable hoisting
// This populates struct_registry so hoisting can use correct types
void collect_struct_declarations(Particle& p) {
    if (std::holds_alternative<Molecule>(p)) {
        Molecule& mol = std::get<Molecule>(p);
        if (mol.atoms.empty()) return;
        
        if (std::holds_alternative<Atom>(mol.subject())) {
            std::string subj = std::get<Atom>(mol.subject()).identifier;
            
            if (subj == "extern-struct" && mol.atoms.size() >= 3) {
                // (extern-struct Color [Char:r Char:g Char:b Char:a])
                std::string struct_name = std::get<Atom>(mol.atoms[1]).identifier;
                Molecule& fields_mol = std::get<Molecule>(mol.atoms[2]);
                
                StructDef def;
                def.name = struct_name;
                def.is_extern = true;
                
                std::vector<llvm::Type*> llvm_field_types;
                for (size_t i = 1; i < fields_mol.atoms.size(); i++) {
                    Atom& field = std::get<Atom>(fields_mol.atoms[i]);
                    def.field_names.push_back(field.identifier);
                    def.field_types.push_back(field.type);
                    llvm_field_types.push_back(get_llvm_type(field.type));
                }
                
                def.llvm_type = llvm::StructType::create(*TheContext, llvm_field_types, struct_name);
                struct_registry[struct_name] = def;
                return;
            } else if (subj == "struct" && mol.atoms.size() >= 3) {
                // (struct Person [Str:name Int:age Bool:active])
                std::string struct_name = std::get<Atom>(mol.atoms[1]).identifier;
                Molecule& fields_mol = std::get<Molecule>(mol.atoms[2]);
                
                StructDef def;
                def.name = struct_name;
                def.is_extern = false;
                
                std::vector<llvm::Type*> llvm_field_types;
                for (size_t i = 1; i < fields_mol.atoms.size(); i++) {
                    Atom& field = std::get<Atom>(fields_mol.atoms[i]);
                    def.field_names.push_back(field.identifier);
                    def.field_types.push_back(field.type);
                    llvm_field_types.push_back(get_llvm_type(field.type));
                }
                
                def.llvm_type = llvm::StructType::create(*TheContext, llvm_field_types, struct_name);
                struct_registry[struct_name] = def;
                return;
            }
        }
        
        // Recurse into child expressions
        for (size_t i = 1; i < mol.atoms.size(); i++) {
            collect_struct_declarations(mol.atoms[i]);
        }
    }
}

void collect_variables(Particle& p, std::unordered_map<std::string, std::string>& vars) {
    if (std::holds_alternative<Molecule>(p)) {
        Molecule& mol = std::get<Molecule>(p);
        if (mol.atoms.empty()) return;
        
        if (std::holds_alternative<Atom>(mol.subject())) {
            std::string subj = std::get<Atom>(mol.subject()).identifier;
            
            // Only def is for declaration; = is for reassignment
            if (subj == "def" && mol.atoms.size() >= 2) {
                if (std::holds_alternative<Atom>(mol.atoms[1])) {
                    Atom& var_atom = std::get<Atom>(mol.atoms[1]);
                    std::string var_name = var_atom.identifier;
                    std::string var_type = var_atom.type;  // Type annotation required for def
                    
                    if (!var_type.empty() && var_type != "Str" && !vars.count(var_name)) {
                        vars[var_name] = var_type;
                    }
                }
            }
        }
        
        for (size_t i = 1; i < mol.atoms.size(); i++) {
            collect_variables(mol.atoms[i], vars);
        }
    }
}

void compile(Particle& p) {
    if (std::holds_alternative<Molecule>(p)) {
        Molecule& mol = std::get<Molecule>(p);
        if (mol.atoms.empty()) return;
        
        if (std::holds_alternative<Atom>(mol.subject())) {
            std::string subj = std::get<Atom>(mol.subject()).identifier;
            
            if (subj == "block") {
                llvm::Function* TheFunction = Builder->GetInsertBlock()->getParent();
                llvm::BasicBlock* NewBB = llvm::BasicBlock::Create(*TheContext, "block", TheFunction);
                Builder->CreateBr(NewBB);
                Builder->SetInsertPoint(NewBB);
                
                for (size_t i = 1; i < mol.atoms.size(); i++) {
                    compile(mol.atoms[i]);
                }
                return;
            } else if (subj == "if") {
                // Compile condition
                compile(mol.atoms[1]);
                llvm::Value* cond_ptr = get_stored_in(mol.atoms[1]);
                // Load condition (expect i1)
                llvm::Value* cond = Builder->CreateLoad(llvm::Type::getInt1Ty(*TheContext), cond_ptr, "ifcond");
                
                llvm::Function* TheFunction = Builder->GetInsertBlock()->getParent();
                llvm::BasicBlock* ThenBB = llvm::BasicBlock::Create(*TheContext, "then", TheFunction);
                
                bool has_else = (mol.atoms.size() == 4);
                llvm::BasicBlock* ElseBB = has_else ? llvm::BasicBlock::Create(*TheContext, "else", TheFunction) : nullptr;
                llvm::BasicBlock* MergeBB = llvm::BasicBlock::Create(*TheContext, "ifcont", TheFunction);
                
                Builder->CreateCondBr(cond, ThenBB, has_else ? ElseBB : MergeBB);
                
                // Then
                Builder->SetInsertPoint(ThenBB);
                compile(mol.atoms[2]);
                if (!Builder->GetInsertBlock()->getTerminator()) {
                    Builder->CreateBr(MergeBB);
                }
                ThenBB = Builder->GetInsertBlock();
                
                // Else
                if (has_else) {
                    Builder->SetInsertPoint(ElseBB);
                    compile(mol.atoms[3]);
                    if (!Builder->GetInsertBlock()->getTerminator()) {
                        Builder->CreateBr(MergeBB);
                    }
                    ElseBB = Builder->GetInsertBlock();
                }
                
                // Only keep MergeBB if it has predecessors
                if (MergeBB->hasNPredecessorsOrMore(1)) {
                    Builder->SetInsertPoint(MergeBB);
                } else {
                    MergeBB->eraseFromParent();
                }
                return;
            } else if (subj == "while") {
                llvm::Function* TheFunction = Builder->GetInsertBlock()->getParent();
                llvm::BasicBlock* CondBB = llvm::BasicBlock::Create(*TheContext, "cond", TheFunction);
                llvm::BasicBlock* LoopBB = llvm::BasicBlock::Create(*TheContext, "loop", TheFunction);
                llvm::BasicBlock* MergeBB = llvm::BasicBlock::Create(*TheContext, "whilecont", TheFunction);
                
                Builder->CreateBr(CondBB);
                
                // Condition
                Builder->SetInsertPoint(CondBB);
                compile(mol.atoms[1]);
                llvm::Value* cond_ptr = get_stored_in(mol.atoms[1]);
                llvm::Value* cond = Builder->CreateLoad(llvm::Type::getInt1Ty(*TheContext), cond_ptr, "whilecond");
                Builder->CreateCondBr(cond, LoopBB, MergeBB);
                
                // Body
                Builder->SetInsertPoint(LoopBB);
                compile(mol.atoms[2]);
                if (!Builder->GetInsertBlock()->getTerminator()) {
                    Builder->CreateBr(CondBB);
                }
                
                Builder->SetInsertPoint(MergeBB);
                return;
            } else if (subj == "web-loop") {
                // (web-loop fps { body })
                // For emscripten: creates UpdateFrame function and calls emscripten_set_main_loop
                
                // Get FPS value
                compile(mol.atoms[1]);
                llvm::Value* fps_ptr = get_stored_in(mol.atoms[1]);
                llvm::Value* fps_val = Builder->CreateLoad(llvm::Type::getInt32Ty(*TheContext), fps_ptr, "fps");
                
                // Save current insert point
                llvm::Function* MainFunc = Builder->GetInsertBlock()->getParent();
                llvm::BasicBlock* ReturnPoint = Builder->GetInsertBlock();
                
                // Create the UpdateFrame function (void -> void)
                llvm::FunctionType* UpdateFT = llvm::FunctionType::get(llvm::Type::getVoidTy(*TheContext), false);
                llvm::Function* UpdateFunc = llvm::Function::Create(UpdateFT, llvm::Function::ExternalLinkage, "UpdateFrame", TheModule.get());
                llvm::BasicBlock* UpdateBB = llvm::BasicBlock::Create(*TheContext, "entry", UpdateFunc);
                Builder->SetInsertPoint(UpdateBB);
                
                // Compile the loop body inside UpdateFrame
                compile(mol.atoms[2]);
                
                // Return void from UpdateFrame
                if (!Builder->GetInsertBlock()->getTerminator()) {
                    Builder->CreateRetVoid();
                }
                
                // Back to main - call emscripten_set_main_loop
                Builder->SetInsertPoint(ReturnPoint);
                
                // Declare emscripten_set_main_loop(void (*func)(void), int fps, int simulate_infinite_loop)
                std::vector<llvm::Type*> loop_args = {
                    llvm::PointerType::getUnqual(*TheContext),  // function pointer
                    llvm::Type::getInt32Ty(*TheContext),         // fps
                    llvm::Type::getInt32Ty(*TheContext)          // simulate_infinite_loop
                };
                llvm::FunctionType* LoopFT = llvm::FunctionType::get(llvm::Type::getVoidTy(*TheContext), loop_args, false);
                llvm::FunctionCallee SetMainLoop = TheModule->getOrInsertFunction("emscripten_set_main_loop", LoopFT);
                
                Builder->CreateCall(SetMainLoop, {
                    UpdateFunc,
                    fps_val,
                    llvm::ConstantInt::get(llvm::Type::getInt32Ty(*TheContext), 1)  // simulate_infinite_loop = 1
                });
                
                return;
            } else if (subj == "fun") {
                // (fun ReturnType:(name Param1Type:p1 Param2Type:p2) { body })
                // mol.atoms[1] is the typed molecule with return type, containing name and params
                // mol.atoms[2] is the body block
                
                Molecule& sig = std::get<Molecule>(mol.atoms[1]);
                std::string return_type = sig.type;
                std::string func_name = std::get<Atom>(sig.atoms[0]).identifier;
                
                // Collect parameter types and names
                std::vector<std::string> param_names;
                std::vector<std::string> param_types;
                std::vector<llvm::Type*> llvm_param_types;
                
                for (size_t i = 1; i < sig.atoms.size(); i++) {
                    Atom& param = std::get<Atom>(sig.atoms[i]);
                    param_names.push_back(param.identifier);
                    param_types.push_back(param.type);
                    llvm_param_types.push_back(get_llvm_type(param.type));
                }
                
                // Create function type and function
                llvm::Type* llvm_ret_type = get_llvm_type(return_type);
                llvm::FunctionType* FT = llvm::FunctionType::get(llvm_ret_type, llvm_param_types, false);
                llvm::Function* Func = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, func_name, TheModule.get());
                
                // Save current state
                llvm::BasicBlock* SavedBB = Builder->GetInsertBlock();
                auto saved_registry = object_registry;
                
                // Create entry block
                llvm::BasicBlock* EntryBB = llvm::BasicBlock::Create(*TheContext, "entry", Func);
                Builder->SetInsertPoint(EntryBB);
                
                // Allocate parameters and add to registry
                size_t idx = 0;
                for (auto& Arg : Func->args()) {
                    llvm::AllocaInst* alloca = Builder->CreateAlloca(llvm_param_types[idx], nullptr, param_names[idx]);
                    Builder->CreateStore(&Arg, alloca);
                    object_registry[param_names[idx]] = MemObject(param_types[idx], alloca);
                    idx++;
                }
                
                // Compile body (skip "block" atom at index 0)
                Molecule& body = std::get<Molecule>(mol.atoms[2]);
                for (size_t i = 1; i < body.atoms.size(); i++) {
                    compile(body.atoms[i]);
                }
                
                // Add implicit return if block doesn't have a terminator
                if (!Builder->GetInsertBlock()->getTerminator()) {
                    if (llvm_ret_type->isVoidTy()) {
                        Builder->CreateRetVoid();
                    } else {
                        // Return a default value (zero) - should probably be an error
                        Builder->CreateRet(llvm::Constant::getNullValue(llvm_ret_type));
                    }
                }
                
                // Restore state
                object_registry = saved_registry;
                Builder->SetInsertPoint(SavedBB);
                
                // Register function as intrinsic for calling
                std::string fn_return_type = return_type;
                Function fn(func_name, 
                    [Func, llvm_param_types, llvm_ret_type](Molecule& call_mol, const std::vector<llvm::Value*>& args) -> IntrinsicResult {
                        std::vector<llvm::Value*> call_args;
                        for (size_t i = 0; i < args.size(); i++) {
                            llvm::Value* loaded = Builder->CreateLoad(llvm_param_types[i], args[i]);
                            call_args.push_back(loaded);
                        }
                        llvm::Value* result = Builder->CreateCall(Func, call_args);
                        if (llvm_ret_type->isVoidTy()) {
                            return {nullptr};
                        }
                        llvm::AllocaInst* alloca = Builder->CreateAlloca(llvm_ret_type);
                        Builder->CreateStore(result, alloca);
                        return {alloca};
                    },
                    [fn_return_type](const std::vector<Particle>&) { return fn_return_type; }
                );
                fn.param_types = param_types;  // Store for overload matching
                INTRINSICS[func_name] = fn;
                return;
            } else if (subj == "overload") {
                // (overload + method) or (overload + [m1 m2 m3])
                std::string op_name = std::get<Atom>(mol.atoms[1]).identifier;
                
                if (std::holds_alternative<Atom>(mol.atoms[2])) {
                    // Single method
                    std::string method = std::get<Atom>(mol.atoms[2]).identifier;
                    overload_registry[op_name].push_back(method);
                } else {
                    // Array of methods
                    Molecule& methods = std::get<Molecule>(mol.atoms[2]);
                    for (size_t i = 1; i < methods.atoms.size(); i++) {
                        std::string method = std::get<Atom>(methods.atoms[i]).identifier;
                        overload_registry[op_name].push_back(method);
                    }
                }
                return;
            } else if (subj == "extern") {
                // (extern RetType:(FuncName ParamType:name ...))
                // Declares an external C function
                
                Molecule& sig = std::get<Molecule>(mol.atoms[1]);
                std::string return_type = sig.type;
                std::string func_name = std::get<Atom>(sig.atoms[0]).identifier;
                
                // Collect parameter types
                std::vector<std::string> param_types;
                std::vector<llvm::Type*> llvm_param_types;
                
                for (size_t i = 1; i < sig.atoms.size(); i++) {
                    Atom& param = std::get<Atom>(sig.atoms[i]);
                    param_types.push_back(param.type);
                    // For Str params passed to C, use ptr (char*)
                    if (param.type == "Str") {
                        llvm_param_types.push_back(llvm::PointerType::getUnqual(*TheContext));
                    } else if (struct_registry.count(param.type) && struct_registry[param.type].is_extern) {
                        // Extern structs on x86_64: small structs (<=8 bytes) passed as integers
                        // For a 4-byte struct like Color, C ABI uses i32
                        StructDef& sdef = struct_registry[param.type];
                        // Calculate actual size based on field types
                        size_t byte_size = 0;
                        for (const auto& ft : sdef.field_types) {
                            if (ft == "Char") byte_size += 1;
                            else if (ft == "Int") byte_size += 4;
                            else if (ft == "Float") byte_size += 4;
                            else if (ft == "Bool") byte_size += 1;
                            else byte_size += 8; // pointer types
                        }
                        if (byte_size <= 4) {
                            llvm_param_types.push_back(llvm::Type::getInt32Ty(*TheContext));
                        } else if (byte_size <= 8) {
                            llvm_param_types.push_back(llvm::Type::getInt64Ty(*TheContext));
                        } else {
                            // Larger structs passed by pointer
                            llvm_param_types.push_back(llvm::PointerType::getUnqual(*TheContext));
                        }
                    } else {
                        llvm_param_types.push_back(get_llvm_type(param.type));
                    }
                }
                
                // Create function type and declare external function
                llvm::Type* llvm_ret_type = get_llvm_type(return_type);
                llvm::FunctionType* FT = llvm::FunctionType::get(llvm_ret_type, llvm_param_types, false);
                llvm::FunctionCallee extern_func = TheModule->getOrInsertFunction(func_name, FT);
                
                // Register as intrinsic for calling
                std::string fn_return_type = return_type;
                std::vector<std::string> captured_param_types = param_types;
                Function fn(func_name, 
                    [extern_func, captured_param_types, llvm_ret_type](Molecule& call_mol, const std::vector<llvm::Value*>& args) -> IntrinsicResult {
                        (void)call_mol; // unused
                        std::vector<llvm::Value*> call_args;
                        for (size_t i = 0; i < args.size(); i++) {
                            if (captured_param_types[i] == "Str") {
                                // Extract char* from Str struct
                                call_args.push_back(extract_cstring(args[i]));
                            } else if (struct_registry.count(captured_param_types[i]) && 
                                       struct_registry[captured_param_types[i]].is_extern) {
                                // Coerce extern struct to integer for C ABI
                                StructDef& sdef = struct_registry[captured_param_types[i]];
                                size_t byte_size = 0;
                                for (const auto& ft : sdef.field_types) {
                                    if (ft == "Char") byte_size += 1;
                                    else if (ft == "Int") byte_size += 4;
                                    else if (ft == "Float") byte_size += 4;
                                    else if (ft == "Bool") byte_size += 1;
                                    else byte_size += 8;
                                }
                                if (byte_size <= 4) {
                                    // Bitcast struct pointer to i32 pointer, then load
                                    llvm::Value* loaded = Builder->CreateLoad(llvm::Type::getInt32Ty(*TheContext), args[i]);
                                    call_args.push_back(loaded);
                                } else if (byte_size <= 8) {
                                    llvm::Value* loaded = Builder->CreateLoad(llvm::Type::getInt64Ty(*TheContext), args[i]);
                                    call_args.push_back(loaded);
                                } else {
                                    // Pass pointer for large structs
                                    call_args.push_back(args[i]);
                                }
                            } else {
                                // Load primitive value
                                llvm::Type* arg_type = get_llvm_type(captured_param_types[i]);
                                llvm::Value* loaded = Builder->CreateLoad(arg_type, args[i]);
                                call_args.push_back(loaded);
                            }
                        }
                        llvm::Value* result = Builder->CreateCall(extern_func, call_args);
                        if (llvm_ret_type->isVoidTy()) {
                            return {nullptr};
                        }
                        llvm::AllocaInst* alloca = Builder->CreateAlloca(llvm_ret_type);
                        Builder->CreateStore(result, alloca);
                        return {alloca};
                    },
                    [fn_return_type](const std::vector<Particle>&) { return fn_return_type; }
                );
                fn.param_types = param_types;
                INTRINSICS[func_name] = fn;
                return;
            } else if (subj == "struct") {
                // (struct Person:[Str:name Int:age Bool:friend])
                // mol.atoms[1] is typed array molecule with struct name as type
                Molecule& fields_mol = std::get<Molecule>(mol.atoms[1]);
                std::string struct_name = fields_mol.type;
                
                // Skip if already registered by collect_struct_declarations
                if (struct_registry.count(struct_name)) {
                    return;
                }
                
                StructDef def;
                def.name = struct_name;
                
                std::vector<llvm::Type*> llvm_field_types;
                for (size_t i = 1; i < fields_mol.atoms.size(); i++) {
                    Atom& field = std::get<Atom>(fields_mol.atoms[i]);
                    def.field_names.push_back(field.identifier);
                    def.field_types.push_back(field.type);
                    llvm_field_types.push_back(get_llvm_type(field.type));
                }
                
                def.llvm_type = llvm::StructType::create(*TheContext, llvm_field_types, struct_name);
                struct_registry[struct_name] = def;
                return;
            } else if (subj == "extern-struct") {
                // (extern-struct Color [Char:r Char:g Char:b Char:a])
                // Skip if already registered by collect_struct_declarations
                std::string struct_name = std::get<Atom>(mol.atoms[1]).identifier;
                if (struct_registry.count(struct_name)) {
                    return;  // Already registered
                }
                
                Molecule& fields_mol = std::get<Molecule>(mol.atoms[2]);
                
                StructDef def;
                def.name = struct_name;
                def.is_extern = true;
                
                std::vector<llvm::Type*> llvm_field_types;
                for (size_t i = 1; i < fields_mol.atoms.size(); i++) {
                    Atom& field = std::get<Atom>(fields_mol.atoms[i]);
                    def.field_names.push_back(field.identifier);
                    def.field_types.push_back(field.type);
                    llvm_field_types.push_back(get_llvm_type(field.type));
                }
                
                def.llvm_type = llvm::StructType::create(*TheContext, llvm_field_types, struct_name);
                struct_registry[struct_name] = def;
                return;
            } else if (subj == "array" && struct_registry.count(mol.type)) {
                // Struct literal: Person:["bob" 67 true]
                StructDef& def = struct_registry[mol.type];
                
                // Compile all field values first
                for (size_t i = 1; i < mol.atoms.size(); i++) {
                    compile(mol.atoms[i]);
                }
                
                // Allocate struct
                llvm::AllocaInst* struct_alloc = Builder->CreateAlloca(def.llvm_type, nullptr, "struct_instance");
                
                // Store each field
                for (size_t i = 1; i < mol.atoms.size(); i++) {
                    llvm::Value* val_ptr = get_stored_in(mol.atoms[i]);
                    llvm::Type* field_llvm_type = get_llvm_type(def.field_types[i-1]);
                    llvm::Value* val = Builder->CreateLoad(field_llvm_type, val_ptr);
                    llvm::Value* field_ptr = Builder->CreateStructGEP(def.llvm_type, struct_alloc, i-1);
                    Builder->CreateStore(val, field_ptr);
                }
                
                if (def.is_extern) {
                    // For extern structs, just store the alloca directly (pass by value later)
                    mol.stored_in = struct_alloc;
                } else {
                    // For internal structs, return pointer to struct (pointer-to-pointer pattern)
                    llvm::AllocaInst* result_ptr = Builder->CreateAlloca(llvm::PointerType::getUnqual(*TheContext), nullptr, "struct_ref");
                    Builder->CreateStore(struct_alloc, result_ptr);
                    mol.stored_in = result_ptr;
                }
                return;
            } else if (subj == "array") {
                
            }
        }

        for (size_t i = 1; i < mol.atoms.size(); i++) {
            auto& child = mol.atoms[i];
            if (!get_stored_in(child)) {
                compile(child);
            }
        }
        
        evaluate(mol);

    } else {
        Atom& atom = std::get<Atom>(p);
        evaluate(atom);
    }
}

