#include "Lexer.hpp"
#include <cctype>
#include <iostream>
#include <vector>
#include <set>

using namespace std;

Lexer::Lexer(string input)
        : input(std::move(input)), pos(0), line(1), currentIndent(0), atLineStart(true) {
    // Initialize keywords map using TokenType enum from Token.hpp
    keywords = {
            {"if", TokenType::TK_IF}, {"else", TokenType::TK_ELSE}, {"for", TokenType::TK_FOR},
            {"while", TokenType::TK_WHILE}, {"def", TokenType::TK_DEF}, {"return", TokenType::TK_RETURN},
            {"False", TokenType::TK_FALSE}, {"None", TokenType::TK_NONE}, {"True", TokenType::TK_TRUE},
            {"and", TokenType::TK_AND}, {"as", TokenType::TK_AS}, {"assert", TokenType::TK_ASSERT},
            {"async", TokenType::TK_ASYNC}, {"await", TokenType::TK_AWAIT}, {"break", TokenType::TK_BREAK},
            {"class", TokenType::TK_CLASS}, {"continue", TokenType::TK_CONTINUE}, {"del", TokenType::TK_DEL},
            {"elif", TokenType::TK_ELIF}, {"except", TokenType::TK_EXCEPT}, {"finally", TokenType::TK_FINALLY},
            {"from", TokenType::TK_FROM}, {"global", TokenType::TK_GLOBAL}, {"import", TokenType::TK_IMPORT},
            {"in", TokenType::TK_IN}, {"is", TokenType::TK_IS}, {"lambda", TokenType::TK_LAMBDA},
            {"nonlocal", TokenType::TK_NONLOCAL}, {"not", TokenType::TK_NOT}, {"or", TokenType::TK_OR},
            {"pass", TokenType::TK_PASS}, {"raise", TokenType::TK_RAISE}, {"try", TokenType::TK_TRY},
            {"with", TokenType::TK_WITH}, {"yield", TokenType::TK_YIELD},
            // Type keywords from Token.hpp
            {"str", TokenType::TK_STR}, {"int", TokenType::TK_INT}, {"float", TokenType::TK_FLOAT},
            {"complex", TokenType::TK_COMPLEX}, {"list", TokenType::TK_LIST}, {"tuple", TokenType::TK_TUPLE},
            {"range", TokenType::TK_RANGE}, {"dict", TokenType::TK_DICT}, {"set", TokenType::TK_SET},
            {"frozenset", TokenType::TK_FROZENSET}, {"bool", TokenType::TK_BOOL}, {"bytes", TokenType::TK_BYTES},
            {"bytearray", TokenType::TK_BYTEARRAY}, {"memoryview", TokenType::TK_MEMORYVIEW},
            {"NoneType", TokenType::TK_NONETYPE},
    };
}

Token Lexer::nextToken() {
    // For debugging
    static int tokenCount = 0;
    tokenCount++;

    // If we have pending indentation tokens, return them first
    if (!pendingTokens.empty()) {
        Token token = pendingTokens.front();
        pendingTokens.erase(pendingTokens.begin());
        tokens.push_back(token);
        return token;
    }

    skipWhitespaceAndComments();

    // Re-check for pending tokens after processing indentation
    if (!pendingTokens.empty()) {
        Token token = pendingTokens.front();
        pendingTokens.erase(pendingTokens.begin());
        tokens.push_back(token);
        return token;
    }

    if (isAtEnd()) {
        // Before returning EOF, check if we need to emit DEDENT tokens
        if (!indentStack.empty()) {
            while (!indentStack.empty()) {
                currentIndent = indentStack.back();
                indentStack.pop_back();
                pendingTokens.push_back(createToken(TokenType::TK_DEDENT, "DEDENT"));
            }

            Token token = pendingTokens.front();
            pendingTokens.erase(pendingTokens.begin());
            tokens.push_back(token);
            return token;
        }

        // Add a newline before EOF if we're not already at the start of a line
        if (!atLineStart && currentIndent > 0) {
            atLineStart = true;

            // Generate DEDENT tokens to get back to level 0
            while (currentIndent > 0) {
                if (!indentStack.empty()) {
                    currentIndent = indentStack.back();
                    indentStack.pop_back();
                } else {
                    currentIndent = 0;
                }
                pendingTokens.push_back(createToken(TokenType::TK_DEDENT, "DEDENT"));
            }

            if (!pendingTokens.empty()) {
                Token token = pendingTokens.front();
                pendingTokens.erase(pendingTokens.begin());
                tokens.push_back(token);
                return token;
            }
        }
        if (tokens.empty() || tokens.back().type != TokenType::TK_EOF) {
            Token eofToken = createToken(TokenType::TK_EOF, "");
            tokens.push_back(eofToken);
            return eofToken;
        }
        return tokens.back(); // Return existing EOF
    }

    const char currentCharacter = getCurrentCharacter();
    Token token;

    // Check for comments again, in case skipWhitespaceAndComments missed it
    // TODO: Re-check logic of skipWhitespaceAndComments, we might have to return next token every time
    if (currentCharacter == '#') {
        skipComment();
        return nextToken();
    }

    if (isalpha(currentCharacter) || currentCharacter == '_') {
        token = handleIdentifierOrKeyword();
    } else if (isdigit(currentCharacter)) {
        token = handleNumeric();
    } else if (currentCharacter == '"' || currentCharacter == '\'') {
        token = handleString();
    } else {
        token = handleSymbol();
    }

    if (token.type != TokenType::TK_EOF) {
        tokens.push_back(token);
    }
    return token;
}

