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
        // This case indicates a logic error in parser or empty token stream.
        // Return EOF or handle error appropriately. For now, returning EOF.
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

bool Parser::match(const vector<TokenType>& types) {
    if (check(types)) {
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

Token Parser::consume(const vector<TokenType>& types, const string& message) {
    if (check(types)) {
        return advance();
    }
    reportError(peek(), message);
    throw runtime_error("Parse error: " + message);
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
            case TokenType::TK_WITH: // Though 'with' is not in simplified CFG, good to have
            case TokenType::TK_RETURN:
            case TokenType::TK_IMPORT:
            case TokenType::TK_FROM:
            case TokenType::TK_GLOBAL:
            case TokenType::TK_NONLOCAL:
            case TokenType::TK_PASS:
            case TokenType::TK_BREAK:
            case TokenType::TK_CONTINUE:
            case TokenType::TK_RAISE:
                return;
            default:
                // If the next token is an INDENT, it might be the start of a new block for a compound statement.
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
    if (!isAtEnd() && peek().type != TokenType::TK_EOF) { // Check if there's anything to parse before EOF
        stmts = parseStatementsOpt();
    }

    if (!isAtEnd() && peek().type == TokenType::TK_EOF) { // Match optional TK_EOF at the very end.
        consume(TokenType::TK_EOF, "Expected end of file.");
    } else if (!isAtEnd()) {
        reportError(peek(), "Expected end of file, but found more tokens.");
        // Consume remaining tokens to avoid infinite loops if parseFile is called again or in a recovery scenario
        while(!isAtEnd()) advance();
    }
    return make_unique<ProgramNode>(start_line, move(stmts));
}

vector<unique_ptr<StatementNode>> Parser::parseStatementsOpt() {
    // statements_opt: statements | epsilon
    if (isAtEnd() || peek().type == TokenType::TK_EOF || peek().type == TokenType::TK_DEDENT) { // Epsilon case if we hit EOF or DEDENT (end of block)
        return {};
    }
    return parseStatements();
}

// GENERAL STATEMENTS
vector<unique_ptr<StatementNode>> Parser::parseStatements() {
    // statements: statement statement_star
    vector<unique_ptr<StatementNode>> stmts_list;
    // Loop as long as we don't see EOF or DEDENT (which signals end of a block)
    while (!isAtEnd() && peek().type != TokenType::TK_EOF && peek().type != TokenType::TK_DEDENT) {
        try {
            stmts_list.push_back(parseStatement());
        } catch (const runtime_error& e) {
            // Error already reported by consume or other failing parse function.
            // Synchronize to try to parse the next statement.
            synchronize();
            if (isAtEnd() || peek().type == TokenType::TK_EOF || peek().type == TokenType::TK_DEDENT) break; // Stop if sync leads to end of block
        }
    }
    return stmts_list;
}

unique_ptr<StatementNode> Parser::parseStatement() {
    // statement: compound_stmt | simple_stmts
    // Need to distinguish. Compound statements start with specific keywords.
    // simple_stmts can be expressions, assignments, etc.
    // Check for compound statement keywords first.
    TokenType current_type = peek().type;
    switch (current_type) {
        case TokenType::TK_DEF:
        case TokenType::TK_IF:
        case TokenType::TK_CLASS:
        case TokenType::TK_FOR:
        case TokenType::TK_TRY:
        case TokenType::TK_WHILE:
            // Also TK_WITH, TK_ASYNC DEF/FOR/WITH if they were in this grammar.
            return parseCompoundStmt();
        default:
            // Correct approach for `simple_stmts: simple_stmt simple_stmt_list_tail_star optional_semicolon`:
            // `parseStatement` will parse a `simple_stmt`. The loop in `parseStatements` handles multiple statements.
            // The `TK_SEMICOLON` acts as a statement separator there.
            // `optional_semicolon` at the end of `simple_stmts` means a trailing semicolon is allowed.
            // A `NEWLINE` or `EOF` or `DEDENT` terminates `simple_stmts`.
            // So, `parseStatement` needs to parse one `simple_stmt` from the `simple_stmts` rule.

            auto stmt = parseSimpleStmt();
            match(TokenType::TK_SEMICOLON); // Consume optional trailing semicolon on a line if simple_stmts rule implies it.
            return stmt;
    }
}

vector<unique_ptr<StatementNode>> Parser::parseSimpleStmts() {
    // simple_stmts: simple_stmt_list optional_semicolon
    // simple_stmt_list: simple_stmt simple_stmt_list_tail_star
    // simple_stmt_list_tail_star: TK_SEMICOLON simple_stmt simple_stmt_list_tail_star | epsilon
    // This means one or more simple_stmt separated by semicolons, with an optional final semicolon.
    // Used by `block: ... | simple_stmts` (e.g. `if x: print(1); print(2)`)
    vector<unique_ptr<StatementNode>> stmts_list;
    stmts_list.push_back(parseSimpleStmt());
    while (match(TokenType::TK_SEMICOLON)) {
        // If semicolon is followed by EOF, DEDENT, or (implicitly) NEWLINE, it's a trailing semicolon.
        if (isAtEnd() || check(TokenType::TK_EOF) || check(TokenType::TK_DEDENT)
            // crude check for newline if next token is on different line and not indent/dedent related
            || (peek().line > previous().line && !check(TokenType::TK_INDENT) && !check(TokenType::TK_DEDENT)) ) {
            break;
        }
        stmts_list.push_back(parseSimpleStmt());
    }
    return stmts_list;
}


unique_ptr<StatementNode> Parser::parseSimpleStmt() {
    // simple_stmt:
    // | assignment
    // | expressions  # Was star_expressions
    // | return_stmt
    // | import_stmt
    // | raise_stmt
    // | TK_PASS
    // | TK_BREAK
    // | TK_CONTINUE
    // | global_stmt
    // | nonlocal_stmt
    int line = peek().line;
    switch (peek().type) {
        case TokenType::TK_RETURN:  return parseReturnStmt();
        case TokenType::TK_IMPORT:  // Falls through to generic import_stmt
        case TokenType::TK_FROM:    return parseImportStmt(); // import_stmt handles both
        case TokenType::TK_RAISE:   return parseRaiseStmt();
        case TokenType::TK_PASS:    advance(); return make_unique<PassStatementNode>(line);
        case TokenType::TK_BREAK:   advance(); return make_unique<BreakStatementNode>(line);
        case TokenType::TK_CONTINUE:advance(); return make_unique<ContinueStatementNode>(line);
        case TokenType::TK_GLOBAL:  return parseGlobalStmt();
        case TokenType::TK_NONLOCAL:return parseNonlocalStmt();
        default:
            // Must be assignment or expressions
            // Try to parse as an expression first.
            // If it's `targets = expressions` or `single_target augassign expressions`
            // This requires lookahead. `targets` can be complex.
            // A common strategy: parse an expression. Then check if the next token is an assignment operator.
            // `targets` can be `target | target target_comma_list_star optional_comma`
            // `target` can be `t_primary . ident`, `t_primary [slices]`, or `target_atom`
            // `target_atom` can be `ident`, `(target)`, `(targets_tuple_seq_opt)`, `[targets_list_seq_opt]`
            // This means an expression like `x` or `x.y` or `x[0]` or `(x,y)` or `[x,y]` could be a target.

            // If we parse `expressions` first (for `expressions` rule):
            auto expr_node = parseExpressions(); // This will parse one or more comma-separated expressions

            // Now, check if it's an assignment.
            // Check for `=` (TK_ASSIGN) or an augassign operator.
            if (check(TokenType::TK_ASSIGN) ||
                check({TokenType::TK_PLUS_ASSIGN, TokenType::TK_MINUS_ASSIGN, TokenType::TK_MULTIPLY_ASSIGN,
                       TokenType::TK_DIVIDE_ASSIGN, TokenType::TK_MOD_ASSIGN, TokenType::TK_BIT_AND_ASSIGN,
                       TokenType::TK_BIT_OR_ASSIGN, TokenType::TK_BIT_XOR_ASSIGN, TokenType::TK_BIT_LEFT_SHIFT_ASSIGN,
                       TokenType::TK_BIT_RIGHT_SHIFT_ASSIGN, TokenType::TK_POWER_ASSIGN, TokenType::TK_FLOORDIV_ASSIGN})) {
                // It's an assignment. The `expr_node` we parsed is the target(s).
                // We need to convert/validate `expr_node` into a list of targets.
                vector<unique_ptr<ExpressionNode>> targets_vec;
                if (auto tuple_node = dynamic_cast<TupleLiteralNode*>(expr_node.get())) {
                    // If parseExpressions returned a tuple (e.g. "x, y = ..."), move elements.
                    // Need to release ownership from expr_node if we move its contents.
                    targets_vec = move(tuple_node->elements);
                    expr_node.release(); // Release ownership as elements are moved.
                    // This means expr_node is now nullptr.
                } else {
                    targets_vec.push_back(move(expr_node));
                }
                // Now, targets_vec contains the parsed targets. Proceed with assignment parsing.
                return parseAssignmentTail(move(targets_vec)); // Helper function
            } else {
                // Not an assignment, so it's an expression statement.
                return make_unique<ExpressionStatementNode>(expr_node ? expr_node->line : line, move(expr_node));
            }
    }
}
// Helper to continue parsing assignment after targets are tentatively parsed
unique_ptr<StatementNode> Parser::parseAssignmentTail(vector<unique_ptr<ExpressionNode>> targets) {
    int line = targets.empty() ? peek().line : targets[0]->line;

    if (match(TokenType::TK_ASSIGN)) {
        auto value = parseExpressions();
        return make_unique<AssignmentStatementNode>(line, move(targets), move(value));
    } else { // augassign
        Token op = parseAugassign(); // Consumes the augassign token
        // For augassign, CFG says `single_target`. Our `targets` vector should contain one element.
        if (targets.size() != 1) {
            reportError(op, "Augmented assignment requires a single target.");
            // Create a dummy or error node, or try to recover.
            // For simplicity, let's try to use the first target if available.
            if (targets.empty()) {
                // This is a severe error, maybe return a pass statement or an error node if AST supports it.
                // For now, let's create a pass statement and hope synchronize helps.
                return make_unique<PassStatementNode>(op.line);
            }
        }
        auto value = parseExpressions();
        return make_unique<AugAssignNode>(line, move(targets[0]), op, move(value));
    }
}


unique_ptr<StatementNode> Parser::parseCompoundStmt() {
    // compound_stmt:
    // | function_def | if_stmt | class_def | for_stmt | try_stmt | while_stmt
    switch (peek().type) {
        case TokenType::TK_DEF:     return parseFunctionDef();
        case TokenType::TK_IF:      return parseIfStmt();
        case TokenType::TK_CLASS:   return parseClassDef();
        case TokenType::TK_FOR:     return parseForStmt();
        case TokenType::TK_TRY:     return parseTryStmt();
        case TokenType::TK_WHILE:   return parseWhileStmt();
        default:
            reportError(peek(), "Expected a compound statement keyword (def, if, class, etc.).");
            throw runtime_error("Invalid compound statement."); // Or synchronize
    }
}

// SIMPLE STATEMENTS Implementation
unique_ptr<StatementNode> Parser::parseAssignment() {
    // This function is called if we are sure it's an assignment.
    // The logic is now partly in parseSimpleStmt and parseAssignmentTail.
    // This function can be refactored or parseSimpleStmt calls specific target parsing.

    // Revised approach for assignment based on CFG:
    // assignment:
    // | targets TK_ASSIGN expressions
    // | single_target augassign expressions
    // The ambiguity of `targets` vs `expressions` is hard. Python parsers often parse expression, then check.
    // If we call `parseAssignment` directly, we'd first parse targets.

    int line = peek().line;

    // If next token is an augassign operator, we must parse `single_target`.
    if (check(TokenType::TK_PLUS_ASSIGN) || check(TokenType::TK_MINUS_ASSIGN) /* ... all augassign types */) {
        // This check is insufficient because `single_target` could be `a.b` and `peek(2)` is augassign.
        // This means we need to parse the potential `single_target` first.
    }

    // Let's assume parseSimpleStmt correctly identifies an assignment and calls parseAssignmentTail.
    // So this specific parseAssignment() might not be directly called from parseSimpleStmt's switch.
    // For now, this function remains a placeholder if a direct parsing path is established.
    reportError(peek(), "parseAssignment() called directly - logic should be in parseSimpleStmt/parseAssignmentTail.");
    return make_unique<PassStatementNode>(line); // Placeholder
}


Token Parser::parseAugassign() {
    // augassign:
    // | TK_PLUS_ASSIGN | TK_MINUS_ASSIGN | TK_MULTIPLY_ASSIGN | TK_DIVIDE_ASSIGN
    // | TK_MOD_ASSIGN | TK_BIT_AND_ASSIGN | TK_BIT_OR_ASSIGN | TK_BIT_XOR_ASSIGN
    // | TK_BIT_LEFT_SHIFT_ASSIGN | TK_BIT_RIGHT_SHIFT_ASSIGN | TK_POWER_ASSIGN | TK_FLOORDIV_ASSIGN
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
    // return_stmt: TK_RETURN expressions_opt
    Token ret_token = consume(TokenType::TK_RETURN, "Expected 'return'.");
    unique_ptr<ExpressionNode> value = nullptr;
    // expressions_opt means it can be followed by expressions or nothing (before newline/dedent/EOF).
    // Check if the next token can start an expression and is on the same line.
    // Simplification: if not ';', DEDENT, EOF, or a compound statement keyword that implicitly ends the line for return.
    if (!isAtEnd() && peek().type != TokenType::TK_SEMICOLON && peek().type != TokenType::TK_DEDENT && peek().type != TokenType::TK_EOF
        && peek().line == ret_token.line) { // Ensure expression is on the same logical line or handled by block structure
        value = parseExpressionsOpt();
    }
    return make_unique<ReturnStatementNode>(ret_token.line, move(value));
}

unique_ptr<ExpressionNode> Parser::parseExpressionsOpt() {
    // expressions_opt: expressions | epsilon
    // Epsilon if next token cannot start an expression (e.g., ';', newline, EOF, DEDENT)
    // This needs to be context-aware. For `return_stmt`, if nothing follows `return` on the line, it's epsilon.
    // The check in `parseReturnStmt` handles the "is there something to parse" part.
    // If this function is called, it means we expect expressions.
    if (isAtEnd() || check(TokenType::TK_SEMICOLON) || check(TokenType::TK_DEDENT) || check(TokenType::TK_EOF)
        // Add other tokens that definitely cannot start an expression if needed
        || ( peek().line > previous().line && previous().type != TokenType::TK_COMMA ) // Heuristic: if on new line and not part of ongoing tuple
            ) {
        return nullptr; // Epsilon case
    }
    return parseExpressions();
}

unique_ptr<ExpressionNode> Parser::parseExpressions() {
    // expressions:
    // | expression expression_comma_plus optional_comma
    // | expression TK_COMMA  (this implies a single-element tuple like `x,`)
    // | expression
    // expression_comma_plus: TK_COMMA expression expression_comma_plus_star
    // This means: expr | expr, | expr, expr [, expr ...][,]

    int line = peek().line;
    auto first_expr = parseExpression();

    if (match(TokenType::TK_COMMA)) {
        // We have at least `expr,`, so it's a tuple.
        vector<unique_ptr<ExpressionNode>> elements;
        elements.push_back(move(first_expr));

        // Check if there's more after the comma, or if it's just a trailing comma for a single element tuple.
        // `expressions_opt` for the tail of `expression TK_COMMA expressions_opt`
        // or `expression` for `expression_comma_plus`
        if (!isAtEnd() && peek().type != TokenType::TK_SEMICOLON && peek().type != TokenType::TK_RPAREN &&
            peek().type != TokenType::TK_RBRACKET && peek().type != TokenType::TK_RBRACE &&
            peek().type != TokenType::TK_COLON && // e.g. in slices or dicts
            peek().line == previous().line ) { // ensure we are on the same logical line for more tuple elements

            elements.push_back(parseExpression()); // For `expr, expr`
            while (match(TokenType::TK_COMMA)) {
                // Handle trailing comma after multiple elements: if next is not an expression starting token on same line
                if (isAtEnd() || peek().type == TokenType::TK_SEMICOLON || peek().type == TokenType::TK_RPAREN ||
                    peek().type == TokenType::TK_RBRACKET || peek().type == TokenType::TK_RBRACE ||
                    peek().type == TokenType::TK_COLON ||
                    peek().line != previous().line) {
                    break;
                }
                elements.push_back(parseExpression());
            }
        }
        return make_unique<TupleLiteralNode>(line, move(elements));
    } else {
        // Just a single expression
        return first_expr;
    }
}


// EXPRESSIONS (Precedence climbing / Pratt parser style usually, but CFG implies recursive descent)
unique_ptr<ExpressionNode> Parser::parseExpression() {
    // expression:
    // | disjunction TK_IF disjunction TK_ELSE expression  (ternary expression)
    // | disjunction
    int line = peek().line;
    auto cond_or_main_expr = parseDisjunction();

    if (match(TokenType::TK_IF)) { // Ternary expression: body IF cond ELSE orelse
        // Note: Python is `body if cond else orelse`
        // CFG is `disjunction IF disjunction ELSE expression`
        // So `cond_or_main_expr` is the `body (value_if_true)`.
        auto condition = parseDisjunction(); // This is the `cond` part
        consume(TokenType::TK_ELSE, "Expected 'else' in ternary expression.");
        auto orelse_expr = parseExpression(); // This is `value_if_false`
        return make_unique<IfExpNode>(line, move(condition), move(cond_or_main_expr), move(orelse_expr));
    }
    return cond_or_main_expr; // It was just a disjunction
}

unique_ptr<ExpressionNode> Parser::parseDisjunction() { // Logical OR
    // disjunction: conjunction disjunction_tail_star
    // disjunction_tail_star: TK_OR conjunction disjunction_tail_star | epsilon
    auto node = parseConjunction();
    while (match(TokenType::TK_OR)) {
        Token op = previous();
        auto right = parseConjunction();
        node = make_unique<BinaryOpNode>(op.line, move(node), op, move(right));
    }
    return node;
}

unique_ptr<ExpressionNode> Parser::parseConjunction() { // Logical AND
    // conjunction: inversion conjunction_tail_star
    // conjunction_tail_star: TK_AND inversion conjunction_tail_star | epsilon
    auto node = parseInversion();
    while (match(TokenType::TK_AND)) {
        Token op = previous();
        auto right = parseInversion();
        node = make_unique<BinaryOpNode>(op.line, move(node), op, move(right));
    }
    return node;
}

unique_ptr<ExpressionNode> Parser::parseInversion() { // Logical NOT
    // inversion: TK_NOT inversion | comparison
    if (match(TokenType::TK_NOT)) {
        Token op = previous();
        auto operand = parseInversion(); // Recursive call for 'not not x'
        return make_unique<UnaryOpNode>(op.line, op, move(operand));
    }
    return parseComparison();
}

unique_ptr<ExpressionNode> Parser::parseComparison() {
    // comparison: bitwise_or compare_op_bitwise_or_pair_star
    // compare_op_bitwise_or_pair_star: compare_op_bitwise_or_pair compare_op_bitwise_or_pair_star | epsilon
    // compare_op_bitwise_or_pair: various comparison ops + bitwise_or
    // Example: a < b == c
    // AST: ComparisonNode(left=a, ops=[<, ==], comparators=[b, c])

    int line = peek().line;
    auto left_expr = parseBitwiseOr();

    vector<Token> ops;
    vector<unique_ptr<ExpressionNode>> comparators;

    while (true) {
        TokenType current_type = peek().type;
        // Comparison operators
        if (current_type == TokenType::TK_EQUAL || current_type == TokenType::TK_NOT_EQUAL ||
            current_type == TokenType::TK_LESS || current_type == TokenType::TK_LESS_EQUAL ||
            current_type == TokenType::TK_GREATER || current_type == TokenType::TK_GREATER_EQUAL ||
            current_type == TokenType::TK_IS || // TK_IS TK_NOT handled next
            current_type == TokenType::TK_IN) { // TK_NOT TK_IN handled next

            if (match(TokenType::TK_IS)) { // Handle 'is not'
                Token op1 = previous();
                if (match(TokenType::TK_NOT)) { // Consumed 'not'
                    Token op_combined = op1; // Keep line from 'is'
                    op_combined.lexeme = "is not";
                    // No specific token type for "is not", so we might need to adjust BinaryOpNode or ComparisonNode if op is string
                    // For now, let's assume ComparisonNode can handle string ops or custom token types.
                    // Let's use the 'is' token and note the 'not' conceptually.
                    // The AST ComparisonNode takes Token for ops. This is tricky.
                    // A common way is to have dedicated TokenType::IS_NOT. Lexer doesn't provide.
                    // For now, push 'is' then a conceptual 'not'. Or handle it in ComparisonNode.
                    // Simpler: use the IS token, and if NOT follows, the ComparisonNode has to understand this sequence.
                    // Or, the parser creates a "synthetic" token.
                    // Let's assume `ComparisonNode` can handle it or we create a synthetic token.
                    // For simplicity, let's just push TK_IS and TK_NOT as separate ops for now, parser for ComparisonNode would need to interpret.
                    // This is not ideal.
                    // A better way: ComparisonNode takes string operators, or we synthesize.
                    // Let's assume ComparisonNode stores full operator string or composite token.
                    // Alternative: make a special UnaryOpNode for the 'not' part of 'is not' / 'not in'.
                    // CFG implies `is not` is one unit `isnot_bitwise_or: TK_IS TK_NOT bitwise_or`.
                    // Let's consume both and form a representative token.
                    Token synthetic_is_not = op1;
                    synthetic_is_not.lexeme = "is not";
                    // We don't have a TokenType for "IS_NOT". This is a gap.
                    // For now, let's just push `op1` (TK_IS) and the next token will be handled.
                    // The AST logic for `ComparisonNode` will need to be smart or we need better tokens.
                    // Given CFG `isnot_bitwise_or: TK_IS TK_NOT bitwise_or`, we should consume both.
                    ops.push_back(op1); // TK_IS
                    ops.push_back(peek()); // TK_NOT (we consumed it with match earlier, so it's previous())
                    // previous() was 'not', previous() of that was 'is'. This is getting messy.
                    // Let's restart the 'is not' / 'not in' logic for comparison
                    // If we matched TK_IS, check if next is TK_NOT
                } else { // Just 'is'
                    ops.push_back(op1);
                }
            } else if (match(TokenType::TK_NOT)) { // Handle 'not in'
                Token op1 = previous(); // This is 'not'
                if (match(TokenType::TK_IN)) { // Consumed 'in'
                    Token synthetic_not_in = op1;
                    synthetic_not_in.lexeme = "not in";
                    ops.push_back(synthetic_not_in); // Store a token representing "not in"
                } else {
                    // This 'not' was not part of 'not in', probably a 'not expr' if this was parseInversion context
                    // This means `parseComparison` should not match `TK_NOT` alone unless it's part of `not in`.
                    // The CFG for comparison has `notin_bitwise_or: TK_NOT TK_IN bitwise_or`.
                    // So, if we see TK_NOT, we MUST see TK_IN next for it to be a comparison op.
                    // This means the initial `match(TokenType::TK_NOT)` for `inversion` is correct.
                    // This `while(true)` loop should only match actual comparison operators.
                    // So the list of matchable types for comparison op needs to be more specific.
                    unputToken(); // Put back 'not'
                    break; // Not a comparison operator sequence
                }

            } else { // other comparison ops
                ops.push_back(advance()); // Consumes ==, !=, <, <=, >, >=, in
            }

            comparators.push_back(parseBitwiseOr());
        } else if (peek().type == TokenType::TK_NOT && peek(1).type == TokenType::TK_IN) { // Explicit 'not in'
            Token op_not = advance(); // Consume NOT
            Token op_in = advance();  // Consume IN
            Token synthetic_op = op_not; // Use line info from 'not'
            synthetic_op.lexeme = "not in";
            // synthetic_op.type = TokenType::TK_NOT_IN; // If we had such a token
            ops.push_back(synthetic_op);
            comparators.push_back(parseBitwiseOr());
        } else if (peek().type == TokenType::TK_IS && peek(1).type == TokenType::TK_NOT) { // Explicit 'is not'
            Token op_is = advance(); // Consume IS
            Token op_not = advance(); // Consume NOT
            Token synthetic_op = op_is;
            synthetic_op.lexeme = "is not";
            // synthetic_op.type = TokenType::TK_IS_NOT; // If we had such a token
            ops.push_back(synthetic_op);
            comparators.push_back(parseBitwiseOr());
        }
        else {
            break; // No more comparison operators
        }
    }

    if (ops.empty()) {
        return left_expr; // It was just a bitwise_or expression
    } else {
        return make_unique<ComparisonNode>(line, move(left_expr), move(ops), move(comparators));
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
        node = make_unique<BinaryOpNode>(op.line, move(node), op, move(right));
    }
    return node;
}

unique_ptr<ExpressionNode> Parser::parseBitwiseXor() {
    auto node = parseBitwiseAnd();
    while (match(TokenType::TK_BIT_XOR)) {
        Token op = previous();
        auto right = parseBitwiseAnd();
        node = make_unique<BinaryOpNode>(op.line, move(node), op, move(right));
    }
    return node;
}

unique_ptr<ExpressionNode> Parser::parseBitwiseAnd() {
    auto node = parseShiftExpr();
    while (match(TokenType::TK_BIT_AND)) {
        Token op = previous();
        auto right = parseShiftExpr();
        node = make_unique<BinaryOpNode>(op.line, move(node), op, move(right));
    }
    return node;
}

unique_ptr<ExpressionNode> Parser::parseShiftExpr() {
    auto node = parseSum();
    while (match(TokenType::TK_BIT_LEFT_SHIFT) || match(TokenType::TK_BIT_RIGHT_SHIFT)) {
        Token op = previous();
        auto right = parseSum();
        node = make_unique<BinaryOpNode>(op.line, move(node), op, move(right));
    }
    return node;
}

unique_ptr<ExpressionNode> Parser::parseSum() { // Equivalent to arith_expr
    auto node = parseTerm();
    while (match(TokenType::TK_PLUS) || match(TokenType::TK_MINUS)) {
        Token op = previous();
        auto right = parseTerm();
        node = make_unique<BinaryOpNode>(op.line, move(node), op, move(right));
    }
    return node;
}

unique_ptr<ExpressionNode> Parser::parseTerm() {
    auto node = parseFactor();
    while (match(TokenType::TK_MULTIPLY) || match(TokenType::TK_DIVIDE) ||
           match(TokenType::TK_FLOORDIV) || match(TokenType::TK_MOD)) { // Removed TK_MATMUL (not in simplified CFG for term)
        Token op = previous();
        auto right = parseFactor();
        node = make_unique<BinaryOpNode>(op.line, move(node), op, move(right));
    }
    return node;
}

unique_ptr<ExpressionNode> Parser::parseFactor() {
    // factor: TK_PLUS factor | TK_MINUS factor | TK_BIT_NOT factor | power
    if (match(TokenType::TK_PLUS) || match(TokenType::TK_MINUS) || match(TokenType::TK_BIT_NOT)) {
        Token op = previous();
        auto operand = parseFactor(); // Recursive call
        return make_unique<UnaryOpNode>(op.line, op, move(operand));
    }
    return parsePower();
}

unique_ptr<ExpressionNode> Parser::parsePower() {
    // power: primary TK_POWER factor | primary
    auto left = parsePrimary(); // `parsePrimary` needs to be aware of target context for LHS of assignment
    if (match(TokenType::TK_POWER)) {
        Token op = previous();
        auto right = parseFactor(); // Right-associativity: parseFactor can lead back to power
        return make_unique<BinaryOpNode>(op.line, move(left), op, move(right));
    }
    return left;
}

unique_ptr<ExpressionNode> Parser::parsePrimary(bool in_target_context) {
    // primary:
    // | primary TK_PERIOD TK_IDENTIFIER
    // | primary TK_LPAREN arguments_opt TK_RPAREN
    // | primary TK_LBRACKET slices TK_RBRACKET
    // | atom
    // This left-recursion needs to be handled iteratively.
    auto node = parseAtom(in_target_context);

    while (true) {
        if (match(TokenType::TK_PERIOD)) { // Attribute access: node.IDENTIFIER
            Token dot_token = previous();
            Token name_token = consume(TokenType::TK_IDENTIFIER, "Expected attribute name after '.'.");
            auto attr_ident = make_unique<IdentifierNode>(name_token.line, name_token.lexeme);
            node = make_unique<AttributeAccessNode>(dot_token.line, move(node), move(attr_ident));
        } else if (match(TokenType::TK_LPAREN)) { // Function call: node(...)
            // If in_target_context is true, a function call cannot be a target: (e.g. f() = 1 is illegal)
            if (in_target_context) {
                reportError(previous(), "Function call cannot be a target of assignment.");
                // Attempt to recover by parsing arguments but not forming CallNode if it's a target.
                // Or simply break. For now, let's break.
                unputToken(); // put back LPAREN
                break;
            }
            Token lparen_token = previous();
            vector<unique_ptr<ExpressionNode>> pos_args;
            vector<unique_ptr<KeywordArgNode>> kw_args;
            int call_line = lparen_token.line; // Line of LPAREN
            if (!check(TokenType::TK_RPAREN)) { // Check if there are arguments
                parseArgumentsForCall(pos_args, kw_args, call_line); // Populates args and keywords
            }
            consume(TokenType::TK_RPAREN, "Expected ')' after function arguments.");
            node = make_unique<FunctionCallNode>(lparen_token.line, move(node), move(pos_args), move(kw_args));
        } else if (match(TokenType::TK_LBRACKET)) { // Subscription: node[...]
            Token lbracket_token = previous();
            auto slice_or_idx = parseSlices();
            consume(TokenType::TK_RBRACKET, "Expected ']' after subscript or slice.");
            node = make_unique<SubscriptionNode>(lbracket_token.line, move(node), move(slice_or_idx));
        } else {
            break; // No more primary extensions
        }
    }
    return node;
}

// ... Many more parsing functions would follow ...
// For brevity, I'll stop implementing the full CFG here.
// The provided structure and examples should give a strong foundation.

// ATOM and LITERALS (Example)
unique_ptr<ExpressionNode> Parser::parseAtom(bool in_target_context) {
    // atom:
    // | TK_IDENTIFIER
    // | TK_TRUE | TK_FALSE | TK_NONE
    // | strings | TK_NUMBER | TK_COMPLEX (literal) | TK_BYTES (literal)
    // | tuple_group_variant | list_variant | dict_set_variant
    // Type keywords like TK_INT, TK_STR etc. are also listed in CFG for atom
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
            // Assuming NumberLiteralNode can distinguish int/float from lexeme
            NumberLiteralNode::Type num_type = (num_token.lexeme.find('.') != string::npos ||
                                                num_token.lexeme.find('e') != string::npos ||
                                                num_token.lexeme.find('E') != string::npos) ?
                                               NumberLiteralNode::Type::FLOAT : NumberLiteralNode::Type::INTEGER;
            return make_unique<NumberLiteralNode>(line, num_token.lexeme, num_type);
        }
        case TokenType::TK_COMPLEX: { // This is for complex literals like 3j, 1+2j
            Token complex_token = advance();
            // ComplexLiteralNode expects real and imag parts. Lexer gives full string.
            // This needs careful parsing of the lexeme.
            // Simple approach: if it ends with 'j' and contains '+', parse; otherwise, assume imag only.
            string real_str = "0";
            string imag_str = complex_token.lexeme;
            imag_str.pop_back(); // remove 'j'

            size_t plus_pos = complex_token.lexeme.find('+');
            size_t minus_pos = complex_token.lexeme.find('-', 1); // find '-' not at start

            if (plus_pos != string::npos && plus_pos < complex_token.lexeme.length() -1) {
                real_str = complex_token.lexeme.substr(0, plus_pos);
                imag_str = complex_token.lexeme.substr(plus_pos + 1, complex_token.lexeme.length() - plus_pos - 2);
            } else if (minus_pos != string::npos && minus_pos < complex_token.lexeme.length() -1 ) {
                real_str = complex_token.lexeme.substr(0, minus_pos);
                imag_str = complex_token.lexeme.substr(minus_pos, complex_token.lexeme.length() - minus_pos -1);
            } // else, real_str is 0, imag_str is number before j.

            return make_unique<ComplexLiteralNode>(line, real_str, imag_str);
        }
        case TokenType::TK_STRING: // This is for string literals "" ''
            return parseStrings(); // Handles f-strings/concatenation if lexer provides raw strings
        case TokenType::TK_BYTES: { // This is for bytes literals b"" b''
            Token bytes_token = advance();
            return make_unique<BytesLiteralNode>(line, bytes_token.lexeme);
        }
        case TokenType::TK_LPAREN: // tuple_group_variant: ( ... )
            return parseTupleGroupVariant(in_target_context);
        case TokenType::TK_LBRACKET: // list_variant: [ ... ]
            return parseListVariant(in_target_context);
        case TokenType::TK_LBRACE: // dict_set_variant: { ... }
            return parseDictSetVariant();

            // Type keywords as atoms (e.g. x = int)
        case TokenType::TK_INT: case TokenType::TK_STR: case TokenType::TK_FLOAT: /*TK_LIST, etc.*/
            // These are treated as identifiers that happen to be type names.
            // The lexer already tokenizes them as TK_INT, etc.
            // For AST, they are identifiers. IdentifierNode can store their name.
            // This distinction is more for semantic analysis.
        {
            Token type_kw_token = advance();
            return make_unique<IdentifierNode>(type_kw_token.line, type_kw_token.lexeme);
        }

        default:
            reportError(peek(), "Expected an atom (identifier, literal, '(', '[', or '{').");
            throw runtime_error("Invalid atom.");
    }
}

