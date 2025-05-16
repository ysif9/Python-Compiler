#include "Parser.hpp"
#include <stdexcept>
using namespace std;


Parser::Parser(Lexer& lexer) : lexer(lexer) {
    advance(); // Initialize currentToken
}

void Parser::advance() {
    currentToken = lexer.nextToken();
}

bool Parser::match(TokenType type) {
    return currentToken.type == type;
}

bool Parser::expect(TokenType type, const string& errorMsg) {
    if (match(type)) {
        advance();
        return true;
    }
    reportError(errorMsg + " but got " + tokenTypeToString(currentToken.type));
    return false;
}

void Parser::reportError(const string& message) {
    errors.push_back("Syntax error at line " + to_string(currentToken.line) + ": " + message);
}

shared_ptr<AstNode> Parser::parse() {
    auto module = parseFile();
    if (!match(TokenType::TK_EOF)) {
        reportError("Expected end of input");
    }
    return module;
}

shared_ptr<AstNode> Parser::parseFile() {
    return parseStatementsOpt();
}

shared_ptr<AstNode> Parser::parseStatementsOpt() {
    auto node = make_shared<AstNode>(NodeType::Module, currentToken);
    if (!match(TokenType::TK_EOF)) {
        node->children.push_back(parseStatements());
    }
    return node;
}

shared_ptr<AstNode> Parser::parseStatements() {
    auto node = make_shared<AstNode>(NodeType::Module, currentToken);
    node->children.push_back(parseStatement());
    node->children.push_back(parseStatementStar());
    return node;
}

shared_ptr<AstNode> Parser::parseStatement() {
    if (match(TokenType::TK_DEF) || match(TokenType::TK_IF) || match(TokenType::TK_CLASS) ||
        match(TokenType::TK_WITH) || match(TokenType::TK_FOR) || match(TokenType::TK_TRY) ||
        match(TokenType::TK_WHILE)) {
        return parseCompoundStmt();
        } else {
            return parseSimpleStmts();
        }
}

shared_ptr<AstNode> Parser::parseStatementStar() {
    auto node = make_shared<AstNode>(NodeType::Stmt, currentToken);
    while (!match(TokenType::TK_EOF)) {
        node->children.push_back(parseStatement());
    }
    return node;
}

shared_ptr<AstNode> Parser::parseSimpleStmts() {
    auto node = make_shared<AstNode>(NodeType::Stmt, currentToken);
    node->children.push_back(parseSimpleStmtList());
    node->children.push_back(parseOptionalSemicolon());
    return node;
}

shared_ptr<AstNode> Parser::parseSimpleStmtList() {
    auto node = make_shared<AstNode>(NodeType::Stmt, currentToken);
    node->children.push_back(parseSimpleStmt());
    node->children.push_back(parseSimpleStmtListTailStar());
    return node;
}

shared_ptr<AstNode> Parser::parseSimpleStmtListTailStar() {
    auto node = make_shared<AstNode>(NodeType::Stmt, currentToken);
    while (match(TokenType::TK_SEMICOLON)) {
        advance();
        node->children.push_back(parseSimpleStmt());
    }
    return node;
}

shared_ptr<AstNode> Parser::parseOptionalSemicolon() {
    auto node = make_shared<AstNode>(NodeType::Stmt, currentToken);
    if (match(TokenType::TK_SEMICOLON)) {
        node->value = ";";
        advance();
    }
    return node;
}

