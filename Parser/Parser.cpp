#include "Parser.hpp"
#include <iostream> // For temporary debugging, remove in production

using namespace std;
// Initialize static EOF token
Token Parser::eof_token = {TokenType::TK_EOF, "", 0, TokenCategory::EOFILE};


Parser::Parser(Lexer& lexer_instance)
        : lexer_ref(lexer_instance), current_pos(0), had_error(false) {
    Token t = lexer_ref.nextToken();
    while(t.type != TokenType::TK_EOF) {
        if (lexer_ref.getErrors().size() > errors_list.size()) { // Propagate lexer errors
            for(size_t i = errors_list.size(); i < lexer_ref.getErrors().size(); ++i) {
                errors_list.push_back("Lexer Error: " + lexer_ref.getErrors()[i].message + " on line " + to_string(lexer_ref.getErrors()[i].line) + " near '" + lexer_ref.getErrors()[i].lexeme + "'");
            }
            had_error = true;
        }
        t = lexer_ref.nextToken();
    }
    // Ensure the final EOF token is added if not already
    if (lexer_ref.tokens.empty() || lexer_ref.tokens.back().type != TokenType::TK_EOF) {
        lexer_ref.tokens.push_back({TokenType::TK_EOF, "", lexer_ref.tokens.empty() ? 1 : lexer_ref.tokens.back().line, TokenCategory::EOFILE});
    }

    lexer_ref.processIdentifierTypes();
    this->tokens = lexer_ref.tokens;

    if (had_error) { // If lexer errors occurred, don't proceed with parsing
        // Optionally, clear tokens to prevent parsing attempts
        this->tokens.clear();
        this->tokens.push_back(eof_token);
    }
}

unique_ptr<ProgramNode> Parser::parse() {
    if (tokens.empty() || (tokens.size() == 1 && tokens[0].type == TokenType::TK_EOF && had_error)) {
        // If only EOF token exists due to lexer error, or no tokens, return empty program
        // reportError might not have a valid token if tokens is empty
        if (!had_error) reportError(eof_token, "No tokens to parse.");
        return make_unique<ProgramNode>(0, vector<unique_ptr<StatementNode>>());
    }
    current_pos = 0;
    // errors_list is not cleared here to preserve lexer errors if any.
    // If parse is called multiple times, caller should handle error state.
    // For a typical compiler, parse is called once.
    return parseFile();
}

// --- Core Helper Methods ---
Token& Parser::peek(int offset) {
    if (current_pos + offset >= tokens.size()) {
        return eof_token;
    }
    return tokens[current_pos + offset];
}

Token& Parser::previous() {
    if (current_pos == 0 || current_pos > tokens.size()) { // Should not happen if used correctly after advance()
        return eof_token;
    }
    return tokens[current_pos - 1];
}

bool Parser::isAtEnd(int offset) {
    return current_pos + offset >= tokens.size() || peek(offset).type == TokenType::TK_EOF;
}

Token Parser::advance() {
    if (!isAtEnd()) {
        current_pos++;
    }
    return previous();
}

bool Parser::check(TokenType type) const {
    if (current_pos >= tokens.size() || tokens[current_pos].type == TokenType::TK_EOF) return false;
    return tokens[current_pos].type == type;
}

bool Parser::check(const vector<TokenType>& types) const {
    if (current_pos >= tokens.size() || tokens[current_pos].type == TokenType::TK_EOF) return false;
    for (TokenType type : types) {
        if (tokens[current_pos].type == type) return true;
    }
    return false;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}


Token Parser::consume(TokenType type, const string& message) {
    if (check(type)) {
        return advance();
    }
    reportError(peek(), message);
    throw runtime_error("Parse error: " + message); // Or a custom ParseError exception
}


void Parser::reportError(const Token& token, const string& message) {
    had_error = true;
    if (token.type == TokenType::TK_EOF) {
        errors_list.push_back("[line " + to_string(token.line) + "] Error at end: " + message);
    } else {
        errors_list.push_back("[line " + to_string(token.line) + "] Error at '" + token.lexeme + "': " + message);
    }
}

void Parser::synchronize() {
    // Panic mode: discard tokens until we're at a likely statement boundary.
    if (isAtEnd()) return;
    advance(); // Consume the token that caused the error.

    while (!isAtEnd()) {
        if (previous().type == TokenType::TK_SEMICOLON) return; // Semicolon often ends a simple statement
        if (peek().type == TokenType::TK_DEDENT) return; // Dedent usually ends a block.

        switch (peek().type) {
            // Keywords that start common statements
            case TokenType::TK_CLASS:
            case TokenType::TK_DEF:
            case TokenType::TK_IF:
            case TokenType::TK_FOR:
            case TokenType::TK_WHILE:
            case TokenType::TK_TRY:
            case TokenType::TK_WITH:
            case TokenType::TK_RETURN:
            case TokenType::TK_IMPORT: // Stays, as 'import' is still valid
            case TokenType::TK_FROM:   // Can stay for general recovery, though not for 'from import'
            case TokenType::TK_GLOBAL:
            case TokenType::TK_NONLOCAL:
            case TokenType::TK_PASS:
            case TokenType::TK_BREAK:
            case TokenType::TK_CONTINUE:
            case TokenType::TK_RAISE:
                return;
            default:
                if (peek().type == TokenType::TK_INDENT) return;
        }
        advance();
    }
}


// --- Recursive Descent Parsing Method Implementations ---

// STARTING RULES
unique_ptr<ProgramNode> Parser::parseFile() {
    int start_line = tokens.empty() ? 0 : tokens[0].line;
    vector<unique_ptr<StatementNode>> stmts;
    if (!isAtEnd() && peek().type != TokenType::TK_EOF) {
        stmts = parseStatementsOpt();
    }

    if (!isAtEnd() && peek().type == TokenType::TK_EOF) {
        consume(TokenType::TK_EOF, "Expected end of file.");
    } else if (!isAtEnd()) {
        reportError(peek(), "Expected end of file, but found more tokens.");
        while(!isAtEnd()) advance();
    }
    return make_unique<ProgramNode>(start_line, std::move(stmts));
}

vector<unique_ptr<StatementNode>> Parser::parseStatementsOpt() {
    if (isAtEnd() || peek().type == TokenType::TK_EOF || peek().type == TokenType::TK_DEDENT) {
        return {};
    }
    return parseStatements();
}

// GENERAL STATEMENTS
vector<unique_ptr<StatementNode>> Parser::parseStatements() {
    vector<unique_ptr<StatementNode>> stmts_list;
    while (!isAtEnd() && peek().type != TokenType::TK_EOF && peek().type != TokenType::TK_DEDENT) {
        try {
            stmts_list.push_back(parseStatement());
        } catch (const runtime_error& e) {
            synchronize();
            if (isAtEnd() || peek().type == TokenType::TK_EOF || peek().type == TokenType::TK_DEDENT) break;
        }
    }
    return stmts_list;
}

unique_ptr<StatementNode> Parser::parseStatement() {
    TokenType current_type = peek().type;
    switch (current_type) {
        case TokenType::TK_DEF:
        case TokenType::TK_IF:
        case TokenType::TK_CLASS:
        case TokenType::TK_FOR:
        case TokenType::TK_TRY:
        case TokenType::TK_WHILE:
            // Potentially TK_WITH, etc. if added to simplified grammar
            return parseCompoundStmt();
        default:
            // simple_stmts part:
            // CFG simple_stmts: simple_stmt_list optional_semicolon
            // where simple_stmt_list is simple_stmt (';' simple_stmt)*.
            // This parseStatement default effectively handles one simple_stmt
            // from the list, and parseStatements() handles the looping for multiple simple_stmt
            // on different lines or if parseSimpleStmts handles multiple on one line.
            // The original code was:
            // auto stmt = parseSimpleStmt(); match(TokenType::TK_SEMICOLON); return stmt;
            // This parses one simple_stmt and consumes an *optional* following semicolon.
            // This is consistent with the "flattening" behavior described.

            // If simple_stmts means a potential sequence like "a=1; b=2\n" to be parsed as a single unit here,
            // then this should be `return parseSimpleStmts();` where parseSimpleStmts returns a StmtListNode or similar.
            // However, the problem description confirms the flattening approach is acceptable.
            // Let's assume simple_stmts can indeed be multiple simple_stmt on one line, separated by semicolons.
            // The current parseStatement would parse the first, parseStatements() would loop for the next.
            // To handle "a=1; print(a)" correctly when parseStatement is called for this whole line:
            // It should parse all simple statements on the line if that's the intent of `simple_stmts` rule.
            // Python's AST structure suggests flattening. `a=1;print(a)` results in a list of two AST nodes.
            // The current structure achieves this via parseStatements() calling parseStatement() multiple times.
            // The `match(TokenType::TK_SEMICOLON)` consumes a semicolon if present after a simple statement,
            // which is fine for single simple statements or the last simple_stmt in a sequence on one line.

            // The existing code `auto stmt = parseSimpleStmt(); match(TokenType::TK_SEMICOLON); return stmt;`
            // correctly handles one `simple_stmt` and an optional following semicolon.
            // The description states: "This effectively flattens simple_stmts into the main statement list,
            // which matches Python's AST structure...This can be considered correct by broader effect."
            // Therefore, keeping this logic.
            auto stmt = parseSimpleStmt();
            match(TokenType::TK_SEMICOLON); // Consumes semicolon if present after the simple_stmt
            return stmt;
    }
}

vector<unique_ptr<StatementNode>> Parser::parseSimpleStmts() {
    vector<unique_ptr<StatementNode>> stmts_list;
    stmts_list.push_back(parseSimpleStmt());
    while (match(TokenType::TK_SEMICOLON)) {
        if (isAtEnd() || check(TokenType::TK_EOF) || check(TokenType::TK_DEDENT)
            || (peek().line > previous().line && !check(TokenType::TK_INDENT) && !check(TokenType::TK_DEDENT)) ) {
            break;
        }
        stmts_list.push_back(parseSimpleStmt());
    }
    return stmts_list;
}