// --- Helper functions (isAtEnd, getCurrentCharacter, etc.) ---
bool Lexer::isAtEnd() const {
    return pos >= input.size();
}

char Lexer::getCurrentCharacter() const {
    return isAtEnd() ? '\0' : input[pos];
}

char Lexer::advanceToNextCharacter() {
    if (!isAtEnd()) {
        const char c = input[pos];
        pos++;
        return c;
    }
    return '\0';
}

bool Lexer::matchAndAdvance(const char expected) {
    if (isAtEnd() || input[pos] != expected)
        return false;
    pos++;
    return true;
}

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = getCurrentCharacter();

        if (c == ' ' || c == '\t') {
            // Check if we're at the start of a line (for indentation)
            if (atLineStart) {
                processIndentation();
                break;
            }
            // Else skip whitespace in the middle of a line
            advanceToNextCharacter();
        } else if (c == '\n') {
            line++;
            advanceToNextCharacter();
            atLineStart = true; // Mark that we're at the start of a new line
        } else if (c == '\r') {
            advanceToNextCharacter();
        }
        else if (c == '#') {
            skipComment();
        } else {
            // If we're at the start of a line with non-whitespace, process for indentation
            if (atLineStart) {
                processIndentation();
            }
            break;
        }
    }
}

void Lexer::skipComment() {
    while (!isAtEnd() && getCurrentCharacter() != '\n') {
        advanceToNextCharacter();
    }
    // After a comment, check for updating new line
    if (!isAtEnd() && getCurrentCharacter() == '\n') {
        line++;
        advanceToNextCharacter();
        atLineStart = true;
    }
}

void Lexer::processIndentation() {
    int spaces = 0;

    // Count spaces or tabs at the beginning of the line
    while (!isAtEnd() && (getCurrentCharacter() == ' ' || getCurrentCharacter() == '\t')) {
        const char c = getCurrentCharacter();
        spaces += (c == '\t') ? 8 : 1;  // A tab is equivalent to 8 spaces in Python
        advanceToNextCharacter();
    }

    // If a line is empty or a comment, ignore indentation
    if (isAtEnd() || getCurrentCharacter() == '\n' || getCurrentCharacter() == '#') {
        return;
    }

    atLineStart = false;

    // Compare with the current indentation level
    if (spaces > currentIndent) {
        // Indent
        indentStack.push_back(currentIndent);
        currentIndent = spaces;
        pendingTokens.push_back(createToken(TokenType::TK_INDENT, "INDENT"));
    } else if (spaces < currentIndent) {
        // Dedent
        while (!indentStack.empty() && spaces < currentIndent) {
            currentIndent = indentStack.back();
            indentStack.pop_back();
            pendingTokens.push_back(createToken(TokenType::TK_DEDENT, "DEDENT"));
        }

        // Ensure indentation is consistent
        if (spaces != currentIndent) {
            // TODO: handle inconsistent indentation (error handling)
            // For now we just adjust to the current indentation
            currentIndent = spaces;
        }
    }
}

Token Lexer::handleIdentifierOrKeyword() {
    const size_t start = pos;
    while (!isAtEnd() && (isalnum(getCurrentCharacter()) || getCurrentCharacter() == '_')) {
        advanceToNextCharacter();
    }
    const string text = input.substr(start, pos - start);

    // Check if it's a keyword (including type keywords)
    auto keyword_it = keywords.find(text);
    if (keyword_it != keywords.end()) {
        return createToken(keyword_it->second, text); // Return specific keyword/type token
    } else {
        // It's an identifier
        return createToken(TokenType::TK_IDENTIFIER, text);
    }
}