shared_ptr<AstNode> Parser::parseSimpleStmt() {
    if (match(TokenType::TK_IDENTIFIER)) {
        auto nextToken = lexer.nextToken();
        lexer.tokens.push_back(currentToken);
        currentToken = nextToken;
        if (match(TokenType::TK_ASSIGN) || match(TokenType::TK_COLON)) {
            lexer.tokens.pop_back();
            currentToken = lexer.tokens.back();
            lexer.tokens.pop_back();
            return parseAssignment();
        } else {
            lexer.tokens.pop_back();
            currentToken = lexer.tokens.back();
            lexer.tokens.pop_back();
            return parseStarExpressions();
        }
    } else if (match(TokenType::TK_RETURN)) {
        return parseReturnStmt();
    } else if (match(TokenType::TK_IMPORT)) {
        return parseImportStmt();
    } else if (match(TokenType::TK_RAISE)) {
        return parseRaiseStmt();
    } else if (match(TokenType::TK_PASS)) {
        auto node = make_shared<AstNode>(NodeType::Pass, currentToken);
        advance();
        return node;
    } else if (match(TokenType::TK_DEL)) {
        return parseDelStmt();
    } else if (match(TokenType::TK_YIELD)) {
        return parseYieldStmt();
    } else if (match(TokenType::TK_ASSERT)) {
        return parseAssertStmt();
    } else if (match(TokenType::TK_BREAK)) {
        auto node = make_shared<AstNode>(NodeType::Break, currentToken);
        advance();
        return node;
    } else if (match(TokenType::TK_CONTINUE)) {
        auto node = make_shared<AstNode>(NodeType::Continue, currentToken);
        advance();
        return node;
    } else if (match(TokenType::TK_GLOBAL)) {
        return parseGlobalStmt();
    } else if (match(TokenType::TK_NONLOCAL)) {
        return parseNonlocalStmt();
    } else {
        return parseStarExpressions();
    }
}

shared_ptr<AstNode> Parser::parseCompoundStmt() {
    if (match(TokenType::TK_DEF)) {
        return parseFunctionDef();
    } else if (match(TokenType::TK_IF)) {
        return parseIfStmt();
    } else if (match(TokenType::TK_CLASS)) {
        return parseClassDef();
    } else if (match(TokenType::TK_WITH)) {
        return parseWithStmt();
    } else if (match(TokenType::TK_FOR)) {
        return parseForStmt();
    } else if (match(TokenType::TK_TRY)) {
        return parseTryStmt();
    } else if (match(TokenType::TK_WHILE)) {
        return parseWhileStmt();
    } else {
        reportError("Expected compound statement, got " + tokenTypeToString(currentToken.type));
        advance();
        return nullptr;
    }
}

shared_ptr<AstNode> Parser::parseAssignment() {
    auto token = currentToken;
    if (match(TokenType::TK_IDENTIFIER)) {
        auto target = make_shared<AstNode>(NodeType::Name, currentToken);
        target->value = currentToken.lexeme;
        advance();
        if (match(TokenType::TK_COLON)) {
            advance();
            auto expr = parseExpression();
            auto rhs = parseAssignmentEqAnnotatedRhsOpt();
            auto node = make_shared<AstNode>(NodeType::Assign, token);
            node->children.push_back(target);
            node->children.push_back(expr);
            if (rhs) node->children.push_back(rhs);
            return node;
        } else {
            auto node = parseStarTargetsEqPlus();
            if (node) return node;
            reportError("Expected ':' or '=' after identifier");
            return nullptr;
        }
    } else if (match(TokenType::TK_LPAREN)) {
        advance();
        auto target = parseSingleTarget();
        expect(TokenType::TK_RPAREN, "Expected ')'");
        expect(TokenType::TK_COLON, "Expected ':'");
        auto expr = parseExpression();
        auto rhs = parseAssignmentEqAnnotatedRhsOpt();
        auto node = make_shared<AstNode>(NodeType::Assign, token);
        node->children.push_back(target);
        node->children.push_back(expr);
        if (rhs) node->children.push_back(rhs);
        return node;
    } else if (match(TokenType::TK_IDENTIFIER) || match(TokenType::TK_LPAREN)) {
        auto target = parseSingleSubscriptAttributeTarget();
        expect(TokenType::TK_COLON, "Expected ':'");
        auto expr = parseExpression();
        auto rhs = parseAssignmentEqAnnotatedRhsOpt();
        auto node = make_shared<AstNode>(NodeType::Assign, token);
        node->children.push_back(target);
        node->children.push_back(expr);
        if (rhs) node->children.push_back(rhs);
        return node;
    } else {
        auto node = parseStarTargetsEqPlus();
        if (node) return node;
        auto target = parseSingleTarget();
        auto aug = parseAugassign();
        if (aug) {
            auto expr = parseYieldExprOrStarExpressions();
            auto node = make_shared<AstNode>(NodeType::Assign, token);
            node->children.push_back(target);
            node->children.push_back(aug);
            node->children.push_back(expr);
            return node;
        }
        reportError("Invalid assignment");
        advance();
        return nullptr;
    }
}

