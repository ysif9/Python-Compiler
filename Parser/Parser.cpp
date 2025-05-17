#include "Parser.hpp"

#include <filesystem>

#include "tempAST.hpp"
#include <stdexcept>
#include <vector>
#include <string>
#include <memory>
#include <utility>

using namespace std;

Parser::Parser(Lexer& lexer_instance)
        : lexer_ref(lexer_instance), current_pos(0), had_error(false) {
    Token t = lexer_ref.nextToken();
    while(t.type != TokenType::TK_EOF) {
        if (lexer_ref.getErrors().size() > errors_list.size()) {
            for(size_t i = errors_list.size(); i < lexer_ref.getErrors().size(); ++i) {
                errors_list.push_back("Lexer Error: " + lexer_ref.getErrors()[i].message + " on line " + to_string(lexer_ref.getErrors()[i].line) + " near '" + lexer_ref.getErrors()[i].lexeme + "'");
            }
            had_error = true;
        }
        t = lexer_ref.nextToken();
    }
    if (lexer_ref.tokens.empty() || lexer_ref.tokens.back().type != TokenType::TK_EOF) {
        lexer_ref.tokens.push_back({TokenType::TK_EOF, "", lexer_ref.tokens.empty() ? 1 : lexer_ref.tokens.back().line, TokenCategory::EOFILE});
    }

    lexer_ref.processIdentifierTypes();
    this->tokens = lexer_ref.tokens;

    if (had_error) {
        this->tokens.clear();
        this->tokens.push_back({TokenType::TK_EOF, "", 0, TokenCategory::EOFILE});
    }
}

shared_ptr<AstNode> Parser::parse() {
    if (tokens.empty() || (tokens.size() == 1 && tokens[0].type == TokenType::TK_EOF && had_error)) {
        if (!had_error) reportError(tokens.empty() ? Token{TokenType::TK_EOF, "", 0, TokenCategory::EOFILE} : tokens[0], "No tokens to parse.");
        return make_shared<AstNode>(NodeType::ProgramNode, Token{TokenType::TK_EOF, "", 0, TokenCategory::EOFILE});
    }
    current_pos = 0;
    auto module = parseFile();
    saveDotFile(generateDot(module), "AST.dot");
    return module;
}

Token& Parser::peek(int offset) {
    if (current_pos + offset >= tokens.size()) {
        return tokens.back().type == TokenType::TK_EOF ? tokens.back() : tokens.emplace_back(TokenType::TK_EOF, "", tokens.empty() ? 1 : tokens.back().line, TokenCategory::EOFILE);
    }
    return tokens[current_pos + offset];
}

