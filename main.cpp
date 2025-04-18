#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>

#include "Lexer.hpp"
#include "Token.hpp" // Ensure this includes the provided Token.hpp content

using namespace std;

int main() {
    const string filename = "../test.py"; // Make sure this path is correct for your setup

    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Could not open file " << filename << endl;
        return 1;
    }

    stringstream buffer;
    buffer << file.rdbuf();
    string input = buffer.str();
    file.close();

    Lexer lexer(input);

    // --- Generate all tokens first ---
    Token token;
    do {
        token = lexer.nextToken();
        // Loop continues until EOF token is generated and stored in lexer.tokens
    } while (token.type != TokenType::TK_EOF);

    // --- Print the raw token stream ---
    cout << "--- Raw Token Stream ---" << endl;
    for (const auto& t : lexer.tokens) {
        // Use the tokenTypeToString from Token.hpp for display name
        string typeName = tokenTypeToString(t.type);
        string lexemeVal = t.lexeme;
        // Escape newlines/tabs in lexeme for cleaner printing
        size_t pos = 0;
        while ((pos = lexemeVal.find('\n', pos)) != string::npos) { lexemeVal.replace(pos, 1, "\\n"); pos += 2; }
        pos = 0;
        while ((pos = lexemeVal.find('\t', pos)) != string::npos) { lexemeVal.replace(pos, 1, "\\t"); pos += 2; }

        cout << "<" << typeName; // Print type name
        // Optionally print lexeme for non-punctuation/operators or always
        if (t.category == TokenCategory::IDENTIFIER ||
            t.category == TokenCategory::NUMBER ||
            t.category == TokenCategory::STRING ||
            t.category == TokenCategory::UNKNOWN ||
            t.type == TokenType::TK_EOF) // Show lexeme for these and EOF
        {
            cout << ", \"" << lexemeVal << "\"";
        } else if (t.category == TokenCategory::OPERATOR || t.category == TokenCategory::PUNCTUATION) {
            // For ops/punc, lexeme is often the type name, maybe just show lexeme
            cout << ", \"" << lexemeVal << "\"";
            // Or just show type name: cout << ">";
        }
        cout << "> Line: " << t.line << endl; // Add line number
    }
    cout << "------------------------" << endl;


    // --- Print the Symbol Table with Inferred Types ---
    cout << "\n--- Symbol Table ---" << endl;
    const unordered_map<string, string>& symbols = lexer.getSymbolTable();

    if (symbols.empty()) {
        cout << "(empty)" << endl;
    } else {
        // Sort keys for consistent output
        vector<string> sortedSymbols;
        sortedSymbols.reserve(symbols.size());
        for (const auto& kv : symbols) {
            sortedSymbols.push_back(kv.first);
        }
        sort(sortedSymbols.begin(), sortedSymbols.end());

        // Print sorted table
        for (const auto& sym : sortedSymbols) {
            cout << sym << " : " << symbols.at(sym) << endl;
        }
    }
    cout << "--------------------" << endl;

    for (const auto& err : lexer.getErrors()) {
        cout << "Lexical Error at line " << err.line
                  << ": " << err.message << " -> '" << err.lexeme << "'\n";
    }




    return 0;
}