#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>
#include <memory>
#include "Lexer.hpp"
#include "tempAST.hpp"

class Parser {
public:
    explicit Parser(Lexer& lexer);
    std::shared_ptr<AstNode> parse(); // Parse the entire input into an AST
    shared_ptr<AstNode> parseFile();

    shared_ptr<AstNode> parseStatementsOpt();

    shared_ptr<AstNode> parseStatements();

    shared_ptr<AstNode> parseStatement();

    shared_ptr<AstNode> parseStatementStar();

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

    shared_ptr<AstNode> parseExpressionsOpt();

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

    shared_ptr<AstNode> parseElseBlockOpt();

    shared_ptr<AstNode> parseElseBlock();

    shared_ptr<AstNode> parseWhileStmt();

    shared_ptr<AstNode> parseForStmt();

    shared_ptr<AstNode> parseTryStmt();

    shared_ptr<AstNode> parseExceptBlockPlus();

    shared_ptr<AstNode> parseExceptBlockPlusStar();

    shared_ptr<AstNode> parseFinallyBlockOpt();

    shared_ptr<AstNode> parseExceptBlock();

    shared_ptr<AstNode> parseExceptAsNameOpt();

    shared_ptr<AstNode> parseFinallyBlock();

    shared_ptr<AstNode> parseExpressions();

    shared_ptr<AstNode> parseExpressionCommaPlus();

    shared_ptr<AstNode> parseExpressionCommaPlusStar();

    shared_ptr<AstNode> parseExpression();

    shared_ptr<AstNode> parseDisjunction();

    shared_ptr<AstNode> parseDisjunctionTailStar();

    shared_ptr<AstNode> parseConjunction();

    shared_ptr<AstNode> parseConjunctionTailStar();

    shared_ptr<AstNode> parseInversion();

    shared_ptr<AstNode> parseComparison();

    shared_ptr<AstNode> parseCompareOpBitwiseOrPairStar();

    shared_ptr<AstNode> parseCompareOpBitwiseOrPair();

    shared_ptr<AstNode> parseBitwiseOr();

    shared_ptr<AstNode> parseBitwiseXor();

    shared_ptr<AstNode> parseBitwiseAnd();

    shared_ptr<AstNode> parseShiftExpr();

    shared_ptr<AstNode> parseSum();

    shared_ptr<AstNode> parseTerm();

    shared_ptr<AstNode> parseFactor();

    shared_ptr<AstNode> parsePower();

    shared_ptr<AstNode> parsePrimary();

    shared_ptr<AstNode> parseSlices();

    shared_ptr<AstNode> parseSliceOrExprCommaList();

    shared_ptr<AstNode> parseSliceOrExprCommaListTailStar();

    shared_ptr<AstNode> parseSliceOrExpr();

    shared_ptr<AstNode> parseSlice();

    shared_ptr<AstNode> parseExpressionOpt();

    shared_ptr<AstNode> parseSliceColonExprOpt();

    shared_ptr<AstNode> parseAtom();

    shared_ptr<AstNode> parseTupleGroupVariant();

    shared_ptr<AstNode> parseListVariant();

    shared_ptr<AstNode> parseDictSetVariant();

    shared_ptr<AstNode> parseGroup();

    shared_ptr<AstNode> parseString();

    shared_ptr<AstNode> parseStrings();

    shared_ptr<AstNode> parseFstringOrStringPlus();

    shared_ptr<AstNode> parseFstringOrStringPlusStar();

    shared_ptr<AstNode> parseFstringOrString();

    shared_ptr<AstNode> parseList();

    shared_ptr<AstNode> parseTuple();

    shared_ptr<AstNode> parseTupleContentOpt();

    shared_ptr<AstNode> parseSet();

    shared_ptr<AstNode> parseDict();

    shared_ptr<AstNode> parseKvpairsOpt();

    shared_ptr<AstNode> parseKvpairs();

    shared_ptr<AstNode> parseKvpairCommaList();

    shared_ptr<AstNode> parseKvpairCommaListTailStar();

    shared_ptr<AstNode> parseKvpair();

    shared_ptr<AstNode> parseArguments();

    shared_ptr<AstNode> parseArgs();

    shared_ptr<AstNode> parsePositionalArgumentsList();

    shared_ptr<AstNode> parsePositionalArgumentsListTailStar();

    shared_ptr<AstNode> parseKeywordArgumentsList();

    shared_ptr<AstNode> parseKeywordArgumentsListTailStar();

    shared_ptr<AstNode> parseKeywordItem();

    shared_ptr<AstNode> parseTargets();

    shared_ptr<AstNode> parseTargetCommaListStar();

    shared_ptr<AstNode> parseTargetsListSeq();

    shared_ptr<AstNode> parseTargetCommaList();

    shared_ptr<AstNode> parseTargetCommaListTailStar();

    shared_ptr<AstNode> parseTargetsTupleSeq();

    shared_ptr<AstNode> parseTargetCommaListPlus();

    shared_ptr<AstNode> parseTarget();

    shared_ptr<AstNode> parseTargetAtom();

    shared_ptr<AstNode> parseTargetsTupleSeqOpt();

    shared_ptr<AstNode> parseTargetsListSeqOpt();

    shared_ptr<AstNode> parseSingleTarget();

    shared_ptr<AstNode> parseSingleSubscriptAttributeTarget();

    shared_ptr<AstNode> parseTPrimary();

    string nodeTypeToString(NodeType type);

    void generateDotNode(shared_ptr<AstNode> node, string &output, int &nodeId, vector<pair<int, int>> &edges);

    string generateDot(shared_ptr<AstNode> root);

    const std::vector<std::string>& getErrors() const; // Return syntax errors
    void saveDotFile(const string &dotContent, const string &filename);

private:
    Lexer& lexer;
    Token currentToken;
    std::vector<std::string> errors;

    // Parsing methods (recursive descent)
    void advance(); // Move to next token
    bool match(TokenType type); // Check if current token matches type
    bool expect(TokenType type, const std::string& errorMsg); // Expect a specific token or report error
    std::shared_ptr<AstNode> parseModule(); // Parse top-level module
    std::shared_ptr<AstNode> parseStmt(); // Parse a statement
    std::shared_ptr<AstNode> parseExpr(); // Parse an expression
    std::shared_ptr<AstNode> parseFunctionDef(); // Parse function definition
    shared_ptr<AstNode> parseFunctionDefRaw();

    shared_ptr<AstNode> parseParamsOpt();

    shared_ptr<AstNode> parseParams();

    shared_ptr<AstNode> parseParameters();

    shared_ptr<AstNode> parseParamNoDefaultStar();

    shared_ptr<AstNode> parseParamWithDefaultStar();

    shared_ptr<AstNode> parseSimplifiedStarEtcOpt();

    shared_ptr<AstNode> parseParamNoDefaultPlus();

    shared_ptr<AstNode> parseParamWithDefaultPlus();

    shared_ptr<AstNode> parseSimplifiedStarEtc();

    shared_ptr<AstNode> parseKwdsOpt();

    shared_ptr<AstNode> parseKwds();

    shared_ptr<AstNode> parseParamNoDefault();

    shared_ptr<AstNode> parseParamWithDefault();

    shared_ptr<AstNode> parseParamEndingChar();

    shared_ptr<AstNode> parseDefault();

    shared_ptr<AstNode> parseParam();

    std::shared_ptr<AstNode> parseIfStmt(); // Parse if statement
    shared_ptr<AstNode> parseElifStmt();

    // Add more parsing methods for other constructs (while, for, class, etc.)

    void reportError(const std::string& message);
};

#endif // PARSER_HPP