shared_ptr<AstNode> Parser::parseAssignmentEqAnnotatedRhsOpt() {
    if (match(TokenType::TK_ASSIGN)) {
        advance();
        return parseAnnotatedRhs();
    }
    return nullptr;
}

shared_ptr<AstNode> Parser::parseStarTargetsEqPlus() {
    auto token = currentToken;
    auto targets = parseStarTargets();
    if (!targets) return nullptr;
    if (match(TokenType::TK_ASSIGN)) {
        advance();
        auto node = make_shared<AstNode>(NodeType::Assign, token);
        node->children.push_back(targets);
        auto rest = parseStarTargetsEqPlusRest();
        if (rest) node->children.push_back(rest);
        return node;
    }
    return nullptr;
}

shared_ptr<AstNode> Parser::parseStarTargetsEqPlusRest() {
    if (match(TokenType::TK_ASSIGN)) {
        advance();
        auto node = make_shared<AstNode>(NodeType::Assign, currentToken);
        auto targets = parseStarTargets();
        if (targets) node->children.push_back(targets);
        auto rest = parseStarTargetsEqPlusRest();
        if (rest) node->children.push_back(rest);
        return node;
    }
    return parseYieldExprOrStarExpressions();
}

shared_ptr<AstNode> Parser::parseYieldExprOrStarExpressions() {
    if (match(TokenType::TK_YIELD)) {
        return parseYieldExpr();
    }
    return parseStarExpressions();
}

shared_ptr<AstNode> Parser::parseAnnotatedRhs() {
    if (match(TokenType::TK_YIELD)) {
        return parseYieldExpr();
    }
    return parseStarExpressions();
}

shared_ptr<AstNode> Parser::parseAugassign() {
    auto token = currentToken;
    if (match(TokenType::TK_PLUS_ASSIGN) || match(TokenType::TK_MINUS_ASSIGN) ||
        match(TokenType::TK_MULTIPLY_ASSIGN) || match(TokenType::TK_IMATMUL) ||
        match(TokenType::TK_DIVIDE_ASSIGN) || match(TokenType::TK_MOD_ASSIGN) ||
        match(TokenType::TK_BIT_AND_ASSIGN) || match(TokenType::TK_BIT_OR_ASSIGN) ||
        match(TokenType::TK_BIT_XOR_ASSIGN) || match(TokenType::TK_BIT_LEFT_SHIFT_ASSIGN) ||
        match(TokenType::TK_BIT_RIGHT_SHIFT_ASSIGN) || match(TokenType::TK_POWER_ASSIGN) ||
        match(TokenType::TK_FLOORDIV_ASSIGN)) {
        auto node = make_shared<AstNode>(NodeType::AugAssign, token);
        node->value = tokenTypeToString(currentToken.type);
        advance();
        return node;
        }
    return nullptr;
}

shared_ptr<AstNode> Parser::parseReturnStmt() {
    auto token = currentToken;
    expect(TokenType::TK_RETURN, "Expected 'return' keyword");
    auto node = make_shared<AstNode>(NodeType::Return, token);
    auto expr = parseStarExpressionsOpt();
    if (expr) node->children.push_back(expr);
    return node;
}

shared_ptr<AstNode> Parser::parseStarExpressionsOpt() {
    if (match(TokenType::TK_IDENTIFIER) || match(TokenType::TK_NUMBER) ||
        match(TokenType::TK_STRING) || match(TokenType::TK_TRUE) ||
        match(TokenType::TK_FALSE) || match(TokenType::TK_NONE) ||
        match(TokenType::TK_LPAREN)) {
        return parseStarExpressions();
        }
    return nullptr;
}

shared_ptr<AstNode> Parser::parseRaiseStmt() {
    auto token = currentToken;
    expect(TokenType::TK_RAISE, "Expected 'raise' keyword");
    auto node = make_shared<AstNode>(NodeType::Raise, token);
    if (match(TokenType::TK_SEMICOLON) || match(TokenType::TK_INDENT) ||
        match(TokenType::TK_EOF)) {
        return node;
        }
    auto expr = parseExpression();
    node->children.push_back(expr);
    auto from = parseRaiseFromOpt();
    if (from) node->children.push_back(from);
    return node;
}