std::unique_ptr<StatementNode> Parser::parseSimpleStmt() {
    int line = peek().line;

    // Handle specific simple statement keywords first
    switch (peek().type) {
        case TokenType::TK_RETURN:  return parseReturnStmt();
        case TokenType::TK_IMPORT:  return parseImportStatement(); // Assuming this handles both import_name and import_from
            // case TokenType::TK_FROM: // This would be part of a more complex import_from dispatcher if not handled by parseImportStatement
        case TokenType::TK_RAISE:   return parseRaiseStmt();
        case TokenType::TK_PASS:    advance(); return make_unique<PassStatementNode>(line);
        case TokenType::TK_BREAK:   advance(); return make_unique<BreakStatementNode>(line);
        case TokenType::TK_CONTINUE:advance(); return make_unique<ContinueStatementNode>(line);
        case TokenType::TK_GLOBAL:  return parseGlobalStmt();
        case TokenType::TK_NONLOCAL:return parseNonlocalStmt();
            // Add other keyword-based simple statements here if any (e.g., DEL, ASSERT, YIELD expressions)
        default:
            // Fallthrough to handle assignment or expression statements
            break;
    }

    // CFG-direct approach for assignment vs. expression statement:
    // 1. Try to parse `targets = expressions`.
    // 2. If not, try to parse `single_target augassign expressions`.
    // 3. If not, parse as `expressions`.
    // This requires backtracking or speculative parsing.

    size_t initial_pos = current_pos;
    bool initial_had_error_flag = had_error; // Store initial error flag
    size_t initial_errors_count = errors_list.size(); // Store initial error count

    // --- Attempt 1: Parse as `targets TK_ASSIGN expressions` ---
    std::vector<std::unique_ptr<ExpressionNode>> potential_targets = parseTargets();

    if (!had_error && !potential_targets.empty()) { // Successfully parsed potential targets
        if (check(TokenType::TK_ASSIGN)) {
            consume(TokenType::TK_ASSIGN, "Expected '=' for assignment.");
            if (had_error) { // Error from consume
                // If consume failed, an error is already reported.
                // Try to return a partially valid node or an error placeholder.
                // For now, let the error propagate, or return a simple pass node.
                return make_unique<PassStatementNode>(line);
            }
            auto rhs_value = parseExpressions();
            if (had_error && !rhs_value) { // Error from parsing RHS
                return make_unique<PassStatementNode>(line); // RHS failed
            }
            // `parseTargets` should ensure its elements are valid assignable targets.
            return make_unique<AssignmentStatementNode>(line, std::move(potential_targets), std::move(rhs_value));
        }
        // If `parseTargets` succeeded but no `TK_ASSIGN` followed, then it's not this form of assignment.
        // We must backtrack because `potential_targets` might have consumed input in a way
        // that's incompatible with `parseSingleTarget` or `parseExpressions` for the same input.
        // E.g., `parseTargets` might parse "a, b" successfully. If no "=", this is not `a,b <augop> ...`.
    }

    // Backtrack if parseTargets failed, or if it succeeded but was not followed by TK_ASSIGN.
    current_pos = initial_pos;
    // Restore error state carefully: only revert errors added by the speculative parse.
    if (errors_list.size() > initial_errors_count) {
        errors_list.resize(initial_errors_count);
    }
    had_error = initial_had_error_flag; // Reset error flag to its state before this attempt


    // --- Attempt 2: Parse as `single_target augassign expressions` ---
    // Need to save/restore error state around this speculative parse too.
    initial_pos = current_pos; // Re-checkpoint, as pos might have been reset above.
    initial_had_error_flag = had_error;
    initial_errors_count = errors_list.size();

    std::unique_ptr<ExpressionNode> potential_single_target = parseSingleTarget();

    if (!had_error && potential_single_target) { // Successfully parsed a potential single_target
        if (check({TokenType::TK_PLUS_ASSIGN, TokenType::TK_MINUS_ASSIGN, TokenType::TK_MULTIPLY_ASSIGN,
                   TokenType::TK_DIVIDE_ASSIGN, TokenType::TK_MOD_ASSIGN, TokenType::TK_BIT_AND_ASSIGN,
                   TokenType::TK_BIT_OR_ASSIGN, TokenType::TK_BIT_XOR_ASSIGN, TokenType::TK_BIT_LEFT_SHIFT_ASSIGN,
                   TokenType::TK_BIT_RIGHT_SHIFT_ASSIGN, TokenType::TK_POWER_ASSIGN, TokenType::TK_FLOORDIV_ASSIGN})) {
            Token op = parseAugassign(); // Consumes the augassign token
            if (had_error) { // Error from parseAugassign
                return make_unique<PassStatementNode>(line);
            }
            auto rhs_value = parseExpressions();
            if (had_error && !rhs_value) { // Error from parsing RHS
                return make_unique<PassStatementNode>(line);
            }
            // `parseSingleTarget` should ensure it's a valid assignable target.
            return make_unique<AugAssignNode>(line, std::move(potential_single_target), op, std::move(rhs_value));
        }
        // If `parseSingleTarget` succeeded but no augassign op, it's not augmented assignment.
        // Backtrack for the final attempt (expression statement).
    }

    // Backtrack if parseSingleTarget failed, or if it succeeded but was not followed by an augassign operator.
    current_pos = initial_pos;
    if (errors_list.size() > initial_errors_count) {
        errors_list.resize(initial_errors_count);
    }
    had_error = initial_had_error_flag;

    // --- Attempt 3: Parse as `expressions` (ExpressionStatement) ---
    auto expr_stmt_expr = parseExpressions();

    if (had_error && !expr_stmt_expr) {
        // If parseExpressions() failed and returned nullptr, an error should have been reported.
        // Return a placeholder to prevent crashes and allow parser to attempt synchronization.
        // If no error was reported by parseExpressions but it returned null, that's an issue with parseExpressions.
        if (!had_error) reportError(peek(), "Expected an expression or assignment.");
        return make_unique<PassStatementNode>(line); // Error placeholder
    }
    // If parseExpressions succeeded, it's an expression statement.
    return make_unique<ExpressionStatementNode>(line, std::move(expr_stmt_expr));
}

unique_ptr<StatementNode> Parser::parseCompoundStmt() {
    switch (peek().type) {
        case TokenType::TK_DEF:     return parseFunctionDef();
        case TokenType::TK_IF:      return parseIfStmt();
        case TokenType::TK_CLASS:   return parseClassDef();
        case TokenType::TK_FOR:     return parseForStmt();
        case TokenType::TK_TRY:     return parseTryStmt();
        case TokenType::TK_WHILE:   return parseWhileStmt();
        default:
            reportError(peek(), "Expected a compound statement keyword (def, if, class, etc.).");
            throw runtime_error("Invalid compound statement.");
    }
}

// SIMPLE STATEMENTS Implementation (Assignment part is now in parseSimpleStmt/parseAssignmentTail)


Token Parser::parseAugassign() {
    vector<TokenType> aug_ops = {
            TokenType::TK_PLUS_ASSIGN, TokenType::TK_MINUS_ASSIGN, TokenType::TK_MULTIPLY_ASSIGN, TokenType::TK_DIVIDE_ASSIGN,
            TokenType::TK_MOD_ASSIGN, TokenType::TK_BIT_AND_ASSIGN, TokenType::TK_BIT_OR_ASSIGN, TokenType::TK_BIT_XOR_ASSIGN,
            TokenType::TK_BIT_LEFT_SHIFT_ASSIGN, TokenType::TK_BIT_RIGHT_SHIFT_ASSIGN, TokenType::TK_POWER_ASSIGN,
            TokenType::TK_FLOORDIV_ASSIGN
    };
    if (check(aug_ops)) {
        return advance();
    }
    reportError(peek(), "Expected augmented assignment operator (+=, -=, etc.).");
    throw runtime_error("Invalid augmented assignment operator.");
}

unique_ptr<ReturnStatementNode> Parser::parseReturnStmt() {
    Token ret_token = consume(TokenType::TK_RETURN, "Expected 'return'.");
    unique_ptr<ExpressionNode> value = nullptr;
    if (!isAtEnd() && peek().type != TokenType::TK_SEMICOLON && peek().type != TokenType::TK_DEDENT && peek().type != TokenType::TK_EOF
        && peek().line == ret_token.line) {
        value = parseExpressionsOpt();
    }
    return make_unique<ReturnStatementNode>(ret_token.line, std::move(value));
}

unique_ptr<ExpressionNode> Parser::parseExpressionsOpt() {
    if (isAtEnd() || check(TokenType::TK_SEMICOLON) || check(TokenType::TK_DEDENT) || check(TokenType::TK_EOF)
        || ( peek().line > previous().line && previous().type != TokenType::TK_COMMA )
            ) {
        return nullptr;
    }
    return parseExpressions();
}

unique_ptr<ExpressionNode> Parser::parseExpressions() {
    int line = peek().line;
    auto first_expr = parseExpression();

    if (match(TokenType::TK_COMMA)) {
        vector<unique_ptr<ExpressionNode>> elements;
        elements.push_back(std::move(first_expr));

        if (!isAtEnd() && peek().type != TokenType::TK_SEMICOLON && peek().type != TokenType::TK_RPAREN &&
            peek().type != TokenType::TK_RBRACKET && peek().type != TokenType::TK_RBRACE &&
            peek().type != TokenType::TK_COLON &&
            peek().line == previous().line ) {

            elements.push_back(parseExpression());
            while (match(TokenType::TK_COMMA)) {
                if (isAtEnd() || peek().type == TokenType::TK_SEMICOLON || peek().type == TokenType::TK_RPAREN ||
                    peek().type == TokenType::TK_RBRACKET || peek().type == TokenType::TK_RBRACE ||
                    peek().type == TokenType::TK_COLON ||
                    peek().line != previous().line) {
                    break;
                }
                elements.push_back(parseExpression());
            }
        }
        return make_unique<TupleLiteralNode>(line, std::move(elements));
    } else {
        return first_expr;
    }
}

unique_ptr<StatementNode> Parser::parseImportStatement() {
    Token import_token = consume(TokenType::TK_IMPORT, "Expected 'import'.");
    if (this->had_error) return make_unique<PassStatementNode>(import_token.line);

    Token module_token = consume(TokenType::TK_IDENTIFIER, "Expected module name after 'import'.");
    if (this->had_error) return make_unique<PassStatementNode>(import_token.line);

    std::string module_name_str = module_token.lexeme;
    int name_line_start = module_token.line;
    std::unique_ptr<IdentifierNode> alias_node = nullptr;

    if (match(TokenType::TK_AS)) {
        Token alias_token = consume(TokenType::TK_IDENTIFIER, "Expected alias name after 'as'.");
        if (this->had_error) return make_unique<PassStatementNode>(import_token.line);
        alias_node = std::make_unique<IdentifierNode>(alias_token.line, alias_token.lexeme);
    }

    // Create a NamedImportNode for this single import
    auto named_import = std::make_unique<NamedImportNode>(name_line_start, module_name_str, std::move(alias_node));

    // ImportStatementNode takes a vector of NamedImportNodes. Add the single one.
    std::vector<std::unique_ptr<NamedImportNode>> names_list;
    names_list.push_back(std::move(named_import));

    return std::make_unique<ImportStatementNode>(import_token.line, std::move(names_list));
}


// EXPRESSIONS (Precedence climbing / Pratt parser style usually, but CFG implies recursive descent)
unique_ptr<ExpressionNode> Parser::parseExpression() {
    int line = peek().line;
    auto cond_or_main_expr = parseDisjunction();

    if (match(TokenType::TK_IF)) {
        auto condition = parseDisjunction();
        consume(TokenType::TK_ELSE, "Expected 'else' in ternary expression.");
        auto orelse_expr = parseExpression();
        return make_unique<IfExpNode>(line, std::move(condition), std::move(cond_or_main_expr), std::move(orelse_expr));
    }
    return cond_or_main_expr;
}

unique_ptr<ExpressionNode> Parser::parseDisjunction() {
    auto node = parseConjunction();
    while (match(TokenType::TK_OR)) {
        Token op = previous();
        auto right = parseConjunction();
        node = make_unique<BinaryOpNode>(op.line, std::move(node), op, std::move(right));
    }
    return node;
}

unique_ptr<ExpressionNode> Parser::parseConjunction() {
    auto node = parseInversion();
    while (match(TokenType::TK_AND)) {
        Token op = previous();
        auto right = parseInversion();
        node = make_unique<BinaryOpNode>(op.line, std::move(node), op, std::move(right));
    }
    return node;
}

unique_ptr<ExpressionNode> Parser::parseInversion() {
    if (match(TokenType::TK_NOT)) {
        Token op = previous();
        auto operand = parseInversion();
        return make_unique<UnaryOpNode>(op.line, op, std::move(operand));
    }
    return parseComparison();
}

unique_ptr<ExpressionNode> Parser::parseComparison() {
    int line = peek().line;
    auto left_expr = parseBitwiseOr();

    vector<Token> ops;
    vector<unique_ptr<ExpressionNode>> comparators;

    while (true) {
        // Simplified direct check for ops
        if (check(TokenType::TK_EQUAL) || check(TokenType::TK_NOT_EQUAL) ||
            check(TokenType::TK_LESS) || check(TokenType::TK_LESS_EQUAL) ||
            check(TokenType::TK_GREATER) || check(TokenType::TK_GREATER_EQUAL) ||
            check(TokenType::TK_IN)) {
            ops.push_back(advance());
            comparators.push_back(parseBitwiseOr());
        } else if (peek().type == TokenType::TK_IS) {
            Token op_is = advance(); // Consume IS
            if (match(TokenType::TK_NOT)) { // 'is not'
                Token synthetic_op = op_is;
                synthetic_op.lexeme = "is not";
                // synthetic_op.type could be a new type if available, e.g., TK_IS_NOT
                ops.push_back(synthetic_op);
            } else { // 'is'
                ops.push_back(op_is);
            }
            comparators.push_back(parseBitwiseOr());
        } else if (peek().type == TokenType::TK_NOT && peek(1).type == TokenType::TK_IN) { // 'not in'
            Token op_not = advance(); // Consume NOT
            advance();  // Consume IN
            Token synthetic_op = op_not;
            synthetic_op.lexeme = "not in";
            // synthetic_op.type could be a new type, e.g., TK_NOT_IN
            ops.push_back(synthetic_op);
            comparators.push_back(parseBitwiseOr());
        }
        else {
            break;
        }
    }

    if (ops.empty()) {
        return left_expr;
    } else {
        return make_unique<ComparisonNode>(line, std::move(left_expr), std::move(ops), std::move(comparators));
    }
}

// To unput a token (simple version, only one token back)
void Parser::unputToken() {
    if (current_pos > 0) current_pos--;
}


unique_ptr<ExpressionNode> Parser::parseBitwiseOr() {
    auto node = parseBitwiseXor();
    while (match(TokenType::TK_BIT_OR)) {
        Token op = previous();
        auto right = parseBitwiseXor();
        node = make_unique<BinaryOpNode>(op.line, std::move(node), op, std::move(right));
    }
    return node;
}

unique_ptr<ExpressionNode> Parser::parseBitwiseXor() {
    auto node = parseBitwiseAnd();
    while (match(TokenType::TK_BIT_XOR)) {
        Token op = previous();
        auto right = parseBitwiseAnd();
        node = make_unique<BinaryOpNode>(op.line, std::move(node), op, std::move(right));
    }
    return node;
}

unique_ptr<ExpressionNode> Parser::parseBitwiseAnd() {
    auto node = parseShiftExpr();
    while (match(TokenType::TK_BIT_AND)) {
        Token op = previous();
        auto right = parseShiftExpr();
        node = make_unique<BinaryOpNode>(op.line, std::move(node), op, std::move(right));
    }
    return node;
}

