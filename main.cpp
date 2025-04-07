#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <vector>

#include "Lexer.hpp"
#include "Token.hpp"

int main() {
    const std::string filename = "../test.py";

    // Open the python file
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return 1;
    }

    // Read the file in as String
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string input = buffer.str();

    // Initialize a Lexer instance and a vector to store all tokens
    Lexer lexer(input);
    std::vector<Token> tokens;

    // Lexical analysis token by token
    Token token;
    do {
        token = lexer.nextToken();
        tokens.push_back(token);
    } while (token.type != TokenType::TK_EOF);

    // Print tokens
    for (const auto &t: tokens) {
        if (t.category != TokenCategory::PUNCTUATION && t.category != TokenCategory::OPERATOR) {
            std::cout << "<" << tokenTypeToString(t.type) << ", " << t.lexeme << ">" << std::endl;
        } else {
            std::cout << "<" << t.lexeme << ">" << std::endl;
        }
    }

    return 0;
}
