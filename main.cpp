#include <bits/stdc++.h>

#include "Lexer.hpp"
#include "Token.hpp"

using namespace std;

int main() {
    const string filename = "../test.py";

    // Open the python file
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Could not open file " << filename << endl;
        return 1;
    }

    // Read the file in as String
    stringstream buffer;
    buffer << file.rdbuf();
    string input = buffer.str();
    file.close();

    // Initialize a Lexer instance and a vector to store all tokens
    Lexer lexer(input);
    vector<Token> tokens;

    Token token;
    do {
        token = lexer.nextToken();
        tokens.push_back(token);

        if (token.type != TokenType::TK_EOF) {
            if (token.category != TokenCategory::PUNCTUATION && token.category != TokenCategory::OPERATOR) {
                cout << "<" << tokenTypeToString(token.type) << ", \"" << token.lexeme << "\">" << endl;
            } else {
                cout << "<\"" << token.lexeme << "\">" << endl;
            }
        } else {
            cout << "<EOF>" << endl;
        }

    } while (token.type != TokenType::TK_EOF);
    cout << "--------------" << endl;

    // --- Print the Symbol Table ---
    cout << "\n--- Symbol Table ---" << endl;
    const unordered_set<string>& symbols = lexer.getSymbolTable();

    if (symbols.empty()) {
        cout << "(empty)" << endl;
    } else {
        vector<string> sortedSymbols(symbols.begin(), symbols.end());
        sort(sortedSymbols.begin(), sortedSymbols.end());

        for (const string& symbol : sortedSymbols) {
            cout << symbol << endl;
        }
    }
    cout << "--------------------" << endl;

    for (const auto& err : lexer.getErrors()) {
        cout << "Lexical Error at line " << err.line
                  << ": " << err.message << " -> '" << err.lexeme << "'\n";
    }




    return 0;
}
