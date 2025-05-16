#ifndef PARSER_HPP
#define PARSER_HPP
using namespace std;
#include <string>
#include <vector>
#include <memory>
#include "Lexer.hpp"

#include "tempAST.hpp"

class Parser {
public:
    explicit Parser(Lexer& lexer);
    shared_ptr<AstNode> parse(); // Parse the entire input into an AST
    shared_ptr<AstNode> parseFile();

    shared_ptr<AstNode> parseStatementsOpt();

    shared_ptr<AstNode> parseStatements();

    shared_ptr<AstNode> parseStatementStar();

    shared_ptr<AstNode> parseStatement();

    shared_ptr<AstNode> parseSimpleStmts();

    shared_ptr<AstNode> parseSimpleStmtList();

    shared_ptr<AstNode> parseSimpleStmtListTailStar();

    shared_ptr<AstNode> parseOptionalSemicolon();

    shared_ptr<AstNode> parseSimpleStmt();

    shared_ptr<AstNode> parseCompoundStmt();

    shared_ptr<AstNode> parseAssignment();

    shared_ptr<AstNode> parseAssignmentEqAnnotatedRhsOpt();

    shared_ptr<AstNode> parseStarTargetsEqPlus();

    shared_ptr<AstNode> parseStarTargetsEqPlusRest();

    shared_ptr<AstNode> parseYieldExprOrStarExpressions();

    shared_ptr<AstNode> parseAnnotatedRhs();

    shared_ptr<AstNode> parseAugassign();

    shared_ptr<AstNode> parseReturnStmt();

    shared_ptr<AstNode> parseStarExpressionsOpt();

    shared_ptr<AstNode> parseRaiseStmt();

    shared_ptr<AstNode> parseRaiseFromOpt();

    shared_ptr<AstNode> parseGlobalStmt();

    shared_ptr<AstNode> parseNonlocalStmt();

    shared_ptr<AstNode> parseNameCommaList();

    shared_ptr<AstNode> parseNameCommaListTailStar();

    shared_ptr<AstNode> parseDelStmt();

    shared_ptr<AstNode> parseYieldStmt();

    shared_ptr<AstNode> parseAssertStmt();

    shared_ptr<AstNode> parseAssertCommaExprOpt();

    shared_ptr<AstNode> parseImportStmt();

    shared_ptr<AstNode> parseImportName();

    shared_ptr<AstNode> parseImportFrom();

    shared_ptr<AstNode> parseDotOrEllipsisStar();

    shared_ptr<AstNode> parseDotOrEllipsisPlus();

    shared_ptr<AstNode> parseImportFromTargets();

    shared_ptr<AstNode> parseOptionalComma();

    shared_ptr<AstNode> parseImportFromAsNames();

    shared_ptr<AstNode> parseImportFromAsNameCommaListStar();

    shared_ptr<AstNode> parseImportFromAsName();

    shared_ptr<AstNode> parseImportFromAsNameAsOpt();

    shared_ptr<AstNode> parseDottedAsNames();

    shared_ptr<AstNode> parseDottedAsNameCommaListStar();

    shared_ptr<AstNode> parseDottedAsName();

    shared_ptr<AstNode> parseDottedAsNameAsOpt();

    shared_ptr<AstNode> parseDottedName();

    shared_ptr<AstNode> parseBlock();

    shared_ptr<AstNode> parseDecorators();

    shared_ptr<AstNode> parseDecoratorPlus();

    shared_ptr<AstNode> parseDecoratorPlusStar();

    shared_ptr<AstNode> parseDecoratorItem();

    shared_ptr<AstNode> parseClassDef();

    shared_ptr<AstNode> parseClassDefRaw();

    shared_ptr<AstNode> parseClassArgumentsOpt();

    shared_ptr<AstNode> parseArgumentsOpt();

    const vector<string>& getErrors() const; // Return syntax errors

private:
    Lexer& lexer;
    Token currentToken;
    vector<string> errors;

    // Parsing methods (recursive descent)
    void advance(); // Move to next token
    bool match(TokenType type); // Check if current token matches type
    bool expect(TokenType type, const string& errorMsg); // Expect a specific token or report error
    shared_ptr<AstNode> parseModule(); // Parse top-level module
    shared_ptr<AstNode> parseStmt(); // Parse a statement
    shared_ptr<AstNode> parseExpr(); // Parse an expression
    shared_ptr<AstNode> parseFunctionDef(); // Parse function definition
    shared_ptr<AstNode> parseIfStmt(); // Parse if statement
    // Add more parsing methods for other constructs (while, for, class, etc.)

    void reportError(const string& message);
};

#endif // PARSER_HPP