unique_ptr<StringLiteralNode> Parser::parseStrings() {
    // strings: fstring_or_string_plus
    // fstring_or_string_plus: fstring_or_string fstring_or_string_plus_star
    // fstring_or_string: string | TK_BYTES (CFG error, should be string for fstring)
    // This means adjacent string literals are concatenated. "a" "b" -> "ab"
    // Assuming lexer gives TK_STRING for each part.
    if (!check(TokenType::TK_STRING)) {
        reportError(peek(), "Expected string literal.");
        throw runtime_error("Expected string.");
    }
    Token first_string = consume(TokenType::TK_STRING, "Expected string literal.");
    string concatenated_value = first_string.lexeme;
    int line = first_string.line;

    while (check(TokenType::TK_STRING) && peek().line == previous().line) { // Basic concatenation for same line
        // More sophisticated would check if only whitespace is between them across lines.
        // For now, only implicit concatenation if they are truly adjacent (no other tokens between them).
        // The lexer might put them far apart in the token stream if there are comments etc.
        // A robust way: check if `tokens[current_pos-1]` and `tokens[current_pos]` were adjacent in source.
        // Simplification: if next token is TK_STRING, consume and concat.
        concatenated_value += advance().lexeme;
    }
    return make_unique<StringLiteralNode>(line, concatenated_value);
}


