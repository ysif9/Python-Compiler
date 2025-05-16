#include "Parser.hpp"
#include <stdexcept>
using namespace std;


Parser::Parser(Lexer& lexer) : lexer(lexer) {
    current_index = 0;
    currentToken = lexer.tokens[current_index];
}
void Parser::advance() {
    currentToken = lexer.tokens[++ current_index ];
}

bool Parser::match(TokenType type) {
    return currentToken.type == type;
}

bool Parser::expect(TokenType type, const string& errorMsg) {
    if (match(type)) {
        advance();
        return true;
    }
    errors.push_back("Syntax error at line " + to_string(currentToken.line) + ": " + errorMsg + " but got " + tokenTypeToString(currentToken.type));
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
    saveDotFile(generateDot(module), "AST.dot");
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

shared_ptr<AstNode> Parser::parseStatementStar() {
    auto node = make_shared<AstNode>(NodeType::Stmt, currentToken);
    while (!match(TokenType::TK_EOF)) {
        node->children.push_back(parseStatement());
    }
    return node;
}

shared_ptr<AstNode> Parser::parseStatement() {
    if (match(TokenType::TK_DEF) || match(TokenType::TK_IF) || match(TokenType::TK_CLASS) ||
        match(TokenType::TK_FOR) || match(TokenType::TK_TRY) || match(TokenType::TK_WHILE)) {
        return parseCompoundStmt();
    } else {
        return parseSimpleStmts();
    }
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
        if (match(TokenType::TK_ASSIGN) || match(TokenType::TK_PLUS_ASSIGN) ||
            match(TokenType::TK_MINUS_ASSIGN) || match(TokenType::TK_MULTIPLY_ASSIGN) ||
            match(TokenType::TK_DIVIDE_ASSIGN) || match(TokenType::TK_MOD_ASSIGN) ||
            match(TokenType::TK_BIT_AND_ASSIGN) || match(TokenType::TK_BIT_OR_ASSIGN) ||
            match(TokenType::TK_BIT_XOR_ASSIGN) || match(TokenType::TK_BIT_LEFT_SHIFT_ASSIGN) ||
            match(TokenType::TK_BIT_RIGHT_SHIFT_ASSIGN) || match(TokenType::TK_POWER_ASSIGN) ||
            match(TokenType::TK_FLOORDIV_ASSIGN)) {
            lexer.tokens.pop_back();
            currentToken = lexer.tokens.back();
            lexer.tokens.pop_back();
            return parseAssignment();
        } else {
            lexer.tokens.pop_back();
            currentToken = lexer.tokens.back();
            lexer.tokens.pop_back();
            return parseExpressions();
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
        return parseExpressions();
    }
}

shared_ptr<AstNode> Parser::parseCompoundStmt() {
    if (match(TokenType::TK_DEF)) {
        return parseFunctionDef();
    } else if (match(TokenType::TK_IF)) {
        return parseIfStmt();
    } else if (match(TokenType::TK_CLASS)) {
        return parseClassDef();
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
    auto node = make_shared<AstNode>(NodeType::Assign, token);
    if (match(TokenType::TK_IDENTIFIER)) {
        auto nextToken = lexer.nextToken();
        lexer.tokens.push_back(currentToken);
        currentToken = nextToken;
        if (match(TokenType::TK_PERIOD) || match(TokenType::TK_LBRACKET) || match(TokenType::TK_LPAREN)) {
            lexer.tokens.pop_back();
            currentToken = lexer.tokens.back();
            lexer.tokens.pop_back();
            auto target = parseSingleTarget();
            auto aug = parseAugassign();
            if (aug) {
                node->children.push_back(target);
                node->children.push_back(aug);
                node->children.push_back(parseExpressions());
                return node;
            }
            lexer.tokens.push_back(currentToken);
            currentToken = nextToken;
        }
        lexer.tokens.pop_back();
        currentToken = lexer.tokens.back();
        lexer.tokens.pop_back();
    }
    auto targets = parseTargets();
    if (targets && match(TokenType::TK_ASSIGN)) {
        advance();
        node->children.push_back(targets);
        node->children.push_back(parseExpressions());
        return node;
    }
    auto single = parseSingleTarget();
    auto aug = parseAugassign();
    if (single && aug) {
        node->children.push_back(single);
        node->children.push_back(aug);
        node->children.push_back(parseExpressions());
        return node;
    }
    reportError("Invalid assignment");
    advance();
    return nullptr;
}

shared_ptr<AstNode> Parser::parseAugassign() {
    auto token = currentToken;
    if (match(TokenType::TK_PLUS_ASSIGN) || match(TokenType::TK_MINUS_ASSIGN) ||
        match(TokenType::TK_MULTIPLY_ASSIGN) || match(TokenType::TK_DIVIDE_ASSIGN) ||
        match(TokenType::TK_MOD_ASSIGN) || match(TokenType::TK_BIT_AND_ASSIGN) ||
        match(TokenType::TK_BIT_OR_ASSIGN) || match(TokenType::TK_BIT_XOR_ASSIGN) ||
        match(TokenType::TK_BIT_LEFT_SHIFT_ASSIGN) || match(TokenType::TK_BIT_RIGHT_SHIFT_ASSIGN) ||
        match(TokenType::TK_POWER_ASSIGN) || match(TokenType::TK_FLOORDIV_ASSIGN)) {
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
    auto expr = parseExpressionsOpt();
    if (expr) node->children.push_back(expr);
    return node;
}

shared_ptr<AstNode> Parser::parseExpressionsOpt() {
    if (match(TokenType::TK_IDENTIFIER) || match(TokenType::TK_NUMBER) ||
        match(TokenType::TK_STRING) || match(TokenType::TK_TRUE) ||
        match(TokenType::TK_FALSE) || match(TokenType::TK_NONE) ||
        match(TokenType::TK_LPAREN)) {
        return parseExpressions();
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

shared_ptr<AstNode> Parser::parseClassDef() {
    return parseClassDefRaw();
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






shared_ptr<AstNode> Parser::parseFunctionDef() {
    return parseFunctionDefRaw();
}

shared_ptr<AstNode> Parser::parseFunctionDefRaw() {
    auto token = currentToken;
    expect(TokenType::TK_DEF, "Expected 'def' keyword");
    expect(TokenType::TK_IDENTIFIER, "Expected function name");
    auto node = make_shared<AstNode>(NodeType::FunctionDef, token);
    node->value = currentToken.lexeme;
    expect(TokenType::TK_LPAREN, "Expected '('");
    auto params = parseParamsOpt();
    if (params) node->children.push_back(params);
    expect(TokenType::TK_RPAREN, "Expected ')'");
    expect(TokenType::TK_COLON, "Expected ':'");
    node->children.push_back(parseBlock());
    return node;
}

shared_ptr<AstNode> Parser::parseParamsOpt() {
    if (match(TokenType::TK_IDENTIFIER) || match(TokenType::TK_MULTIPLY) || match(TokenType::TK_POWER)) {
        return parseParams();
    }
    return nullptr;
}

shared_ptr<AstNode> Parser::parseParams() {
    return parseParameters();
}

shared_ptr<AstNode> Parser::parseParameters() {
    auto token = currentToken;
    auto node = make_shared<AstNode>(NodeType::Params, token);
    if (match(TokenType::TK_MULTIPLY)) {
        auto starEtc = parseSimplifiedStarEtc();
        if (starEtc) node->children.push_back(starEtc);
    } else if (match(TokenType::TK_POWER)) {
        auto kwds = parseKwds();
        if (kwds) node->children.push_back(kwds);
    } else {
        auto noDefault = parseParamNoDefaultPlus();
        if (noDefault) node->children.push_back(noDefault);
        auto withDefault = parseParamWithDefaultStar();
        if (withDefault) node->children.push_back(withDefault);
        auto starEtc = parseSimplifiedStarEtcOpt();
        if (starEtc) node->children.push_back(starEtc);
    }
    return node;
}

shared_ptr<AstNode> Parser::parseParamNoDefaultStar() {
    auto node = make_shared<AstNode>(NodeType::Params, currentToken);
    while (match(TokenType::TK_IDENTIFIER) && !match(TokenType::TK_ASSIGN)) {
        node->children.push_back(parseParamNoDefault());
    }
    return node;
}

shared_ptr<AstNode> Parser::parseParamWithDefaultStar() {
    auto node = make_shared<AstNode>(NodeType::Params, currentToken);
    while (match(TokenType::TK_IDENTIFIER)) {
        auto nextToken = lexer.nextToken();
        lexer.tokens.push_back(currentToken);
        currentToken = nextToken;
        if (match(TokenType::TK_ASSIGN)) {
            lexer.tokens.pop_back();
            currentToken = lexer.tokens.back();
            lexer.tokens.pop_back();
            node->children.push_back(parseParamWithDefault());
        } else {
            lexer.tokens.pop_back();
            currentToken = lexer.tokens.back();
            lexer.tokens.pop_back();
            break;
        }
    }
    return node;
}

shared_ptr<AstNode> Parser::parseSimplifiedStarEtcOpt() {
    if (match(TokenType::TK_MULTIPLY) || match(TokenType::TK_POWER)) {
        return parseSimplifiedStarEtc();
    }
    return nullptr;
}

shared_ptr<AstNode> Parser::parseParamNoDefaultPlus() {
    auto token = currentToken;
    auto param = parseParamNoDefault();
    if (!param) return nullptr;
    auto node = make_shared<AstNode>(NodeType::Params, token);
    node->children.push_back(param);
    auto star = parseParamNoDefaultStar();
    if (star) node->children.push_back(star);
    return node;
}

shared_ptr<AstNode> Parser::parseParamWithDefaultPlus() {
    auto token = currentToken;
    auto param = parseParamWithDefault();
    if (!param) return nullptr;
    auto node = make_shared<AstNode>(NodeType::Params, token);
    node->children.push_back(param);
    auto star = parseParamWithDefaultStar();
    if (star) node->children.push_back(star);
    return node;
}

shared_ptr<AstNode> Parser::parseSimplifiedStarEtc() {
    auto token = currentToken;
    auto node = make_shared<AstNode>(NodeType::Params, token);
    if (match(TokenType::TK_MULTIPLY)) {
        advance();
        auto param = parseParamNoDefault();
        node->children.push_back(param);
        auto kwds = parseKwdsOpt();
        if (kwds) node->children.push_back(kwds);
    } else if (match(TokenType::TK_POWER)) {
        auto kwds = parseKwds();
        if (kwds) node->children.push_back(kwds);
    } else {
        reportError("Expected '*' or '**' for star arguments");
        advance();
        return nullptr;
    }
    return node;
}

shared_ptr<AstNode> Parser::parseKwdsOpt() {
    if (match(TokenType::TK_POWER)) {
        return parseKwds();
    }
    return nullptr;
}

shared_ptr<AstNode> Parser::parseKwds() {
    auto token = currentToken;
    expect(TokenType::TK_POWER, "Expected '**' for kwargs");
    auto node = make_shared<AstNode>(NodeType::Params, token);
    node->children.push_back(parseParamNoDefault());
    return node;
}

shared_ptr<AstNode> Parser::parseParamNoDefault() {
    auto token = currentToken;
    auto param = parseParam();
    if (!param) return nullptr;
    auto node = make_shared<AstNode>(NodeType::Param, token);
    node->children.push_back(param);
    auto ending = parseParamEndingChar();
    if (ending) node->children.push_back(ending);
    return node;
}

shared_ptr<AstNode> Parser::parseParamWithDefault() {
    auto token = currentToken;
    auto param = parseParam();
    if (!param) return nullptr;
    auto node = make_shared<AstNode>(NodeType::Param, token);
    node->children.push_back(param);
    auto def = parseDefault();
    if (def) node->children.push_back(def);
    auto ending = parseParamEndingChar();
    if (ending) node->children.push_back(ending);
    return node;
}

shared_ptr<AstNode> Parser::parseParamEndingChar() {
    if (match(TokenType::TK_COMMA)) {
        auto node = make_shared<AstNode>(NodeType::Stmt, currentToken);
        node->value = ",";
        advance();
        return node;
    }
    return nullptr;
}

shared_ptr<AstNode> Parser::parseDefault() {
    if (match(TokenType::TK_ASSIGN)) {
        advance();
        return parseExpression();
    }
    return nullptr;
}

shared_ptr<AstNode> Parser::parseParam() {
    if (match(TokenType::TK_IDENTIFIER)) {
        auto node = make_shared<AstNode>(NodeType::Name, currentToken);
        node->value = currentToken.lexeme;
        advance();
        return node;
    }
    reportError("Expected identifier for parameter");
    advance();
    return nullptr;
}

shared_ptr<AstNode> Parser::parseIfStmt() {
    auto token = currentToken;
    expect(TokenType::TK_IF, "Expected 'if' keyword");
    auto node = make_shared<AstNode>(NodeType::If, token);
    node->children.push_back(parseExpression());
    expect(TokenType::TK_COLON, "Expected ':'");
    node->children.push_back(parseBlock());
    if (match(TokenType::TK_ELIF)) {
        node->children.push_back(parseElifStmt());
    } else {
        auto elseBlock = parseElseBlockOpt();
        if (elseBlock) node->children.push_back(elseBlock);
    }
    return node;
}

shared_ptr<AstNode> Parser::parseElifStmt() {
    auto token = currentToken;
    expect(TokenType::TK_ELIF, "Expected 'elif' keyword");
    auto node = make_shared<AstNode>(NodeType::If, token);
    node->children.push_back(parseExpression());
    expect(TokenType::TK_COLON, "Expected ':'");
    node->children.push_back(parseBlock());
    if (match(TokenType::TK_ELIF)) {
        node->children.push_back(parseElifStmt());
    } else {
        auto elseBlock = parseElseBlockOpt();
        if (elseBlock) node->children.push_back(elseBlock);
    }
    return node;
}

shared_ptr<AstNode> Parser::parseElseBlockOpt() {
    if (match(TokenType::TK_ELSE)) {
        return parseElseBlock();
    }
    return nullptr;
}

shared_ptr<AstNode> Parser::parseElseBlock() {
    auto token = currentToken;
    expect(TokenType::TK_ELSE, "Expected 'else' keyword");
    expect(TokenType::TK_COLON, "Expected ':'");
    auto node = make_shared<AstNode>(NodeType::Stmt, token);
    node->children.push_back(parseBlock());
    return node;
}

shared_ptr<AstNode> Parser::parseWhileStmt() {
    auto token = currentToken;
    expect(TokenType::TK_WHILE, "Expected 'while' keyword");
    auto node = make_shared<AstNode>(NodeType::While, token);
    node->children.push_back(parseExpression());
    expect(TokenType::TK_COLON, "Expected ':'");
    node->children.push_back(parseBlock());
    auto elseBlock = parseElseBlockOpt();
    if (elseBlock) node->children.push_back(elseBlock);
    return node;
}

shared_ptr<AstNode> Parser::parseForStmt() {
    auto token = currentToken;
    expect(TokenType::TK_FOR, "Expected 'for' keyword");
    auto node = make_shared<AstNode>(NodeType::For, token);
    node->children.push_back(parseTargets());
    expect(TokenType::TK_IN, "Expected 'in' keyword");
    node->children.push_back(parseExpressions());
    expect(TokenType::TK_COLON, "Expected ':'");
    node->children.push_back(parseBlock());
    auto elseBlock = parseElseBlockOpt();
    if (elseBlock) node->children.push_back(elseBlock);
    return node;
}

shared_ptr<AstNode> Parser::parseTryStmt() {
    auto token = currentToken;
    expect(TokenType::TK_TRY, "Expected 'try' keyword");
    expect(TokenType::TK_COLON, "Expected ':'");
    auto node = make_shared<AstNode>(NodeType::Try, token);
    node->children.push_back(parseBlock());
    if (match(TokenType::TK_EXCEPT)) {
        auto except = parseExceptBlockPlus();
        if (except) node->children.push_back(except);
        auto elseBlock = parseElseBlockOpt();
        if (elseBlock) node->children.push_back(elseBlock);
        auto finally = parseFinallyBlockOpt();
        if (finally) node->children.push_back(finally);
    } else {
        auto finally = parseFinallyBlock();
        if (finally) node->children.push_back(finally);
    }
    return node;
}

shared_ptr<AstNode> Parser::parseExceptBlockPlus() {
    auto token = currentToken;
    auto node = make_shared<AstNode>(NodeType::Stmt, token);
    node->children.push_back(parseExceptBlock());
    auto star = parseExceptBlockPlusStar();
    if (star) node->children.push_back(star);
    return node;
}

shared_ptr<AstNode> Parser::parseExceptBlockPlusStar() {
    auto node = make_shared<AstNode>(NodeType::Stmt, currentToken);
    while (match(TokenType::TK_EXCEPT)) {
        node->children.push_back(parseExceptBlock());
    }
    return node;
}

shared_ptr<AstNode> Parser::parseFinallyBlockOpt() {
    if (match(TokenType::TK_FINALLY)) {
        return parseFinallyBlock();
    }
    return nullptr;
}

shared_ptr<AstNode> Parser::parseExceptBlock() {
    auto token = currentToken;
    expect(TokenType::TK_EXCEPT, "Expected 'except' keyword");
    auto node = make_shared<AstNode>(NodeType::Except, token);
    if (!match(TokenType::TK_COLON)) {
        node->children.push_back(parseExpression());
        auto asName = parseExceptAsNameOpt();
        if (asName) node->children.push_back(asName);
    }
    expect(TokenType::TK_COLON, "Expected ':'");
    node->children.push_back(parseBlock());
    return node;
}

shared_ptr<AstNode> Parser::parseExceptAsNameOpt() {
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

shared_ptr<AstNode> Parser::parseFinallyBlock() {
    auto token = currentToken;
    expect(TokenType::TK_FINALLY, "Expected 'finally' keyword");
    expect(TokenType::TK_COLON, "Expected ':'");
    auto node = make_shared<AstNode>(NodeType::Finally, token);
    node->children.push_back(parseBlock());
    return node;
}

shared_ptr<AstNode> Parser::parseExpressions() {
    auto token = currentToken;
    auto node = make_shared<AstNode>(NodeType::Expr, token);
    if (match(TokenType::TK_COMMA)) {
        advance();
        return parseExpression();
    }
    auto expr = parseExpression();
    node->children.push_back(expr);
    if (match(TokenType::TK_COMMA)) {
        auto commaList = parseExpressionCommaPlus();
        if (commaList) node->children.push_back(commaList);
        auto comma = parseOptionalComma();
        if (comma) node->children.push_back(comma);
    }
    return node;
}

shared_ptr<AstNode> Parser::parseExpressionCommaPlus() {
    auto token = currentToken;
    expect(TokenType::TK_COMMA, "Expected ','");
    auto node = make_shared<AstNode>(NodeType::Expr, token);
    node->children.push_back(parseExpression());
    auto star = parseExpressionCommaPlusStar();
    if (star) node->children.push_back(star);
    return node;
}

shared_ptr<AstNode> Parser::parseExpressionCommaPlusStar() {
    auto node = make_shared<AstNode>(NodeType::Expr, currentToken);
    while (match(TokenType::TK_COMMA)) {
        advance();
        node->children.push_back(parseExpression());
    }
    return node;
}

shared_ptr<AstNode> Parser::parseExpression() {
    auto token = currentToken;
    auto disjunction = parseDisjunction();
    if (match(TokenType::TK_IF)) {
        advance();
        auto node = make_shared<AstNode>(NodeType::Expr, token);
        node->children.push_back(disjunction);
        auto condition = parseDisjunction();
        node->children.push_back(condition);
        expect(TokenType::TK_ELSE, "Expected 'else' keyword");
        node->children.push_back(parseExpression());
        return node;
    }
    return disjunction;
}

shared_ptr<AstNode> Parser::parseDisjunction() {
    auto token = currentToken;
    auto node = make_shared<AstNode>(NodeType::Expr, token);
    node->children.push_back(parseConjunction());
    auto tail = parseDisjunctionTailStar();
    if (tail) node->children.push_back(tail);
    return node;
}

shared_ptr<AstNode> Parser::parseDisjunctionTailStar() {
    auto node = make_shared<AstNode>(NodeType::Expr, currentToken);
    while (match(TokenType::TK_OR)) {
        auto token = currentToken;
        advance();
        auto op = make_shared<AstNode>(NodeType::BinOp, token);
        op->value = "or";
        op->children.push_back(parseConjunction());
        node->children.push_back(op);
    }
    return node;
}

shared_ptr<AstNode> Parser::parseConjunction() {
    auto token = currentToken;
    auto node = make_shared<AstNode>(NodeType::Expr, token);
    node->children.push_back(parseInversion());
    auto tail = parseConjunctionTailStar();
    if (tail) node->children.push_back(tail);
    return node;
}

shared_ptr<AstNode> Parser::parseConjunctionTailStar() {
    auto node = make_shared<AstNode>(NodeType::Expr, currentToken);
    while (match(TokenType::TK_AND)) {
        auto token = currentToken;
        advance();
        auto op = make_shared<AstNode>(NodeType::BinOp, token);
        op->value = "and";
        op->children.push_back(parseInversion());
        node->children.push_back(op);
    }
    return node;
}

shared_ptr<AstNode> Parser::parseInversion() {
    if (match(TokenType::TK_NOT)) {
        auto token = currentToken;
        advance();
        auto node = make_shared<AstNode>(NodeType::Expr, token);
        node->value = "not";
        node->children.push_back(parseInversion());
        return node;
    }
    return parseComparison();
}

shared_ptr<AstNode> Parser::parseComparison() {
    auto token = currentToken;
    auto node = make_shared<AstNode>(NodeType::Expr, token);
    node->children.push_back(parseBitwiseOr());
    auto tail = parseCompareOpBitwiseOrPairStar();
    if (tail) node->children.push_back(tail);
    return node;
}

shared_ptr<AstNode> Parser::parseCompareOpBitwiseOrPairStar() {
    auto node = make_shared<AstNode>(NodeType::Expr, currentToken);
    while (match(TokenType::TK_EQUAL) || match(TokenType::TK_NOT_EQUAL) ||
           match(TokenType::TK_LESS_EQUAL) || match(TokenType::TK_LESS) ||
           match(TokenType::TK_GREATER_EQUAL) || match(TokenType::TK_GREATER) ||
           match(TokenType::TK_NOT) || match(TokenType::TK_IN) ||
           match(TokenType::TK_IS)) {
        node->children.push_back(parseCompareOpBitwiseOrPair());
    }
    return node;
}

shared_ptr<AstNode> Parser::parseCompareOpBitwiseOrPair() {
    auto token = currentToken;
    auto node = make_shared<AstNode>(NodeType::BinOp, token);
    if (match(TokenType::TK_EQUAL)) {
        node->value = "==";
        advance();
    } else if (match(TokenType::TK_NOT_EQUAL)) {
        node->value = "!=";
        advance();
    } else if (match(TokenType::TK_LESS_EQUAL)) {
        node->value = "<=";
        advance();
    } else if (match(TokenType::TK_LESS)) {
        node->value = "<";
        advance();
    } else if (match(TokenType::TK_GREATER_EQUAL)) {
        node->value = ">=";
        advance();
    } else if (match(TokenType::TK_GREATER)) {
        node->value = ">";
        advance();
    } else if (match(TokenType::TK_NOT)) {
        expect(TokenType::TK_IN, "Expected 'in' after 'not'");
        node->value = "not in";
        advance();
    } else if (match(TokenType::TK_IN)) {
        node->value = "in";
        advance();
    } else if (match(TokenType::TK_IS)) {
        auto nextToken = lexer.nextToken();
        lexer.tokens.push_back(currentToken);
        currentToken = nextToken;
        if (match(TokenType::TK_NOT)) {
            lexer.tokens.pop_back();
            currentToken = lexer.tokens.back();
            lexer.tokens.pop_back();
            advance();
            expect(TokenType::TK_NOT, "Expected 'not' after 'is'");
            node->value = "is not";
            advance();
        } else {
            lexer.tokens.pop_back();
            currentToken = lexer.tokens.back();
            lexer.tokens.pop_back();
            advance();
            node->value = "is";
        }
    } else {
        reportError("Expected comparison operator");
        advance();
        return nullptr;
    }
    node->children.push_back(parseBitwiseOr());
    return node;
}

shared_ptr<AstNode> Parser::parseBitwiseOr() {
    auto left = parseBitwiseXor();
    if (match(TokenType::TK_BIT_OR)) {
        auto token = currentToken;
        advance();
        auto node = make_shared<AstNode>(NodeType::BinOp, token);
        node->value = "|";
        node->children.push_back(left);
        node->children.push_back(parseBitwiseOr());
        return node;
    }
    return left;
}

shared_ptr<AstNode> Parser::parseBitwiseXor() {
    auto left = parseBitwiseAnd();
    if (match(TokenType::TK_BIT_XOR)) {
        auto token = currentToken;
        advance();
        auto node = make_shared<AstNode>(NodeType::BinOp, token);
        node->value = "^";
        node->children.push_back(left);
        node->children.push_back(parseBitwiseXor());
        return node;
    }
    return left;
}

shared_ptr<AstNode> Parser::parseBitwiseAnd() {
    auto left = parseShiftExpr();
    if (match(TokenType::TK_BIT_AND)) {
        auto token = currentToken;
        advance();
        auto node = make_shared<AstNode>(NodeType::BinOp, token);
        node->value = "&";
        node->children.push_back(left);
        node->children.push_back(parseBitwiseAnd());
        return node;
    }
    return left;
}

shared_ptr<AstNode> Parser::parseShiftExpr() {
    auto left = parseSum();
    if (match(TokenType::TK_BIT_LEFT_SHIFT)) {
        auto token = currentToken;
        advance();
        auto node = make_shared<AstNode>(NodeType::BinOp, token);
        node->value = "<<";
        node->children.push_back(left);
        node->children.push_back(parseSum());
        return node;
    } else if (match(TokenType::TK_BIT_RIGHT_SHIFT)) {
        auto token = currentToken;
        advance();
        auto node = make_shared<AstNode>(NodeType::BinOp, token);
        node->value = ">>";
        node->children.push_back(left);
        node->children.push_back(parseSum());
        return node;
    }
    return left;
}

shared_ptr<AstNode> Parser::parseSum() {
    auto left = parseTerm();
    while (match(TokenType::TK_PLUS) || match(TokenType::TK_MINUS)) {
        auto token = currentToken;
        auto node = make_shared<AstNode>(NodeType::BinOp, token);
        if (match(TokenType::TK_PLUS)) {
            node->value = "+";
        } else {
            node->value = "-";
        }
        advance();
        node->children.push_back(left);
        node->children.push_back(parseTerm());
        left = node;
    }
    return left;
}

shared_ptr<AstNode> Parser::parseTerm() {
    auto left = parseFactor();
    while (match(TokenType::TK_MULTIPLY) || match(TokenType::TK_DIVIDE) ||
           match(TokenType::TK_FLOORDIV) || match(TokenType::TK_MOD)) {
        auto token = currentToken;
        auto node = make_shared<AstNode>(NodeType::BinOp, token);
        if (match(TokenType::TK_MULTIPLY)) {
            node->value = "*";
        } else if (match(TokenType::TK_DIVIDE)) {
            node->value = "/";
        } else if (match(TokenType::TK_FLOORDIV)) {
            node->value = "//";
        } else {
            node->value = "%";
        }
        advance();
        node->children.push_back(left);
        node->children.push_back(parseFactor());
        left = node;
    }
    return left;
}

shared_ptr<AstNode> Parser::parseFactor() {
    if (match(TokenType::TK_PLUS) || match(TokenType::TK_MINUS) || match(TokenType::TK_BIT_NOT)) {
        auto token = currentToken;
        auto node = make_shared<AstNode>(NodeType::Expr, token);
        if (match(TokenType::TK_PLUS)) {
            node->value = "+";
        } else if (match(TokenType::TK_MINUS)) {
            node->value = "-";
        } else {
            node->value = "~";
        }
        advance();
        node->children.push_back(parseFactor());
        return node;
    }
    return parsePower();
}

shared_ptr<AstNode> Parser::parsePower() {
    auto left = parsePrimary();
    if (match(TokenType::TK_POWER)) {
        auto token = currentToken;
        advance();
        auto node = make_shared<AstNode>(NodeType::BinOp, token);
        node->value = "**";
        node->children.push_back(left);
        node->children.push_back(parseFactor());
        return node;
    }
    return left;
}

shared_ptr<AstNode> Parser::parsePrimary() {
    auto left = parseAtom();
    while (match(TokenType::TK_PERIOD) || match(TokenType::TK_LPAREN) || match(TokenType::TK_LBRACKET)) {
        auto token = currentToken;
        auto node = make_shared<AstNode>(NodeType::Expr, token);
        node->children.push_back(left);
        if (match(TokenType::TK_PERIOD)) {
            advance();
            expect(TokenType::TK_IDENTIFIER, "Expected identifier after '.'");
            auto id = make_shared<AstNode>(NodeType::Name, currentToken);
            id->value = currentToken.lexeme;
            advance();
            node->children.push_back(id);
            node->value = ".";
        } else if (match(TokenType::TK_LPAREN)) {
            advance();
            auto args = parseArgumentsOpt();
            if (args) node->children.push_back(args);
            expect(TokenType::TK_RPAREN, "Expected ')'");
            node->value = "call";
        } else if (match(TokenType::TK_LBRACKET)) {
            advance();
            node->children.push_back(parseSlices());
            expect(TokenType::TK_RBRACKET, "Expected ']'");
            node->value = "subscript";
        }
        left = node;
    }
    return left;
}

shared_ptr<AstNode> Parser::parseSlices() {
    auto token = currentToken;
    auto node = make_shared<AstNode>(NodeType::Stmt, token);
    if (match(TokenType::TK_COMMA)) {
        node->children.push_back(parseSliceOrExprCommaList());
        auto comma = parseOptionalComma();
        if (comma) node->children.push_back(comma);
    } else {
        node->children.push_back(parseSlice());
    }
    return node;
}

shared_ptr<AstNode> Parser::parseSliceOrExprCommaList() {
    auto token = currentToken;
    auto node = make_shared<AstNode>(NodeType::Stmt, token);
    node->children.push_back(parseSliceOrExpr());
    auto tail = parseSliceOrExprCommaListTailStar();
    if (tail) node->children.push_back(tail);
    return node;
}

shared_ptr<AstNode> Parser::parseSliceOrExprCommaListTailStar() {
    auto node = make_shared<AstNode>(NodeType::Stmt, currentToken);
    while (match(TokenType::TK_COMMA)) {
        advance();
        node->children.push_back(parseSliceOrExpr());
    }
    return node;
}

shared_ptr<AstNode> Parser::parseSliceOrExpr() {
    auto token = currentToken;
    if (match(TokenType::TK_COLON)) {
        return parseSlice();
    }
    auto expr = parseExpression();
    if (match(TokenType::TK_COLON)) {
        return parseSlice();
    }
    return expr;
}

shared_ptr<AstNode> Parser::parseSlice() {
    auto token = currentToken;
    auto node = make_shared<AstNode>(NodeType::Slice, token);
    auto start = parseExpressionOpt();
    if (start) node->children.push_back(start);
    if (match(TokenType::TK_COLON)) {
        advance();
        auto stop = parseExpressionOpt();
        if (stop) node->children.push_back(stop);
        auto step = parseSliceColonExprOpt();
        if (step) node->children.push_back(step);
    } else {
        if (!start) {
            reportError("Expected expression or ':' for slice");
            advance();
            return nullptr;
        }
    }
    return node;
}

shared_ptr<AstNode> Parser::parseExpressionOpt() {
    if (match(TokenType::TK_IDENTIFIER) || match(TokenType::TK_NUMBER) ||
        match(TokenType::TK_STRING) || match(TokenType::TK_TRUE) ||
        match(TokenType::TK_FALSE) || match(TokenType::TK_NONE) ||
        match(TokenType::TK_LPAREN)) {
        return parseExpression();
    }
    return nullptr;
}

shared_ptr<AstNode> Parser::parseSliceColonExprOpt() {
    if (match(TokenType::TK_COLON)) {
        advance();
        auto node = make_shared<AstNode>(NodeType::Stmt, currentToken);
        auto expr = parseExpressionOpt();
        if (expr) node->children.push_back(expr);
        return node;
    }
    return nullptr;
}

shared_ptr<AstNode> Parser::parseAtom() {
    auto token = currentToken;
    if (match(TokenType::TK_IDENTIFIER)) {
        auto node = make_shared<AstNode>(NodeType::Name, token);
        node->value = currentToken.lexeme;
        advance();
        return node;
    } else if (match(TokenType::TK_TRUE) || match(TokenType::TK_FALSE) || match(TokenType::TK_NONE)) {
        auto node = make_shared<AstNode>(NodeType::Name, token);
        node->value = currentToken.lexeme;
        advance();
        return node;
    } else if (match(TokenType::TK_STRING)) {
        return parseStrings();
    } else if (match(TokenType::TK_NUMBER)) {
        auto node = make_shared<AstNode>(NodeType::Num, token);
        node->value = currentToken.lexeme;
        advance();
        return node;
    } else if (match(TokenType::TK_STR) || match(TokenType::TK_INT) || match(TokenType::TK_FLOAT) ||
               match(TokenType::TK_COMPLEX) || match(TokenType::TK_LIST) || match(TokenType::TK_TUPLE) ||
               match(TokenType::TK_RANGE) || match(TokenType::TK_DICT) || match(TokenType::TK_SET) ||
               match(TokenType::TK_FROZENSET) || match(TokenType::TK_BOOL) || match(TokenType::TK_BYTES) ||
               match(TokenType::TK_BYTEARRAY) || match(TokenType::TK_MEMORYVIEW) || match(TokenType::TK_NONETYPE)) {
        auto node = make_shared<AstNode>(NodeType::Name, token);
        node->value = currentToken.lexeme;
        advance();
        return node;
    } else if (match(TokenType::TK_LPAREN)) {
        return parseTupleGroupVariant();
    } else if (match(TokenType::TK_LBRACKET)) {
        return parseListVariant();
    } else if (match(TokenType::TK_LBRACE)) {
        return parseDictSetVariant();
    } else {
        reportError("Expected atom, got " + tokenTypeToString(currentToken.type));
        advance();
        return nullptr;
    }
}

shared_ptr<AstNode> Parser::parseTupleGroupVariant() {
    if (match(TokenType::TK_LPAREN)) {
        auto token = currentToken;
        advance();
        auto expr = parseExpression();
        expect(TokenType::TK_RPAREN, "Expected ')'");
        auto node = make_shared<AstNode>(NodeType::Expr, token);
        node->children.push_back(expr);
        return node;
    }
    return parseTuple();
}

shared_ptr<AstNode> Parser::parseListVariant() {
    auto token = currentToken;
    expect(TokenType::TK_LBRACKET, "Expected '['");
    auto node = make_shared<AstNode>(NodeType::List, token);
    if (!match(TokenType::TK_RBRACKET)) {
        node->children.push_back(parseExpressions());
    }
    expect(TokenType::TK_RBRACKET, "Expected ']'");
    return node;
}

shared_ptr<AstNode> Parser::parseDictSetVariant() {
    auto token = currentToken;
    expect(TokenType::TK_LBRACE, "Expected '{'");
    auto node = make_shared<AstNode>(NodeType::Dict, token);
    if (!match(TokenType::TK_RBRACE)) {
        node->children.push_back(parseExpressions());
    }
    expect(TokenType::TK_RBRACE, "Expected '}'");
    return node;
}

shared_ptr<AstNode> Parser::parseGroup() {
    auto token = currentToken;
    expect(TokenType::TK_LPAREN, "Expected '('");
    auto node = make_shared<AstNode>(NodeType::Expr, token);
    node->children.push_back(parseExpression());
    expect(TokenType::TK_RPAREN, "Expected ')'");
    return node;
}

shared_ptr<AstNode> Parser::parseString() {
    auto token = currentToken;
    if (match(TokenType::TK_STRING)) {
        auto node = make_shared<AstNode>(NodeType::Str, token);
        node->value = currentToken.lexeme;
        advance();
        return node;
    }
    reportError("Expected string literal");
    advance();
    return nullptr;
}

shared_ptr<AstNode> Parser::parseStrings() {
    return parseFstringOrStringPlus();
}

shared_ptr<AstNode> Parser::parseFstringOrStringPlus() {
    auto token = currentToken;
    auto node = make_shared<AstNode>(NodeType::Str, token);
    node->children.push_back(parseFstringOrString());
    auto star = parseFstringOrStringPlusStar();
    if (star) node->children.push_back(star);
    return node;
}

shared_ptr<AstNode> Parser::parseFstringOrStringPlusStar() {
    auto node = make_shared<AstNode>(NodeType::Str, currentToken);
    while (match(TokenType::TK_STRING) || match(TokenType::TK_BYTES)) {
        node->children.push_back(parseFstringOrString());
    }
    return node;
}

shared_ptr<AstNode> Parser::parseFstringOrString() {
    auto token = currentToken;
    if (match(TokenType::TK_STRING)) {
        return parseString();
    } else if (match(TokenType::TK_BYTES)) {
        auto node = make_shared<AstNode>(NodeType::Bytes, token);
        node->value = currentToken.lexeme;
        advance();
        return node;
    }
    reportError("Expected string or bytes literal");
    advance();
    return nullptr;
}

shared_ptr<AstNode> Parser::parseList() {
    auto token = currentToken;
    expect(TokenType::TK_LBRACKET, "Expected '['");
    auto node = make_shared<AstNode>(NodeType::List, token);
    auto exprs = parseExpressionsOpt();
    if (exprs) node->children.push_back(exprs);
    expect(TokenType::TK_RBRACKET, "Expected ']'");
    return node;
}

shared_ptr<AstNode> Parser::parseTuple() {
    auto token = currentToken;
    expect(TokenType::TK_LPAREN, "Expected '('");
    auto node = make_shared<AstNode>(NodeType::Tuple, token);
    auto content = parseTupleContentOpt();
    if (content) node->children.push_back(content);
    expect(TokenType::TK_RPAREN, "Expected ')'");
    return node;
}

shared_ptr<AstNode> Parser::parseTupleContentOpt() {
    auto token = currentToken;
    if (match(TokenType::TK_RPAREN)) {
        return nullptr;
    }
    auto node = make_shared<AstNode>(NodeType::Expr, token);
    auto expr = parseExpression();
    node->children.push_back(expr);
    if (match(TokenType::TK_COMMA)) {
        advance();
        auto exprs = parseExpressionsOpt();
        if (exprs) node->children.push_back(exprs);
    }
    return node;
}

shared_ptr<AstNode> Parser::parseSet() {
    auto token = currentToken;
    expect(TokenType::TK_LBRACE, "Expected '{'");
    auto node = make_shared<AstNode>(NodeType::Set, token);
    node->children.push_back(parseExpressions());
    expect(TokenType::TK_RBRACE, "Expected '}'");
    return node;
}

shared_ptr<AstNode> Parser::parseDict() {
    auto token = currentToken;
    expect(TokenType::TK_LBRACE, "Expected '{'");
    auto node = make_shared<AstNode>(NodeType::Dict, token);
    auto kvpairs = parseKvpairsOpt();
    if (kvpairs) node->children.push_back(kvpairs);
    expect(TokenType::TK_RBRACE, "Expected '}'");
    return node;
}

shared_ptr<AstNode> Parser::parseKvpairsOpt() {
    if (match(TokenType::TK_RBRACE)) {
        return nullptr;
    }
    return parseKvpairs();
}

shared_ptr<AstNode> Parser::parseKvpairs() {
    auto token = currentToken;
    auto node = make_shared<AstNode>(NodeType::Dict, token);
    node->children.push_back(parseKvpairCommaList());
    auto comma = parseOptionalComma();
    if (comma) node->children.push_back(comma);
    return node;
}

shared_ptr<AstNode> Parser::parseKvpairCommaList() {
    auto token = currentToken;
    auto node = make_shared<AstNode>(NodeType::Dict, token);
    node->children.push_back(parseKvpair());
    auto tail = parseKvpairCommaListTailStar();
    if (tail) node->children.push_back(tail);
    return node;
}

shared_ptr<AstNode> Parser::parseKvpairCommaListTailStar() {
    auto node = make_shared<AstNode>(NodeType::Dict, currentToken);
    while (match(TokenType::TK_COMMA)) {
        advance();
        node->children.push_back(parseKvpair());
    }
    return node;
}

shared_ptr<AstNode> Parser::parseKvpair() {
    auto token = currentToken;
    auto node = make_shared<AstNode>(NodeType::DictItem, token);
    node->children.push_back(parseExpression());
    expect(TokenType::TK_COLON, "Expected ':'");
    node->children.push_back(parseExpression());
    return node;
}

shared_ptr<AstNode> Parser::parseArguments() {
    auto token = currentToken;
    auto node = make_shared<AstNode>(NodeType::Args, token);
    node->children.push_back(parseArgs());
    auto comma = parseOptionalComma();
    if (comma) node->children.push_back(comma);
    return node;
}

shared_ptr<AstNode> Parser::parseArgs() {
    auto token = currentToken;
    auto node = make_shared<AstNode>(NodeType::Args, token);
    if (match(TokenType::TK_IDENTIFIER)) {
        auto nextToken = lexer.nextToken();
        lexer.tokens.push_back(currentToken);
        currentToken = nextToken;
        if (match(TokenType::TK_ASSIGN)) {
            lexer.tokens.pop_back();
            currentToken = lexer.tokens.back();
            lexer.tokens.pop_back();
            node->children.push_back(parseKeywordArgumentsList());
            return node;
        }
        lexer.tokens.pop_back();
        currentToken = lexer.tokens.back();
        lexer.tokens.pop_back();
    }
    if (match(TokenType::TK_IDENTIFIER) || match(TokenType::TK_NUMBER) ||
        match(TokenType::TK_STRING) || match(TokenType::TK_TRUE) ||
        match(TokenType::TK_FALSE) || match(TokenType::TK_NONE) ||
        match(TokenType::TK_LPAREN)) {
        auto posArgs = parsePositionalArgumentsList();
        if (posArgs) node->children.push_back(posArgs);
        if (match(TokenType::TK_COMMA)) {
            advance();
            auto kwArgs = parseKeywordArgumentsList();
            if (kwArgs) node->children.push_back(kwArgs);
        }
    } else if (match(TokenType::TK_IDENTIFIER)) {
        auto kwArgs = parseKeywordArgumentsList();
        if (kwArgs) node->children.push_back(kwArgs);
    }
    return node;
}

shared_ptr<AstNode> Parser::parsePositionalArgumentsList() {
    auto token = currentToken;
    auto node = make_shared<AstNode>(NodeType::Args, token);
    node->children.push_back(parseExpression());
    auto tail = parsePositionalArgumentsListTailStar();
    if (tail) node->children.push_back(tail);
    return node;
}

shared_ptr<AstNode> Parser::parsePositionalArgumentsListTailStar() {
    auto node = make_shared<AstNode>(NodeType::Args, currentToken);
    while (match(TokenType::TK_COMMA)) {
        advance();
        node->children.push_back(parseExpression());
    }
    return node;
}

shared_ptr<AstNode> Parser::parseKeywordArgumentsList() {
    auto token = currentToken;
    auto node = make_shared<AstNode>(NodeType::Args, token);
    node->children.push_back(parseKeywordItem());
    auto tail = parseKeywordArgumentsListTailStar();
    if (tail) node->children.push_back(tail);
    return node;
}

shared_ptr<AstNode> Parser::parseKeywordArgumentsListTailStar() {
    auto node = make_shared<AstNode>(NodeType::Args, currentToken);
    while (match(TokenType::TK_COMMA)) {
        advance();
        node->children.push_back(parseKeywordItem());
    }
    return node;
}

shared_ptr<AstNode> Parser::parseKeywordItem() {
    auto token = currentToken;
    expect(TokenType::TK_IDENTIFIER, "Expected identifier for keyword argument");
    auto node = make_shared<AstNode>(NodeType::KwArg, token);
    node->value = currentToken.lexeme;
    expect(TokenType::TK_ASSIGN, "Expected '='");
    node->children.push_back(parseExpression());
    return node;
}

shared_ptr<AstNode> Parser::parseTargets() {
    auto token = currentToken;
    auto node = make_shared<AstNode>(NodeType::Assign, token);
    if (match(TokenType::TK_COMMA)) {
        advance();
        node->children.push_back(parseTarget());
        return node;
    }
    auto target = parseTarget();
    node->children.push_back(target);
    if (match(TokenType::TK_COMMA)) {
        auto list = parseTargetCommaListStar();
        if (list) node->children.push_back(list);
        auto comma = parseOptionalComma();
        if (comma) node->children.push_back(comma);
    }
    return node;
}

shared_ptr<AstNode> Parser::parseTargetCommaListStar() {
    auto node = make_shared<AstNode>(NodeType::Assign, currentToken);
    while (match(TokenType::TK_COMMA)) {
        advance();
        node->children.push_back(parseTarget());
    }
    return node;
}

shared_ptr<AstNode> Parser::parseTargetsListSeq() {
    auto token = currentToken;
    auto node = make_shared<AstNode>(NodeType::Assign, token);
    node->children.push_back(parseTargetCommaList());
    auto comma = parseOptionalComma();
    if (comma) node->children.push_back(comma);
    return node;
}

shared_ptr<AstNode> Parser::parseTargetCommaList() {
    auto token = currentToken;
    auto node = make_shared<AstNode>(NodeType::Assign, token);
    node->children.push_back(parseTarget());
    auto tail = parseTargetCommaListTailStar();
    if (tail) node->children.push_back(tail);
    return node;
}

shared_ptr<AstNode> Parser::parseTargetCommaListTailStar() {
    auto node = make_shared<AstNode>(NodeType::Assign, currentToken);
    while (match(TokenType::TK_COMMA)) {
        advance();
        node->children.push_back(parseTarget());
    }
    return node;
}

shared_ptr<AstNode> Parser::parseTargetsTupleSeq() {
    auto token = currentToken;
    auto node = make_shared<AstNode>(NodeType::Assign, token);
    auto target = parseTarget();
    node->children.push_back(target);
    if (match(TokenType::TK_COMMA)) {
        advance();
        auto list = parseTargetCommaListPlus();
        if (list) node->children.push_back(list);
        auto comma = parseOptionalComma();
        if (comma) node->children.push_back(comma);
    }
    return node;
}

shared_ptr<AstNode> Parser::parseTargetCommaListPlus() {
    auto token = currentToken;
    expect(TokenType::TK_COMMA, "Expected ','");
    auto node = make_shared<AstNode>(NodeType::Assign, token);
    node->children.push_back(parseTarget());
    auto star = parseTargetCommaListStar();
    if (star) node->children.push_back(star);
    return node;
}

shared_ptr<AstNode> Parser::parseTarget() {
    auto token = currentToken;
    if (match(TokenType::TK_IDENTIFIER)) {
        auto nextToken = lexer.nextToken();
        lexer.tokens.push_back(currentToken);
        currentToken = nextToken;
        if (match(TokenType::TK_PERIOD) || match(TokenType::TK_LBRACKET)) {
            lexer.tokens.pop_back();
            currentToken = lexer.tokens.back();
            lexer.tokens.pop_back();
            auto primary = parseTPrimary();
            auto node = make_shared<AstNode>(NodeType::Assign, token);
            node->children.push_back(primary);
            if (match(TokenType::TK_PERIOD)) {
                advance();
                expect(TokenType::TK_IDENTIFIER, "Expected identifier after '.'");
                auto id = make_shared<AstNode>(NodeType::Name, currentToken);
                id->value = currentToken.lexeme;
                advance();
                node->children.push_back(id);
            } else if (match(TokenType::TK_LBRACKET)) {
                advance();
                node->children.push_back(parseSlices());
                expect(TokenType::TK_RBRACKET, "Expected ']'");
            }
            return node;
        } else {
            lexer.tokens.pop_back();
            currentToken = lexer.tokens.back();
            lexer.tokens.pop_back();
            return parseTargetAtom();
        }
    } else {
        return parseTargetAtom();
    }
}

shared_ptr<AstNode> Parser::parseTargetAtom() {
    auto token = currentToken;
    if (match(TokenType::TK_IDENTIFIER)) {
        auto node = make_shared<AstNode>(NodeType::Name, token);
        node->value = currentToken.lexeme;
        advance();
        return node;
    } else if (match(TokenType::TK_LPAREN)) {
        advance();
        auto node = make_shared<AstNode>(NodeType::Assign, token);
        auto target = parseTarget();
        expect(TokenType::TK_RPAREN, "Expected ')'");
        node->children.push_back(target);
        return node;
    } else if (match(TokenType::TK_LBRACKET)) {
        advance();
        auto node = make_shared<AstNode>(NodeType::Assign, token);
        auto seq = parseTargetsListSeqOpt();
        if (seq) node->children.push_back(seq);
        expect(TokenType::TK_RBRACKET, "Expected ']'");
        return node;
    } else {
        reportError("Expected target, got " + tokenTypeToString(currentToken.type));
        advance();
        return nullptr;
    }
}

shared_ptr<AstNode> Parser::parseTargetsTupleSeqOpt() {
    if (match(TokenType::TK_RPAREN)) {
        return nullptr;
    }
    return parseTargetsTupleSeq();
}

shared_ptr<AstNode> Parser::parseTargetsListSeqOpt() {
    if (match(TokenType::TK_RBRACKET)) {
        return nullptr;
    }
    return parseTargetsListSeq();
}

shared_ptr<AstNode> Parser::parseSingleTarget() {
    auto token = currentToken;
    if (match(TokenType::TK_IDENTIFIER)) {
        auto nextToken = lexer.nextToken();
        lexer.tokens.push_back(currentToken);
        currentToken = nextToken;
        if (match(TokenType::TK_PERIOD) || match(TokenType::TK_LBRACKET)) {
            lexer.tokens.pop_back();
            currentToken = lexer.tokens.back();
            lexer.tokens.pop_back();
            return parseSingleSubscriptAttributeTarget();
        }
        lexer.tokens.pop_back();
        currentToken = lexer.tokens.back();
        lexer.tokens.pop_back();
        auto node = make_shared<AstNode>(NodeType::Name, token);
        node->value = currentToken.lexeme;
        advance();
        return node;
    } else if (match(TokenType::TK_LPAREN)) {
        advance();
        auto node = make_shared<AstNode>(NodeType::Assign, token);
        auto target = parseSingleTarget();
        expect(TokenType::TK_RPAREN, "Expected ')'");
        node->children.push_back(target);
        return node;
    }
    reportError("Expected single target");
    advance();
    return nullptr;
}

shared_ptr<AstNode> Parser::parseSingleSubscriptAttributeTarget() {
    auto token = currentToken;
    auto primary = parseTPrimary();
    auto node = make_shared<AstNode>(NodeType::Assign, token);
    node->children.push_back(primary);
    if (match(TokenType::TK_PERIOD)) {
        advance();
        expect(TokenType::TK_IDENTIFIER, "Expected identifier after '.'");
        auto id = make_shared<AstNode>(NodeType::Name, currentToken);
        id->value = currentToken.lexeme;
        advance();
        node->children.push_back(id);
    } else if (match(TokenType::TK_LBRACKET)) {
        advance();
        node->children.push_back(parseSlices());
        expect(TokenType::TK_RBRACKET, "Expected ']'");
    } else {
        reportError("Expected '.' or '[' for subscript/attribute target");
        return nullptr;
    }
    return node;
}

shared_ptr<AstNode> Parser::parseTPrimary() {
    auto left = parseAtom();
    while (match(TokenType::TK_PERIOD) || match(TokenType::TK_LBRACKET) || match(TokenType::TK_LPAREN)) {
        auto token = currentToken;
        auto node = make_shared<AstNode>(NodeType::Expr, token);
        node->children.push_back(left);
        if (match(TokenType::TK_PERIOD)) {
            advance();
            expect(TokenType::TK_IDENTIFIER, "Expected identifier after '.'");
            auto id = make_shared<AstNode>(NodeType::Name, currentToken);
            id->value = currentToken.lexeme;
            advance();
            node->children.push_back(id);
            node->value = ".";
        } else if (match(TokenType::TK_LBRACKET)) {
            advance();
            node->children.push_back(parseSlices());
            expect(TokenType::TK_RBRACKET, "Expected ']'");
            node->value = "subscript";
        } else if (match(TokenType::TK_LPAREN)) {
            advance();
            auto args = parseArgumentsOpt();
            if (args) node->children.push_back(args);
            expect(TokenType::TK_RPAREN, "Expected ')'");
            node->value = "call";
        }
        left = node;
    }
    return left;
}

string Parser::nodeTypeToString(NodeType type) {
    switch (type) {
        case NodeType::Module: return "Module";
        case NodeType::FunctionDef: return "FunctionDef";
        case NodeType::Assign: return "Assign";
        case NodeType::Expr: return "Expr";
        case NodeType::BinOp: return "BinOp";
        case NodeType::Name: return "Name";
        case NodeType::Num: return "Num";
        case NodeType::Str: return "Str";
        case NodeType::If: return "If";
        case NodeType::While: return "While";
        case NodeType::For: return "For";
        case NodeType::Return: return "Return";
        case NodeType::Pass: return "Pass";
        case NodeType::Raise: return "Raise";
        case NodeType::Global: return "Global";
        case NodeType::Nonlocal: return "Nonlocal";
        case NodeType::Del: return "Del";
        case NodeType::Assert: return "Assert";
        case NodeType::Break: return "Break";
        case NodeType::Continue: return "Continue";
        case NodeType::Import: return "Import";
        case NodeType::ImportFrom: return "ImportFrom";
        case NodeType::ClassDef: return "ClassDef";
        case NodeType::Decorator: return "Decorator";
        case NodeType::Stmt: return "Stmt";
        case NodeType::AugAssign: return "AugAssign";
        case NodeType::Params: return "Params";
        case NodeType::Param: return "Param";
        case NodeType::Try: return "Try";
        case NodeType::Except: return "Except";
        case NodeType::Finally: return "Finally";
        case NodeType::Slice: return "Slice";
        case NodeType::List: return "List";
        case NodeType::Dict: return "Dict";
        case NodeType::Bytes: return "Bytes";
        case NodeType::Tuple: return "Tuple";
        case NodeType::Set: return "Set";
        case NodeType::DictItem: return "DictItem";
        case NodeType::Args: return "Args";
        case NodeType::KwArg: return "KwArg";
        default: return "Unknown";
    }
}

void Parser::generateDotNode(shared_ptr<AstNode> node, string& output, int& nodeId, vector<pair<int, int>>& edges) {
    if (!node) return;

    int currentId = nodeId++;
    string label = nodeTypeToString(node->type);
    if (!node->value.empty()) {
        label += " \n " + node->value + "";
    }
    output += "    node" + to_string(currentId) + " [label=\"" + label + "\"];\n";

    for (const auto& child : node->children) {
        if (child) {
            int childId = nodeId;
            edges.push_back({currentId, childId});
            generateDotNode(child, output, nodeId, edges);
        }
    }
}

string Parser::generateDot(shared_ptr<AstNode> root) {
    string output = "digraph AST {\n";
    output += "    rankdir=TB;\n"; // Top-to-bottom layout
    output += "    node [shape=box];\n"; // Box shape for nodes

    int nodeId = 0;
    vector<pair<int, int>> edges;
    generateDotNode(root, output, nodeId, edges);

    for (const auto& edge : edges) {
        output += "    node" + to_string(edge.first) + " -> node" + to_string(edge.second) + ";\n";
    }

    output += "}\n";
    return output;
}


const vector<string>& Parser::getErrors() const {
    return errors;
}
#include <fstream>

void Parser::saveDotFile(const string& dotContent, const string& filename) {
    ofstream out(filename);
    if (!out.is_open()) {
        errors.push_back("Failed to open file for writing: " + filename);
        return;
    }
    out << dotContent;
    out.close();
}