Token& Parser::previous() {
    if (current_pos == 0 || current_pos > tokens.size()) {
        return tokens.back().type == TokenType::TK_EOF ? tokens.back() : tokens.emplace_back(TokenType::TK_EOF, "", tokens.empty() ? 1 : tokens.back().line, TokenCategory::EOFILE);
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
    if (isAtEnd()) return;
    advance();

    while (!isAtEnd()) {
        if (previous().type == TokenType::TK_SEMICOLON) return;
        if (peek().type == TokenType::TK_DEDENT) return;

        switch (peek().type) {
            case TokenType::TK_CLASS:
            case TokenType::TK_DEF:
            case TokenType::TK_IF:
            case TokenType::TK_FOR:
            case TokenType::TK_WHILE:
            case TokenType::TK_TRY:
            case TokenType::TK_WITH:
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
                if (peek().type == TokenType::TK_INDENT) return;
        }
        advance();
    }
}

shared_ptr<AstNode> Parser::parseFile() {
    int start_line = tokens.empty() ? 0 : tokens[0].line;
    auto program_node = make_shared<AstNode>(NodeType::ProgramNode, Token{TokenType::TK_EOF, "", start_line, TokenCategory::EOFILE});
    if (!isAtEnd() && peek().type != TokenType::TK_EOF) {
        program_node->children = parseStatementsOpt();
    }

    if (!isAtEnd() && peek().type == TokenType::TK_EOF) {
        consume(TokenType::TK_EOF, "Expected end of file.");
    } else if (!isAtEnd()) {
        reportError(peek(), "Expected end of file, but found more tokens.");
        while(!isAtEnd()) advance();
    }
    return program_node;
}

vector<shared_ptr<AstNode>> Parser::parseStatementsOpt() {
    if (isAtEnd() || peek().type == TokenType::TK_EOF || peek().type == TokenType::TK_DEDENT) {
        return {};
    }
    return parseStatements();
}

vector<shared_ptr<AstNode>> Parser::parseStatements() {
    vector<shared_ptr<AstNode>> stmts_list;
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

shared_ptr<AstNode> Parser::parseStatement() {
    TokenType current_type = peek().type;
    switch (current_type) {
        case TokenType::TK_DEF:
        case TokenType::TK_IF:
        case TokenType::TK_CLASS:
        case TokenType::TK_FOR:
        case TokenType::TK_TRY:
        case TokenType::TK_WHILE:
            return parseCompoundStmt();
        default:
            auto stmt = parseSimpleStmt();
            match(TokenType::TK_SEMICOLON);
            return stmt;
    }
}

vector<shared_ptr<AstNode>> Parser::parseSimpleStmts() {
    vector<shared_ptr<AstNode>> stmts_list;
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

shared_ptr<AstNode> Parser::parseSimpleStmt() {
    int line = peek().line;

    switch (peek().type) {
        case TokenType::TK_RETURN:  return parseReturnStmt();
        case TokenType::TK_IMPORT:  return parseImportStatement();
        case TokenType::TK_RAISE:   return parseRaiseStmt();
        case TokenType::TK_PASS:    advance(); return make_shared<AstNode>(NodeType::PassStatementNode, Token{TokenType::TK_PASS, "pass", line, TokenCategory::KEYWORD});
        case TokenType::TK_BREAK:   advance(); return make_shared<AstNode>(NodeType::BreakStatementNode, Token{TokenType::TK_BREAK, "break", line, TokenCategory::KEYWORD});
        case TokenType::TK_CONTINUE:advance(); return make_shared<AstNode>(NodeType::ContinueStatementNode, Token{TokenType::TK_CONTINUE, "continue", line, TokenCategory::KEYWORD});
        case TokenType::TK_GLOBAL:  return parseGlobalStmt();
        case TokenType::TK_NONLOCAL:return parseNonlocalStmt();
        default:
            break;
    }

    size_t initial_pos = current_pos;
    bool initial_had_error_flag = had_error;
    size_t initial_errors_count = errors_list.size();

    vector<shared_ptr<AstNode>> potential_targets = parseTargets();

    if (!had_error && !potential_targets.empty()) {
        if (check(TokenType::TK_ASSIGN)) {
            consume(TokenType::TK_ASSIGN, "Expected '=' for assignment.");
            if (had_error) {
                return make_shared<AstNode>(NodeType::PassStatementNode, Token{TokenType::TK_PASS, "pass", line, TokenCategory::KEYWORD});
            }
            auto rhs_value = parseExpressions();
            if (had_error && !rhs_value) {
                return make_shared<AstNode>(NodeType::PassStatementNode, Token{TokenType::TK_PASS, "pass", line, TokenCategory::KEYWORD});
            }
            auto assignment_node = make_shared<AstNode>(NodeType::AssignmentStatementNode, Token{TokenType::TK_ASSIGN, "=", line, TokenCategory::OPERATOR});
            assignment_node->children = std::move(potential_targets);
            assignment_node->children.push_back(rhs_value);
            return assignment_node;
        }
    }

    current_pos = initial_pos;
    if (errors_list.size() > initial_errors_count) {
        errors_list.resize(initial_errors_count);
    }
    had_error = initial_had_error_flag;

    initial_pos = current_pos;
    initial_had_error_flag = had_error;
    initial_errors_count = errors_list.size();

    shared_ptr<AstNode> potential_single_target = parseSingleTarget();

    if (!had_error && potential_single_target) {
        if (check({TokenType::TK_PLUS_ASSIGN, TokenType::TK_MINUS_ASSIGN, TokenType::TK_MULTIPLY_ASSIGN,
                   TokenType::TK_DIVIDE_ASSIGN, TokenType::TK_MOD_ASSIGN, TokenType::TK_BIT_AND_ASSIGN,
                   TokenType::TK_BIT_OR_ASSIGN, TokenType::TK_BIT_XOR_ASSIGN, TokenType::TK_BIT_LEFT_SHIFT_ASSIGN,
                   TokenType::TK_BIT_RIGHT_SHIFT_ASSIGN, TokenType::TK_POWER_ASSIGN, TokenType::TK_FLOORDIV_ASSIGN})) {
            Token op = parseAugassign();
            if (had_error) {
                return make_shared<AstNode>(NodeType::PassStatementNode, Token{TokenType::TK_PASS, "pass", line, TokenCategory::KEYWORD});
            }
            auto rhs_value = parseExpressions();
            if (had_error && !rhs_value) {
                return make_shared<AstNode>(NodeType::PassStatementNode, Token{TokenType::TK_PASS, "pass", line, TokenCategory::KEYWORD});
            }
            auto augassign_node = make_shared<AstNode>(NodeType::AugAssignNode, op);
            augassign_node->children.push_back(std::move(potential_single_target));
            augassign_node->children.push_back(rhs_value);
            return augassign_node;
        }
    }

    current_pos = initial_pos;
    if (errors_list.size() > initial_errors_count) {
        errors_list.resize(initial_errors_count);
    }
    had_error = initial_had_error_flag;

    auto expr_stmt_expr = parseExpressions();

    if (had_error && !expr_stmt_expr) {
        if (!had_error) reportError(peek(), "Expected an expression or assignment.");
        return make_shared<AstNode>(NodeType::PassStatementNode, Token{TokenType::TK_PASS, "pass", line, TokenCategory::KEYWORD});
    }
    auto expr_stmt_node = make_shared<AstNode>(NodeType::ExpressionStatementNode, Token{TokenType::TK_EOF, "", line, TokenCategory::EOFILE});
    expr_stmt_node->children.push_back(std::move(expr_stmt_expr));
    return expr_stmt_node;
}

shared_ptr<AstNode> Parser::parseCompoundStmt() {
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

shared_ptr<AstNode> Parser::parseReturnStmt() {
    Token ret_token = consume(TokenType::TK_RETURN, "Expected 'return'.");
    shared_ptr<AstNode> value = nullptr;
    if (!isAtEnd() && peek().type != TokenType::TK_SEMICOLON && peek().type != TokenType::TK_DEDENT && peek().type != TokenType::TK_EOF
        && peek().line == ret_token.line) {
        value = parseExpressionsOpt();
    }
    auto return_node = make_shared<AstNode>(NodeType::ReturnStatementNode, ret_token);
    if (value) return_node->children.push_back(std::move(value));
    return return_node;
}

shared_ptr<AstNode> Parser::parseExpressionsOpt() {
    if (isAtEnd() || check(TokenType::TK_SEMICOLON) || check(TokenType::TK_DEDENT) || check(TokenType::TK_EOF)
        || ( peek().line > previous().line && previous().type != TokenType::TK_COMMA )
            ) {
        return nullptr;
    }
    return parseExpressions();
}

shared_ptr<AstNode> Parser::parseExpressions() {
    int line = peek().line;
    auto first_expr = parseExpression();

    if (match(TokenType::TK_COMMA)) {
        vector<shared_ptr<AstNode>> elements;
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
        auto tuple_node = make_shared<AstNode>(NodeType::TupleLiteralNode, Token{TokenType::TK_LPAREN, "(", line, TokenCategory::KEYWORD});
        tuple_node->children = std::move(elements);
        return tuple_node;
    } else {
        return first_expr;
    }
}

shared_ptr<AstNode> Parser::parseImportStatement() {
    Token import_token = consume(TokenType::TK_IMPORT, "Expected 'import'.");
    if (this->had_error) return make_shared<AstNode>(NodeType::PassStatementNode, Token{TokenType::TK_PASS, "pass", import_token.line, TokenCategory::KEYWORD});

    Token module_token = consume(TokenType::TK_IDENTIFIER, "Expected module name after 'import'.");
    if (this->had_error) return make_shared<AstNode>(NodeType::PassStatementNode, Token{TokenType::TK_PASS, "pass", import_token.line, TokenCategory::KEYWORD});

    string module_name_str = module_token.lexeme;
    int name_line_start = module_token.line;
    shared_ptr<AstNode> alias_node = nullptr;

    if (match(TokenType::TK_AS)) {
        Token alias_token = consume(TokenType::TK_IDENTIFIER, "Expected alias name after 'as'.");
        if (this->had_error) return make_shared<AstNode>(NodeType::PassStatementNode, Token{TokenType::TK_PASS, "pass", import_token.line, TokenCategory::KEYWORD});
        alias_node = make_shared<AstNode>(NodeType::IdentifierNode, alias_token);
        alias_node->value = alias_token.lexeme;
    }

    auto named_import = make_shared<AstNode>(NodeType::NamedImportNode, Token{TokenType::TK_IDENTIFIER, module_name_str, name_line_start, TokenCategory::IDENTIFIER});
    named_import->value = module_name_str;
    if (alias_node) named_import->children.push_back(std::move(alias_node));

    vector<shared_ptr<AstNode>> names_list;
    names_list.push_back(std::move(named_import));

    auto import_stmt_node = make_shared<AstNode>(NodeType::ImportStatementNode, import_token);
    import_stmt_node->children = std::move(names_list);
    return import_stmt_node;
}

shared_ptr<AstNode> Parser::parseExpression() {
    int line = peek().line;
    auto cond_or_main_expr = parseDisjunction();

    if (match(TokenType::TK_IF)) {
        auto condition = parseDisjunction();
        consume(TokenType::TK_ELSE, "Expected 'else' in ternary expression.");
        auto orelse_expr = parseExpression();
        auto if_exp_node = make_shared<AstNode>(NodeType::IfExpNode, Token{TokenType::TK_IF, "if", line, TokenCategory::KEYWORD});
        if_exp_node->children.push_back(std::move(condition));
        if_exp_node->children.push_back(std::move(cond_or_main_expr));
        if_exp_node->children.push_back(std::move(orelse_expr));
        return if_exp_node;
    }
    return cond_or_main_expr;
}

shared_ptr<AstNode> Parser::parseDisjunction() {
    auto node = parseConjunction();
    while (match(TokenType::TK_OR)) {
        Token op = previous();
        auto right = parseConjunction();
        auto bin_op_node = make_shared<AstNode>(NodeType::BinaryOpNode, op);
        bin_op_node->children.push_back(std::move(node));
        bin_op_node->children.push_back(std::move(right));
        node = bin_op_node;
    }
    return node;
}

shared_ptr<AstNode> Parser::parseConjunction() {
    auto node = parseInversion();
    while (match(TokenType::TK_AND)) {
        Token op = previous();
        auto right = parseInversion();
        auto bin_op_node = make_shared<AstNode>(NodeType::BinaryOpNode, op);
        bin_op_node->children.push_back(std::move(node));
        bin_op_node->children.push_back(std::move(right));
        node = bin_op_node;
    }
    return node;
}

shared_ptr<AstNode> Parser::parseInversion() {
    if (match(TokenType::TK_NOT)) {
        Token op = previous();
        auto operand = parseInversion();
        auto unary_op_node = make_shared<AstNode>(NodeType::UnaryOpNode, op);
        unary_op_node->children.push_back(std::move(operand));
        return unary_op_node;
    }
    return parseComparison();
}

shared_ptr<AstNode> Parser::parseComparison() {
    int line = peek().line;
    auto left_expr = parseBitwiseOr();

    vector<Token> ops;
    vector<shared_ptr<AstNode>> comparators;

    while (true) {
        if (check(TokenType::TK_EQUAL) || check(TokenType::TK_NOT_EQUAL) ||
            check(TokenType::TK_LESS) || check(TokenType::TK_LESS_EQUAL) ||
            check(TokenType::TK_GREATER) || check(TokenType::TK_GREATER_EQUAL) ||
            check(TokenType::TK_IN)) {
            ops.push_back(advance());
            comparators.push_back(parseBitwiseOr());
        } else if (peek().type == TokenType::TK_IS) {
            Token op_is = advance();
            if (match(TokenType::TK_NOT)) {
                Token synthetic_op = op_is;
                synthetic_op.lexeme = "is not";
                ops.push_back(synthetic_op);
            } else {
                ops.push_back(op_is);
            }
            comparators.push_back(parseBitwiseOr());
        } else if (peek().type == TokenType::TK_NOT && peek(1).type == TokenType::TK_IN) {
            Token op_not = advance();
            advance();
            Token synthetic_op = op_not;
            synthetic_op.lexeme = "not in";
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
        auto comparison_node = make_shared<AstNode>(NodeType::ComparisonNode, Token{TokenType::TK_EOF, "", line, TokenCategory::EOFILE});
        comparison_node->children.push_back(std::move(left_expr));
        for(const auto& op : ops) {
            auto op_node = make_shared<AstNode>(NodeType::BinaryOpNode, op);
            comparison_node->children.push_back(std::move(op_node));
        }
        for(auto& comp : comparators) {
            comparison_node->children.push_back(std::move(comp));
        }
        return comparison_node;
    }
}

void Parser::unputToken() {
    if (current_pos > 0) current_pos--;
}

shared_ptr<AstNode> Parser::parseBitwiseOr() {
    auto node = parseBitwiseXor();
    while (match(TokenType::TK_BIT_OR)) {
        Token op = previous();
        auto right = parseBitwiseXor();
        auto bin_op_node = make_shared<AstNode>(NodeType::BinaryOpNode, op);
        bin_op_node->children.push_back(std::move(node));
        bin_op_node->children.push_back(std::move(right));
        node = bin_op_node;
    }
    return node;
}

shared_ptr<AstNode> Parser::parseBitwiseXor() {
    auto node = parseBitwiseAnd();
    while (match(TokenType::TK_BIT_XOR)) {
        Token op = previous();
        auto right = parseBitwiseAnd();
        auto bin_op_node = make_shared<AstNode>(NodeType::BinaryOpNode, op);
        bin_op_node->children.push_back(std::move(node));
        bin_op_node->children.push_back(std::move(right));
        node = bin_op_node;
    }
    return node;
}

shared_ptr<AstNode> Parser::parseBitwiseAnd() {
    auto node = parseShiftExpr();
    while (match(TokenType::TK_BIT_AND)) {
        Token op = previous();
        auto right = parseShiftExpr();
        auto bin_op_node = make_shared<AstNode>(NodeType::BinaryOpNode, op);
        bin_op_node->children.push_back(std::move(node));
        bin_op_node->children.push_back(std::move(right));
        node = bin_op_node;
    }
    return node;
}

shared_ptr<AstNode> Parser::parseShiftExpr() {
    auto node = parseSum();
    while (match(TokenType::TK_BIT_LEFT_SHIFT) || match(TokenType::TK_BIT_RIGHT_SHIFT)) {
        Token op = previous();
        auto right = parseSum();
        auto bin_op_node = make_shared<AstNode>(NodeType::BinaryOpNode, op);
        bin_op_node->children.push_back(std::move(node));
        bin_op_node->children.push_back(std::move(right));
        node = bin_op_node;
    }
    return node;
}

shared_ptr<AstNode> Parser::parseSum() {
    auto node = parseTerm();
    while (match(TokenType::TK_PLUS) || match(TokenType::TK_MINUS)) {
        Token op = previous();
        auto right = parseTerm();
        auto bin_op_node = make_shared<AstNode>(NodeType::BinaryOpNode, op);
        bin_op_node->children.push_back(std::move(node));
        bin_op_node->children.push_back(std::move(right));
        node = bin_op_node;
    }
    return node;
}

shared_ptr<AstNode> Parser::parseTerm() {
    auto node = parseFactor();
    while (match(TokenType::TK_MULTIPLY) || match(TokenType::TK_DIVIDE) ||
           match(TokenType::TK_FLOORDIV) || match(TokenType::TK_MOD)) {
        Token op = previous();
        auto right = parseFactor();
        auto bin_op_node = make_shared<AstNode>(NodeType::BinaryOpNode, op);
        bin_op_node->children.push_back(std::move(node));
        bin_op_node->children.push_back(std::move(right));
        node = bin_op_node;
    }
    return node;
}

shared_ptr<AstNode> Parser::parseFactor() {
    if (match(TokenType::TK_PLUS) || match(TokenType::TK_MINUS) || match(TokenType::TK_BIT_NOT)) {
        Token op = previous();
        auto operand = parseFactor();
        auto unary_op_node = make_shared<AstNode>(NodeType::UnaryOpNode, op);
        unary_op_node->children.push_back(std::move(operand));
        return unary_op_node;
    }
    return parsePower();
}

shared_ptr<AstNode> Parser::parsePower() {
    auto left = parsePrimary(false);
    if (match(TokenType::TK_POWER)) {
        Token op = previous();
        auto right = parseFactor();
        auto bin_op_node = make_shared<AstNode>(NodeType::BinaryOpNode, op);
        bin_op_node->children.push_back(std::move(left));
        bin_op_node->children.push_back(std::move(right));
        return bin_op_node;
    }
    return left;
}

shared_ptr<AstNode> Parser::parsePrimary(bool in_target_context) {
    auto node = parseAtom(in_target_context);

    while (true) {
        if (match(TokenType::TK_PERIOD)) {
            Token dot_token = previous();
            Token name_token = consume(TokenType::TK_IDENTIFIER, "Expected attribute name after '.'.");
            auto attr_ident = make_shared<AstNode>(NodeType::IdentifierNode, name_token);
            attr_ident->value = name_token.lexeme;
            auto attr_access_node = make_shared<AstNode>(NodeType::AttributeAccessNode, dot_token);
            attr_access_node->children.push_back(std::move(node));
            attr_access_node->children.push_back(std::move(attr_ident));
            node = attr_access_node;
        } else if (match(TokenType::TK_LPAREN)) {
            if (in_target_context) {
                reportError(previous(), "Function call cannot be a target of assignment.");
                unputToken();
                break;
            }
            Token lparen_token = previous();
            vector<shared_ptr<AstNode>> pos_args;
            vector<shared_ptr<AstNode>> kw_args;
            int call_line = lparen_token.line;
            if (!check(TokenType::TK_RPAREN)) {
                parseArgumentsForCall(pos_args, kw_args, call_line);
            }
            consume(TokenType::TK_RPAREN, "Expected ')' after function arguments.");
            auto func_call_node = make_shared<AstNode>(NodeType::FunctionCallNode, lparen_token);
            func_call_node->children.push_back(std::move(node));
            auto args_node = make_shared<AstNode>(NodeType::ArgumentsNode, lparen_token);
            args_node->children.insert(args_node->children.end(), std::make_move_iterator(pos_args.begin()), std::make_move_iterator(pos_args.end()));
            args_node->children.insert(args_node->children.end(), std::make_move_iterator(kw_args.begin()), std::make_move_iterator(kw_args.end()));
            func_call_node->children.push_back(std::move(args_node));
            node = func_call_node;
        } else if (match(TokenType::TK_LBRACKET)) {
            Token lbracket_token = previous();
            auto slice_or_idx = parseSlices();
            consume(TokenType::TK_RBRACKET, "Expected ']' after subscript or slice.");
            auto subscription_node = make_shared<AstNode>(NodeType::SubscriptionNode, lbracket_token);
            subscription_node->children.push_back(std::move(node));
            subscription_node->children.push_back(std::move(slice_or_idx));
            node = subscription_node;
        } else {
            break;
        }
    }
    return node;
}

shared_ptr<AstNode> Parser::parseAtom(bool in_target_context) {
    int line = peek().line;
    switch (peek().type) {
        case TokenType::TK_IDENTIFIER: {
            Token id_token = advance();
            auto id_node = make_shared<AstNode>(NodeType::IdentifierNode, id_token);
            id_node->value = id_token.lexeme;
            return id_node;
        }
        case TokenType::TK_TRUE:  advance(); return make_shared<AstNode>(NodeType::BooleanLiteralNode, Token{TokenType::TK_TRUE, "True", line, TokenCategory::KEYWORD});
        case TokenType::TK_FALSE: advance(); return make_shared<AstNode>(NodeType::BooleanLiteralNode, Token{TokenType::TK_FALSE, "False", line, TokenCategory::KEYWORD});
        case TokenType::TK_NONE:  advance(); return make_shared<AstNode>(NodeType::NoneLiteralNode, Token{TokenType::TK_NONE, "None", line, TokenCategory::KEYWORD});
        case TokenType::TK_NUMBER: {
            Token num_token = advance();
            auto num_node = make_shared<AstNode>(NodeType::NumberLiteralNode, num_token);
            num_node->value = num_token.lexeme;
            return num_node;
        }
        case TokenType::TK_COMPLEX: {
            Token complex_token = advance();
            string value = complex_token.lexeme;
            string real_part = "0";
            string imag_part = value;
            if (imag_part.empty() || imag_part.back() != 'j') {
                reportError(complex_token, "Complex literal must end with 'j'.");
                return make_shared<AstNode>(NodeType::ComplexLiteralNode, complex_token);
            }
            imag_part.pop_back();

            size_t plus_pos = string::npos;
            size_t minus_pos = string::npos;

            for (size_t i = 1; i < imag_part.length(); ++i) {
                if (imag_part[i] == '+') plus_pos = i;
                if (imag_part[i] == '-') minus_pos = i;
            }

            size_t op_pos = string::npos;
            if (plus_pos != string::npos) op_pos = plus_pos;
            if (minus_pos != string::npos && (plus_pos == string::npos || minus_pos > plus_pos) ) {
                op_pos = minus_pos;
            }

            if (op_pos != string::npos) {
                real_part = imag_part.substr(0, op_pos);
                imag_part = imag_part.substr(op_pos);
            }
            auto complex_node = make_shared<AstNode>(NodeType::ComplexLiteralNode, complex_token);
            complex_node->value = real_part + imag_part;
            return complex_node;
        }
        case TokenType::TK_STRING:
            return parseStrings();
        case TokenType::TK_BYTES:
            return parseBytes();
        case TokenType::TK_LPAREN:
            return parseTupleGroupVariant(in_target_context);
        case TokenType::TK_LBRACKET:
            return parseListVariant(in_target_context);
        case TokenType::TK_LBRACE:
            return parseDictSetVariant();
        case TokenType::TK_INT: case TokenType::TK_STR: case TokenType::TK_FLOAT:
        case TokenType::TK_LIST: case TokenType::TK_TUPLE: case TokenType::TK_RANGE:
        case TokenType::TK_DICT: case TokenType::TK_SET: case TokenType::TK_FROZENSET:
        case TokenType::TK_BOOL: case TokenType::TK_BYTEARRAY: case TokenType::TK_MEMORYVIEW:
        case TokenType::TK_NONETYPE:
        {
            Token type_kw_token = advance();
            auto id_node = make_shared<AstNode>(NodeType::IdentifierNode, type_kw_token);
            id_node->value = type_kw_token.lexeme;
            return id_node;
        }
        default:
            reportError(peek(), "Expected an atom (identifier, literal, '(', '[', or '{').");
            throw runtime_error("Invalid atom: Unrecognized token " + peek().lexeme);
    }
}

shared_ptr<AstNode> Parser::parseStrings() {
    if (!check(TokenType::TK_STRING)) {
        reportError(peek(), "Expected string literal.");
        throw runtime_error("Expected string.");
    }
    Token first_string = consume(TokenType::TK_STRING, "Expected string literal.");
    string concatenated_value = first_string.lexeme;
    int line = first_string.line;

    while (check(TokenType::TK_STRING) && tokens[current_pos-1].line == tokens[current_pos].line) {
        concatenated_value += advance().lexeme;
    }
    auto string_node = make_shared<AstNode>(NodeType::StringLiteralNode, first_string);
    string_node->value = concatenated_value;
    return string_node;
}

shared_ptr<AstNode> Parser::parseTupleGroupVariant(bool in_target_context) {
    Token lparen = consume(TokenType::TK_LPAREN, "Expected '('.");

    if (match(TokenType::TK_RPAREN)) {
        return make_shared<AstNode>(NodeType::TupleLiteralNode, lparen);
    }
    auto first_element = parseExpression();

    if (match(TokenType::TK_COMMA)) {
        vector<shared_ptr<AstNode>> elements;
        elements.push_back(std::move(first_element));

        if (!check(TokenType::TK_RPAREN)) {
            auto remaining_exprs_node = parseExpressionsOpt();
            if (remaining_exprs_node) {
                 elements.push_back(std::move(remaining_exprs_node));
            }
        }
        consume(TokenType::TK_RPAREN, "Expected ')' to close tuple literal.");
        auto tuple_node = make_shared<AstNode>(NodeType::TupleLiteralNode, lparen);
        tuple_node->children = std::move(elements);
        return tuple_node;
    } else {
        consume(TokenType::TK_RPAREN, "Expected ')' to close parenthesized expression.");
        return first_element;
    }
}

shared_ptr<AstNode> Parser::parseListVariant(bool in_target_context) {
    return parseListLiteral(in_target_context);
}

shared_ptr<AstNode> Parser::parseListLiteral(bool in_target_context) {
    Token lbracket = consume(TokenType::TK_LBRACKET, "Expected '[' to start list literal.");
    vector<shared_ptr<AstNode>> elements;

    if (!check(TokenType::TK_RBRACKET)) {
        auto exprs_node = parseExpressionsOpt();
        if (exprs_node) {
            elements.push_back(std::move(exprs_node));
        }
    }
    consume(TokenType::TK_RBRACKET, "Expected ']' to close list literal.");
    auto list_node = make_shared<AstNode>(NodeType::ListLiteralNode, lbracket);
    list_node->children = std::move(elements);
    return list_node;
}


shared_ptr<AstNode> Parser::parseBlock() {
    int line = peek().line;
    if (match(TokenType::TK_INDENT)) {
        auto stmts = parseStatements();
        consume(TokenType::TK_DEDENT, "Expected DEDENT to end indented block.");
        auto block_node = make_shared<AstNode>(NodeType::BlockNode, Token{TokenType::TK_INDENT, "    ", line, TokenCategory::PUNCTUATION});
        block_node->children = std::move(stmts);
        return block_node;
    } else {
        auto stmts_vec = parseSimpleStmts();
        auto block_node = make_shared<AstNode>(NodeType::BlockNode, Token{TokenType::TK_EOF, "", line, TokenCategory::EOFILE});
        block_node->children = std::move(stmts_vec);
        return block_node;
    }
}

shared_ptr<AstNode> Parser::parseFunctionDef() {
    Token def_token = consume(TokenType::TK_DEF, "Expected 'def'.");
    Token name_token = consume(TokenType::TK_IDENTIFIER, "Expected function name.");
    auto name_ident = make_shared<AstNode>(NodeType::IdentifierNode, name_token);
    name_ident->value = name_token.lexeme;

    consume(TokenType::TK_LPAREN, "Expected '(' after function name.");
    int params_line = peek().line;
    shared_ptr<AstNode> args_spec = parseParamsOpt(params_line);
    consume(TokenType::TK_RPAREN, "Expected ')' after function parameters.");
    consume(TokenType::TK_COLON, "Expected ':' after function signature.");
    auto body = parseBlock();

    auto func_def_node = make_shared<AstNode>(NodeType::FunctionDefinitionNode, def_token);
    func_def_node->children.push_back(std::move(name_ident));
    if (args_spec) func_def_node->children.push_back(std::move(args_spec));
    func_def_node->children.push_back(std::move(body));
    return func_def_node;
}

shared_ptr<AstNode> Parser::parseParamsOpt(int& line_start) {
    line_start = peek().line;
    if (check(TokenType::TK_RPAREN)) {
        return make_shared<AstNode>(NodeType::ArgumentsNode, Token{TokenType::TK_RPAREN, ")", line_start, TokenCategory::KEYWORD});
    }
    return parseParameters(line_start);
}

shared_ptr<AstNode> Parser::parseRaiseStmt() {
    Token raise_token = consume(TokenType::TK_RAISE, "Expected 'raise'.");
    shared_ptr<AstNode> exception_expr = nullptr;
    shared_ptr<AstNode> cause_expr = nullptr;

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
    auto raise_node = make_shared<AstNode>(NodeType::RaiseStatementNode, raise_token);
    if (exception_expr) raise_node->children.push_back(std::move(exception_expr));
    if (cause_expr) raise_node->children.push_back(std::move(cause_expr));
    return raise_node;
}

shared_ptr<AstNode> Parser::parseGlobalStmt() {
    Token global_token = consume(TokenType::TK_GLOBAL, "Expected 'global'.");
    if (this->had_error) {
        return make_shared<AstNode>(NodeType::GlobalStatementNode, global_token);
    }

    int name_list_line_start = peek().line;
    vector<shared_ptr<AstNode>> names = parseNameCommaList(name_list_line_start);

    if (names.empty() && !this->had_error) {
        reportError(peek(), "Expected at least one identifier after 'global'.");
    }
    auto global_node = make_shared<AstNode>(NodeType::GlobalStatementNode, global_token);
    global_node->children = std::move(names);
    return global_node;
}

shared_ptr<AstNode> Parser::parseNonlocalStmt() {
    Token nonlocal_token = consume(TokenType::TK_NONLOCAL, "Expected 'nonlocal'.");
    if (this->had_error) {
        return make_shared<AstNode>(NodeType::NonlocalStatementNode, nonlocal_token);
    }

    int name_list_line_start = peek().line;
    vector<shared_ptr<AstNode>> names = parseNameCommaList(name_list_line_start);

    if (names.empty() && !this->had_error) {
        reportError(peek(), "Expected at least one identifier after 'nonlocal'.");
    }
    auto nonlocal_node = make_shared<AstNode>(NodeType::NonlocalStatementNode, nonlocal_token);
    nonlocal_node->children = std::move(names);
    return nonlocal_node;
}

shared_ptr<AstNode> Parser::parseIfStmt() {
    Token if_tok = consume(TokenType::TK_IF, "Expect 'if' keyword.");
    if (this->had_error) {
        return nullptr;
    }
    int line = if_tok.line;

    shared_ptr<AstNode> condition = parseExpression();
    if (this->had_error || !condition) {
        if (!this->had_error) reportError(previous(), "Expect expression after 'if'.");
        return nullptr;
    }

    consume(TokenType::TK_COLON, "Expect ':' after 'if' condition.");
    if (this->had_error) {
        return nullptr;
    }

    shared_ptr<AstNode> then_block = parseBlock();
    if (this->had_error || !then_block) {
        if (!this->had_error) reportError(previous(), "Expect indented block for 'if' body.");
        return nullptr;
    }

    vector<pair<shared_ptr<AstNode>, shared_ptr<AstNode>>> elif_blocks;

    while (peek().type == TokenType::TK_ELIF) {
        consume(TokenType::TK_ELIF, "Internal error with 'elif'.");
        if (this->had_error) return nullptr;

        shared_ptr<AstNode> elif_condition = parseExpression();
        if (this->had_error || !elif_condition) {
            if (!this->had_error) reportError(previous(), "Expect expression after 'elif'.");
            return nullptr;
        }

        consume(TokenType::TK_COLON, "Expect ':' after 'elif' condition.");
        if (this->had_error) return nullptr;

        shared_ptr<AstNode> elif_body = parseBlock();
        if (this->had_error || !elif_body) {
            if (!this->had_error) reportError(previous(), "Expect indented block for 'elif' body.");
            return nullptr;
        }
        elif_blocks.emplace_back(std::move(elif_condition), std::move(elif_body));
    }

    shared_ptr<AstNode> else_block = parseElseBlockOpt();
    if (this->had_error && else_block == nullptr && previous().type == TokenType::TK_ELSE) {
        return nullptr;
    }

    auto if_stmt_node = make_shared<AstNode>(NodeType::IfStatementNode, if_tok);
    if_stmt_node->children.push_back(std::move(condition));
    if_stmt_node->children.push_back(std::move(then_block));
    for(auto& pair : elif_blocks) {
        if_stmt_node->children.push_back(std::move(pair.first));
        if_stmt_node->children.push_back(std::move(pair.second));
    }
    if (else_block) if_stmt_node->children.push_back(std::move(else_block));

    return if_stmt_node;
}

shared_ptr<AstNode> Parser::parseElseBlockOpt() {
    if (peek().type == TokenType::TK_ELSE) {
        consume(TokenType::TK_ELSE, "Internal error: Expected 'else' based on peek.");
        if (this->had_error) return nullptr;

        consume(TokenType::TK_COLON, "Expect ':' after 'else' keyword.");
        if (this->had_error) return nullptr;

        shared_ptr<AstNode> else_b = parseBlock();
        if (this->had_error || !else_b) {
            if (!this->had_error) reportError(previous(), "Expected indented block for 'else' body.");
            return nullptr;
        }
        return else_b;
    }
    return nullptr;
}

shared_ptr<AstNode> Parser::parseClassDef() {
    Token class_tok = consume(TokenType::TK_CLASS, "Expect 'class' keyword.");
    if (this->had_error) {
        return nullptr;
    }
    int line = class_tok.line;

    Token name_tok = consume(TokenType::TK_IDENTIFIER, "Expect class name.");
    if (this->had_error) {
        return nullptr;
    }
    auto class_name = make_shared<AstNode>(NodeType::IdentifierNode, name_tok);
    class_name->value = name_tok.lexeme;

    vector<shared_ptr<AstNode>> base_classes;
    vector<shared_ptr<AstNode>> keyword_args;
    int arg_list_line = name_tok.line;

    if (match(TokenType::TK_LPAREN)) {
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

    shared_ptr<AstNode> body = parseBlock();
    if (this->had_error || !body) {
        if (!this->had_error) reportError(previous(), "Expect indented block for class body.");
        return nullptr;
    }

    auto class_def_node = make_shared<AstNode>(NodeType::ClassDefinitionNode, class_tok);
    class_def_node->children.push_back(std::move(class_name));
    auto bases_node = make_shared<AstNode>(NodeType::ArgumentsNode, Token{TokenType::TK_LPAREN, "(", arg_list_line, TokenCategory::KEYWORD});
    bases_node->children = std::move(base_classes);
    class_def_node->children.push_back(std::move(bases_node));
    auto keywords_node = make_shared<AstNode>(NodeType::ArgumentsNode, Token{TokenType::TK_LPAREN, "(", arg_list_line, TokenCategory::KEYWORD});
    keywords_node->children = std::move(keyword_args);
    class_def_node->children.push_back(std::move(keywords_node));
    class_def_node->children.push_back(std::move(body));

    return class_def_node;
}


shared_ptr<AstNode> Parser::parseForStmt() {
    Token for_token = consume(TokenType::TK_FOR, "Expected 'for'.");
    if (this->had_error) return nullptr;

    vector<shared_ptr<AstNode>> targets_vec = parseTargets();
    if (this->had_error || targets_vec.empty()) {
        if (!this->had_error) reportError(peek(), "Expected target(s) for 'for' loop.");
        return nullptr;
    }
    shared_ptr<AstNode> final_target_expr;
    if (targets_vec.size() == 1) {
        final_target_expr = std::move(targets_vec[0]);
    } else {
        final_target_expr = make_shared<AstNode>(NodeType::TupleLiteralNode, Token{TokenType::TK_LPAREN, "(", targets_vec[0]->token.line, TokenCategory::KEYWORD});
        final_target_expr->children = std::move(targets_vec);
    }


    consume(TokenType::TK_IN, "Expected 'in' after for-loop target(s).");
    if (this->had_error) return nullptr;

    auto iterable_expr = parseExpressions();
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

    shared_ptr<AstNode> else_block = nullptr;
    if (match(TokenType::TK_ELSE)) {
        consume(TokenType::TK_COLON, "Expected ':' after 'else' in for loop.");
        if (this->had_error) return nullptr;
        else_block = parseBlock();
        if (this->had_error && else_block == nullptr) {
            if (!this->had_error) reportError(previous(), "Expected indented block for 'for' loop 'else' body.");
            return nullptr;
        }
    }

    auto for_stmt_node = make_shared<AstNode>(NodeType::ForStatementNode, for_token);
    for_stmt_node->children.push_back(std::move(final_target_expr));
    for_stmt_node->children.push_back(std::move(iterable_expr));
    for_stmt_node->children.push_back(std::move(body_block));
    if (else_block) for_stmt_node->children.push_back(std::move(else_block));

    return for_stmt_node;
}

shared_ptr<AstNode> Parser::parseTryStmt() {
    Token try_token = consume(TokenType::TK_TRY, "Expected 'try'.");
    if (this->had_error) return nullptr;

    consume(TokenType::TK_COLON, "Expected ':' after 'try'.");
    if (this->had_error) return nullptr;

    auto try_block = parseBlock();
    if (this->had_error || !try_block) {
        if (!this->had_error) reportError(previous(), "Expected indented block for 'try' body.");
        return nullptr;
    }

    vector<shared_ptr<AstNode>> handlers;
    shared_ptr<AstNode> else_block_node = nullptr;
    shared_ptr<AstNode> finally_block_node = nullptr;
    bool has_except_clause = false;

    while (check(TokenType::TK_EXCEPT)) {
        has_except_clause = true;
        auto handler = parseExceptBlock();
        if (this->had_error) {
            return nullptr;
        }
        if (!handler) {
            reportError(peek(), "Internal error: parseExceptBlock returned null without error.");
            return nullptr;
        }
        handlers.push_back(std::move(handler));
    }

    if (has_except_clause && match(TokenType::TK_ELSE)) {
        consume(TokenType::TK_COLON, "Expected ':' after 'else' in try-except statement.");
        if (this->had_error) return nullptr;
        else_block_node = parseBlock();
        if (this->had_error || !else_block_node) {
            if (!this->had_error) reportError(previous(), "Expected indented block for 'else' body.");
            return nullptr;
        }
    }

    finally_block_node = parseFinallyBlockOpt();
    if (this->had_error && finally_block_node == nullptr && previous().type == TokenType::TK_FINALLY) {
        return nullptr;
    }


    if (handlers.empty() && !finally_block_node) {
        reportError(try_token, "Try statement must have at least one 'except' or 'finally' clause.");
    }

    auto try_stmt_node = make_shared<AstNode>(NodeType::TryStatementNode, try_token);
    try_stmt_node->children.push_back(std::move(try_block));
    auto handlers_node = make_shared<AstNode>(NodeType::ExceptionHandlerNode, try_token);
    handlers_node->children = std::move(handlers);
    try_stmt_node->children.push_back(std::move(handlers_node));
    if (else_block_node) try_stmt_node->children.push_back(std::move(else_block_node));
    if (finally_block_node) try_stmt_node->children.push_back(std::move(finally_block_node));

    return try_stmt_node;
}

void Parser::parseClassArgumentsOpt(vector<shared_ptr<AstNode>>& bases,
                                    vector<shared_ptr<AstNode>>& keywords,
                                    int& line_start_ref) {
    bool first_arg = true;
    if (peek().type != TokenType::TK_RPAREN && !isAtEnd()) {
        line_start_ref = peek().line;
    } else {
        return;
    }

    while (peek().type != TokenType::TK_RPAREN && !isAtEnd()) {
        if (!first_arg) {
            consume(TokenType::TK_COMMA, "Expected ',' to separate class arguments.");
            if (this->had_error) return;
            if (peek().type == TokenType::TK_RPAREN) break;
        }
        first_arg = false;

        if (peek(0).type == TokenType::TK_IDENTIFIER && peek(1).type == TokenType::TK_ASSIGN) {
            auto kw_item = parseKeywordItem();
            if (this->had_error) return;
            if (!kw_item) {
                reportError(peek(), "Internal error: parseKeywordItem returned null without error flag for class arguments.");
                return;
            }
            keywords.push_back(std::move(kw_item));
        } else {
            shared_ptr<AstNode> base_expr = parseExpression();
            if (this->had_error || !base_expr) {
                if (!this->had_error) reportError(previous(), "Expected expression for base class.");
                return;
            }
            bases.push_back(std::move(base_expr));
        }
    }
}

shared_ptr<AstNode> Parser::parseParamIdentifier() {
    if (!check(TokenType::TK_IDENTIFIER)) {
        reportError(peek(), "Expected parameter identifier.");
        return nullptr;
    }
    Token id_token = consume(TokenType::TK_IDENTIFIER, "Expected parameter identifier.");
    if (this->had_error) return nullptr;
    auto id_node = make_shared<AstNode>(NodeType::IdentifierNode, id_token);
    id_node->value = id_token.lexeme;
    return id_node;
}

shared_ptr<AstNode> Parser::parseDefault() {
    if (match(TokenType::TK_ASSIGN)) {
        auto default_expr = parseExpression();
        if (this->had_error || !default_expr) {
            if (!this->had_error) reportError(previous(), "Expected expression for default parameter value.");
            return nullptr;
        }
        return default_expr;
    }
    return nullptr;
}

shared_ptr<AstNode> Parser::parseParamWithDefault() {
    shared_ptr<AstNode> param_ident_node = parseParamIdentifier();
    if (this->had_error || !param_ident_node) {
        return nullptr;
    }

    string param_name_str = param_ident_node->value;
    int name_line = param_ident_node->token.line;

    shared_ptr<AstNode> default_expr = parseDefault();
    if (this->had_error && default_expr == nullptr && previous().type == TokenType::TK_ASSIGN) {
        return nullptr;
    }
    auto param_node = make_shared<AstNode>(NodeType::ParameterNode, Token{TokenType::TK_IDENTIFIER, param_name_str, name_line, TokenCategory::IDENTIFIER});
    param_node->value = param_name_str;
    if (default_expr) param_node->children.push_back(std::move(default_expr));
    return param_node;
}

shared_ptr<AstNode> Parser::parseParamNoDefault() {
    shared_ptr<AstNode> param_ident_node = parseParamIdentifier();
    if (this->had_error || !param_ident_node) {
        return nullptr;
    }

    string param_name_str = param_ident_node->value;
    int name_line = param_ident_node->token.line;

    if (peek().type == TokenType::TK_ASSIGN) {
        reportError(peek(), "Unexpected default value for a parameter expected to have no default.");
        consume(TokenType::TK_ASSIGN, "Internal error: Consuming unexpected default.");
        if(!this->had_error) parseExpression();
        return nullptr;
    }
    auto param_node = make_shared<AstNode>(NodeType::ParameterNode, Token{TokenType::TK_IDENTIFIER, param_name_str, name_line, TokenCategory::IDENTIFIER});
    param_node->value = param_name_str;
    return param_node;
}

shared_ptr<AstNode> Parser::parseWhileStmt() {
    Token while_token = consume(TokenType::TK_WHILE, "Expected 'while'.");
    auto condition = parseExpression();
    consume(TokenType::TK_COLON, "Expected ':' after while condition.");
    auto body = parseBlock();

    shared_ptr<AstNode> else_block = nullptr;
    if (match(TokenType::TK_ELSE)) {
        consume(TokenType::TK_COLON, "Expected ':' after 'else' in while loop.");
        else_block = parseBlock();
    }
    auto while_node = make_shared<AstNode>(NodeType::WhileStatementNode, while_token);
    while_node->children.push_back(std::move(condition));
    while_node->children.push_back(std::move(body));
    if (else_block) while_node->children.push_back(std::move(else_block));
    return while_node;
}

void Parser::parseArgumentsForCall(vector<shared_ptr<AstNode>>& pos_args,
                                   vector<shared_ptr<AstNode>>& kw_args,
                                   int& /*line_start_call unused here */) {
    if (check(TokenType::TK_RPAREN)) return;

    bool keyword_args_started = false;

    do {
        if (check(TokenType::TK_IDENTIFIER) && peek(1).type == TokenType::TK_ASSIGN) {
            keyword_args_started = true;
            auto kw_item = parseKeywordItem();
            if (this->had_error) {
                synchronize();
                if (isAtEnd() || check(TokenType::TK_RPAREN)) break;
                if (check(TokenType::TK_COMMA)) continue;
                break;
            }
            if (!kw_item) {
                reportError(peek(), "Internal error: parseKeywordItem returned null without error flag.");
                break;
            }
            kw_args.push_back(std::move(kw_item));
        } else {
            if (keyword_args_started) {
                reportError(peek(), "Positional argument cannot follow keyword arguments.");
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
}

shared_ptr<AstNode> Parser::parseSlices() {
    int line = peek().line;
    vector<shared_ptr<AstNode>> elements;

    do {
        shared_ptr<AstNode> current_item = nullptr;
        int item_line = peek().line;

        bool is_slice_item = false;
        if (check(TokenType::TK_COLON)) {
            is_slice_item = true;
        } else {
            size_t checkpoint = current_pos;
            bool could_be_expr_then_colon = false;
            if (!check(TokenType::TK_COMMA) && !check(TokenType::TK_RBRACKET)) {
                try {
                    parseExpression();
                    if (check(TokenType::TK_COLON)) {
                        could_be_expr_then_colon = true;
                    }
                } catch (const std::runtime_error&) {
                }
                current_pos = checkpoint;
                if (this->had_error) {
                    this->had_error = false;
                    if (!errors_list.empty()) errors_list.pop_back();
                }
            }
            if (could_be_expr_then_colon) {
                is_slice_item = true;
            }
        }

        if (is_slice_item) {
            current_item = parseSlice();
        } else {
            if (check(TokenType::TK_COMMA) || check(TokenType::TK_RBRACKET)) {
                if (check(TokenType::TK_RBRACKET) && elements.empty() && previous().type == TokenType::TK_LBRACKET) {
                    reportError(peek(), "Empty subscript '[]' is not allowed.");
                    return nullptr;
                } else if (previous().type == TokenType::TK_COMMA){
                    reportError(peek(), "Expected expression after comma in subscript.");
                    return nullptr;
                }
            }
            current_item = parseExpression();
        }

        if (this->had_error) return nullptr;

        if (!current_item) {
            reportError(peek(), "Missing item in subscript list.");
            return nullptr;
        }
        elements.push_back(std::move(current_item));

    } while (match(TokenType::TK_COMMA) && !check(TokenType::TK_RBRACKET));

    if (elements.empty()) {
        reportError(peek(-1).type != TokenType::TK_EOF ? peek(-1) : Token{TokenType::TK_EOF, "", 0, TokenCategory::EOFILE}, "Subscript cannot be empty.");
        return nullptr;
    }

    if (elements.size() == 1) {
        return std::move(elements[0]);
    } else {
        auto tuple_node = make_shared<AstNode>(NodeType::TupleLiteralNode, Token{TokenType::TK_LPAREN, "(", line, TokenCategory::KEYWORD});
        tuple_node->children = std::move(elements);
        return tuple_node;
    }
}

shared_ptr<AstNode> Parser::parseDictSetVariant() {
    Token lbrace_token = consume(TokenType::TK_LBRACE, "Expected '{'.");
    if (this->had_error) return nullptr;

    if (check(TokenType::TK_RBRACE)) {
        consume(TokenType::TK_RBRACE, "Expected '}' to close empty dictionary.");
        if (this->had_error) return nullptr;
        return make_shared<AstNode>(NodeType::DictLiteralNode, lbrace_token);
    }

    auto first_expr = parseExpression();
    if (this->had_error || !first_expr) {
        if (!this->had_error) reportError(lbrace_token, "Expected expression in set/dict literal.");
        while(!isAtEnd() && !match(TokenType::TK_RBRACE)) advance();
        return nullptr;
    }

    if (match(TokenType::TK_COLON)) {
        vector<shared_ptr<AstNode>> keys;
        vector<shared_ptr<AstNode>> values;

        keys.push_back(std::move(first_expr));

        auto first_value = parseExpression();
        if(this->had_error || !first_value) {
            if(!this->had_error) reportError(previous(), "Expected value after ':' in dictionary literal.");
            while(!isAtEnd() && !match(TokenType::TK_RBRACE)) advance();
            return nullptr;
        }
        values.push_back(std::move(first_value));

        while (match(TokenType::TK_COMMA)) {
            if (check(TokenType::TK_RBRACE)) break;
            int kv_line = peek().line;
            parseKvPair(keys, values, kv_line);
            if (this->had_error) {
                while(!isAtEnd() && !match(TokenType::TK_RBRACE)) advance();
                return nullptr;
            }
        }
        consume(TokenType::TK_RBRACE, "Expected '}' to close dictionary literal.");
        if (this->had_error) return nullptr;
        auto dict_node = make_shared<AstNode>(NodeType::DictLiteralNode, lbrace_token);
        dict_node->children = std::move(keys);
        dict_node->children.insert(dict_node->children.end(), std::make_move_iterator(values.begin()), std::make_move_iterator(values.end()));
        return dict_node;

    } else {
        vector<shared_ptr<AstNode>> elements;
        elements.push_back(std::move(first_expr));

        while (match(TokenType::TK_COMMA)) {
            if (check(TokenType::TK_RBRACE)) break;
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
        auto set_node = make_shared<AstNode>(NodeType::SetLiteralNode, lbrace_token);
        set_node->children = std::move(elements);
        return set_node;
    }
}

shared_ptr<AstNode> Parser::parseParameters(int& line_start) {
    line_start = peek().line;
    auto args_node = make_shared<AstNode>(NodeType::ArgumentsNode, Token{TokenType::TK_LPAREN, "(", line_start, TokenCategory::KEYWORD});

    bool default_seen = false;
    bool star_etc_seen = false;

    while (!check(TokenType::TK_RPAREN)) {
        if (star_etc_seen) {
            reportError(peek(), "Unexpected token after *args or **kwargs.");
            synchronize();
            break;
        }

        if (check(TokenType::TK_MULTIPLY) || check(TokenType::TK_POWER)) {
            parseSimplifiedStarEtc(args_node);
            if (had_error) {
                synchronize();
                break;
            }
            star_etc_seen = true;
        } else if (check(TokenType::TK_IDENTIFIER)) {
            shared_ptr<AstNode> param_node;

            bool will_have_default = false;
            if (check(TokenType::TK_IDENTIFIER) && peek(1).type == TokenType::TK_ASSIGN) {
                will_have_default = true;
            }

            if (default_seen) {
                if (!will_have_default) {
                    Token current_param_token = peek();
                    reportError(current_param_token, "Non-default argument follows default argument.");
                    param_node = parseParamNoDefault();
                } else {
                    param_node = parseParamWithDefault();
                }
            } else {
                if (will_have_default) {
                    param_node = parseParamWithDefault();
                    if (param_node && !param_node->children.empty()) {
                        default_seen = true;
                    }
                } else {
                    param_node = parseParamNoDefault();
                }
            }

            if (param_node) {
                args_node->children.push_back(std::move(param_node));
            } else {
                if (had_error) {
                    synchronize();
                    break;
                } else {
                    reportError(peek(), "Internal error: Failed to parse parameter.");
                    synchronize();
                    break;
                }
            }
        } else {
            reportError(peek(), "Expected parameter name, '*', '**', or ')'.");
            synchronize();
            if (isAtEnd() || check(TokenType::TK_RPAREN)) break;
            continue;
        }

        if (check(TokenType::TK_RPAREN)) {
            break;
        } else if (match(TokenType::TK_COMMA)) {
            if (check(TokenType::TK_RPAREN)) {
                break;
            }
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

vector<shared_ptr<AstNode>> Parser::parseNameCommaList(int& line_start) {
    vector<shared_ptr<AstNode>> names;
    if (!check(TokenType::TK_IDENTIFIER)) {
        reportError(peek(), "Expected an identifier.");
        return names;
    }

    Token first_id_token = consume(TokenType::TK_IDENTIFIER, "Expected identifier.");
    if (this->had_error) return names;
    line_start = first_id_token.line;
    auto id_node = make_shared<AstNode>(NodeType::IdentifierNode, first_id_token);
    id_node->value = first_id_token.lexeme;
    names.push_back(std::move(id_node));

    while (match(TokenType::TK_COMMA)) {
        if (!check(TokenType::TK_IDENTIFIER)) {
            reportError(peek(), "Expected identifier after comma in name list.");
            break;
        }
        Token id_token = consume(TokenType::TK_IDENTIFIER, "Expected identifier after comma.");
        if (this->had_error) break;
        auto next_id_node = make_shared<AstNode>(NodeType::IdentifierNode, id_token);
        next_id_node->value = id_token.lexeme;
        names.push_back(std::move(next_id_node));
    }
    return names;
}

shared_ptr<AstNode> Parser::parseExceptBlock() {
    if (!check(TokenType::TK_EXCEPT)) {
        reportError(peek(), "Expected 'except' keyword.");
        return nullptr;
    }
    Token except_token = consume(TokenType::TK_EXCEPT, "Expected 'except'.");
    if (this->had_error) return nullptr;

    shared_ptr<AstNode> exc_type = nullptr;
    shared_ptr<AstNode> exc_name = nullptr;

    if (!check(TokenType::TK_COLON)) {
        exc_type = parseExpression();
        if (this->had_error) return nullptr;

        if (match(TokenType::TK_AS)) {
            if (!check(TokenType::TK_IDENTIFIER)) {
                reportError(peek(), "Expected identifier after 'as' in except clause.");
                return nullptr;
            }
            Token name_token = consume(TokenType::TK_IDENTIFIER, "Expected identifier for exception name.");
            if (this->had_error) return nullptr;
            exc_name = make_shared<AstNode>(NodeType::IdentifierNode, name_token);
            exc_name->value = name_token.lexeme;
        }
    }

    consume(TokenType::TK_COLON, "Expected ':' after except clause.");
    if (this->had_error) return nullptr;

    shared_ptr<AstNode> body = parseBlock();
    if (this->had_error || !body) {
        if (!this->had_error) reportError(previous(), "Expected block for except handler.");
        return nullptr;
    }

    auto handler_node = make_shared<AstNode>(NodeType::ExceptionHandlerNode, except_token);
    handler_node->children.push_back(std::move(body));
    if (exc_type) handler_node->children.push_back(std::move(exc_type));
    if (exc_name) handler_node->children.push_back(std::move(exc_name));
    return handler_node;
}

shared_ptr<AstNode> Parser::parseFinallyBlockOpt() {
    if (check(TokenType::TK_FINALLY)) {
        return parseFinallyBlock();
    }
    return nullptr;
}

shared_ptr<AstNode> Parser::parseFinallyBlock() {
    if (!check(TokenType::TK_FINALLY)) {
        reportError(peek(), "Expected 'finally' keyword.");
        return nullptr;
    }
    Token finally_token = consume(TokenType::TK_FINALLY, "Expected 'finally'.");
    if (this->had_error) return nullptr;

    consume(TokenType::TK_COLON, "Expected ':' after 'finally'.");
    if (this->had_error) return nullptr;

    shared_ptr<AstNode> body = parseBlock();
    if (this->had_error || !body) {
        if (!this->had_error) reportError(previous(), "Expected block for finally clause.");
        return nullptr;
    }
    auto finally_node = make_shared<AstNode>(NodeType::BlockNode, finally_token);
    finally_node->children.push_back(std::move(body));
    return finally_node;
}

shared_ptr<AstNode> Parser::parseSlice() {
    int line = peek().line;
    shared_ptr<AstNode> lower = nullptr;
    shared_ptr<AstNode> upper = nullptr;
    shared_ptr<AstNode> step = nullptr;

    if (!isAtEnd() && peek().type != TokenType::TK_COLON &&
        peek().type != TokenType::TK_COMMA &&
        peek().type != TokenType::TK_RBRACKET) {
        lower = parseExpression();
        if (had_error) {
            return nullptr;
        }
    }

    if (!match(TokenType::TK_COLON)) {
        reportError(peek(), "Expected ':' to define a slice structure (e.g., start:stop:step).");
        return nullptr;
    }

    if (!isAtEnd() && peek().type != TokenType::TK_COLON &&
        peek().type != TokenType::TK_COMMA &&
        peek().type != TokenType::TK_RBRACKET) {
        upper = parseExpression();
        if (had_error) {
            return nullptr;
        }
    }

    if (match(TokenType::TK_COLON)) {
        if (!isAtEnd() && peek().type != TokenType::TK_COMMA &&
            peek().type != TokenType::TK_RBRACKET) {
            step = parseExpression();
            if (had_error) {
                return nullptr;
            }
        }
    }

    auto slice_node = make_shared<AstNode>(NodeType::SliceNode, Token{TokenType::TK_COLON, ":", line, TokenCategory::KEYWORD});
    if (lower) slice_node->children.push_back(std::move(lower));
    if (upper) slice_node->children.push_back(std::move(upper));
    if (step) slice_node->children.push_back(std::move(step));

    return slice_node;
}

shared_ptr<AstNode> Parser::parseBytes() {
    if (!check(TokenType::TK_BYTES)) {
        reportError(peek(), "Expected bytes literal.");
        return nullptr;
    }
    Token first_bytes = consume(TokenType::TK_BYTES, "Expected bytes literal.");
    if (this->had_error) return nullptr;

    string concatenated_value = first_bytes.lexeme;
    int line = first_bytes.line;

    while (check(TokenType::TK_BYTES) && !isAtEnd() && tokens[current_pos-1].line == tokens[current_pos].line) {
        Token next_bytes = advance();
        concatenated_value += next_bytes.lexeme;
    }
    auto bytes_node = make_shared<AstNode>(NodeType::BytesLiteralNode, first_bytes);
    bytes_node->value = concatenated_value;
    return bytes_node;
}

void Parser::parseKvPair(vector<shared_ptr<AstNode>>& keys,
                         vector<shared_ptr<AstNode>>& values,
                         int /*line*/) {
    auto key = parseExpression();
    if (this->had_error || !key) {
        if(!this->had_error) reportError(peek(), "Expected key in dictionary K:V pair.");
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

shared_ptr<AstNode> Parser::parseKeywordItem() {
    if (!(check(TokenType::TK_IDENTIFIER) && peek(1).type == TokenType::TK_ASSIGN)) {
        reportError(peek(), "Expected 'identifier = expression' for keyword argument.");
        return nullptr;
    }

    Token id_token = consume(TokenType::TK_IDENTIFIER, "Expected identifier for keyword argument name.");
    if (this->had_error) return nullptr;
    auto arg_name_node = make_shared<AstNode>(NodeType::IdentifierNode, id_token);
    arg_name_node->value = id_token.lexeme;

    consume(TokenType::TK_ASSIGN, "Expected '=' for keyword argument.");
    if (this->had_error) return nullptr;

    auto value = parseExpression();
    if (this->had_error || !value) {
        if (!this->had_error) reportError(previous(), "Expected expression for keyword argument value.");
        return nullptr;
    }

    auto kw_arg_node = make_shared<AstNode>(NodeType::KeywordArgNode, id_token);
    kw_arg_node->children.push_back(std::move(arg_name_node));
    kw_arg_node->children.push_back(std::move(value));
    return kw_arg_node;
}

vector<shared_ptr<AstNode>> Parser::parseTargets() {
    vector<shared_ptr<AstNode>> targets_vec;

    auto first_target = parseTarget();
    if (this->had_error || !first_target) {
        if(!this->had_error) reportError(peek(), "Expected a target for assignment.");
        return targets_vec;
    }
    targets_vec.push_back(std::move(first_target));

    while (match(TokenType::TK_COMMA)) {
        if (check(TokenType::TK_ASSIGN) || check(TokenType::TK_SEMICOLON) || check(TokenType::TK_EOF) ||
            (peek().line > previous().line && !check(TokenType::TK_INDENT))) {
            break;
        }
        auto next_target = parseTarget();
        if (this->had_error || !next_target) {
            if(!this->had_error) reportError(peek(), "Expected a target after comma.");
            break;
        }
        targets_vec.push_back(std::move(next_target));
    }
    return targets_vec;
}

shared_ptr<AstNode> Parser::parseTarget() {
    shared_ptr<AstNode> node;

    if (peek().type == TokenType::TK_LPAREN || peek().type == TokenType::TK_LBRACKET) {
        node = parseTargetAtom();
        if (this->had_error) return nullptr;
        if(node) return node;
    }

    node = parseTPrimary();
    if (this->had_error || !node) return nullptr;


    while (!isAtEnd()) {
        if (match(TokenType::TK_PERIOD)) {
            Token dot_token = previous();
            Token name_token = consume(TokenType::TK_IDENTIFIER, "Expected attribute name after '.'.");
            if (this->had_error) return nullptr;
            auto attr_ident = make_shared<AstNode>(NodeType::IdentifierNode, name_token);
            attr_ident->value = name_token.lexeme;
            auto attr_access_node = make_shared<AstNode>(NodeType::AttributeAccessNode, dot_token);
            attr_access_node->children.push_back(std::move(node));
            attr_access_node->children.push_back(std::move(attr_ident));
            node = attr_access_node;
        } else if (match(TokenType::TK_LBRACKET)) {
            Token lbracket_token = previous();
            auto slice_or_idx = parseSlices();
            if (this->had_error || !slice_or_idx) {
                if (!this->had_error) reportError(lbracket_token, "Invalid slice or index for target.");
                return nullptr;
            }
            consume(TokenType::TK_RBRACKET, "Expected ']' after subscript or slice in target.");
            if (this->had_error) return nullptr;
            auto subscription_node = make_shared<AstNode>(NodeType::SubscriptionNode, lbracket_token);
            subscription_node->children.push_back(std::move(node));
            subscription_node->children.push_back(std::move(slice_or_idx));
            node = subscription_node;
        } else {
            break;
        }
    }

    if (dynamic_cast<AstNode*>(node.get()) && node->type == NodeType::FunctionCallNode) {
        reportError(previous(), "Function call cannot be a target of assignment.");
        return nullptr;
    }

    return node;
}

shared_ptr<AstNode> Parser::parseTargetAtom() {
    int line = peek().line;
    if (check(TokenType::TK_IDENTIFIER)) {
        return parsePrimary(true);
    } else if (match(TokenType::TK_LPAREN)) {
        Token lparen = previous();
        if (match(TokenType::TK_RPAREN)) {
            return make_shared<AstNode>(NodeType::TupleLiteralNode, lparen);
        }

        auto first_elem = parseTarget();
        if (this->had_error || !first_elem) {
            if (!this->had_error) reportError(lparen, "Invalid content in parenthesized target.");
            return nullptr;
        }

        if (match(TokenType::TK_COMMA)) {
            vector<shared_ptr<AstNode>> elements;
            elements.push_back(std::move(first_elem));

            while (!check(TokenType::TK_RPAREN) && !isAtEnd()) {
                auto next_target = parseTarget();
                if (this->had_error || !next_target) {
                    if (!this->had_error) reportError(peek(), "Expected target in tuple target sequence.");
                    return nullptr;
                }
                elements.push_back(std::move(next_target));
                if (!match(TokenType::TK_COMMA)) break;
                if (check(TokenType::TK_RPAREN)) break;
            }
            consume(TokenType::TK_RPAREN, "Expected ')' to close tuple target.");
            if (this->had_error) return nullptr;
            auto tuple_node = make_shared<AstNode>(NodeType::TupleLiteralNode, lparen);
            tuple_node->children = std::move(elements);
            return tuple_node;
        } else {
            consume(TokenType::TK_RPAREN, "Expected ')' to close parenthesized target.");
            if (this->had_error) return nullptr;
            return first_elem;
        }
    } else if (match(TokenType::TK_LBRACKET)) {
        Token lbracket = previous();
        vector<shared_ptr<AstNode>> elements;
        if (!check(TokenType::TK_RBRACKET)) {
            do {
                auto target_elem = parseTarget();
                if (this->had_error || !target_elem) {
                    if(!this->had_error) reportError(peek(), "Expected target in list target sequence.");
                    return nullptr;
                }
                elements.push_back(std::move(target_elem));
                if (!match(TokenType::TK_COMMA)) break;
                if (check(TokenType::TK_RBRACKET)) break;
            } while (!check(TokenType::TK_RBRACKET) && !isAtEnd());
        }
        consume(TokenType::TK_RBRACKET, "Expected ']' to close list target.");
        if (this->had_error) return nullptr;
        auto list_node = make_shared<AstNode>(NodeType::ListLiteralNode, lbracket);
        list_node->children = std::move(elements);
        return list_node;
    } else {
        return nullptr;
    }
}

shared_ptr<AstNode> Parser::parseSingleTarget() {
    if (check(TokenType::TK_IDENTIFIER)) {
        auto node = parseTPrimary();
        if (this->had_error || !node) return nullptr;

        bool is_chained = false;
        while(true){
            if (check(TokenType::TK_PERIOD) && peek(1).type == TokenType::TK_IDENTIFIER) {
                consume(TokenType::TK_PERIOD, "");
                Token name_token = consume(TokenType::TK_IDENTIFIER, "Expected attribute name after '.' in single_target.");
                if (this->had_error) return nullptr;
                auto attr_ident = make_shared<AstNode>(NodeType::IdentifierNode, name_token);
                attr_ident->value = name_token.lexeme;
                auto attr_access_node = make_shared<AstNode>(NodeType::AttributeAccessNode, name_token);
                attr_access_node->children.push_back(std::move(node));
                attr_access_node->children.push_back(std::move(attr_ident));
                node = attr_access_node;
                is_chained = true;
            } else if (check(TokenType::TK_LBRACKET)) {
                consume(TokenType::TK_LBRACKET, "");
                auto slice_or_idx = parseSlices();
                if (this->had_error || !slice_or_idx) return nullptr;
                consume(TokenType::TK_RBRACKET, "Expected ']' after subscript in single_target.");
                if (this->had_error) return nullptr;
                auto subscription_node = make_shared<AstNode>(NodeType::SubscriptionNode, previous());
                subscription_node->children.push_back(std::move(node));
                subscription_node->children.push_back(std::move(slice_or_idx));
                node = subscription_node;
                is_chained = true;
            } else {
                break;
            }
        }

        if (!is_chained && !(dynamic_cast<AstNode*>(node.get()) && node->type == NodeType::IdentifierNode)) {
            if(dynamic_cast<AstNode*>(node.get()) && (node->type == NodeType::FunctionCallNode ||
               node->type == NodeType::ListLiteralNode ||
               node->type == NodeType::TupleLiteralNode ||
               node->type == NodeType::SetLiteralNode ||
               node->type == NodeType::DictLiteralNode ||
               node->type == NodeType::NumberLiteralNode ||
               node->type == NodeType::StringLiteralNode ||
               node->type == NodeType::BooleanLiteralNode ||
               node->type == NodeType::NoneLiteralNode ||
               node->type == NodeType::BytesLiteralNode
                    )) {
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

shared_ptr<AstNode> Parser::parseTPrimary() {
    return parsePrimary(false);
}

void Parser::parseSimplifiedStarEtc(shared_ptr<AstNode> args_node_ref) {
    if (match(TokenType::TK_MULTIPLY)) {
        Token star_token = previous();
        shared_ptr<AstNode> vararg_node = nullptr;
        shared_ptr<AstNode> kwarg_node = nullptr;

        if (!check(TokenType::TK_IDENTIFIER)) {
            reportError(peek(), "Expected identifier for *args parameter name.");
            return;
        }
        Token name_token = consume(TokenType::TK_IDENTIFIER, "Expected identifier for *args name.");
        if (this->had_error) return;
        vararg_node = make_shared<AstNode>(NodeType::ParameterNode, star_token);
        vararg_node->value = name_token.lexeme;

        if (check(TokenType::TK_COMMA) && peek(1).type == TokenType::TK_POWER) {
            consume(TokenType::TK_COMMA, "Expected comma before **kwargs after *args.");
            if (this->had_error) return;
        }

        if (match(TokenType::TK_POWER)) {
            Token power_token = previous();
            if (!check(TokenType::TK_IDENTIFIER)) {
                reportError(peek(), "Expected identifier for **kwargs parameter name.");
                return;
            }
            Token kw_name_token = consume(TokenType::TK_IDENTIFIER, "Expected identifier for **kwargs name.");
            if (this->had_error) return;
            kwarg_node = make_shared<AstNode>(NodeType::ParameterNode, power_token);
            kwarg_node->value = kw_name_token.lexeme;
        }
        if (vararg_node) args_node_ref->children.push_back(std::move(vararg_node));
        if (kwarg_node) args_node_ref->children.push_back(std::move(kwarg_node));

    } else if (match(TokenType::TK_POWER)) {
        Token power_token = previous();
        shared_ptr<AstNode> kwarg_node = nullptr;
        if (!check(TokenType::TK_IDENTIFIER)) {
            reportError(peek(), "Expected identifier for **kwargs parameter name.");
            return;
        }
        Token name_token = consume(TokenType::TK_IDENTIFIER, "Expected identifier for **kwargs name.");
        if (this->had_error) return;
        kwarg_node = make_shared<AstNode>(NodeType::ParameterNode, power_token);
        kwarg_node->value = name_token.lexeme;
        if (kwarg_node) args_node_ref->children.push_back(std::move(kwarg_node));
    }
}

const vector<string>& Parser::getErrors() const {
    return errors_list;
}
void Parser::generateDotNode(shared_ptr<AstNode> node, string& output, int& nodeId, vector<pair<int, int>>& edges) {
    if (!node) return;

    int currentId = nodeId++;
    string label = nodeTypeToString(node->type);
    if (!node->value.empty()) {
        label += " \n ( Value: " + node->value + " )";
    }
    label += "\n ( Line: " + to_string( node->token.line) + " )";
    output += "    node" + to_string(currentId) + " [label=\"" + label + "\"];\n";

    for (const auto& child : node->children) {
        if (child) {
            int childId = nodeId;
            edges.push_back({currentId, childId});
            generateDotNode(child, output, nodeId, edges);
        }
    }
}
string Parser::nodeTypeToString(NodeType type) {
    switch (type) {
        case NodeType::ProgramNode: return "Program";
        case NodeType::StatementNode: return "Statement";
        case NodeType::ExpressionNode: return "Expression";
        case NodeType::PassStatementNode: return "PassStatement";
        case NodeType::AssignmentStatementNode: return "AssignmentStatement";
        case NodeType::AugAssignNode: return "AugAssign";
        case NodeType::ExpressionStatementNode: return "ExpressionStatement";
        case NodeType::ReturnStatementNode: return "ReturnStatement";
        case NodeType::TupleLiteralNode: return "TupleLiteral";
        case NodeType::IdentifierNode: return "Identifier";
        case NodeType::NamedImportNode: return "NamedImport";
        case NodeType::ImportStatementNode: return "ImportStatement";
        case NodeType::IfExpNode: return "IfExp";
        case NodeType::BinaryOpNode: return "BinaryOp";
        case NodeType::UnaryOpNode: return "UnaryOp";
        case NodeType::ComparisonNode: return "Comparison";
        case NodeType::AttributeAccessNode: return "AttributeAccess";
        case NodeType::KeywordArgNode: return "KeywordArg";
        case NodeType::FunctionCallNode: return "FunctionCall";
        case NodeType::SubscriptionNode: return "Subscription";
        case NodeType::BooleanLiteralNode: return "BooleanLiteral";
        case NodeType::NoneLiteralNode: return "NoneLiteral";
        case NodeType::NumberLiteralNode: return "NumberLiteral";
        case NodeType::ComplexLiteralNode: return "ComplexLiteral";
        case NodeType::StringLiteralNode: return "StringLiteral";
        case NodeType::ListLiteralNode: return "ListLiteral";
        case NodeType::BlockNode: return "Block";
        case NodeType::FunctionDefinitionNode: return "FunctionDefinition";
        case NodeType::ArgumentsNode: return "Arguments";
        case NodeType::RaiseStatementNode: return "RaiseStatement";
        case NodeType::GlobalStatementNode: return "GlobalStatement";
        case NodeType::NonlocalStatementNode: return "NonlocalStatement";
        case NodeType::IfStatementNode: return "IfStatement";
        case NodeType::ClassDefinitionNode: return "ClassDefinition";
        case NodeType::ForStatementNode: return "ForStatement";
        case NodeType::TryStatementNode: return "TryStatement";
        case NodeType::ExceptionHandlerNode: return "ExceptionHandler";
        case NodeType::ParameterNode: return "Parameter";
        case NodeType::WhileStatementNode: return "WhileStatement";
        case NodeType::DictLiteralNode: return "DictLiteral";
        case NodeType::SliceNode: return "Slice";
        case NodeType::BytesLiteralNode: return "BytesLiteral";
        case NodeType::SetLiteralNode: return "SetLiteral";
        case NodeType::BreakStatementNode: return "BreakStatement";
        case NodeType::ContinueStatementNode: return "ContinueStatement";
        default: return "Unknown";
    }
}
string Parser::generateDot(shared_ptr<AstNode> root) {
    string output = "digraph AST {\n";
    output += "    rankdir=TB;\n"; // Top-to-bottom layout
    output += "    node [shape=box, style=filled, fillcolor=lightblue];\n"; // Box shape for nodes

    int nodeId = 0;
    vector<pair<int, int>> edges;
    generateDotNode(root, output, nodeId, edges);

    for (const auto& edge : edges) {
        output += "    node" + to_string(edge.first) + " -> node" + to_string(edge.second) + ";\n";
    }

    output += "}\n";
    return output;
}



#include <fstream>

void Parser::saveDotFile(const string& dotContent, const string& filename) {
    ofstream out(filename);

    out << dotContent;
    out.close();
    std::filesystem::path fullPath = std::filesystem::absolute(filename);
    dotFilePath = fullPath.string(); // Return the full path as std::string
}

string Parser::getDotFilePath() const {
    return dotFilePath;
}