Token Lexer::handleNumeric() {
    const size_t start = pos;
    bool isFloat = false;
    while (!isAtEnd() && isdigit(getCurrentCharacter())) {
        advanceToNextCharacter();
    }

    // Handle floating point
    if (!isAtEnd() && getCurrentCharacter() == '.') {
        if (pos + 1 < input.size() && isdigit(input[pos + 1])) {
            isFloat = true;
            advanceToNextCharacter(); // Consume '.'
            while (!isAtEnd() && isdigit(getCurrentCharacter())) {
                advanceToNextCharacter();
            }
        }
        // Else: it's an integer followed by '.', don't consume '.'
    }

    // Handle scientific notation
    if (!isAtEnd() && (getCurrentCharacter() == 'e' || getCurrentCharacter() == 'E')) {
        if (pos + 1 < input.size()) {
            char nextChar = input[pos+1];
            if (isdigit(nextChar) || ((nextChar == '+' || nextChar == '-') && pos + 2 < input.size() && isdigit(input[pos+2]))) {
                isFloat = true; // Scientific notation implies float
                advanceToNextCharacter(); // Consume 'e' or 'E'
                if (input[pos] == '+' || input[pos] == '-') {
                    advanceToNextCharacter(); // Consume sign
                }
                while (!isAtEnd() && isdigit(getCurrentCharacter())) {
                    advanceToNextCharacter();
                }
            }
        }
    }

    // Handle complex numbers AFTER potential float part
    if (!isAtEnd() && getCurrentCharacter() == 'j') {
        advanceToNextCharacter(); // Consume 'j'
        const string text = input.substr(start, pos - start);
        return createToken(TokenType::TK_COMPLEX, text); // Return specific complex token
    }

    // If not complex, return TK_NUMBER for both int and float
    const string text = input.substr(start, pos - start);
    // Although we detected float, the required TokenType is TK_NUMBER
    return createToken(TokenType::TK_NUMBER, text);
}


Token Lexer::handleString() {
    bool isBytes = false;
    size_t prefix_len = 0;
    if (!isAtEnd() && (getCurrentCharacter() == 'b' || getCurrentCharacter() == 'B')) {
        if (pos + 1 < input.size() && (input[pos+1] == '\'' || input[pos+1] == '"')) {
            isBytes = true;
            advanceToNextCharacter(); // Consume 'b' or 'B'
            prefix_len = 1;
        }
    }
    // Could add 'r', 'f', 'u' handling here if needed, but they usually affect parsing/value, not base type

    const char quote = getCurrentCharacter();
    if (quote != '\'' && quote != '"') {
        // Should not happen if called correctly
        return createToken(TokenType::TK_UNKNOWN, string(1, quote));
    }
    advanceToNextCharacter(); // Consume opening quote
    const size_t start = pos; // Start of string content

    while (!isAtEnd()) {
        char c = getCurrentCharacter();
        if (c == quote) {
            break; // Found closing quote
        }
        if (c == '\\' && pos + 1 < input.size()) {
            advanceToNextCharacter(); // Skip escaped char
        }
        advanceToNextCharacter(); // Consume content char
    }

    if (isAtEnd()) {
        // Unterminated string
        const string text = input.substr(start - 1 - prefix_len, pos - (start - 1 - prefix_len));
        cerr << "Error: Unterminated string literal starting at line " << line << " text: " << text << endl;
        return createToken(TokenType::TK_UNKNOWN, text);
    }

    const string content = input.substr(start, pos - start);
    advanceToNextCharacter(); // Consume closing quote

    // Return TK_BYTES or TK_STRING based on prefix
    return createToken(isBytes ? TokenType::TK_BYTES : TokenType::TK_STRING, content);
}


