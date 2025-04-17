#include <iostream> // Prefer specific includes
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm> // For sort
#include <iomanip>   // For setw, left
#include <unordered_map> // Needed for symbol table type

#include "Lexer.hpp"
#include "Token.hpp" // Includes SymbolInfo

using namespace std;

void analyzeSimpleAssignments(const vector<Token>& tokens, unordered_map<string, SymbolInfo>& symbolTable) {
    for (size_t i = 0; i + 2 < tokens.size(); ++i) {
        const Token& varToken = tokens[i];
        const Token& assignToken = tokens[i + 1];
        const Token& valToken = tokens[i + 2];

        bool isAssignment = (assignToken.type == TokenType::TK_ASSIGN || assignToken.type == TokenType::TK_WALNUT);

        if (varToken.type == TokenType::TK_IDENTIFIER && isAssignment) {
            string varName = varToken.lexeme;

            if (symbolTable.count(varName)) {
                string assignedType = "unknown";
                string assignedValue = "unknown";

                if (valToken.type == TokenType::TK_NUMBER) {
                    assignedType = "int";
                    if (valToken.lexeme.find('.') != string::npos) {
                         assignedType = "float";
                    }
                    assignedValue = valToken.lexeme;
                } else if (valToken.type == TokenType::TK_STRING) {
                    assignedType = "str";
                    assignedValue = valToken.lexeme;
                } else if (valToken.type == TokenType::TK_TRUE || valToken.type == TokenType::TK_FALSE) {
                    assignedType = "bool";
                    assignedValue = valToken.lexeme;
                } else if (valToken.type == TokenType::TK_NONE) {
                    assignedType = "NoneType";
                    assignedValue = valToken.lexeme;
                }

                if (assignedType != "unknown") {
                     symbolTable[varName].type = assignedType;
                     symbolTable[varName].value = assignedValue;

                }
            }
        }
    }
}


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

    unordered_map<string, SymbolInfo>& symbols = lexer.getSymbolTable();


    analyzeSimpleAssignments(tokens, symbols);

    cout << "\n--- Symbol Table ---" << endl;

    if (symbols.empty()) {
        cout << "(empty)" << endl;
    } else {

        vector<string> sortedNames;
        for (const auto& pair : symbols) {
            sortedNames.push_back(pair.first);
        }
        sort(sortedNames.begin(), sortedNames.end());

        cout << left;
        cout << setw(7) << "Index" << " | "
             << setw(15) << "Name" << " | "
             << setw(10) << "Type" << " | "
             << "Value" << endl;
        cout << setfill('-') << setw(7) << "" << "-|-"
             << setw(15) << "" << "-|-"
             << setw(10) << "" << "-|-"
             << setw(20) << "" << setfill(' ') << endl;


        int index = 0;
        for (const string& name : sortedNames) {

            const SymbolInfo& info = symbols.at(name);
            cout << setw(7) << index++ << " | "
                 << setw(15) << info.name << " | "
                 << setw(10) << info.type << " | "
                 << info.value << endl;
        }
    }
    cout << "--------------------------------------" << endl;

    return 0;
}