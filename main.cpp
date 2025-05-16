#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>    // For std::sort
#include <memory>       // For std::unique_ptr

#include "Lexer.hpp"
#include "Token.hpp"    // Ensure this includes tokenTypeToString and TokenCategory
#include "Parser.hpp"   // Include your Parser header
#include "DOTGenerator.hpp" // Include your DOTGenerator header


using namespace std;

int main() {
    const string filename = "../test.py"; // Make sure this path is correct for your setup
    const string dot_filename = "ast.dot"; // Output DOT file name

    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Could not open file " << filename << endl;
        return 1;
    }

    stringstream buffer;
    buffer << file.rdbuf();
    string input = buffer.str();
    file.close();

    cout << "--- Lexing Input ---" << endl;
    Lexer lexer(input);
    // The Parser constructor will drive the lexer
    // No need to manually loop lexer.nextToken() here if Parser does it.

    cout << "--- Parsing ---" << endl;
    Parser parser(lexer); // Parser constructor populates lexer.tokens and its own tokens list

    // --- Print Lexer Tokens (after parser construction has triggered full lexing) ---
    cout << "\n--- Lexer Token Stream (from lexer.tokens) ---" << endl;
    if (lexer.tokens.empty()) {
        cout << "(No tokens found in lexer.tokens or lexer failed early)" << endl;
    }
    for (const auto& t : lexer.tokens) { // Using lexer.tokens as per your Parser constructor
        string typeName = tokenTypeToString(t.type);
        string lexemeVal = t.lexeme;
        // Escape newlines/tabs in lexeme for cleaner printing
        size_t pos = 0;
        while ((pos = lexemeVal.find('\n', pos)) != string::npos) { lexemeVal.replace(pos, 1, "\\n"); pos += 2; }
        pos = 0;
        while ((pos = lexemeVal.find('\t', pos)) != string::npos) { lexemeVal.replace(pos, 1, "\\t"); pos += 2; }

        cout << "<" << typeName;
        // Print lexeme for relevant categories
        if (t.category == TokenCategory::IDENTIFIER ||
            t.category == TokenCategory::NUMBER ||
            t.category == TokenCategory::STRING ||
            t.category == TokenCategory::UNKNOWN ||
            t.type == TokenType::TK_EOF || // Show lexeme for EOF for completeness
            t.category == TokenCategory::OPERATOR || // Show lexeme for operators
            t.category == TokenCategory::PUNCTUATION || // Show lexeme for punctuation
            t.category == TokenCategory::KEYWORD)       // Show lexeme for keywords
        {
            cout << ", \"" << lexemeVal << "\"";
        }
        cout << "> Line: " << t.line << endl;
    }
    cout << "--------------------------------------------------------------------" << endl;


    // --- Print Lexer Errors (if any, collected by Parser or Lexer itself) ---
    // Your Parser copies lexer errors into its own getErrors().
    // We'll print the parser's consolidated error list later.
    // If you want to see *only* lexer errors before parsing, you'd check lexer.getErrors()
    // *before* parser.parse() and *after* the lexer has run.
    // But since Parser's constructor runs the lexer, it's simpler to check Parser's error list.

    shared_ptr<ProgramNode> astRoot = parser.parse(); // This is the main parsing call

    // --- Print Parser and Lexer Errors (collected in parser.getErrors()) ---
    // Accessing public member `getErrors()` directly as per your Parser.cpp
    if (!parser.getErrors().empty()) {
        cout << "\n--- Collected Errors (Lexer & Parser) ---" << endl;
        for (const auto& errMsg : parser.getErrors()) {
            cout << errMsg << endl;
        }
        cout << "-------------------------------------------" << endl;
    }

    if (!astRoot) {
        cout << "\nParsing failed critically: AST root is null." << endl;
        // This case might occur if `parser.parse()` returns nullptr before even trying `parseFile`,
        // e.g., if `tokens` is empty and `hasError()` was already true.
        return 1;
    }

    // Accessing public member `hasError()` directly
    if (!parser.hasError()) {
        cout << "\nParsing deemed successful by parser's error flag. AST generated." << endl;

        cout << "\n--- Generating AST DOT file (" << dot_filename << ") ---" << endl;
        DOTGenerator dotGenerator;
        dotGenerator.generate(astRoot.get(), dot_filename);
        cout << "DOT file generated. Use Graphviz to visualize (e.g., dot -Tpng " << dot_filename << " -o ast.png)" << endl;
        cout << "------------------------------------------" << endl;
    } else {
        cout << "\nParsing completed with errors indicated by parser's error flag." << endl;
        if (astRoot && astRoot->statements.empty() && !parser.getErrors().empty()) {
            cout << "AST root exists but is empty, likely due to parsing errors preventing statement generation." << endl;
        }
        cout << "\n--- Generating AST DOT file (" << dot_filename << ") for potentially partial AST ---" << endl;
        DOTGenerator dotGenerator;
        dotGenerator.generate(astRoot.get(), dot_filename); // Attempt to generate for partial AST
        cout << "DOT file generated. It might represent a partial AST due to errors." << endl;
        cout << "-----------------------------------------------------------------------" << endl;
    }

    // --- Symbol Table from Lexer (if processIdentifierTypes populates it meaningfully) ---
    // This part assumes lexer.getSymbolTable() and lexer.processIdentifierTypes() are relevant
    // for your testing. Often, a more structured symbol table is built by the parser/semantic analyzer.
    cout << "\n--- Lexer's Basic Symbol Table (from processIdentifierTypes) ---" << endl;
    const unordered_map<string, string>& symbols = lexer.getSymbolTable(); // Assuming Lexer has this getter

    if (symbols.empty()) {
        cout << "(empty or not populated by lexer.processIdentifierTypes)" << endl;
    } else {
        // Sort keys for consistent output
        vector<string> sortedSymbols;
        sortedSymbols.reserve(symbols.size());
        for (const auto& kv : symbols) {
            sortedSymbols.push_back(kv.first);
        }
        sort(sortedSymbols.begin(), sortedSymbols.end());

        for (const auto& sym_name : sortedSymbols) {
            cout << sym_name << " : " << symbols.at(sym_name) << endl;
        }
    }
    cout << "-----------------------------------------------------------------" << endl;


    // --- Final check on Lexer's direct error list (if different from parser's) ---
    if (!lexer.getErrors().empty()) {
        bool new_lexer_errors_found = false;
        for(const auto& lex_err : lexer.getErrors()){
            // Check if this lexer error was already reported via parser.getErrors()
            string full_lex_err_msg = "Lexer Error: " + lex_err.message + " on line " + to_string(lex_err.line) + " near '" + lex_err.lexeme + "'";
            bool found_in_parser_list = false;
            for(const auto& p_err : parser.getErrors()){
                if(p_err == full_lex_err_msg){
                    found_in_parser_list = true;
                    break;
                }
            }
            if(!found_in_parser_list){
                if(!new_lexer_errors_found) {
                    cout << "\n--- Additional Lexer Errors (not in parser's list, check logic) ---" << endl;
                    new_lexer_errors_found = true;
                }
                cout << "Lexical Error at line " << lex_err.line
                     << ": " << lex_err.message << " -> '" << lex_err.lexeme << "'\n";
            }
        }
        if(new_lexer_errors_found) cout << "-------------------------------------------------------------------" << endl;
    }


    cout << "\nExecution finished." << endl;
    return 0;
}