unique_ptr<ExpressionNode> Parser::parseShiftExpr() {
    auto node = parseSum();
    while (match(TokenType::TK_BIT_LEFT_SHIFT) || match(TokenType::TK_BIT_RIGHT_SHIFT)) {
        Token op = previous();
        auto right = parseSum();
        node = make_unique<BinaryOpNode>(op.line, std::move(node), op, std::move(right));
    }
    return node;
}

unique_ptr<ExpressionNode> Parser::parseSum() {
    auto node = parseTerm();
    while (match(TokenType::TK_PLUS) || match(TokenType::TK_MINUS)) {
        Token op = previous();
        auto right = parseTerm();
        node = make_unique<BinaryOpNode>(op.line, std::move(node), op, std::move(right));
    }
    return node;
}

unique_ptr<ExpressionNode> Parser::parseTerm() {
    auto node = parseFactor();
    while (match(TokenType::TK_MULTIPLY) || match(TokenType::TK_DIVIDE) ||
           match(TokenType::TK_FLOORDIV) || match(TokenType::TK_MOD)) {
        Token op = previous();
        auto right = parseFactor();
        node = make_unique<BinaryOpNode>(op.line, std::move(node), op, std::move(right));
    }
    return node;
}

unique_ptr<ExpressionNode> Parser::parseFactor() {
    if (match(TokenType::TK_PLUS) || match(TokenType::TK_MINUS) || match(TokenType::TK_BIT_NOT)) {
        Token op = previous();
        auto operand = parseFactor();
        return make_unique<UnaryOpNode>(op.line, op, std::move(operand));
    }
    return parsePower();
}

unique_ptr<ExpressionNode> Parser::parsePower() {
    auto left = parsePrimary();
    if (match(TokenType::TK_POWER)) {
        Token op = previous();
        auto right = parseFactor();
        return make_unique<BinaryOpNode>(op.line, std::move(left), op, std::move(right));
    }
    return left;
}

unique_ptr<ExpressionNode> Parser::parsePrimary(bool in_target_context) {
    auto node = parseAtom(in_target_context);

    while (true) {
        if (match(TokenType::TK_PERIOD)) {
            Token dot_token = previous();
            Token name_token = consume(TokenType::TK_IDENTIFIER, "Expected attribute name after '.'.");
            auto attr_ident = make_unique<IdentifierNode>(name_token.line, name_token.lexeme);
            node = make_unique<AttributeAccessNode>(dot_token.line, std::move(node), std::move(attr_ident));
        } else if (match(TokenType::TK_LPAREN)) {
            if (in_target_context) {
                reportError(previous(), "Function call cannot be a target of assignment.");
                unputToken();
                break;
            }
            Token lparen_token = previous();
            vector<unique_ptr<ExpressionNode>> pos_args;
            vector<unique_ptr<KeywordArgNode>> kw_args;
            int call_line = lparen_token.line;
            if (!check(TokenType::TK_RPAREN)) {
                parseArgumentsForCall(pos_args, kw_args, call_line);
            }
            consume(TokenType::TK_RPAREN, "Expected ')' after function arguments.");
            node = make_unique<FunctionCallNode>(lparen_token.line, std::move(node), std::move(pos_args), std::move(kw_args));
        } else if (match(TokenType::TK_LBRACKET)) {
            Token lbracket_token = previous();
            auto slice_or_idx = parseSlices();
            consume(TokenType::TK_RBRACKET, "Expected ']' after subscript or slice.");
            node = make_unique<SubscriptionNode>(lbracket_token.line, std::move(node), std::move(slice_or_idx));
        } else {
            break;
        }
    }
    return node;
}

// ATOM and LITERALS (Example)
std::unique_ptr<ExpressionNode> Parser::parseAtom(bool in_target_context) {
    int line = peek().line;
    switch (peek().type) {
        case TokenType::TK_IDENTIFIER: {
            Token id_token = advance();
            return make_unique<IdentifierNode>(id_token.line, id_token.lexeme);
        }
        case TokenType::TK_TRUE:  advance(); return make_unique<BooleanLiteralNode>(line, true);
        case TokenType::TK_FALSE: advance(); return make_unique<BooleanLiteralNode>(line, false);
        case TokenType::TK_NONE:  advance(); return make_unique<NoneLiteralNode>(line);
        case TokenType::TK_NUMBER: {
            Token num_token = advance();
            NumberLiteralNode::Type num_type = (num_token.lexeme.find('.') != string::npos ||
                                                num_token.lexeme.find('e') != string::npos ||
                                                num_token.lexeme.find('E') != string::npos) ?
                                               NumberLiteralNode::Type::FLOAT : NumberLiteralNode::Type::INTEGER;
            return make_unique<NumberLiteralNode>(line, num_token.lexeme, num_type);
        }
        case TokenType::TK_COMPLEX: { // Assuming TK_COMPLEX stores the full 'Nj' or 'N+Nj' string
            Token complex_token = advance();
            std::string value = complex_token.lexeme;
            std::string real_part = "0"; // Default for pure imaginary like "5j"
            std::string imag_part = value;
            if (imag_part.empty() || imag_part.back() != 'j') {
                reportError(complex_token, "Complex literal must end with 'j'.");
                // Fallback or error recovery
                return make_unique<ComplexLiteralNode>(line, "0", "0"); // Dummy
            }
            imag_part.pop_back(); // Remove 'j'

            // Attempt to split N+Mj or N-Mj form
            // Find the last + or - that is not at the beginning and not part of an exponent (though 'e' unlikely in imag_part here)
            size_t plus_pos = std::string::npos;
            size_t minus_pos = std::string::npos;

            // Search for operator splitting real and imaginary parts
            for (size_t i = 1; i < imag_part.length(); ++i) { // Start from 1 to avoid sign of first number
                if (imag_part[i] == '+') plus_pos = i;
                if (imag_part[i] == '-') minus_pos = i;
            }

            size_t op_pos = std::string::npos;
            if (plus_pos != std::string::npos) op_pos = plus_pos;
            if (minus_pos != std::string::npos && (plus_pos == std::string::npos || minus_pos > plus_pos) ) { // prefer last operator
                op_pos = minus_pos;
            }


            if (op_pos != std::string::npos) {
                real_part = imag_part.substr(0, op_pos);
                imag_part = imag_part.substr(op_pos); // Includes sign for imag part like in "1-2j" -> real="1", imag="-2"
            } else {
                // No explicit real part, imag_part already holds the numeric part (e.g. "5" from "5j")
                // Or it's a single real number that was misclassified as complex (e.g. "5" if lexer was wrong)
                // If imag_part is now just a number, it was a pure imaginary. Real part remains "0".
                // If imag_part still contains non-numeric characters (other than initial sign), it's an error.
                // This simplistic parsing might need refinement based on lexer's TK_COMPLEX output.
            }
            return make_unique<ComplexLiteralNode>(line, real_part, imag_part);
        }
        case TokenType::TK_STRING:
            return parseStrings(); // Handles concatenation of adjacent string literals
        case TokenType::TK_BYTES:
            return parseBytes();     // Use the specific helper for bytes literals (handles concatenation)
        case TokenType::TK_LPAREN:
            // parseTupleGroupVariant handles both tuple and group (parenthesized expr)
            // It internally decides if it's a group (e.g. (expr)) or a tuple (e.g. (), (expr,), (expr1, expr2))
            return parseTupleGroupVariant(in_target_context);
        case TokenType::TK_LBRACKET:
            // parseListVariant calls parseListLiteral
            return parseListVariant(in_target_context);
        case TokenType::TK_LBRACE:
            // parseDictSetVariant handles both dict and set literals
            // It internally decides based on K:V structure vs just elements.
            return parseDictSetVariant();
        case TokenType::TK_INT: case TokenType::TK_STR: case TokenType::TK_FLOAT:
        case TokenType::TK_LIST: case TokenType::TK_TUPLE: case TokenType::TK_RANGE:
        case TokenType::TK_DICT: case TokenType::TK_SET: case TokenType::TK_FROZENSET:
        case TokenType::TK_BOOL: case TokenType::TK_BYTEARRAY: case TokenType::TK_MEMORYVIEW:
        case TokenType::TK_NONETYPE:
            // These are built-in type names, treated as identifiers in expression context
        {
            Token type_kw_token = advance();
            return make_unique<IdentifierNode>(type_kw_token.line, type_kw_token.lexeme);
        }
        default:
            reportError(peek(), "Expected an atom (identifier, literal, '(', '[', or '{').");
            throw runtime_error("Invalid atom: Unrecognized token " + peek().lexeme); // Or return nullptr after reporting error
    }
}

unique_ptr<StringLiteralNode> Parser::parseStrings() {
    if (!check(TokenType::TK_STRING)) {
        reportError(peek(), "Expected string literal.");
        throw runtime_error("Expected string.");
    }
    Token first_string = consume(TokenType::TK_STRING, "Expected string literal.");
    string concatenated_value = first_string.lexeme;
    int line = first_string.line;

    while (check(TokenType::TK_STRING) && tokens[current_pos-1].line == tokens[current_pos].line) {
        // A more robust check would involve lexer information about adjacency.
        // This simple check concatenates if they are on the same line and next in token stream.
        concatenated_value += advance().lexeme;
    }
    return make_unique<StringLiteralNode>(line, concatenated_value);
}


unique_ptr<ExpressionNode> Parser::parseTupleGroupVariant(bool in_target_context) {
    Token lparen = consume(TokenType::TK_LPAREN, "Expected '('.");

    if (match(TokenType::TK_RPAREN)) {
        return make_unique<TupleLiteralNode>(lparen.line, vector<unique_ptr<ExpressionNode>>());
    }
    auto first_element = parseExpression();

    if (match(TokenType::TK_COMMA)) {
        vector<unique_ptr<ExpressionNode>> elements;
        elements.push_back(std::move(first_element));

        if (!check(TokenType::TK_RPAREN)) {
            auto remaining_exprs_node = parseExpressionsOpt();
            if (remaining_exprs_node) {
                if (auto tuple_node = dynamic_cast<TupleLiteralNode*>(remaining_exprs_node.get())) {
                    for (auto& el : tuple_node->elements) {
                        elements.push_back(std::move(el));
                    }
                    // tuple_node elements are moved, tuple_node itself can be discarded.
                } else {
                    elements.push_back(std::move(remaining_exprs_node));
                }
            }
        }
        consume(TokenType::TK_RPAREN, "Expected ')' to close tuple literal.");
        return make_unique<TupleLiteralNode>(lparen.line, std::move(elements));
    } else {
        consume(TokenType::TK_RPAREN, "Expected ')' to close parenthesized expression.");
        return first_element;
    }
}

unique_ptr<ListLiteralNode> Parser::parseListVariant(bool in_target_context) {
    return parseListLiteral(in_target_context);
}

unique_ptr<ListLiteralNode> Parser::parseListLiteral(bool in_target_context) {
    Token lbracket = consume(TokenType::TK_LBRACKET, "Expected '[' to start list literal.");
    vector<unique_ptr<ExpressionNode>> elements;

    if (!check(TokenType::TK_RBRACKET)) {
        auto exprs_node = parseExpressionsOpt();
        if (exprs_node) {
            if (auto tuple_node = dynamic_cast<TupleLiteralNode*>(exprs_node.get())) {
                elements = std::move(tuple_node->elements);
            } else {
                elements.push_back(std::move(exprs_node));
            }
        }
    }
    consume(TokenType::TK_RBRACKET, "Expected ']' to close list literal.");
    return make_unique<ListLiteralNode>(lbracket.line, std::move(elements));
}


unique_ptr<BlockNode> Parser::parseBlock() {
    int line = peek().line;
    if (match(TokenType::TK_INDENT)) {
        auto stmts = parseStatements();
        consume(TokenType::TK_DEDENT, "Expected DEDENT to end indented block.");
        return make_unique<BlockNode>(line, std::move(stmts));
    } else {
        auto stmts_vec = parseSimpleStmts();
        return make_unique<BlockNode>(line, std::move(stmts_vec));
    }
}

unique_ptr<FunctionDefinitionNode> Parser::parseFunctionDef() {
    Token def_token = consume(TokenType::TK_DEF, "Expected 'def'.");
    Token name_token = consume(TokenType::TK_IDENTIFIER, "Expected function name.");
    auto name_ident = make_unique<IdentifierNode>(name_token.line, name_token.lexeme);

    consume(TokenType::TK_LPAREN, "Expected '(' after function name.");
    int params_line = peek().line;
    unique_ptr<ArgumentsNode> args_spec = parseParamsOpt(params_line);
    consume(TokenType::TK_RPAREN, "Expected ')' after function parameters.");
    consume(TokenType::TK_COLON, "Expected ':' after function signature.");
    auto body = parseBlock();

    return make_unique<FunctionDefinitionNode>(def_token.line, std::move(name_ident), std::move(args_spec), std::move(body));
}

unique_ptr<ArgumentsNode> Parser::parseParamsOpt(int& line_start) {
    line_start = peek().line;
    if (check(TokenType::TK_RPAREN)) {
        return make_unique<ArgumentsNode>(line_start);
    }
    return parseParameters(line_start);
}