Token Lexer::handleSymbol() {
    const char currentCharacter = getCurrentCharacter();
    switch (currentCharacter) {
        // Single character punctuation
        case '(': advanceToNextCharacter(); return createToken(TokenType::TK_LPAREN, "(");
        case ')': advanceToNextCharacter(); return createToken(TokenType::TK_RPAREN, ")");
        case '[': advanceToNextCharacter(); return createToken(TokenType::TK_LBRACKET, "[");
        case ']': advanceToNextCharacter(); return createToken(TokenType::TK_RBRACKET, "]");
        case '{': advanceToNextCharacter(); return createToken(TokenType::TK_LBRACE, "{");
        case '}': advanceToNextCharacter(); return createToken(TokenType::TK_RBRACE, "}");
        case ',': advanceToNextCharacter(); return createToken(TokenType::TK_COMMA, ",");
        case ';': advanceToNextCharacter(); return createToken(TokenType::TK_SEMICOLON, ";");
        case '.': advanceToNextCharacter(); return createToken(TokenType::TK_PERIOD, ".");
        case '~': advanceToNextCharacter(); return createToken(TokenType::TK_BIT_NOT, "~");

            // Potential multi-char operators/punctuation
        case ':':
            advanceToNextCharacter();
            if (matchAndAdvance('=')) {
                return createToken(TokenType::TK_WALNUT, ":="); // :=
            }
            return createToken(TokenType::TK_COLON, ":");
        case '-':
            advanceToNextCharacter();
            if (matchAndAdvance('>')) {
                return createToken(TokenType::TK_FUNC_RETURN_TYPE, "->"); // ->
            }
            if (matchAndAdvance('=')) {
                return createToken(TokenType::TK_MINUS_ASSIGN, "-="); // -=
            }
            return createToken(TokenType::TK_MINUS, "-");
        case '+': return operatorToken(TokenType::TK_PLUS, TokenType::TK_PLUS_ASSIGN, '+'); // + or +=
        case '*':
            advanceToNextCharacter();
            if (matchAndAdvance('*')) {
                if (matchAndAdvance('=')) {
                    return createToken(TokenType::TK_POWER_ASSIGN, "**="); // **=
                }
                return createToken(TokenType::TK_POWER, "**"); // **
            }
            if (matchAndAdvance('=')) {
                return createToken(TokenType::TK_MULTIPLY_ASSIGN, "*="); // *=
            }
            return createToken(TokenType::TK_MULTIPLY, "*"); // *
        case '/':
            advanceToNextCharacter();
            if (matchAndAdvance('/')) {
                if (matchAndAdvance('=')) {
                    return createToken(TokenType::TK_FLOORDIV_ASSIGN, "//="); // //=
                }
                return createToken(TokenType::TK_FLOORDIV, "//"); // //
            }
            if (matchAndAdvance('=')) {
                return createToken(TokenType::TK_DIVIDE_ASSIGN, "/="); // /=
            }
            return createToken(TokenType::TK_DIVIDE, "/"); // /
        case '%': return operatorToken(TokenType::TK_MOD, TokenType::TK_MOD_ASSIGN, '%'); // % or %=
        case '@':
            advanceToNextCharacter();
            if (matchAndAdvance('=')) {
                // Use TK_IMATMUL for @= as defined in the provided Token.hpp
                return createToken(TokenType::TK_IMATMUL, "@=");
            }
            return createToken(TokenType::TK_MATMUL, "@"); // @
        case '&': return operatorToken(TokenType::TK_BIT_AND, TokenType::TK_BIT_AND_ASSIGN, '&'); // & or &=
        case '|': return operatorToken(TokenType::TK_BIT_OR, TokenType::TK_BIT_OR_ASSIGN, '|'); // | or |=
        case '^': return operatorToken(TokenType::TK_BIT_XOR, TokenType::TK_BIT_XOR_ASSIGN, '^'); // ^ or ^=
        case '=':
            advanceToNextCharacter();
            if (matchAndAdvance('=')) {
                return createToken(TokenType::TK_EQUAL, "=="); // ==
            }
            return createToken(TokenType::TK_ASSIGN, "="); // =
        case '!':
            advanceToNextCharacter();
            if (matchAndAdvance('=')) {
                return createToken(TokenType::TK_NOT_EQUAL, "!="); // !=
            }
            // '!' alone is not a standard Python operator
            return createToken(TokenType::TK_UNKNOWN, "!");
        case '>':
            advanceToNextCharacter();
            if (matchAndAdvance('=')) {
                return createToken(TokenType::TK_GREATER_EQUAL, ">="); // >=
            }
            if (matchAndAdvance('>')) {
                if (matchAndAdvance('=')) {
                    return createToken(TokenType::TK_BIT_RIGHT_SHIFT_ASSIGN, ">>="); // >>=
                }
                return createToken(TokenType::TK_BIT_RIGHT_SHIFT, ">>"); // >>
            }
            return createToken(TokenType::TK_GREATER, ">"); // >
        case '<':
            advanceToNextCharacter();
            if (matchAndAdvance('=')) {
                return createToken(TokenType::TK_LESS_EQUAL, "<="); // <=
            }
            if (matchAndAdvance('<')) {
                if (matchAndAdvance('=')) {
                    return createToken(TokenType::TK_BIT_LEFT_SHIFT_ASSIGN, "<<="); // <<=
                }
                return createToken(TokenType::TK_BIT_LEFT_SHIFT, "<<"); // <<
            }
            return createToken(TokenType::TK_LESS, "<"); // <

        default:
            // Unknown single character
            advanceToNextCharacter();
            return createToken(TokenType::TK_UNKNOWN, string(1, currentCharacter));
    }
}

// Creates a token using the provided type and text, automatically determining category
Token Lexer::createToken(const TokenType type, const string &text) const {
    return Token{
            type,
            text,
            line,
            getTokenCategory(type) // Use the category function from Token.hpp
    };
}

// Helper for single-character operators that might be part of an assignment operator (e.g., +, +=)
Token Lexer::operatorToken(const TokenType simpleType, const TokenType assignType, const char opChar) {
    advanceToNextCharacter(); // Consume the operator char itself
    if (matchAndAdvance('=')) { // Check for following '='
        string opStr;
        opStr.push_back(opChar);
        opStr.push_back('=');
        return createToken(assignType, opStr); // Return the assignment operator token
    }
    // No '=', just the simple operator
    string opStr;
    opStr.push_back(opChar);
    return createToken(simpleType, opStr); // Return the simple operator token
}

// Get the symbol table (const reference)
const unordered_map<string, string>& Lexer::getSymbolTable() {
    return symbolTable;
}