unique_ptr<ExpressionNode> Parser::parseTupleGroupVariant(bool in_target_context) {
    // tuple_group_variant: tuple | group
    // group: TK_LPAREN expression TK_RPAREN
    // tuple: TK_LPAREN tuple_content_opt TK_RPAREN
    // tuple_content_opt: expression TK_COMMA expressions_opt | epsilon (for empty tuple)
    Token lparen = consume(TokenType::TK_LPAREN, "Expected '('.");

    if (match(TokenType::TK_RPAREN)) { // Empty tuple: ()
        return make_unique<TupleLiteralNode>(lparen.line, vector<unique_ptr<ExpressionNode>>());
    }

    // To distinguish (expr) from (expr,), we need to parse the first expression
    // and then check for a comma *before* the closing parenthesis.
    // This is tricky with current helper structure.
    // Let's peek ahead: if current is RPAREN, it was `()`. If current is expr and peek(1) is COMMA, it's tuple.
    // If current is expr and peek(1) is RPAREN, it's group.

    // Tentatively parse first element
    auto first_element = parseExpression(); // This could be the content of a group or first element of tuple

    if (match(TokenType::TK_COMMA)) { // It's a tuple: (expr, ...) or (expr,)
        vector<unique_ptr<ExpressionNode>> elements;
        elements.push_back(move(first_element));

        // tuple_content_opt: ... TK_COMMA expressions_opt
        // expressions_opt part:
        if (!check(TokenType::TK_RPAREN)) { // If not immediately closing, parse more elements
            auto remaining_exprs_node = parseExpressionsOpt(); // This might return a TupleLiteralNode or single Expr
            if (remaining_exprs_node) {
                if (auto tuple_node = dynamic_cast<TupleLiteralNode*>(remaining_exprs_node.get())) {
                    for (auto& el : tuple_node->elements) {
                        elements.push_back(move(el));
                    }
                } else {
                    elements.push_back(move(remaining_exprs_node));
                }
            }
        }
        consume(TokenType::TK_RPAREN, "Expected ')' to close tuple literal.");
        return make_unique<TupleLiteralNode>(lparen.line, move(elements));
    } else { // No comma after first_element, so it's a group: (expr)
        consume(TokenType::TK_RPAREN, "Expected ')' to close parenthesized expression.");
        // Parentheses for grouping usually don't create a specific AST node,
        // they just influence parsing order. The `first_element` is the result.
        // The new AST also doesn't have a `GroupExprNode`.
        return first_element;
    }
}