std::unique_ptr<RaiseStatementNode> Parser::parseRaiseStmt() {
    Token raise_token = consume(TokenType::TK_RAISE, "Expected 'raise'.");
    std::unique_ptr<ExpressionNode> exception_expr = nullptr;
    std::unique_ptr<ExpressionNode> cause_expr = nullptr;

    if (!isAtEnd() && peek().type != TokenType::TK_SEMICOLON &&
        peek().type != TokenType::TK_EOF && peek().type != TokenType::TK_DEDENT &&
        peek().type != TokenType::TK_FROM &&
        (tokens.size() > current_pos && tokens[current_pos-1].line == peek().line)
            ) {
        exception_expr = parseExpression();
    }

    if (match(TokenType::TK_FROM)) {
        if (!exception_expr && !isAtEnd() && previous().line == peek().line) {
            reportError(previous(), "'from' in raise statement must follow an explicit exception expression. Cannot be used with bare 'raise'.");
        }
        cause_expr = parseExpression();
    }
    return std::make_unique<RaiseStatementNode>(raise_token.line, std::move(exception_expr), std::move(cause_expr));
}

std::unique_ptr<GlobalStatementNode> Parser::parseGlobalStmt() {
    Token global_token = consume(TokenType::TK_GLOBAL, "Expected 'global'.");
    if (this->had_error) {
        // Return a dummy node to allow parsing to continue if possible,
        // but mark that an error occurred.
        return std::make_unique<GlobalStatementNode>(global_token.line, std::vector<std::unique_ptr<IdentifierNode>>());
    }

    int name_list_line_start = peek().line; // Line where the name list starts
    std::vector<std::unique_ptr<IdentifierNode>> names = parseNameCommaList(name_list_line_start);

    if (names.empty() && !this->had_error) {
        // parseNameCommaList might return empty if it encounters an error or no identifiers.
        // If no error was reported by parseNameCommaList but names is empty, it's an issue here.
        reportError(peek(), "Expected at least one identifier after 'global'.");
    }
    // Even if names is empty due to an error, proceed to create the node.
    return std::make_unique<GlobalStatementNode>(global_token.line, std::move(names));
}

std::unique_ptr<NonlocalStatementNode> Parser::parseNonlocalStmt() {
    Token nonlocal_token = consume(TokenType::TK_NONLOCAL, "Expected 'nonlocal'.");
    if (this->had_error) {
        return std::make_unique<NonlocalStatementNode>(nonlocal_token.line, std::vector<std::unique_ptr<IdentifierNode>>());
    }

    int name_list_line_start = peek().line;
    std::vector<std::unique_ptr<IdentifierNode>> names = parseNameCommaList(name_list_line_start);

    if (names.empty() && !this->had_error) {
        reportError(peek(), "Expected at least one identifier after 'nonlocal'.");
    }
    return std::make_unique<NonlocalStatementNode>(nonlocal_token.line, std::move(names));
}

std::unique_ptr<IfStatementNode> Parser::parseIfStmt() {
    Token if_tok = consume(TokenType::TK_IF, "Expect 'if' keyword.");
    if (this->had_error) {
        return nullptr;
    }
    int line = if_tok.line;

    std::unique_ptr<ExpressionNode> condition = parseExpression();
    if (this->had_error || !condition) {
        if (!this->had_error) reportError(previous(), "Expect expression after 'if'.");
        return nullptr;
    }

    consume(TokenType::TK_COLON, "Expect ':' after 'if' condition.");
    if (this->had_error) {
        return nullptr;
    }

    std::unique_ptr<BlockNode> then_block = parseBlock();
    if (this->had_error || !then_block) {
        if (!this->had_error) reportError(previous(), "Expect indented block for 'if' body.");
        return nullptr;
    }

    std::vector<std::pair<std::unique_ptr<ExpressionNode>, std::unique_ptr<BlockNode>>> elif_blocks;

    while (peek().type == TokenType::TK_ELIF) {
        consume(TokenType::TK_ELIF, "Internal error with 'elif'.");
        if (this->had_error) return nullptr;

        std::unique_ptr<ExpressionNode> elif_condition = parseExpression();
        if (this->had_error || !elif_condition) {
            if (!this->had_error) reportError(previous(), "Expect expression after 'elif'.");
            return nullptr;
        }

        consume(TokenType::TK_COLON, "Expect ':' after 'elif' condition.");
        if (this->had_error) return nullptr;

        std::unique_ptr<BlockNode> elif_body = parseBlock();
        if (this->had_error || !elif_body) {
            if (!this->had_error) reportError(previous(), "Expect indented block for 'elif' body.");
            return nullptr;
        }
        elif_blocks.emplace_back(std::move(elif_condition), std::move(elif_body));
    }

    std::unique_ptr<BlockNode> else_block = parseElseBlockOpt();
    if (this->had_error && else_block == nullptr && previous().type == TokenType::TK_ELSE) {
        return nullptr;
    }

    return std::make_unique<IfStatementNode>(line, std::move(condition), std::move(then_block),
                                             std::move(elif_blocks), std::move(else_block));
}

std::unique_ptr<BlockNode> Parser::parseElseBlockOpt() {
    if (peek().type == TokenType::TK_ELSE) {
        consume(TokenType::TK_ELSE, "Internal error: Expected 'else' based on peek.");
        if (this->had_error) return nullptr;

        consume(TokenType::TK_COLON, "Expect ':' after 'else' keyword.");
        if (this->had_error) return nullptr;

        std::unique_ptr<BlockNode> else_b = parseBlock();
        if (this->had_error || !else_b) {
            if (!this->had_error) reportError(previous(), "Expected indented block for 'else' body.");
            return nullptr;
        }
        return else_b;
    }
    return nullptr;
}

std::unique_ptr<ClassDefinitionNode> Parser::parseClassDef() {
    Token class_tok = consume(TokenType::TK_CLASS, "Expect 'class' keyword.");
    if (this->had_error) {
        return nullptr;
    }
    int line = class_tok.line;

    Token name_tok = consume(TokenType::TK_IDENTIFIER, "Expect class name.");
    if (this->had_error) {
        return nullptr;
    }
    auto class_name = std::make_unique<IdentifierNode>(name_tok.line, name_tok.lexeme);

    std::vector<std::unique_ptr<ExpressionNode>> base_classes;
    std::vector<std::unique_ptr<KeywordArgNode>> keyword_args;
    int arg_list_line = name_tok.line; // Default line, updated by parseClassArgumentsOpt

    if (match(TokenType::TK_LPAREN)) { // Use match for optional parentheses
        // arg_list_line will be updated by parseClassArgumentsOpt if there are args.
        // If it's just "()", peek().line would be ')'
        if (peek().type != TokenType::TK_RPAREN) {
            parseClassArgumentsOpt(base_classes, keyword_args, arg_list_line);
            if (this->had_error) return nullptr;
        }
        consume(TokenType::TK_RPAREN, "Expect ')' after class argument list.");
        if (this->had_error) return nullptr;
    }

    consume(TokenType::TK_COLON, "Expect ':' after class definition header.");
    if (this->had_error) {
        return nullptr;
    }

    std::unique_ptr<BlockNode> body = parseBlock();
    if (this->had_error || !body) {
        if (!this->had_error) reportError(previous(), "Expect indented block for class body.");
        return nullptr;
    }

    return std::make_unique<ClassDefinitionNode>(line, std::move(class_name),
                                                 std::move(base_classes),
                                                 std::move(keyword_args),
                                                 std::move(body));
}


std::unique_ptr<ForStatementNode> Parser::parseForStmt() {
    Token for_token = consume(TokenType::TK_FOR, "Expected 'for'.");
    if (this->had_error) return nullptr;

    // CFG: TK_FOR targets TK_IN expressions ...
    // parseTargets() returns a vector. If it's a single target, vector has 1 element.
    // If multiple (e.g. for x, y in ...), vector has multiple.
    // The ForStatementNode's 'target' field is unique_ptr<ExpressionNode>.
    // If parseTargets() returns >1 elements, they should be wrapped in a TupleLiteralNode.
    std::vector<std::unique_ptr<ExpressionNode>> targets_vec = parseTargets();
    if (this->had_error || targets_vec.empty()) {
        if (!this->had_error) reportError(peek(), "Expected target(s) for 'for' loop.");
        return nullptr;
    }
    std::unique_ptr<ExpressionNode> final_target_expr;
    if (targets_vec.size() == 1) {
        final_target_expr = std::move(targets_vec[0]);
    } else {
        final_target_expr = std::make_unique<TupleLiteralNode>(targets_vec[0]->line, std::move(targets_vec));
    }


    consume(TokenType::TK_IN, "Expected 'in' after for-loop target(s).");
    if (this->had_error) return nullptr;

    auto iterable_expr = parseExpressions(); // CFG: expressions
    if (this->had_error || !iterable_expr) {
        if (!this->had_error) reportError(peek(), "Expected iterable expression in 'for' loop.");
        return nullptr;
    }

    consume(TokenType::TK_COLON, "Expected ':' after for-loop iterable.");
    if (this->had_error) return nullptr;

    auto body_block = parseBlock();
    if (this->had_error || !body_block) {
        if (!this->had_error) reportError(previous(), "Expected indented block for 'for' loop body.");
        return nullptr;
    }

    std::unique_ptr<BlockNode> else_block = nullptr;
    if (match(TokenType::TK_ELSE)) {
        consume(TokenType::TK_COLON, "Expected ':' after 'else' in for loop.");
        if (this->had_error) return nullptr;
        else_block = parseBlock();
        if (this->had_error && !else_block) { // Error during else block parsing
            if (!this->had_error) reportError(previous(), "Expected indented block for 'for' loop 'else' body.");
            return nullptr;
        }
    }

    return std::make_unique<ForStatementNode>(for_token.line, std::move(final_target_expr), std::move(iterable_expr), std::move(body_block), std::move(else_block));
}

std::unique_ptr<TryStatementNode> Parser::parseTryStmt() {
    Token try_token = consume(TokenType::TK_TRY, "Expected 'try'.");
    if (this->had_error) return nullptr; // Cannot proceed if 'try' is missing

    consume(TokenType::TK_COLON, "Expected ':' after 'try'.");
    if (this->had_error) return nullptr; // Syntax error

    auto try_block = parseBlock();
    if (this->had_error || !try_block) {
        if (!this->had_error) reportError(previous(), "Expected indented block for 'try' body.");
        return nullptr; // Critical part missing
    }

    std::vector<std::unique_ptr<ExceptionHandlerNode>> handlers;
    std::unique_ptr<BlockNode> else_block_node = nullptr;
    std::unique_ptr<BlockNode> finally_block_node = nullptr;
    bool has_except_clause = false;

    // Parse 'except_block_plus'
    while (check(TokenType::TK_EXCEPT)) {
        has_except_clause = true;
        auto handler = parseExceptBlock(); // Call the specific helper
        if (this->had_error) {
            // If parseExceptBlock itself had an error and returned nullptr,
            // we might try to synchronize and see if there's another except/else/finally.
            // For now, if an error occurs in an except block, we stop parsing this try statement.
            return nullptr;
        }
        if (!handler) { // Should not happen if no error was reported by parseExceptBlock
            reportError(peek(), "Internal error: parseExceptBlock returned null without error.");
            return nullptr;
        }
        handlers.push_back(std::move(handler));
    }

    // Parse optional 'else_block'
    if (has_except_clause && match(TokenType::TK_ELSE)) {
        consume(TokenType::TK_COLON, "Expected ':' after 'else' in try-except statement.");
        if (this->had_error) return nullptr;
        else_block_node = parseBlock();
        if (this->had_error || !else_block_node) {
            if (!this->had_error) reportError(previous(), "Expected indented block for 'else' body.");
            return nullptr;
        }
    }

    // Parse optional 'finally_block' using parseFinallyBlockOpt
    finally_block_node = parseFinallyBlockOpt();
    if (this->had_error && finally_block_node == nullptr && previous().type == TokenType::TK_FINALLY) {
        // This implies an error happened within parseFinallyBlockOpt (e.g., 'finally' without ':' or block)
        return nullptr;
    }


    if (handlers.empty() && !finally_block_node) {
        reportError(try_token, "Try statement must have at least one 'except' or 'finally' clause.");
        // Consider returning a special error node or a partially valid one.
        // For now, a TryStatementNode will be created, but it's semantically invalid.
    }

    return std::make_unique<TryStatementNode>(try_token.line, std::move(try_block), std::move(handlers), std::move(else_block_node), std::move(finally_block_node));
}