// Process identifier types AFTER all tokens are generated
// Simplified version focusing on assignments and basic hints
void Lexer::processIdentifierTypes() {
    symbolTable.clear(); // Start fresh
    string currentClass; // Track current class context for 'self'

    size_t i = 0;
    while (i < tokens.size() && tokens[i].type != TokenType::TK_EOF) {
        Token currentToken = tokens[i];

        // --- Class Definition ---
        if (currentToken.type == TokenType::TK_CLASS && i + 1 < tokens.size() && tokens[i + 1].type == TokenType::TK_IDENTIFIER) {
            currentClass = tokens[i + 1].lexeme;
            symbolTable[currentClass] = "type"; // Class name represents a type
            i += 2; // Skip 'class' and identifier
            // Basic skipping of potential inheritance (...) and ':'
            if (i < tokens.size() && tokens[i].type == TokenType::TK_LPAREN) {
                int paren_depth = 1; i++;
                while(i < tokens.size() && paren_depth > 0) {
                    if (tokens[i].type == TokenType::TK_LPAREN) paren_depth++;
                    else if (tokens[i].type == TokenType::TK_RPAREN) paren_depth--;
                    i++;
                }
            }
            if (i < tokens.size() && tokens[i].type == TokenType::TK_COLON) i++;
            continue;
        }

        // --- Function Definition (for 'self' and params) ---
        if (currentToken.type == TokenType::TK_DEF && i + 1 < tokens.size() && tokens[i + 1].type == TokenType::TK_IDENTIFIER) {
            string funcName = tokens[i+1].lexeme;
            symbolTable[funcName] = "function"; // Mark the function name
            i += 2; // Skip 'def' and funcName
            if (i < tokens.size() && tokens[i].type == TokenType::TK_LPAREN) {
                i++; // Skip '('
                bool firstParam = true;
                while(i < tokens.size() && tokens[i].type != TokenType::TK_RPAREN) {
                    if (tokens[i].type == TokenType::TK_IDENTIFIER) {
                        string paramName = tokens[i].lexeme;
                        if (firstParam && !currentClass.empty() && paramName == "self") {
                            symbolTable["self"] = currentClass; // Infer 'self' type
                        } else if (!symbolTable.count(paramName)) { // Don't overwrite 'self'
                            symbolTable[paramName] = "unknown"; // Default param type
                        }
                        i++; // Skip identifier
                        // Basic type hint check (simple type keyword or identifier)
                        if (i < tokens.size() && tokens[i].type == TokenType::TK_COLON) {
                            i++; // Skip ':'
                            if(i < tokens.size()) {
                                TokenType hintType = tokens[i].type;
                                // Check if it's a type keyword defined in Token.hpp
                                if (hintType >= TokenType::TK_STR && hintType <= TokenType::TK_NONETYPE) {
                                    symbolTable[paramName] = tokens[i].lexeme; // Use 'int', 'str', etc.
                                    i++;
                                } else if (hintType == TokenType::TK_IDENTIFIER) { // Could be custom class
                                    symbolTable[paramName] = tokens[i].lexeme;
                                    i++;
                                } else { // Skip complex hints
                                    while(i < tokens.size() && tokens[i].type != TokenType::TK_COMMA && tokens[i].type != TokenType::TK_RPAREN && tokens[i].type != TokenType::TK_ASSIGN) { i++; }
                                }
                            }
                        }
                        // Basic default value check (just to advance index)
                        if (i < tokens.size() && tokens[i].type == TokenType::TK_ASSIGN) {
                            i++; // Skip '='
                            if (i < tokens.size()) {
                                size_t valIdx = i;
                                string inferredDefault = inferType(valIdx); // Infer type of default
                                i = valIdx; // Advance main index past default value
                                // Optionally update type if it was unknown
                                if (symbolTable[paramName] == "unknown" && inferredDefault != "unknown") {
                                    symbolTable[paramName] = inferredDefault;
                                }
                            }
                        }
                    } else { i++; } // Skip other tokens like ',', '*', etc.

                    firstParam = false;
                    if (i < tokens.size() && tokens[i].type == TokenType::TK_COMMA) { i++; firstParam = true; }
                }
                if (i < tokens.size() && tokens[i].type == TokenType::TK_RPAREN) i++; // Skip ')'
                // Skip return type hint '->' and the type itself
                if (i < tokens.size() && tokens[i].type == TokenType::TK_FUNC_RETURN_TYPE) {
                    i++; // Skip '->'
                    while(i < tokens.size() && tokens[i].type != TokenType::TK_COLON) { i++; } // Skip until ':'
                }
                if (i < tokens.size() && tokens[i].type == TokenType::TK_COLON) i++; // Skip ':'
            }
            continue;
        }

        // --- Assignment: identifier = value ---
        if (currentToken.type == TokenType::TK_IDENTIFIER && i + 1 < tokens.size() && tokens[i + 1].type == TokenType::TK_ASSIGN)
        {
            string identifier = currentToken.lexeme;
            // Avoid overwriting 'self' type if already set
            if (identifier == "self" && symbolTable.count("self") && symbolTable["self"] != "unknown") {
                i = i + 2; // Skip identifier and '='
                if (i < tokens.size()) { inferType(i); } // Skip value by inferring its type to advance index
                continue;
            }

            size_t valueIndex = i + 2; // Index of token after '='
            if (valueIndex < tokens.size()) {
                string inferred = inferType(valueIndex); // Infer type, advances valueIndex
                symbolTable[identifier] = inferred;
                i = valueIndex; // Update main loop index
                continue; // Skip normal i++
            } else {
                i += 2; // Skip identifier and '=', value missing
                continue;
            }
        }

        // --- Type Hinted Variable: identifier : type [= value] ---
        if (currentToken.type == TokenType::TK_IDENTIFIER && i + 1 < tokens.size() && tokens[i + 1].type == TokenType::TK_COLON)
        {
            string identifier = currentToken.lexeme;
            size_t typeIndex = i + 2;
            string typeName = "unknown";

            if (typeIndex < tokens.size()) {
                Token typeToken = tokens[typeIndex];
                // Check if it's a type keyword or identifier hint
                if (typeToken.type >= TokenType::TK_STR && typeToken.type <= TokenType::TK_NONETYPE) {
                    typeName = typeToken.lexeme;
                    typeIndex++;
                } else if (typeToken.type == TokenType::TK_IDENTIFIER) {
                    typeName = typeToken.lexeme; // Custom type
                    typeIndex++;
                } else {
                    // Skip complex hints like list[int] - just advance past the type part
                    // Basic skip: assume type hint ends before '=' or newline (simplification)
                    while (typeIndex < tokens.size() && tokens[typeIndex].type != TokenType::TK_ASSIGN && tokens[typeIndex].type != TokenType::TK_SEMICOLON /* add other statement terminators if needed */ ) {
                        if (tokens[typeIndex].line != currentToken.line) break; // Stop at newline
                        typeIndex++;
                    }
                    typeName = "complex_hint"; // Mark as complex/unparsed
                }

                // Update symbol table if not already known
                if (!symbolTable.count(identifier) || symbolTable[identifier] == "unknown") {
                    symbolTable[identifier] = typeName;
                }

                i = typeIndex; // Update main loop index past the type hint

                // Check for optional assignment after hint
                if (i < tokens.size() && tokens[i].type == TokenType::TK_ASSIGN) {
                    i++; // Skip '='
                    if (i < tokens.size()) {
                        inferType(i); // Infer value type mainly to advance index correctly
                        continue; // Skip normal i++
                    }
                } else {
                    // No assignment after hint, just continue
                    continue; // Skip normal i++
                }
            } else {
                i += 2; // Skip identifier and ':', type missing
                continue;
            }
        }

        // If none of the above patterns matched, just move to the next token
        i++;
    }
}


