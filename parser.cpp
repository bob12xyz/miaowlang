#include "parser.hpp"

Molecule lexparse(std::string_view view) {
    auto first_paren = view.find_first_of("([{");
    if (first_paren == std::string_view::npos) {
        return Molecule{};
    }
    
    bool eval = true;
    bool is_block = (view[first_paren] == '{');
    bool is_array = (view[first_paren] == '[');

    view = view.substr(first_paren + 1);

    Molecule molecule({}, eval);

    if (is_block) {
        molecule.add_atom("block");
    } else if (is_array) {
        molecule.add_atom("array");
    }

    bool more_words = true;
    while (more_words) {
        size_t word_begin = view.find_first_not_of(WHITESPACE);

        if (word_begin == std::string::npos) {
            more_words = false;
            break;
        }
        if (view[word_begin] == ')' || view[word_begin] == ']' || view[word_begin] == '}') {
            more_words = false;
        } else if (view[word_begin] == '(' || view[word_begin] == '[' || view[word_begin] == '{') {
            view = view.substr(word_begin);

            int layers = 0;
            size_t word_end = std::string_view::npos;
            for (size_t p = 0; p < view.length(); p++) {
                if (view[p] == '(' || view[p] == '[' || view[p] == '{') {
                    layers++;
                } else if (view [p] == ')' || view[p] == ']' || view[p] == '}') {
                    layers--;
                }

                if (layers == 0) {
                    word_end = p;
                    break;
                }
            }
            if (word_end == std::string_view::npos) {
                break;
            }

            if (view[word_begin] == '(') {
                std::string_view sub_molecule = view.substr(0, word_end + 1);
                
                molecule.add_molecule(lexparse(sub_molecule));
                view = view.substr(word_end+1);
            } else if (view[word_begin] == '[') {
                std::string_view sub_molecule = view.substr(0, word_end + 1);
                
                molecule.add_molecule(lexparse(sub_molecule));
                view = view.substr(word_end+1);
            } else if (view[word_begin] == '{') {
                std::string_view sub_molecule = view.substr(0, word_end + 1);
                
                molecule.add_molecule(lexparse(sub_molecule));
                view = view.substr(word_end+1);
            }
        } else if (view[word_begin] == '\"') {
            int end_quote = view.substr(1).find_first_of("\"");
            molecule.add_atom(std::string(view.substr(0,end_quote+2)));
            view = view.substr(end_quote+3);
        } else {
            // Check for Type:(molecule) syntax - word ending with : followed by (
            size_t rel_end = view.substr(word_begin).find_first_of(WHITESPACE "([{");
            size_t word_end = (rel_end == std::string::npos) ? view.size() : word_begin + rel_end;
            std::string word(view.substr(word_begin, word_end - word_begin));
            
            if (word.back() == ')' || word.back() == ']' || word.back() == '}') {
                more_words = false;
                word.pop_back();
            }
            
            // Handle Type:(molecule) or Type:[array] - type prefix on sub-expression
            if (!word.empty() && word.back() == ':' && word_end < view.size() && (view[word_end] == '(' || view[word_end] == '[')) {
                std::string type_prefix = word.substr(0, word.size() - 1);  // Remove trailing :
                view = view.substr(word_end);  // Start from the ( or [
                
                // Find matching closing paren/bracket
                int layers = 0;
                size_t paren_end = std::string_view::npos;
                for (size_t p = 0; p < view.length(); p++) {
                    if (view[p] == '(' || view[p] == '[' || view[p] == '{') {
                        layers++;
                    } else if (view[p] == ')' || view[p] == ']' || view[p] == '}') {
                        layers--;
                    }
                    if (layers == 0) {
                        paren_end = p;
                        break;
                    }
                }
                if (paren_end != std::string_view::npos) {
                    std::string_view sub_molecule = view.substr(0, paren_end + 1);
                    Molecule* added = molecule.add_molecule(lexparse(sub_molecule));
                    if (added) {
                        added->type = type_prefix;  // Attach type to the molecule
                    }
                    view = view.substr(paren_end + 1);
                }
            } else {
                molecule.add_atom(word);
                if (word_end < view.size()) {
                    view = view.substr(word_end+1);
                } else {
                    more_words = false;
                }
            }
        }
    }

    return molecule;
}