void Parser::parseClassArgumentsOpt(std::vector<std::unique_ptr<ExpressionNode>>& bases,
                                    std::vector<std::unique_ptr<KeywordArgNode>>& keywords,
                                    int& line_start_ref) {
    bool first_arg = true;
    if (peek().type != TokenType::TK_RPAREN && !isAtEnd()) { // Only proceed if there are arguments
        line_start_ref = peek().line; // Line of the first argument or structure within
    } else {
        return; // No arguments if ')' is next or at end
    }

    while (peek().type != TokenType::TK_RPAREN && !isAtEnd()) {
        if (!first_arg) {
            consume(TokenType::TK_COMMA, "Expected ',' to separate class arguments.");
            if (this->had_error) return; // Stop on error
            if (peek().type == TokenType::TK_RPAREN) break; // Trailing comma before ')'
        }
        first_arg = false;

        // Check for keyword argument: IDENTIFIER = expression
        if (peek(0).type == TokenType::TK_IDENTIFIER && peek(1).type == TokenType::TK_ASSIGN) {
            auto kw_item = parseKeywordItem(); // Use the specific parser
            if (this->had_error) return; // Stop on error from parseKeywordItem
            if (!kw_item) { // Should not happen if no error
                reportError(peek(), "Internal error: parseKeywordItem returned null without error flag for class arguments.");
                return;
            }
            keywords.push_back(std::move(kw_item));
        } else { // Positional argument (base class)
            std::unique_ptr<ExpressionNode> base_expr = parseExpression();
            if (this->had_error || !base_expr) {
                if (!this->had_error) reportError(previous(), "Expected expression for base class.");
                return; // Stop on error
            }
            bases.push_back(std::move(base_expr));
        }
    }
}

// Helper for parsing identifier in parameters
std::unique_ptr<IdentifierNode> Parser::parseParamIdentifier() {
    if (!check(TokenType::TK_IDENTIFIER)) {
        reportError(peek(), "Expected parameter identifier.");
        return nullptr;
    }
    Token id_token = consume(TokenType::TK_IDENTIFIER, "Expected parameter identifier.");
    if (this->had_error) return nullptr;
    return std::make_unique<IdentifierNode>(id_token.line, id_token.lexeme);
}

// Helper for parsing default value
std::unique_ptr<ExpressionNode> Parser::parseDefault() {
    if (match(TokenType::TK_ASSIGN)) {
        auto default_expr = parseExpression();
        if (this->had_error || !default_expr) {
            if (!this->had_error) reportError(previous(), "Expected expression for default parameter value.");
            return nullptr; // Error or no expression
        }
        return default_expr;
    }
    return nullptr; // No default value
}


std::unique_ptr<ParameterNode> Parser::parseParamWithDefault() {
    ParameterNode::Kind kind = ParameterNode::Kind::POSITIONAL_OR_KEYWORD;
    std::unique_ptr<IdentifierNode> param_ident_node = parseParamIdentifier();
    if (this->had_error || !param_ident_node) {
        return nullptr;
    }

    std::string param_name_str = param_ident_node->name;
    int name_line = param_ident_node->line;

    std::unique_ptr<ExpressionNode> default_expr = parseDefault();
    if (this->had_error && default_expr == nullptr && previous().type == TokenType::TK_ASSIGN) {
        return nullptr;
    }
    return std::make_unique<ParameterNode>(name_line, param_name_str, kind, std::move(default_expr));
}


std::unique_ptr<ParameterNode> Parser::parseParamNoDefault(ParameterNode::Kind kind) {
    std::unique_ptr<IdentifierNode> param_ident_node = parseParamIdentifier();
    if (this->had_error || !param_ident_node) {
        return nullptr;
    }

    std::string param_name_str = param_ident_node->name;
    int name_line = param_ident_node->line;

    if (peek().type == TokenType::TK_ASSIGN) {
        reportError(peek(), "Unexpected default value for a parameter expected to have no default.");
        /*Token assign_tok =*/ consume(TokenType::TK_ASSIGN, "Internal error: Consuming unexpected default."); // consume to try to recover
        if(!this->had_error) parseExpression(); // consume the expression too
        return nullptr; // Error state
    }
    return std::make_unique<ParameterNode>(name_line, param_name_str, kind, nullptr);
}


std::unique_ptr<WhileStatementNode> Parser::parseWhileStmt() {
    Token while_token = consume(TokenType::TK_WHILE, "Expected 'while'.");
    auto condition = parseExpression();
    consume(TokenType::TK_COLON, "Expected ':' after while condition.");
    auto body = parseBlock();

    std::unique_ptr<BlockNode> else_block = nullptr;
    if (match(TokenType::TK_ELSE)) {
        consume(TokenType::TK_COLON, "Expected ':' after 'else' in while loop.");
        else_block = parseBlock();
    }
    return std::make_unique<WhileStatementNode>(while_token.line, std::move(condition), std::move(body), std::move(else_block));
}

void Parser::parseArgumentsForCall(std::vector<std::unique_ptr<ExpressionNode>>& pos_args,
                                   std::vector<std::unique_ptr<KeywordArgNode>>& kw_args,
                                   int& /*line_start_call unused here */) {
    if (check(TokenType::TK_RPAREN)) return; // No arguments

    bool keyword_args_started = false;

    do {
        // Check for keyword argument: IDENTIFIER = expression
        if (check(TokenType::TK_IDENTIFIER) && peek(1).type == TokenType::TK_ASSIGN) {
            keyword_args_started = true;
            auto kw_item = parseKeywordItem(); // Use the specific parser
            if (this->had_error) {
                // Error reported by parseKeywordItem.
                // Attempt to recover by synchronizing or breaking.
                synchronize(); // Might be too aggressive.
                if (isAtEnd() || check(TokenType::TK_RPAREN)) break;
                if (check(TokenType::TK_COMMA)) continue; // If sync lands on comma
                break; // Otherwise, cannot continue.
            }
            if (!kw_item) { // Should not happen if no error from parseKeywordItem
                reportError(peek(), "Internal error: parseKeywordItem returned null without error flag.");
                break;
            }
            kw_args.push_back(std::move(kw_item));
        } else { // Positional argument
            if (keyword_args_started) {
                reportError(peek(), "Positional argument cannot follow keyword arguments.");
                // Try to parse the expression to consume it and then recover
                auto temp_expr = parseExpression();
                if (this->had_error) {
                    synchronize();
                    if (isAtEnd() || check(TokenType::TK_RPAREN)) break;
                    if (check(TokenType::TK_COMMA)) continue;
                    break;
                }
            } else {
                auto pos_arg_expr = parseExpression();
                if (this->had_error || !pos_arg_expr) {
                    if(!this->had_error) reportError(peek(), "Invalid positional argument.");
                    synchronize();
                    if (isAtEnd() || check(TokenType::TK_RPAREN)) break;
                    if (check(TokenType::TK_COMMA)) continue;
                    break;
                }
                pos_args.push_back(std::move(pos_arg_expr));
            }
        }
    } while (match(TokenType::TK_COMMA) && !check(TokenType::TK_RPAREN));
    // Trailing comma is allowed: loop ends if next is RPAREN or no comma.
}


std::unique_ptr<ExpressionNode> Parser::parseSlices() {
    int line = peek().line; // Line of the first item in slices
    std::vector<std::unique_ptr<ExpressionNode>> elements;

    do { // Loop for comma-separated items (slice_or_expr_comma_list)
        std::unique_ptr<ExpressionNode> current_item = nullptr;
        int item_line = peek().line;

        // Decide if the current part is a slice (contains colons) or a simple expression (index).
        // A slice starts with ':', 'expr:', '::', 'expr::', ':expr:', etc.
        // A simple expression index does not contain a colon at the top level of this item.
        bool is_slice_item = false;
        // Lookahead: check current token and potentially the one after next expression
        if (check(TokenType::TK_COLON)) {
            is_slice_item = true;
        } else {
            // Need to check if an expression is followed by a colon.
            // This is tricky without full backtracking or arbitrary lookahead.
            // Tentative parse:
            size_t checkpoint = current_pos;
            bool could_be_expr_then_colon = false;
            if (!check(TokenType::TK_COMMA) && !check(TokenType::TK_RBRACKET)) { // If not an immediate delimiter
                try {
                    parseExpression(); // Tentatively parse an expression
                    if (check(TokenType::TK_COLON)) { // Check token *after* the expression
                        could_be_expr_then_colon = true;
                    }
                } catch (const std::runtime_error&) {
                    // Parsing expression failed, probably not expr:
                    // Or an error occurred, which will be handled.
                }
                current_pos = checkpoint; // Backtrack
                if (this->had_error) { // If tentative parse caused an error, clear it for now, re-parse properly
                    this->had_error = false; // This is risky, assumes error was purely speculative
                    // A better way: clone parser state or use a non-modifying check.
                    // For now, we just reset the flag if the speculative parse failed.
                    // The actual parse below will re-trigger errors if they are real.
                    errors_list.pop_back(); // Remove speculative error
                }
            }
            if (could_be_expr_then_colon) {
                is_slice_item = true;
            }
        }

        if (is_slice_item) {
            current_item = parseSlice(); // Call the specific SliceNode parser
        } else {
            // If not a slice item, it must be a simple expression (index)
            // or an error if nothing is here (e.g. `[,]` or empty `[]` context)
            if (check(TokenType::TK_COMMA) || check(TokenType::TK_RBRACKET)) {
                if (check(TokenType::TK_RBRACKET) && elements.empty() && previous().type == TokenType::TK_LBRACKET) {
                    // This case is `[]` - means empty list, handled by `parseListLiteral`.
                    // If `parseSlices` is called, it implies `obj[slices]`, so `obj[]` is an error.
                    reportError(peek(), "Empty subscript '[]' is not allowed.");
                    return nullptr;
                } else if (previous().type == TokenType::TK_COMMA){
                    reportError(peek(), "Expected expression after comma in subscript.");
                    return nullptr;
                }
                // Fall through will likely cause error in parseExpression or if it returns null
            }
            current_item = parseExpression();
        }

        if (this->had_error) return nullptr; // Error during item parsing

        if (!current_item) { // Should not happen if no error, unless parseExpression can return null for valid empty constructs
            reportError(peek(), "Missing item in subscript list.");
            return nullptr;
        }
        elements.push_back(std::move(current_item));

    } while (match(TokenType::TK_COMMA) && !check(TokenType::TK_RBRACKET));

    if (elements.empty()) {
        // This means `obj[]` was parsed, which is a syntax error.
        // `peek(-1)` should give LBRACKET if called correctly.
        reportError(peek(-1).type != TokenType::TK_EOF ? peek(-1) : eof_token, "Subscript cannot be empty.");
        return nullptr;
    }

    if (elements.size() == 1) {
        return std::move(elements[0]); // Single index or single slice
    } else {
        // Multiple items, comma-separated: forms a tuple for advanced indexing
        return std::make_unique<TupleLiteralNode>(line, std::move(elements));
    }
}

std::unique_ptr<ExpressionNode> Parser::parseDictSetVariant() {
    Token lbrace_token = consume(TokenType::TK_LBRACE, "Expected '{'.");
    if (this->had_error) return nullptr;

    if (check(TokenType::TK_RBRACE)) { // Empty dictionary {}
        consume(TokenType::TK_RBRACE, "Expected '}' to close empty dictionary.");
        if (this->had_error) return nullptr;
        return std::make_unique<DictLiteralNode>(lbrace_token.line,
                                                 std::vector<std::unique_ptr<ExpressionNode>>(),
                                                 std::vector<std::unique_ptr<ExpressionNode>>());
    }

    // To distinguish dict from set, we need to see if the first expression is followed by a colon.
    // This requires some lookahead or parsing the first expression and then checking.
    auto first_expr = parseExpression();
    if (this->had_error || !first_expr) {
        if (!this->had_error) reportError(lbrace_token, "Expected expression in set/dict literal.");
        // Try to consume up to RBRACE to recover for the caller
        while(!isAtEnd() && !match(TokenType::TK_RBRACE)) advance();
        return nullptr;
    }

    if (match(TokenType::TK_COLON)) { // It's a dictionary: {key: value, ...}
        // We've parsed `key` (first_expr) and `COLON`. Now parse `value` and subsequent pairs.
        // This means `parseDictLiteral` should be structured to take a pre-parsed first key
        // or `unputToken` for `first_expr` and `COLON` would be needed if `parseDictLiteral` starts from `{`.
        // For now, let's re-implement the dict parsing here using parseKvPair.
        std::vector<std::unique_ptr<ExpressionNode>> keys;
        std::vector<std::unique_ptr<ExpressionNode>> values;

        keys.push_back(std::move(first_expr)); // This was the key

        auto first_value = parseExpression(); // Parse the value for the first key
        if(this->had_error || !first_value) {
            if(!this->had_error) reportError(previous(), "Expected value after ':' in dictionary literal.");
            while(!isAtEnd() && !match(TokenType::TK_RBRACE)) advance();
            return nullptr;
        }
        values.push_back(std::move(first_value));

        while (match(TokenType::TK_COMMA)) {
            if (check(TokenType::TK_RBRACE)) break; // Trailing comma
            int kv_line = peek().line;
            parseKvPair(keys, values, kv_line); // Use the helper
            if (this->had_error) { // Error in parseKvPair
                while(!isAtEnd() && !match(TokenType::TK_RBRACE)) advance();
                return nullptr;
            }
        }
        consume(TokenType::TK_RBRACE, "Expected '}' to close dictionary literal.");
        if (this->had_error) return nullptr;
        return std::make_unique<DictLiteralNode>(lbrace_token.line, std::move(keys), std::move(values));

    } else { // It's a set: {element, ...}
        // `first_expr` is the first element.
        // This means `parseSetLiteral` would need to take `first_expr` or unput tokens.
        // Re-implementing set parsing here:
        std::vector<std::unique_ptr<ExpressionNode>> elements;
        elements.push_back(std::move(first_expr));

        while (match(TokenType::TK_COMMA)) {
            if (check(TokenType::TK_RBRACE)) break; // Trailing comma
            auto next_element = parseExpression();
            if(this->had_error || !next_element) {
                if(!this->had_error) reportError(previous(), "Expected expression in set literal.");
                while(!isAtEnd() && !match(TokenType::TK_RBRACE)) advance();
                return nullptr;
            }
            elements.push_back(std::move(next_element));
        }
        consume(TokenType::TK_RBRACE, "Expected '}' to close set literal.");
        if (this->had_error) return nullptr;
        return std::make_unique<SetLiteralNode>(lbrace_token.line, std::move(elements));
    }
}