// Infer type from the token(s) starting at 'index'. Advances 'index'.
std::string Lexer::inferType(size_t& index) {
    if (index >= tokens.size() || tokens[index].type == TokenType::TK_EOF) {
        return "unknown";
    }

    Token& token = tokens[index];
    std::string inferred_type = "unknown";

    switch (token.type) {
        case TokenType::TK_NUMBER:
            // Check lexeme to differentiate int/float for TK_NUMBER
            if (token.lexeme.find('.') != std::string::npos ||
                token.lexeme.find('e') != std::string::npos ||
                token.lexeme.find('E') != std::string::npos) {
                inferred_type = "float";
            } else {
                inferred_type = "int";
            }
            index++;
            break;
        case TokenType::TK_COMPLEX: // Specific token for complex literals
            inferred_type = "complex";
            index++;
            break;
        case TokenType::TK_STRING: // Normal string literal
            inferred_type = "str";
            index++;
            break;
        case TokenType::TK_BYTES: // Bytes literal
            inferred_type = "bytes";
            index++;
            break;
        case TokenType::TK_TRUE:
        case TokenType::TK_FALSE:
            inferred_type = "bool";
            index++;
            break;
        case TokenType::TK_NONE:
            inferred_type = "NoneType";
            index++;
            break;
        case TokenType::TK_LBRACKET: // Start of list literal [...]
            inferred_type = inferListType(index); // Advances index past ']'
            break;
        case TokenType::TK_LPAREN: // Start of tuple literal (...)
            inferred_type = inferTupleType(index); // Advances index past ')'
            break;
        case TokenType::TK_LBRACE: // Start of dict/set literal {...}
            inferred_type = inferDictOrSetType(index); // Advances index past '}'
            break;
        case TokenType::TK_IDENTIFIER:
            // If it's a known variable, use its type. Otherwise, unknown.
            // Could be a function call too - difficult to know return type here.
            if (symbolTable.count(token.lexeme)) {
                inferred_type = symbolTable[token.lexeme];
            } else {
                inferred_type = "unknown"; // Treat as unknown or potential function call
            }
            index++; // Consume identifier
            // Basic handling for function call: skip (...)
            if (index < tokens.size() && tokens[index].type == TokenType::TK_LPAREN) {
                int depth = 1; index++;
                while(index < tokens.size() && depth > 0) {
                    if(tokens[index].type == TokenType::TK_LPAREN) depth++;
                    else if(tokens[index].type == TokenType::TK_RPAREN) depth--;
                    index++;
                }
                // Type remains as initially inferred (e.g., 'function' or 'unknown')
            }
            break;

            // Type keywords used as values (e.g., x = int)
        case TokenType::TK_INT: case TokenType::TK_STR: case TokenType::TK_FLOAT: case TokenType::TK_BOOL:
        case TokenType::TK_LIST: case TokenType::TK_TUPLE: case TokenType::TK_DICT: case TokenType::TK_SET:
            // ... other type keywords
            inferred_type = "type"; // The value is a type object itself
            index++;
            break;

        default:
            // Other tokens (operators, punctuation, non-type keywords) don't represent simple data types
            inferred_type = "unknown";
            index++; // Consume the token
            break;
    }
    return inferred_type;
}