shared_ptr<AstNode> Parser::parseRaiseFromOpt() {
    if (match(TokenType::TK_FROM)) {
        advance();
        return parseExpression();
    }
    return nullptr;
}

shared_ptr<AstNode> Parser::parseGlobalStmt() {
    auto token = currentToken;
    expect(TokenType::TK_GLOBAL, "Expected 'global' keyword");
    auto node = make_shared<AstNode>(NodeType::Global, token);
    node->children.push_back(parseNameCommaList());
    return node;
}

shared_ptr<AstNode> Parser::parseNonlocalStmt() {
    auto token = currentToken;
    expect(TokenType::TK_NONLOCAL, "Expected 'nonlocal' keyword");
    auto node = make_shared<AstNode>(NodeType::Nonlocal, token);
    node->children.push_back(parseNameCommaList());
    return node;
}

shared_ptr<AstNode> Parser::parseNameCommaList() {
    auto token = currentToken;
    expect(TokenType::TK_IDENTIFIER, "Expected identifier");
    auto node = make_shared<AstNode>(NodeType::Name, token);
    node->value = currentToken.lexeme;
    auto tail = parseNameCommaListTailStar();
    if (tail) node->children.push_back(tail);
    return node;
}

shared_ptr<AstNode> Parser::parseNameCommaListTailStar() {
    auto node = make_shared<AstNode>(NodeType::Stmt, currentToken);
    while (match(TokenType::TK_COMMA)) {
        advance();
        expect(TokenType::TK_IDENTIFIER, "Expected identifier after comma");
        auto id = make_shared<AstNode>(NodeType::Name, currentToken);
        id->value = currentToken.lexeme;
        node->children.push_back(id);
    }
    return node;
}

shared_ptr<AstNode> Parser::parseDelStmt() {
    auto token = currentToken;
    expect(TokenType::TK_DEL, "Expected 'del' keyword");
    auto node = make_shared<AstNode>(NodeType::Del, token);
    node->children.push_back(parseDelTargets());
    return node;
}

shared_ptr<AstNode> Parser::parseYieldStmt() {
    return parseYieldExpr();
}

shared_ptr<AstNode> Parser::parseAssertStmt() {
    auto token = currentToken;
    expect(TokenType::TK_ASSERT, "Expected 'assert' keyword");
    auto node = make_shared<AstNode>(NodeType::Assert, token);
    auto expr = parseExpression();
    node->children.push_back(expr);
    auto commaExpr = parseAssertCommaExprOpt();
    if (commaExpr) node->children.push_back(commaExpr);
    return node;
}

shared_ptr<AstNode> Parser::parseAssertCommaExprOpt() {
    if (match(TokenType::TK_COMMA)) {
        advance();
        return parseExpression();
    }
    return nullptr;
}

shared_ptr<AstNode> Parser::parseImportStmt() {
    if (match(TokenType::TK_IMPORT)) {
        return parseImportName();
    } else if (match(TokenType::TK_FROM)) {
        return parseImportFrom();
    }
    reportError("Expected 'import' or 'from' for import statement");
    advance();
    return nullptr;
}

shared_ptr<AstNode> Parser::parseImportName() {
    auto token = currentToken;
    expect(TokenType::TK_IMPORT, "Expected 'import' keyword");
    auto node = make_shared<AstNode>(NodeType::Import, token);
    node->children.push_back(parseDottedAsNames());
    return node;
}

shared_ptr<AstNode> Parser::parseImportFrom() {
    auto token = currentToken;
    expect(TokenType::TK_FROM, "Expected 'from' keyword");
    auto node = make_shared<AstNode>(NodeType::ImportFrom, token);
    auto dots = parseDotOrEllipsisStar();
    if (dots) node->children.push_back(dots);
    auto name = parseDottedName();
    node->children.push_back(name);
    expect(TokenType::TK_IMPORT, "Expected 'import' after from");
    node->children.push_back(parseImportFromTargets());
    return node;
}