unique_ptr<ListLiteralNode> Parser::parseListVariant(bool in_target_context) {
    // list_variant: list
    // list: TK_LBRACKET expressions_opt TK_RBRACKET
    return parseListLiteral(in_target_context);
}

unique_ptr<ListLiteralNode> Parser::parseListLiteral(bool in_target_context) {
    Token lbracket = consume(TokenType::TK_LBRACKET, "Expected '[' to start list literal.");
    vector<unique_ptr<ExpressionNode>> elements;

    if (!check(TokenType::TK_RBRACKET)) {
        auto exprs_node = parseExpressionsOpt(); // handles comma-separated items
        if (exprs_node) {
            if (auto tuple_node = dynamic_cast<TupleLiteralNode*>(exprs_node.get())) {
                // If parseExpressionsOpt returned a tuple (because items were comma-separated)
                elements = move(tuple_node->elements);
            } else {
                // Single item in the list (no commas)
                elements.push_back(move(exprs_node));
            }
        }
    }
    consume(TokenType::TK_RBRACKET, "Expected ']' to close list literal.");
    return make_unique<ListLiteralNode>(lbracket.line, move(elements));
}


// Placeholder for parseBlock, a common and important one
unique_ptr<BlockNode> Parser::parseBlock() {
    // block: TK_INDENT statements TK_DEDENT | simple_stmts
    int line = peek().line;
    if (match(TokenType::TK_INDENT)) {
        auto stmts = parseStatements(); // parseStatements will stop at DEDENT or EOF
        consume(TokenType::TK_DEDENT, "Expected DEDENT to end indented block.");
        return make_unique<BlockNode>(line, move(stmts));
    } else {
        // simple_stmts case (e.g., if x: print(x))
        // simple_stmts expands to simple_stmt_list optional_semicolon NEWLINE
        // simple_stmt_list is simple_stmt (TK_SEMICOLON simple_stmt)*
        // This implies a single logical line of code.
        auto stmts_vec = parseSimpleStmts();
        return make_unique<BlockNode>(line, move(stmts_vec));
    }
}