// Infer list type: list[...]
std::string Lexer::inferListType(size_t& index) {
    index++; // Consume '['
    std::vector<std::string> elementTypes;
    bool firstElement = true;

    while (index < tokens.size() && tokens[index].type != TokenType::TK_RBRACKET) {
        if (!firstElement) {
            if (index < tokens.size() && tokens[index].type == TokenType::TK_COMMA) {
                index++; // Consume ','
                if (index >= tokens.size() || tokens[index].type == TokenType::TK_RBRACKET) break; // Trailing comma
            } else {
                cerr << "Syntax Error: Expected ',' or ']' in list at line " << (index < tokens.size() ? tokens[index].line : -1) << endl;
                while (index < tokens.size() && tokens[index].type != TokenType::TK_RBRACKET) { index++; }
                break;
            }
        }
        firstElement = false;

        if (index < tokens.size() && tokens[index].type != TokenType::TK_RBRACKET) {
            elementTypes.push_back(inferType(index)); // Advances index past element
        } else if (index >= tokens.size()) {
            cerr << "Syntax Error: Unexpected end of input within list literal." << endl;
            break;
        }
    }

    if (index < tokens.size() && tokens[index].type == TokenType::TK_RBRACKET) {
        index++; // Consume ']'
    } else if (index >= tokens.size()){
        cerr << "Syntax Error: Unexpected end of input, expected ']' for list literal." << endl;
    } else {
        cerr << "Syntax Error: Expected ']' to close list at line " << tokens[index].line << endl;
    }

    return "list[" + combineTypes(elementTypes) + "]";
}

// Infer tuple type: tuple(...)
std::string Lexer::inferTupleType(size_t& index) {
    index++; // Consume '('
    std::vector<std::string> elementTypes;
    bool firstElement = true;
    bool trailingComma = false; // Needed to distinguish (elem) from (elem,)

    while (index < tokens.size() && tokens[index].type != TokenType::TK_RPAREN) {
        trailingComma = false; // Reset before processing element or comma
        if (!firstElement) {
            if (index < tokens.size() && tokens[index].type == TokenType::TK_COMMA) {
                index++; // Consume ','
                trailingComma = true;
                if (index >= tokens.size() || tokens[index].type == TokenType::TK_RPAREN) break; // Trailing comma case
            } else {
                cerr << "Syntax Error: Expected ',' or ')' in tuple at line " << (index < tokens.size() ? tokens[index].line : -1) << endl;
                while (index < tokens.size() && tokens[index].type != TokenType::TK_RPAREN) { index++; }
                break;
            }
        }
        firstElement = false;

        if (index < tokens.size() && tokens[index].type != TokenType::TK_RPAREN) {
            elementTypes.push_back(inferType(index)); // Advances index past element
        } else if (index >= tokens.size()) {
            cerr << "Syntax Error: Unexpected end of input within tuple literal." << endl;
            break;
        }
    }

    if (index < tokens.size() && tokens[index].type == TokenType::TK_RPAREN) {
        index++; // Consume ')'
    } else if (index >= tokens.size()){
        cerr << "Syntax Error: Unexpected end of input, expected ')' for tuple literal." << endl;
    } else {
        cerr << "Syntax Error: Expected ')' to close tuple at line " << tokens[index].line << endl;
    }

    // Special case: single element tuple `(elem,)` needs the comma
    // If only one element was found AND a trailing comma was consumed right before ')', it's a tuple.
    // If only one element and NO trailing comma, it's just parentheses for precedence, not a tuple.
    // However, since we are called ONLY when a '(' literal is found, we treat `(x)` as `tuple[type(x)]` here.
    // Python's type system might disagree, but for literal inference, this seems reasonable.
    // Let `combineTypes` handle the actual content representation.
    if (elementTypes.empty()) return "tuple[]"; // Handle () empty tuple
    return "tuple[" + combineTypes(elementTypes) + "]";

}