std::unique_ptr<ArgumentsNode> Parser::parseParameters(int& line_start) {
    line_start = peek().line;
    auto args_node = std::make_unique<ArgumentsNode>(line_start);

    bool default_seen = false; // True if a param with default has been encountered
    bool star_etc_seen = false; // True if *args or **kwargs has been encountered

    while (!check(TokenType::TK_RPAREN)) {
        if (star_etc_seen) {
            reportError(peek(), "Unexpected token after *args or **kwargs.");
            synchronize(); // Attempt to recover to ')'
            break;
        }

        if (check(TokenType::TK_MULTIPLY) || check(TokenType::TK_POWER)) {
            parseSimplifiedStarEtc(*args_node);
            if (had_error) {
                synchronize(); // Error in *args/**kwargs, try to recover
                break;
            }
            star_etc_seen = true; // After *args/**kwargs, no more regular params
            // The loop for comma handling will continue, to consume a potential comma after *args
        } else if (check(TokenType::TK_IDENTIFIER)) {
            std::unique_ptr<ParameterNode> param_node;

            // To decide whether to call parseParamWithDefault or parseParamNoDefault,
            // we need to lookahead for an '=' after the identifier.
            // The helpers themselves also make this check.
            // Let's call parseParamNoDefault if no default has been seen OR if the next tokens don't indicate a default.
            // Call parseParamWithDefault if a default has been seen OR if next tokens indicate a default.

            // We need to "unput" the identifier if the helper expects to parse it.
            // `parseParamNoDefault` and `parseParamWithDefault` both call `parseParamIdentifier`.

            // Store current position to unput if needed, or pass the ID token.
            // For simplicity, let the helpers parse the ID. We need to unput the current ID if we just checked it.
            // This is tricky. Alternative: Helpers take an already-parsed ID.
            // Current helpers don't. They expect to parse ID themselves.

            // Simpler: make a decision based on `default_seen` and presence of `=`
            // Peek for `IDENTIFIER = ` sequence
            bool will_have_default = false;
            if (check(TokenType::TK_IDENTIFIER) && peek(1).type == TokenType::TK_ASSIGN) {
                will_have_default = true;
            }

            if (default_seen) {
                if (!will_have_default) {
                    // We've seen a default, now comes a param without default. This is an error.
                    // Report error before calling, then call parseParamNoDefault to consume and attempt recovery.
                    Token current_param_token = peek(); // Token for error reporting
                    reportError(current_param_token, "Non-default argument follows default argument.");
                    // Still try to parse it as a non-default to keep parser moving
                    param_node = parseParamNoDefault(ParameterNode::Kind::POSITIONAL_OR_KEYWORD);
                } else {
                    param_node = parseParamWithDefault();
                    // default_seen is already true
                }
            } else { // No default_seen yet
                if (will_have_default) {
                    param_node = parseParamWithDefault();
                    if (param_node && param_node->default_value) { // Successfully got a param with default
                        default_seen = true;
                    } else if (param_node && !param_node->default_value && !had_error) {
                        // parseParamWithDefault was called, parsed ID, but failed to get a default value expression
                        // (e.g. "def foo(a=):"). Error should be reported by parseDefault or parseParamWithDefault.
                        // For robustness, if param_node exists but default_value is null here, it's an issue.
                        // This case should ideally be covered by error reporting in parseParamWithDefault itself.
                    }
                } else {
                    // Expecting param without default, or an error if not an ID.
                    param_node = parseParamNoDefault(ParameterNode::Kind::POSITIONAL_OR_KEYWORD);
                    // default_seen remains false
                }
            }

            if (param_node) {
                args_node->args.push_back(std::move(param_node));
            } else {
                if (had_error) { // Error already reported by helper
                    synchronize(); // Attempt to recover
                    break;
                } else { // Helper returned null without error - should not happen
                    reportError(peek(), "Internal error: Failed to parse parameter.");
                    synchronize();
                    break;
                }
            }
        } else {
            reportError(peek(), "Expected parameter name, '*', '**', or ')'.");
            synchronize(); // Attempt to recover
            if (isAtEnd() || check(TokenType::TK_RPAREN)) break;
            continue;
        }

        // After processing a parameter or *args/**kwargs block
        if (check(TokenType::TK_RPAREN)) {
            break; // End of parameters
        } else if (match(TokenType::TK_COMMA)) {
            if (check(TokenType::TK_RPAREN)) { // Trailing comma
                // Trailing comma is allowed. Behavior for `def func(a, **b,)` might be a subtle point.
                // The simplified CFG's `param_ending_char` on `param_no_default` (used by *args/**kw) allows this.
                break;
            }
            // If after comma, we expect another param or * / **, loop continues.
            if (star_etc_seen && (check(TokenType::TK_MULTIPLY) || check(TokenType::TK_POWER) || check(TokenType::TK_IDENTIFIER)) ) {
                reportError(peek(), "Unexpected token after parameters and comma (e.g., after **kwargs).");
                synchronize();
                break;
            }
        } else {
            reportError(peek(), "Expected ',' or ')' after parameter.");
            synchronize();
            break;
        }
    }
    return args_node;
}

// -------------- Added functions for Parser.cpp --------------

// Corresponds to CFG: name_comma_list: TK_IDENTIFIER name_comma_list_tail_star
// name_comma_list_tail_star: TK_COMMA TK_IDENTIFIER name_comma_list_tail_star | epsilon
std::vector<std::unique_ptr<IdentifierNode>> Parser::parseNameCommaList(int& line_start) {
    std::vector<std::unique_ptr<IdentifierNode>> names;
    if (!check(TokenType::TK_IDENTIFIER)) {
        reportError(peek(), "Expected an identifier.");
        return names; // Return empty list on error
    }

    Token first_id_token = consume(TokenType::TK_IDENTIFIER, "Expected identifier.");
    if (this->had_error) return names;
    line_start = first_id_token.line;
    names.push_back(std::make_unique<IdentifierNode>(first_id_token.line, first_id_token.lexeme));

    while (match(TokenType::TK_COMMA)) {
        if (!check(TokenType::TK_IDENTIFIER)) {
            // Handles trailing comma, which is not allowed by this specific CFG rule for name_comma_list
            // but common in Python lists. Here, CFG implies an identifier must follow.
            reportError(peek(), "Expected identifier after comma in name list.");
            // unputToken(); // unput comma? Or let it be consumed.
            break; // Stop parsing this list
        }
        Token id_token = consume(TokenType::TK_IDENTIFIER, "Expected identifier after comma.");
        if (this->had_error) break;
        names.push_back(std::make_unique<IdentifierNode>(id_token.line, id_token.lexeme));
    }
    return names;
}

// Corresponds to CFG: except_block: TK_EXCEPT expression except_as_name_opt TK_COLON block
//                                 | TK_EXCEPT TK_COLON block
// except_as_name_opt: TK_AS TK_IDENTIFIER | epsilon
std::unique_ptr<ExceptionHandlerNode> Parser::parseExceptBlock() {
    if (!check(TokenType::TK_EXCEPT)) { // Should have been matched by caller
        reportError(peek(), "Expected 'except' keyword.");
        return nullptr;
    }
    Token except_token = consume(TokenType::TK_EXCEPT, "Expected 'except'."); // Already checked, but consume it.
    if (this->had_error) return nullptr;

    std::unique_ptr<ExpressionNode> exc_type = nullptr;
    std::unique_ptr<IdentifierNode> exc_name = nullptr;

    if (!check(TokenType::TK_COLON)) { // If there's something before the colon, it's an expression
        exc_type = parseExpression();
        if (this->had_error) return nullptr;

        if (match(TokenType::TK_AS)) {
            if (!check(TokenType::TK_IDENTIFIER)) {
                reportError(peek(), "Expected identifier after 'as' in except clause.");
                return nullptr;
            }
            Token name_token = consume(TokenType::TK_IDENTIFIER, "Expected identifier for exception name.");
            if (this->had_error) return nullptr;
            exc_name = std::make_unique<IdentifierNode>(name_token.line, name_token.lexeme);
        }
    }

    consume(TokenType::TK_COLON, "Expected ':' after except clause.");
    if (this->had_error) return nullptr;

    std::unique_ptr<BlockNode> body = parseBlock();
    if (this->had_error || !body) {
        if (!this->had_error) reportError(previous(), "Expected block for except handler.");
        return nullptr;
    }

    return std::make_unique<ExceptionHandlerNode>(except_token.line, std::move(body), std::move(exc_type), std::move(exc_name));
}

// Corresponds to CFG: finally_block_opt: finally_block | epsilon
std::unique_ptr<BlockNode> Parser::parseFinallyBlockOpt() {
    if (check(TokenType::TK_FINALLY)) {
        return parseFinallyBlock();
    }
    return nullptr; // Epsilon case
}

// Corresponds to CFG: finally_block: TK_FINALLY TK_COLON block
std::unique_ptr<BlockNode> Parser::parseFinallyBlock() {
    if (!check(TokenType::TK_FINALLY)) { // Should be checked by caller or parseFinallyBlockOpt
        reportError(peek(), "Expected 'finally' keyword.");
        return nullptr;
    }
    consume(TokenType::TK_FINALLY, "Expected 'finally'.");
    if (this->had_error) return nullptr;

    consume(TokenType::TK_COLON, "Expected ':' after 'finally'.");
    if (this->had_error) return nullptr;

    std::unique_ptr<BlockNode> body = parseBlock();
    if (this->had_error || !body) {
        if (!this->had_error) reportError(previous(), "Expected block for finally clause.");
        return nullptr;
    }
    return body;
}

// Corresponds to CFG: slice: expression_opt TK_COLON expression_opt slice_colon_expr_opt | expression
// This function parses one item of a slice tuple, which can be an actual slice or an index expression.
// Corresponds to parsing the colon-separated part of a slice:
// expression_opt TK_COLON expression_opt (TK_COLON expression_opt)?
// This function's contract is to return a SliceNode if the syntax matches,
// or nullptr if it doesn't (e.g., no colon is found where expected).
std::unique_ptr<SliceNode> Parser::parseSlice() {
    int line = peek().line; // Line number for the SliceNode, taken from the first token looked at.
    std::unique_ptr<ExpressionNode> lower = nullptr;
    std::unique_ptr<ExpressionNode> upper = nullptr;
    std::unique_ptr<ExpressionNode> step = nullptr;

    // Try to parse `lower` (expression_opt before the first colon)
    // This happens if the current token is not a colon and not a delimiter for the next slice item or end of slices.
    if (!isAtEnd() && peek().type != TokenType::TK_COLON &&
        peek().type != TokenType::TK_COMMA &&    // Delimiter for tuple of slices/indices
        peek().type != TokenType::TK_RBRACKET) { // End of the overall subscript
        lower = parseExpression();
        if (had_error) {
            return nullptr; // Error occurred while parsing the 'lower' expression
        }
    }

    // A colon is now mandatory to form a SliceNode according to this function's contract.
    if (!match(TokenType::TK_COLON)) {
        // If 'lower' was parsed but no colon follows, it means the input was just a simple expression (e.g., `data[index]`).
        // Such a case should ideally be handled by parseExpression() directly, not this specialized parseSlice().
        // If this function is called, a slice structure (with colons) is expected.
        reportError(peek(), "Expected ':' to define a slice structure (e.g., start:stop:step).");
        return nullptr; // Cannot form a SliceNode without at least one colon.
    }
    // First colon has been consumed.

    // Try to parse `upper` (expression_opt after the first colon, before a potential second colon or end)
    if (!isAtEnd() && peek().type != TokenType::TK_COLON &&
        peek().type != TokenType::TK_COMMA &&
        peek().type != TokenType::TK_RBRACKET) {
        upper = parseExpression();
        if (had_error) {
            return nullptr; // Error occurred while parsing the 'upper' expression
        }
    }

    // Check for the optional second colon and `step` expression_opt
    if (match(TokenType::TK_COLON)) {
        // Second colon has been consumed.
        if (!isAtEnd() && peek().type != TokenType::TK_COMMA &&
            peek().type != TokenType::TK_RBRACKET) {
            step = parseExpression();
            if (had_error) {
                return nullptr; // Error occurred while parsing the 'step' expression
            }
        }
    }

    return std::make_unique<SliceNode>(line, std::move(lower), std::move(upper), std::move(step));
}

