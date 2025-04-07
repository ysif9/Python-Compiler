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

    // Initialize a Lexer instance and a vector to store all tokens
    Lexer lexer(input);
    vector<Token> tokens;

    // Lexical analysis token by token
    Token token;
    do {
        token = lexer.nextToken();
        tokens.push_back(token);
    } while (token.type != TokenType::TK_EOF);

    // Print tokens
    for (const auto &t: tokens) {
        if (t.category != TokenCategory::PUNCTUATION && t.category != TokenCategory::OPERATOR) {
            cout << "<" << tokenTypeToString(t.type) << ", " << t.lexeme << ">" << endl;
        } else {
            cout << "<" << t.lexeme << ">" << endl;
        }
    }

    return 0;
}