// Infer dict or set type: {...}
std::string Lexer::inferDictOrSetType(size_t& index) {
    index++; // Consume '{'
    std::vector<std::string> keyTypes, valueTypes, elementTypes;
    bool isDict = false, isSet = false, first = true, determined = false;

    // Handle empty literal {} -> dict
    if (index < tokens.size() && tokens[index].type == TokenType::TK_RBRACE) {
        index++; // Consume '}'
        return "dict[Any, Any]"; // Python defaults {} to empty dict
    }

    while (index < tokens.size() && tokens[index].type != TokenType::TK_RBRACE) {
        if (!first) {
            if (index < tokens.size() && tokens[index].type == TokenType::TK_COMMA) {
                index++; // Consume ','
                if (index >= tokens.size() || tokens[index].type == TokenType::TK_RBRACE) break; // Trailing comma
            } else {
                cerr << "Syntax Error: Expected ',' or '}' in dict/set at line " << (index < tokens.size() ? tokens[index].line : -1) << endl;
                while (index < tokens.size() && tokens[index].type != TokenType::TK_RBRACE) { index++; }
                break;
            }
        }
        first = false;

        // Peek ahead for ':' after the first element/key to determine dict vs set
        size_t peekIndex = index;
        if (peekIndex >= tokens.size() || tokens[peekIndex].type == TokenType::TK_RBRACE) break; // Empty after comma

        string tempType = inferType(peekIndex); // Infer type without advancing main index
        bool colonFollows = (peekIndex < tokens.size() && tokens[peekIndex].type == TokenType::TK_COLON);

        if (!determined) {
            isDict = colonFollows;
            isSet = !colonFollows;
            determined = true;
        } else { // Check consistency
            if ((isDict && !colonFollows) || (isSet && colonFollows)) {
                cerr << "Syntax Error: Mixing dict key-value pairs and set elements at line " << tokens[index].line << endl;
                while (index < tokens.size() && tokens[index].type != TokenType::TK_RBRACE) { index++; }
                break;
            }
        }

        // Process based on determined type
        if (isDict) {
            keyTypes.push_back(inferType(index)); // Consume key, advance main index
            if (index < tokens.size() && tokens[index].type == TokenType::TK_COLON) {
                index++; // Consume ':'
                if (index >= tokens.size() || tokens[index].type == TokenType::TK_RBRACE || tokens[index].type == TokenType::TK_COMMA ) {
                    cerr << "Syntax Error: Expected value after ':' in dict at line " << (index > 0 ? tokens[index-1].line : 0) << endl;
                    while (index < tokens.size() && tokens[index].type != TokenType::TK_RBRACE) { index++; }
                    break;
                }
                valueTypes.push_back(inferType(index)); // Consume value, advance main index
            } else {
                cerr << "Syntax Error: Expected ':' after key in dict at line " << (index > 0 ? tokens[index-1].line : 0) << endl;
                while (index < tokens.size() && tokens[index].type != TokenType::TK_RBRACE) { index++; }
                break;
            }
        } else { // isSet
            elementTypes.push_back(inferType(index)); // Consume element, advance main index
        }
    }

    if (index < tokens.size() && tokens[index].type == TokenType::TK_RBRACE) {
        index++; // Consume '}'
    } else if (index >= tokens.size()){
        cerr << "Syntax Error: Unexpected end of input, expected '}' for dict/set literal." << endl;
    } else {
        cerr << "Syntax Error: Expected '}' to close dict/set at line " << tokens[index].line << endl;
    }

    if (isDict) {
        return "dict[" + combineTypes(keyTypes) + ", " + combineTypes(valueTypes) + "]";
    } else if (isSet) {
        return "set[" + combineTypes(elementTypes) + "]";
    } else {
        // This case should only be hit for empty {} which is handled above, or after errors.
        return "dict[Any, Any]"; // Default to dict if unsure/error
    }
}

// Combine types found within a collection (list, tuple, set, dict key/value)
std::string Lexer::combineTypes(const std::vector<std::string>& types) {
    if (types.empty()) return "Any"; // Represent empty collection or unknown element type

    std::set<std::string> unique_types(types.begin(), types.end());

    // If any element's type is unknown or complex, the combined type is uncertain
    if (unique_types.count("unknown") || unique_types.count("complex_hint") || unique_types.count("function")) {
        return "Any"; // Or "unknown" depending on desired strictness
    }
    if (unique_types.count("Any")) {
        return "Any"; // Propagate Any if present
    }

    if (unique_types.size() == 1) {
        return *unique_types.begin(); // All elements have the same single type
    } else {
        // Multiple distinct known types found. Represent as "Any" for simplicity.
        // A more advanced system might use "Union[type1, type2]".
        return "Any";
    }
}