shared_ptr<AstNode> Parser::parseDotOrEllipsisStar() {
    auto node = make_shared<AstNode>(NodeType::Stmt, currentToken);
    while (match(TokenType::TK_PERIOD)) {
        auto dot = make_shared<AstNode>(NodeType::Stmt, currentToken);
        dot->value = ".";
        advance();
        node->children.push_back(dot);
    }
    return node;
}

shared_ptr<AstNode> Parser::parseDotOrEllipsisPlus() {
    auto node = make_shared<AstNode>(NodeType::Stmt, currentToken);
    expect(TokenType::TK_PERIOD, "Expected '.'");
    auto dot = make_shared<AstNode>(NodeType::Stmt, currentToken);
    dot->value = ".";
    node->children.push_back(dot);
    auto star = parseDotOrEllipsisStar();
    if (star) node->children.push_back(star);
    return node;
}

shared_ptr<AstNode> Parser::parseImportFromTargets() {
    if (match(TokenType::TK_LPAREN)) {
        advance();
        auto node = make_shared<AstNode>(NodeType::Import, currentToken);
        node->children.push_back(parseImportFromAsNames());
        expect(TokenType::TK_RPAREN, "Expected ')'");
        auto comma = parseOptionalComma();
        if (comma) node->children.push_back(comma);
        return node;
    } else if (match(TokenType::TK_MULTIPLY)) {
        auto node = make_shared<AstNode>(NodeType::Import, currentToken);
        node->value = "*";
        advance();
        return node;
    }
    return parseImportFromAsNames();
}

shared_ptr<AstNode> Parser::parseOptionalComma() {
    if (match(TokenType::TK_COMMA)) {
        auto node = make_shared<AstNode>(NodeType::Stmt, currentToken);
        node->value = ",";
        advance();
        return node;
    }
    return nullptr;
}

shared_ptr<AstNode> Parser::parseImportFromAsNames() {
    auto node = make_shared<AstNode>(NodeType::Import, currentToken);
    node->children.push_back(parseImportFromAsName());
    auto tail = parseImportFromAsNameCommaListStar();
    if (tail) node->children.push_back(tail);
    return node;
}

shared_ptr<AstNode> Parser::parseImportFromAsNameCommaListStar() {
    auto node = make_shared<AstNode>(NodeType::Import, currentToken);
    while (match(TokenType::TK_COMMA)) {
        advance();
        node->children.push_back(parseImportFromAsName());
    }
    return node;
}

shared_ptr<AstNode> Parser::parseImportFromAsName() {
    auto token = currentToken;
    expect(TokenType::TK_IDENTIFIER, "Expected identifier");
    auto node = make_shared<AstNode>(NodeType::Name, token);
    node->value = currentToken.lexeme;
    auto as = parseImportFromAsNameAsOpt();
    if (as) node->children.push_back(as);
    return node;
}

shared_ptr<AstNode> Parser::parseImportFromAsNameAsOpt() {
    if (match(TokenType::TK_AS)) {
        advance();
        expect(TokenType::TK_IDENTIFIER, "Expected identifier after 'as'");
        auto node = make_shared<AstNode>(NodeType::Name, currentToken);
        node->value = currentToken.lexeme;
        advance();
        return node;
    }
    return nullptr;
}

shared_ptr<AstNode> Parser::parseDottedAsNames() {
    auto node = make_shared<AstNode>(NodeType::Import, currentToken);
    node->children.push_back(parseDottedAsName());
    auto tail = parseDottedAsNameCommaListStar();
    if (tail) node->children.push_back(tail);
    return node;
}

shared_ptr<AstNode> Parser::parseDottedAsNameCommaListStar() {
    auto node = make_shared<AstNode>(NodeType::Import, currentToken);
    while (match(TokenType::TK_COMMA)) {
        advance();
        node->children.push_back(parseDottedAsName());
    }
    return node;
}

shared_ptr<AstNode> Parser::parseDottedAsName() {
    auto node = make_shared<AstNode>(NodeType::Import, currentToken);
    node->children.push_back(parseDottedName());
    auto as = parseDottedAsNameAsOpt();
    if (as) node->children.push_back(as);
    return node;
}