// For bytes literals, handles concatenation like b"a" b"b" -> b"ab"
std::unique_ptr<BytesLiteralNode> Parser::parseBytes() {
    if (!check(TokenType::TK_BYTES)) {
        reportError(peek(), "Expected bytes literal.");
        return nullptr;
    }
    Token first_bytes = consume(TokenType::TK_BYTES, "Expected bytes literal.");
    if (this->had_error) return nullptr;

    std::string concatenated_value = first_bytes.lexeme;
    int line = first_bytes.line;

    // Concatenate adjacent bytes literals on the same line.
    // A more robust check might involve lexer hints about whitespace.
    while (check(TokenType::TK_BYTES) && !isAtEnd() && tokens[current_pos-1].line == tokens[current_pos].line) {
        // Python's lexer usually combines adjacent string/bytes literals if only separated by whitespace.
        // This parser check is simpler: if they are consecutive tokens of the same type on the same line.
        Token next_bytes = advance();
        concatenated_value += next_bytes.lexeme;
    }
    return std::make_unique<BytesLiteralNode>(line, concatenated_value);
}

// Corresponds to CFG: kvpair: expression TK_COLON expression
void Parser::parseKvPair(std::vector<std::unique_ptr<ExpressionNode>>& keys,
                         std::vector<std::unique_ptr<ExpressionNode>>& values,
                         int /*line, unused for now, could be used for specific error messages */) {
    auto key = parseExpression();
    if (this->had_error || !key) {
        if(!this->had_error) reportError(peek(), "Expected key in dictionary K:V pair.");
        // To try to recover, one might consume until a comma or rbrace, but for a helper, just return.
        // Set had_error if not already set.
        this->had_error = true;
        return;
    }

    consume(TokenType::TK_COLON, "Expected ':' after key in dictionary K:V pair.");
    if (this->had_error) return;

    auto value = parseExpression();
    if (this->had_error || !value) {
        if(!this->had_error) reportError(peek(), "Expected value in dictionary K:V pair after ':'.");
        this->had_error = true;
        return;
    }

    keys.push_back(std::move(key));
    values.push_back(std::move(value));
}

// Corresponds to CFG: keyword_item: TK_IDENTIFIER TK_ASSIGN expression
std::unique_ptr<KeywordArgNode> Parser::parseKeywordItem() {
    if (!(check(TokenType::TK_IDENTIFIER) && peek(1).type == TokenType::TK_ASSIGN)) {
        reportError(peek(), "Expected 'identifier = expression' for keyword argument.");
        return nullptr;
    }

    Token id_token = consume(TokenType::TK_IDENTIFIER, "Expected identifier for keyword argument name.");
    if (this->had_error) return nullptr;
    auto arg_name_node = std::make_unique<IdentifierNode>(id_token.line, id_token.lexeme);

    consume(TokenType::TK_ASSIGN, "Expected '=' for keyword argument.");
    if (this->had_error) return nullptr;

    auto value = parseExpression();
    if (this->had_error || !value) {
        if (!this->had_error) reportError(previous(), "Expected expression for keyword argument value.");
        return nullptr;
    }

    return std::make_unique<KeywordArgNode>(id_token.line, std::move(arg_name_node), std::move(value));
}

// Corresponds to CFG: targets: target | target target_comma_list_star optional_comma
// target_comma_list_star: TK_COMMA target target_comma_list_star | epsilon
std::vector<std::unique_ptr<ExpressionNode>> Parser::parseTargets() {
    std::vector<std::unique_ptr<ExpressionNode>> targets_vec;

    auto first_target = parseTarget();
    if (this->had_error || !first_target) {
        if(!this->had_error) reportError(peek(), "Expected a target for assignment.");
        return targets_vec; // Return empty or partially filled on error
    }
    targets_vec.push_back(std::move(first_target));

    while (match(TokenType::TK_COMMA)) {
        if (check(TokenType::TK_ASSIGN) || check(TokenType::TK_SEMICOLON) || check(TokenType::TK_EOF) ||
            (peek().line > previous().line && !check(TokenType::TK_INDENT))) { // Optional trailing comma
            // This check is for Python's general trailing comma. CFG 'optional_comma'
            // If followed by '=', ';', EOF, or NEWLINE, it's a trailing comma.
            break;
        }
        auto next_target = parseTarget();
        if (this->had_error || !next_target) {
            if(!this->had_error) reportError(peek(), "Expected a target after comma.");
            // Error, stop parsing targets for this list
            break;
        }
        targets_vec.push_back(std::move(next_target));
    }
    return targets_vec;
}

// Corresponds to CFG: target: t_primary TK_PERIOD TK_IDENTIFIER
//                            | t_primary TK_LBRACKET slices TK_RBRACKET
//                            | target_atom
std::unique_ptr<ExpressionNode> Parser::parseTarget() {
    // Determine if it's target_atom or t_primary chain.
    // target_atom can be (target), (targets_tuple_seq), [targets_list_seq], or IDENTIFIER.
    // t_primary can be atom which includes IDENTIFIER.
    // This requires some lookahead or a combined parsing approach.
    // Let's try parsing as t_primary first, then check for chains,
    // and if no chain, ensure the t_primary result is a valid standalone target (like an ID).

    std::unique_ptr<ExpressionNode> node;

    // Check for target_atom forms like (target_tuple) or [target_list] which don't start like t_primary chains.
    if (check(TokenType::TK_LPAREN) || check(TokenType::TK_LBRACKET)) {
        // Could be TK_LPAREN target_atom_variant TK_RPAREN or TK_LBRACKET target_atom_variant TK_RBRACKET
        // or t_primary starting with (group) or [list_literal_not_target] (if t_primary can be non-target atom).
        // The CFG is: target_atom: TK_IDENTIFIER | TK_LPAREN target TK_RPAREN | TK_LPAREN targets_tuple_seq_opt TK_RPAREN | TK_LBRACKET targets_list_seq_opt TK_RBRACKET
        // So, if it's LPAREN or LBRACKET, it's likely target_atom or a t_primary that resolves to an atom.
        // Let parseTargetAtom handle these ambiguous starts.
        // If parseTargetAtom returns something, that's our node.
        // If it doesn't match an atom form it might be t_primary -> atom -> (group) which isn't a target atom.
        // This is tricky. The most direct approach for target_atom is to call parseTargetAtom.
        // A t_primary can also be an atom.
        // Python's official grammar makes t_primary include atom, and target_atom is a subset of t_primary forms.
        // E.g. `atom` can be `(group)`. `target_atom` can be `(target)`.
        // Let's assume `parseTPrimary(true)` will correctly parse identifiable atoms and then we chain.
        // And special cases of `target_atom` like `(a,b)` or `[a,b]` are handled.

        // If starts with ( or [, try target_atom specific parsing.
        if (peek().type == TokenType::TK_LPAREN || peek().type == TokenType::TK_LBRACKET) {
            // This could be `(target)` or `(t1, t2)` or `[t1, t2]`. These are target_atom forms.
            // It could also be `(expr)` if `t_primary -> atom -> group`.
            // parseTargetAtom is best here.
            node = parseTargetAtom();
            if (this->had_error) return nullptr;
            // If parseTargetAtom returns a node, we assume it's a complete target.
            // It doesn't make sense to chain after (a,b).attr for example.
            // So, if parseTargetAtom succeeds, we return its result.
            if(node) return node;
            // If node is null, it means it wasn't a target_atom form recognized by parseTargetAtom,
            // but it could still be a t_primary like (func()).attr.
            // Fall through to parseTPrimary logic. This might re-parse ( if parseTargetAtom didn't consume.
            // This indicates parseTargetAtom should probably consume if it identifies a target_atom pattern.

            // Re-evaluating: `target` is either a chained `t_primary` or a `target_atom`.
            // `t_primary` itself can be an `atom`. `target_atom` contains `IDENTIFIER`, `(target)`, `(t,...)`, `[t,...]`.
            // If `peek()` is `IDENTIFIER`, it could be `target_atom` or start of `t_primary`.
            // If `peek()` is `LPAREN`, it could be `target_atom`'s `(target)` or `(t1,t2)`, or `t_primary`'s `(expr)`.
            // If `peek()` is `LBRACKET`, it could be `target_atom`'s `[t1,t2]`, or `t_primary`'s `[expr]`.

            // Let's try to parse the most complex structure first, i.e., t_primary chain.
            // parseTPrimary(true) should correctly handle simple identifiers and reject calls if it's a standalone target.
        }
    }

    // Default path: try to parse as t_primary and then chain.
    // `t_primary` might be an identifier, or `(target)` or `[target_list]`.
    // `in_target_context=true` for the components of `t_primary` if they are to be targets.
    // `t_primary` needs to allow calls if it's base of an attr/subscript.
    // So, initial parse of `t_primary` as non-terminal target.
    node = parseTPrimary(); // This will use in_target_context flags appropriately.
    if (this->had_error || !node) return nullptr;


    // Loop for attribute access and subscripting, which are valid target extensions.
    while (!isAtEnd()) {
        if (match(TokenType::TK_PERIOD)) {
            Token dot_token = previous();
            Token name_token = consume(TokenType::TK_IDENTIFIER, "Expected attribute name after '.'.");
            if (this->had_error) return nullptr;
            auto attr_ident = std::make_unique<IdentifierNode>(name_token.line, name_token.lexeme);
            node = std::make_unique<AttributeAccessNode>(dot_token.line, std::move(node), std::move(attr_ident));
        } else if (match(TokenType::TK_LBRACKET)) {
            Token lbracket_token = previous();
            auto slice_or_idx = parseSlices(); // Slices themselves contain expressions, not necessarily targets.
            if (this->had_error || !slice_or_idx) {
                if (!this->had_error) reportError(lbracket_token, "Invalid slice or index for target.");
                return nullptr;
            }
            consume(TokenType::TK_RBRACKET, "Expected ']' after subscript or slice in target.");
            if (this->had_error) return nullptr;
            node = std::make_unique<SubscriptionNode>(lbracket_token.line, std::move(node), std::move(slice_or_idx));
        } else {
            break;
        }
    }

    // After parsing, check if the constructed node is a valid target type.
    // IdentifierNode, AttributeAccessNode, SubscriptionNode are fine.
    // TupleLiteralNode and ListLiteralNode are fine IF their elements are targets (handled by parseTargetAtom).
    // Other types (e.g. CallNode if parseTPrimary was misconfigured, literals) are not.
    // This structural check is implicitly done by the CFG rules if parseTPrimary/parseTargetAtom are correct.
    // If node is a CallNode, it's an error. `parsePrimary(true)` should prevent this.
    if (dynamic_cast<FunctionCallNode*>(node.get())) {
        reportError(previous(), "Function call cannot be a target of assignment."); // Should be caught earlier by parsePrimary(true)
        return nullptr;
    }
    // Similar checks for literals etc. if parseTPrimary could produce them as standalone targets.
    // For now, trust that parseTPrimary and parseTargetAtom build valid structures or error out.

    return node;
}


