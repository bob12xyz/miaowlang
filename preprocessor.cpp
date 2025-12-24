#include "preprocessor.hpp"
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>

// Step 1: Remove comments (;)
static std::string remove_comments(const std::string& source) {
    std::string result;
    bool in_string = false;
    
    for (size_t i = 0; i < source.size(); i++) {
        char c = source[i];
        
        if (c == '"') {
            in_string = !in_string;
            result += c;
        } else if (c == ';' && !in_string) {
            // Skip until end of line
            while (i < source.size() && source[i] != '\n') {
                i++;
            }
            // Keep the newline to preserve line numbers
            if (i < source.size()) {
                result += '\n';
            }
        } else {
            result += c;
        }
    }
    return result;
}


static std::string expand_defines(const std::string& source) {
    std::unordered_map<std::string, std::string> defines;
    std::string result;
    std::istringstream stream(source);
    std::string line;
    
    while (std::getline(stream, line)) {
        size_t start = line.find_first_not_of(" \t");
        if (start != std::string::npos && line.substr(start, 7) == "!define") {

            size_t pos = start + 7;

            while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) pos++;

            size_t name_start = pos;
            while (pos < line.size() && line[pos] != ' ' && line[pos] != '\t') pos++;
            std::string name = line.substr(name_start, pos - name_start);

            while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) pos++;

            std::string value = line.substr(pos);
            defines[name] = value;
            result += '\n'; 
        } else {
            result += line + '\n';
        }
    }
    

    std::string expanded;
    bool in_string = false;
    
    for (size_t i = 0; i < result.size(); i++) {
        char c = result[i];
        
        if (c == '"') {
            in_string = !in_string;
            expanded += c;
        } else if (!in_string && (std::isalpha(c) || c == '_')) {
            size_t start = i;
            while (i < result.size() && (std::isalnum(result[i]) || result[i] == '_')) {
                i++;
            }
            std::string ident = result.substr(start, i - start);
            i--;
            
            if (defines.count(ident)) {
                expanded += defines[ident];
            } else {
                expanded += ident;
            }
        } else {
            expanded += c;
        }
    }
    
    return expanded;
}

// Preprocess a single file (comments + defines) in isolation
static std::string preprocess_single(const std::string& source) {
    std::string result = source;
    result = remove_comments(result);
    result = expand_defines(result);
    return result;
}

// Read a file and return its contents
static std::string read_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open import file: " << filename << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Strip the outermost {} block from source, returning just the contents
static std::string strip_outer_block(const std::string& source) {
    // Find the first {
    size_t open_brace = source.find('{');
    if (open_brace == std::string::npos) {
        return source; // No block, return as-is
    }
    
    // Find the matching closing }
    int depth = 0;
    size_t close_brace = std::string::npos;
    bool in_string = false;
    
    for (size_t i = open_brace; i < source.size(); i++) {
        char c = source[i];
        if (c == '"') {
            in_string = !in_string;
        } else if (!in_string) {
            if (c == '{') depth++;
            else if (c == '}') {
                depth--;
                if (depth == 0) {
                    close_brace = i;
                    break;
                }
            }
        }
    }
    
    if (close_brace == std::string::npos) {
        return source; // Unmatched brace, return as-is
    }
    
    // Return content between braces (excluding the braces themselves)
    return source.substr(open_brace + 1, close_brace - open_brace - 1);
}

// Collect all !import directives that appear before the main {} block
static void collect_imports_before_block(const std::string& source, 
                                         std::vector<std::string>& import_names,
                                         std::string& remaining) {
    std::istringstream stream(source);
    std::string line;
    bool found_block = false;
    std::string before_block;
    std::string after_imports;
    
    while (std::getline(stream, line)) {
        size_t start = line.find_first_not_of(" \t");
        
        // Check if this line starts the main block
        if (!found_block && start != std::string::npos && line[start] == '{') {
            found_block = true;
            // Include this line and everything after in remaining
            after_imports += line + '\n';
            while (std::getline(stream, line)) {
                after_imports += line + '\n';
            }
            break;
        }
        
        // Check for !import before block
        if (!found_block && start != std::string::npos && line.substr(start, 7) == "!import") {
            size_t pos = start + 7;
            while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) pos++;
            size_t end = line.find_last_not_of(" \t\r\n");
            std::string import_name = line.substr(pos, end - pos + 1);
            import_names.push_back(import_name);
            before_block += '\n'; // Preserve line numbers
        } else {
            before_block += line + '\n';
        }
    }
    
    remaining = before_block + after_imports;
}

// Process imports: imports before {} get their contents inserted at start of {}
static std::string process_imports(const std::string& source, std::unordered_set<std::string>& imported) {
    // First, collect imports that appear before the main {} block
    std::vector<std::string> early_imports;
    std::string remaining;
    collect_imports_before_block(source, early_imports, remaining);
    
    // Process early imports - strip their {} and collect content
    std::string imported_content;
    for (const std::string& import_name : early_imports) {
        std::string filename = import_name;
        if (filename.size() < 4 || filename.substr(filename.size() - 4) != ".inf") {
            filename += ".inf";
        }
        
        if (imported.count(filename)) {
            continue; // Skip circular imports
        }
        imported.insert(filename);
        
        std::string imported_source = read_file(filename);
        if (!imported_source.empty()) {
            // Recursively process imports in the imported file
            std::string with_imports = process_imports(imported_source, imported);
            // Preprocess in isolation
            std::string preprocessed = preprocess_single(with_imports);
            // Strip the outer {} block
            std::string content = strip_outer_block(preprocessed);
            imported_content += content + '\n';
        }
    }
    
    // Now inject the imported content at the start of the main {} block
    std::string result;
    if (!imported_content.empty()) {
        size_t open_brace = remaining.find('{');
        if (open_brace != std::string::npos) {
            result = remaining.substr(0, open_brace + 1) + '\n' + imported_content + remaining.substr(open_brace + 1);
        } else {
            result = remaining;
        }
    } else {
        result = remaining;
    }
    
    // Now process any !import directives that appear inside the {} block
    std::string final_result;
    std::istringstream stream(result);
    std::string line;
    
    while (std::getline(stream, line)) {
        size_t start = line.find_first_not_of(" \t");
        if (start != std::string::npos && line.substr(start, 7) == "!import") {
            size_t pos = start + 7;
            while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) pos++;
            size_t end = line.find_last_not_of(" \t\r\n");
            std::string import_name = line.substr(pos, end - pos + 1);
            
            std::string filename = import_name;
            if (filename.size() < 4 || filename.substr(filename.size() - 4) != ".inf") {
                filename += ".inf";
            }
            
            if (imported.count(filename)) {
                final_result += '\n';
                continue;
            }
            imported.insert(filename);
            
            std::string imported_source = read_file(filename);
            if (!imported_source.empty()) {
                std::string with_imports = process_imports(imported_source, imported);
                std::string preprocessed = preprocess_single(with_imports);
                // For inline imports, strip outer block too
                std::string content = strip_outer_block(preprocessed);
                final_result += content + '\n';
            }
            final_result += '\n';
        } else {
            final_result += line + '\n';
        }
    }
    
    return final_result;
}

std::string preprocess(const std::string& source) {
    // Step 1: Process imports (each file preprocessed in isolation)
    std::unordered_set<std::string> imported;
    std::string with_imports = process_imports(source, imported);
    
    // Step 2: Preprocess the main file (comments + defines)
    std::string result = preprocess_single(with_imports);
    
    return result;
}