shared_ptr<AstNode> Parser::parseDottedAsNameAsOpt() {
    if (match(TokenType::TK_AS)) {
        advance();
        expect(TokenType::TK_IDENTIFIER, "Expected identifier after 'as'");
        auto node = make_shared<AstNode>(NodeType::Name, currentToken);
        node->value = currentToken.lexeme;
        advance();
        return node;
    }
    return nullptr;
}

shared_ptr<AstNode> Parser::parseDottedName() {
    auto token = currentToken;
    expect(TokenType::TK_IDENTIFIER, "Expected identifier");
    auto node = make_shared<AstNode>(NodeType::Name, token);
    node->value = currentToken.lexeme;
    while (match(TokenType::TK_PERIOD)) {
        advance();
        expect(TokenType::TK_IDENTIFIER, "Expected identifier after '.'");
        auto child = make_shared<AstNode>(NodeType::Name, currentToken);
        child->value = currentToken.lexeme;
        node->children.push_back(child);
    }
    return node;
}

shared_ptr<AstNode> Parser::parseBlock() {
    if (match(TokenType::TK_INDENT)) {
        advance();
        auto node = make_shared<AstNode>(NodeType::Stmt, currentToken);
        node->children.push_back(parseStatements());
        expect(TokenType::TK_DEDENT, "Expected dedent after block");
        return node;
    }
    return parseSimpleStmts();
}

shared_ptr<AstNode> Parser::parseDecorators() {
    auto node = make_shared<AstNode>(NodeType::Stmt, currentToken);
    node->children.push_back(parseDecoratorPlus());
    return node;
}

shared_ptr<AstNode> Parser::parseDecoratorPlus() {
    auto node = make_shared<AstNode>(NodeType::Stmt, currentToken);
    node->children.push_back(parseDecoratorItem());
    auto tail = parseDecoratorPlusStar();
    if (tail) node->children.push_back(tail);
    return node;
}

shared_ptr<AstNode> Parser::parseDecoratorPlusStar() {
    auto node = make_shared<AstNode>(NodeType::Stmt, currentToken);
    while (match(TokenType::TK_MATMUL)) {
        node->children.push_back(parseDecoratorItem());
    }
    return node;
}

shared_ptr<AstNode> Parser::parseDecoratorItem() {
    auto token = currentToken;
    expect(TokenType::TK_MATMUL, "Expected '@' for decorator");
    auto node = make_shared<AstNode>(NodeType::Decorator, token);
    node->children.push_back(parseNamedExpression());
    return node;
}

shared_ptr<AstNode> Parser::parseClassDef() {
    auto token = currentToken;
    auto decorators = match(TokenType::TK_MATMUL) ? parseDecorators() : nullptr;
    auto node = parseClassDefRaw();
    if (decorators) node->children.insert(node->children.begin(), decorators);
    return node;
}

shared_ptr<AstNode> Parser::parseClassDefRaw() {
    auto token = currentToken;
    expect(TokenType::TK_CLASS, "Expected 'class' keyword");
    expect(TokenType::TK_IDENTIFIER, "Expected class name");
    auto node = make_shared<AstNode>(NodeType::ClassDef, token);
    node->value = currentToken.lexeme;
    auto args = parseClassArgumentsOpt();
    if (args) node->children.push_back(args);
    expect(TokenType::TK_COLON, "Expected ':'");
    node->children.push_back(parseBlock());
    return node;
}

shared_ptr<AstNode> Parser::parseClassArgumentsOpt() {
    if (match(TokenType::TK_LPAREN)) {
        advance();
        auto node = make_shared<AstNode>(NodeType::Stmt, currentToken);
        auto args = parseArgumentsOpt();
        if (args) node->children.push_back(args);
        expect(TokenType::TK_RPAREN, "Expected ')'");
        return node;
    }
    return nullptr;
}

shared_ptr<AstNode> Parser::parseArgumentsOpt() {
    if (match(TokenType::TK_IDENTIFIER) || match(TokenType::TK_NUMBER) ||
        match(TokenType::TK_STRING) || match(TokenType::TK_TRUE) ||
        match(TokenType::TK_FALSE) || match(TokenType::TK_NONE) ||
        match(TokenType::TK_LPAREN)) {
        return parseArguments();
    }
    return nullptr;
}


const vector<string>& Parser::getErrors() const {
    return errors;
}