// Corresponds to CFG: target_atom: TK_IDENTIFIER
//                                 | TK_LPAREN target TK_RPAREN
//                                 | TK_LPAREN targets_tuple_seq_opt TK_RPAREN
//                                 | TK_LBRACKET targets_list_seq_opt TK_RBRACKET
// targets_tuple_seq_opt: targets_tuple_seq | epsilon
// targets_list_seq_opt: targets_list_seq | epsilon
std::unique_ptr<ExpressionNode> Parser::parseTargetAtom() {
    int line = peek().line;
    if (check(TokenType::TK_IDENTIFIER)) {
        // This is the `TK_IDENTIFIER` case of `target_atom`.
        // `parsePrimary(true)` would produce an IdentifierNode.
        return parsePrimary(true); // `parsePrimary` with `in_target_context = true` handles identifiers.
    } else if (match(TokenType::TK_LPAREN)) {
        Token lparen = previous();
        if (match(TokenType::TK_RPAREN)) { // `()` empty tuple target.
            return std::make_unique<TupleLiteralNode>(lparen.line, std::vector<std::unique_ptr<ExpressionNode>>());
        }

        // Distinguish (target) from (targets_tuple_seq)
        // Try parsing `targets_tuple_seq` first, as `target` is a prefix.
        // targets_tuple_seq: target target_comma_list_plus optional_comma | target TK_COMMA
        // target_comma_list_plus: TK_COMMA target ...
        // So if after first target, there's a comma, it's `targets_tuple_seq`.

        auto first_elem = parseTarget(); // Could be `target` in `(target)` or first `target` in `(target, ...)`
        if (this->had_error || !first_elem) {
            if (!this->had_error) reportError(lparen, "Invalid content in parenthesized target.");
            return nullptr;
        }

        if (match(TokenType::TK_COMMA)) { // Indicates `targets_tuple_seq` or `target TK_COMMA`
            std::vector<std::unique_ptr<ExpressionNode>> elements;
            elements.push_back(std::move(first_elem));

            // Parse `target_comma_list_plus` (already consumed one comma) or `expressions_opt` from `targets_tuple_seq`'s original full form.
            // The CFG for targets_tuple_seq is complex. Simplified: parse comma-separated targets.
            while (!check(TokenType::TK_RPAREN) && !isAtEnd()) {
                auto next_target = parseTarget();
                if (this->had_error || !next_target) {
                    if (!this->had_error) reportError(peek(), "Expected target in tuple target sequence.");
                    return nullptr;
                }
                elements.push_back(std::move(next_target));
                if (!match(TokenType::TK_COMMA)) break; // Expect comma between elements
                if (check(TokenType::TK_RPAREN)) break; // Trailing comma before )
            }
            consume(TokenType::TK_RPAREN, "Expected ')' to close tuple target.");
            if (this->had_error) return nullptr;
            return std::make_unique<TupleLiteralNode>(lparen.line, std::move(elements));
        } else { // No comma after first_elem, so it must be `(target)`
            consume(TokenType::TK_RPAREN, "Expected ')' to close parenthesized target.");
            if (this->had_error) return nullptr;
            return first_elem; // This is the `(target)` case.
        }
    } else if (match(TokenType::TK_LBRACKET)) {
        Token lbracket = previous();
        std::vector<std::unique_ptr<ExpressionNode>> elements;
        if (!check(TokenType::TK_RBRACKET)) { // targets_list_seq_opt -> targets_list_seq
            // targets_list_seq: target_comma_list optional_comma
            // target_comma_list: target target_comma_list_tail_star
            // Simplified: parse comma-separated targets.
            do {
                auto target_elem = parseTarget();
                if (this->had_error || !target_elem) {
                    if(!this->had_error) reportError(peek(), "Expected target in list target sequence.");
                    return nullptr;
                }
                elements.push_back(std::move(target_elem));
                if (!match(TokenType::TK_COMMA)) break;
                if (check(TokenType::TK_RBRACKET)) break; // Trailing comma
            } while (!check(TokenType::TK_RBRACKET) && !isAtEnd());
        }
        consume(TokenType::TK_RBRACKET, "Expected ']' to close list target.");
        if (this->had_error) return nullptr;
        return std::make_unique<ListLiteralNode>(lbracket.line, std::move(elements));
    } else {
        // Not an identifier, '(', or '['. So not a target_atom by this function's direct rules.
        // Caller (parseTarget) might then try t_primary chains if this returns null.
        // However, `target_atom` is one of the alternatives for `target`. If `peek()` doesn't match,
        // `parseTarget` should try `t_primary` chains.
        // This function should only parse if it IS a target_atom. So if no match, return null.
        // reportError(peek(), "Expected an atomic target (identifier, (target), or [target_list]).");
        return nullptr;
    }
}


// Corresponds to CFG: single_target: single_subscript_attribute_target | TK_IDENTIFIER | TK_LPAREN single_target TK_RPAREN
// single_subscript_attribute_target: t_primary TK_PERIOD TK_IDENTIFIER | t_primary TK_LBRACKET slices TK_RBRACKET
std::unique_ptr<ExpressionNode> Parser::parseSingleTarget() {
    if (check(TokenType::TK_IDENTIFIER)) {
        // could be IDENTIFIER or start of t_primary chain.
        // If just IDENTIFIER, it's parsePrimary(true).
        // If t_primary chain (obj.attr or obj[idx]), parsePrimary(false) for obj.
        // This structure is similar to parseTarget but more restrictive (no tuples/lists).
        // Let's parse as a potential t_primary chain.
        auto node = parseTPrimary(); // parseTPrimary internally handles in_target_context
        if (this->had_error || !node) return nullptr;

        // Check for chains
        bool is_chained = false;
        while(true){
            if (check(TokenType::TK_PERIOD) && peek(1).type == TokenType::TK_IDENTIFIER) {
                consume(TokenType::TK_PERIOD, "");
                Token name_token = consume(TokenType::TK_IDENTIFIER, "Expected attribute name after '.' in single_target.");
                if (this->had_error) return nullptr;
                auto attr_ident = std::make_unique<IdentifierNode>(name_token.line, name_token.lexeme);
                node = std::make_unique<AttributeAccessNode>(name_token.line, std::move(node), std::move(attr_ident));
                is_chained = true;
            } else if (check(TokenType::TK_LBRACKET)) {
                consume(TokenType::TK_LBRACKET, "");
                auto slice_or_idx = parseSlices();
                if (this->had_error || !slice_or_idx) return nullptr;
                consume(TokenType::TK_RBRACKET, "Expected ']' after subscript in single_target.");
                if (this->had_error) return nullptr;
                node = std::make_unique<SubscriptionNode>(previous().line, std::move(node), std::move(slice_or_idx));
                is_chained = true;
            } else {
                break;
            }
        }


        // If it wasn't chained, node must be a simple Identifier.
        // If it was chained, node is AttributeAccess or Subscription.
        // If node ended up being a call or literal (from parseTPrimary not being strict enough), error.
        // parseTPrimary -> parsePrimary(true) should prevent calls/literals if not chained.
        if (!is_chained && !(dynamic_cast<IdentifierNode*>(node.get()))) {
            if(dynamic_cast<FunctionCallNode*>(node.get()) ||
               dynamic_cast<ListLiteralNode*>(node.get()) ||
               dynamic_cast<TupleLiteralNode*>(node.get()) ||
               dynamic_cast<SetLiteralNode*>(node.get()) ||
               dynamic_cast<DictLiteralNode*>(node.get()) ||
               dynamic_cast<NumberLiteralNode*>(node.get()) ||
               dynamic_cast<StringLiteralNode*>(node.get()) ||
               dynamic_cast<BooleanLiteralNode*>(node.get()) ||
               dynamic_cast<NoneLiteralNode*>(node.get())
                    ) {
                reportError(previous(), "Invalid single target for assignment (e.g. literal, call). Must be identifier, attribute, or subscript.");
                return nullptr;
            }
        }
        return node;

    } else if (match(TokenType::TK_LPAREN)) {
        auto inner_target = parseSingleTarget();
        if (this->had_error || !inner_target) {
            if(!this->had_error) reportError(peek(), "Expected single target inside parentheses.");
            return nullptr;
        }
        consume(TokenType::TK_RPAREN, "Expected ')' to close parenthesized single target.");
        if (this->had_error) return nullptr;
        return inner_target;
    } else {
        reportError(peek(), "Expected identifier or '(' for single target.");
        return nullptr;
    }
}

// Corresponds to CFG: t_primary: t_primary TK_PERIOD TK_IDENTIFIER
//                               | t_primary TK_LBRACKET slices TK_RBRACKET
//                               | t_primary TK_LPAREN arguments_opt TK_RPAREN
//                               | atom
// This is identical to 'primary' CFG. So it's essentially parsePrimary.
// The context (whether it's a target or part of one) determines the 'in_target_context' flag.
// This helper is called by parseTarget and parseSingleTarget.
// If t_primary is the *entire* target (e.g. via target_atom -> IDENTIFIER, parsed by atom()),
// then in_target_context should be true.
// If t_primary is the base of an attribute/subscript (e.g. `obj` in `obj.attr`),
// then `obj` can be a call, so in_target_context should be false for parsing `obj`.
// This function itself will call parsePrimary. The key is what `in_target_context` value to pass.
// Let's assume parseTPrimary is for contexts where the result *could* be a target or part of a target.
// For `t_primary TK_PERIOD ID`, the `t_primary` part is `parsePrimary(false)`.
// For `t_primary` that resolves to `atom` which is the final target, it's `parsePrimary(true)`.
// This logic is already in parsePrimary. So parseTPrimary just calls parsePrimary.
// The caller of parseTPrimary needs to know the context.
// For simplicity, let parseTPrimary behave like parsePrimary.
// The callers like parseTarget/parseSingleTarget will ensure the final result is valid.
// A common pattern is: parse a base expression (which could be a call), then add attrs/subs.
// The 'in_target_context' is for when an *atom* itself is the target.
std::unique_ptr<ExpressionNode> Parser::parseTPrimary() {
    // When parseTPrimary is called to get the base of an attribute or subscript,
    // that base can be a function call (e.g., `get_obj().attribute`).
    // So, in_target_context should be `false` for this initial parse.
    // If this t_primary ends up being an atom that is a standalone target (like an identifier),
    // parseAtom(false) would allow `x` but also `call()`.
    // The check for "is it a valid target type" must happen *after* the full t_primary chain is parsed.
    //
    // Let's use `parsePrimary(false)` as the general way to parse `t_primary`'s structure.
    // The calling functions (`parseTarget`, `parseSingleTarget`) are responsible
    // for validating if the resulting ExpressionNode is a valid target.
    // The `in_target_context = true` in `parsePrimary` is primarily for when an `atom` itself must be a target.
    return parsePrimary(false); // Base of a t_primary chain can be a call.
}


// Corresponds to CFG: simplified_star_etc: TK_MULTIPLY param_no_default kwds_opt | kwds
// kwds_opt: kwds | epsilon
// kwds: TK_POWER param_no_default
// param_no_default: param param_ending_char (param: TK_IDENTIFIER)
void Parser::parseSimplifiedStarEtc(ArgumentsNode& args_node_ref) {
    if (match(TokenType::TK_MULTIPLY)) { // *args
        Token star_token = previous();
        if (args_node_ref.vararg) {
            reportError(star_token, "Multiple *args (var-positional) parameters not allowed.");
            // Consume identifier to try to recover
            if (check(TokenType::TK_IDENTIFIER)) consume(TokenType::TK_IDENTIFIER, "Consuming duplicate *args name.");
            return; // Error
        }
        if (!check(TokenType::TK_IDENTIFIER)) {
            reportError(peek(), "Expected identifier for *args parameter name.");
            return;
        }
        Token name_token = consume(TokenType::TK_IDENTIFIER, "Expected identifier for *args name.");
        if (this->had_error) return;
        args_node_ref.vararg = std::make_unique<ParameterNode>(star_token.line, name_token.lexeme, ParameterNode::Kind::VAR_POSITIONAL, nullptr);

        // Check for optional kwds (**kwargs)
        if (check(TokenType::TK_COMMA) && peek(1).type == TokenType::TK_POWER) { // must have comma before **kwargs if *args present
            consume(TokenType::TK_COMMA, "Expected comma before **kwargs after *args."); // Consume comma
            if (this->had_error) return;
        }


        if (match(TokenType::TK_POWER)) { // kwds_opt -> kwds
            Token power_token = previous();
            if (args_node_ref.kwarg) { // Should not happen if logic is right, means **kwarg already parsed
                reportError(power_token, "Multiple **kwargs (var-keyword) parameters not allowed.");
                if (check(TokenType::TK_IDENTIFIER)) consume(TokenType::TK_IDENTIFIER, "Consuming duplicate **kwargs name.");
                return;
            }
            if (!check(TokenType::TK_IDENTIFIER)) {
                reportError(peek(), "Expected identifier for **kwargs parameter name.");
                return;
            }
            Token kw_name_token = consume(TokenType::TK_IDENTIFIER, "Expected identifier for **kwargs name.");
            if (this->had_error) return;
            args_node_ref.kwarg = std::make_unique<ParameterNode>(power_token.line, kw_name_token.lexeme, ParameterNode::Kind::VAR_KEYWORD, nullptr);
        }
    } else if (match(TokenType::TK_POWER)) { // kwds (standalone **kwargs)
        Token power_token = previous();
        if (args_node_ref.kwarg) {
            reportError(power_token, "Multiple **kwargs (var-keyword) parameters not allowed.");
            if (check(TokenType::TK_IDENTIFIER)) consume(TokenType::TK_IDENTIFIER, "Consuming duplicate **kwargs name.");
            return;
        }
        if (!check(TokenType::TK_IDENTIFIER)) {
            reportError(peek(), "Expected identifier for **kwargs parameter name.");
            return;
        }
        Token name_token = consume(TokenType::TK_IDENTIFIER, "Expected identifier for **kwargs name.");
        if (this->had_error) return;
        args_node_ref.kwarg = std::make_unique<ParameterNode>(power_token.line, name_token.lexeme, ParameterNode::Kind::VAR_KEYWORD, nullptr);
    }
    // If neither * nor ** is found, this function does nothing (epsilon part of simplified_star_etc_opt)
}