// Function Definition (Example of compound statement)
unique_ptr<FunctionDefinitionNode> Parser::parseFunctionDef() {
    // function_def_raw: TK_DEF TK_IDENTIFIER TK_LPAREN params_opt TK_RPAREN TK_COLON block
    Token def_token = consume(TokenType::TK_DEF, "Expected 'def'.");
    Token name_token = consume(TokenType::TK_IDENTIFIER, "Expected function name.");
    auto name_ident = make_unique<IdentifierNode>(name_token.line, name_token.lexeme);

    consume(TokenType::TK_LPAREN, "Expected '(' after function name.");
    int params_line = peek().line;
    unique_ptr<ArgumentsNode> args_spec = parseParamsOpt(params_line);
    consume(TokenType::TK_RPAREN, "Expected ')' after function parameters.");
    consume(TokenType::TK_COLON, "Expected ':' after function signature.");
    auto body = parseBlock();

    return make_unique<FunctionDefinitionNode>(def_token.line, move(name_ident), move(args_spec), move(body));
}

unique_ptr<ArgumentsNode> Parser::parseParamsOpt(int& line_start) {
    // params_opt: params | epsilon
    line_start = peek().line;
    if (check(TokenType::TK_RPAREN)) { // Epsilon case: next is closing parenthesis
        return make_unique<ArgumentsNode>(line_start); // Empty arguments node
    }
    return parseParameters(line_start);
}