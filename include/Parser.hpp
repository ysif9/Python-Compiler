#ifndef PARSER_HPP
#define PARSER_HPP

#include <vector>
#include <string>
#include <memory>
#include <stdexcept> // For std::runtime_error for ParseError (optional)
#include <algorithm> // For std::find

#include "Lexer.hpp"
#include "Token.hpp"

// Include all new AST header files
#include "ASTNode.hpp"
#include "Literals.hpp"
#include "Expressions.hpp"
#include "Statements.hpp"
#include "UtilNodes.hpp"
#include "Helpers.hpp"

class Parser {
public:
    explicit Parser(Lexer& lexer_instance);
    std::unique_ptr<ProgramNode> parse();

    bool hasError() const { return had_error; }
    const std::vector<std::string>& getErrors() const { return errors_list; }

private:
    Lexer& lexer_ref; // Reference to the lexer
    std::vector<Token> tokens;
    size_t current_pos;
    bool had_error;
    std::vector<std::string> errors_list;
    static Token eof_token; // Static EOF token for boundary conditions

    // Core helper methods
    Token& peek(int offset = 0);
    Token& previous();
    bool isAtEnd(int offset = 0);
    Token advance();
    bool check(TokenType type) const;
    bool check(const std::vector<TokenType>& types) const;
    bool match(TokenType type);
    Token consume(TokenType type, const std::string& message);
    void reportError(const Token& token, const std::string& message);
    void synchronize();

    // --- Recursive Descent Parsing Methods (Declarations) ---

    // STARTING RULES
    std::unique_ptr<ProgramNode> parseFile();
    std::vector<std::unique_ptr<StatementNode>> parseStatementsOpt();

    // GENERAL STATEMENTS
    std::vector<std::unique_ptr<StatementNode>> parseStatements();
    std::unique_ptr<StatementNode> parseStatement();
    std::vector<std::unique_ptr<StatementNode>> parseSimpleStmts();
    std::unique_ptr<StatementNode> parseSimpleStmt();
    std::unique_ptr<StatementNode> parseCompoundStmt();

    // SIMPLE STATEMENTS
    Token parseAugassign();
    std::unique_ptr<ReturnStatementNode> parseReturnStmt();
    std::unique_ptr<ExpressionNode> parseExpressionsOpt();
    std::unique_ptr<ExpressionNode> parseExpressions();
    std::unique_ptr<RaiseStatementNode> parseRaiseStmt();
    std::unique_ptr<GlobalStatementNode> parseGlobalStmt();
    std::unique_ptr<NonlocalStatementNode> parseNonlocalStmt();
    std::vector<std::unique_ptr<IdentifierNode>> parseNameCommaList(int& line_start);

    // COMPOUND STATEMENTS & Common elements
    std::unique_ptr<BlockNode> parseBlock();
    std::unique_ptr<ClassDefinitionNode> parseClassDef();
    void parseClassArgumentsOpt(std::vector<std::unique_ptr<ExpressionNode>>& bases, std::vector<std::unique_ptr<KeywordArgNode>>& keywords, int& line);
    std::unique_ptr<FunctionDefinitionNode> parseFunctionDef();
    std::unique_ptr<ArgumentsNode> parseParamsOpt(int& line_start);
    std::unique_ptr<ArgumentsNode> parseParameters(int& line_start);
    std::unique_ptr<ParameterNode> parseParamNoDefault(ParameterNode::Kind kind);
    std::unique_ptr<ParameterNode> parseParamWithDefault();
    std::unique_ptr<IdentifierNode> parseParamIdentifier();
    std::unique_ptr<ExpressionNode> parseDefault();
    std::unique_ptr<IfStatementNode> parseIfStmt();
    std::unique_ptr<BlockNode> parseElseBlockOpt();
    std::unique_ptr<WhileStatementNode> parseWhileStmt();
    std::unique_ptr<ForStatementNode> parseForStmt();
    std::unique_ptr<TryStatementNode> parseTryStmt();
    std::unique_ptr<ExceptionHandlerNode> parseExceptBlock();
    std::unique_ptr<BlockNode> parseFinallyBlockOpt();
    std::unique_ptr<BlockNode> parseFinallyBlock();

    // EXPRESSIONS
    std::unique_ptr<ExpressionNode> parseExpression();
    std::unique_ptr<ExpressionNode> parseDisjunction();
    std::unique_ptr<ExpressionNode> parseConjunction();
    std::unique_ptr<ExpressionNode> parseInversion();
    std::unique_ptr<ExpressionNode> parseComparison();
    std::unique_ptr<ExpressionNode> parseBitwiseOr();
    std::unique_ptr<ExpressionNode> parseBitwiseXor();
    std::unique_ptr<ExpressionNode> parseBitwiseAnd();
    std::unique_ptr<ExpressionNode> parseShiftExpr();
    std::unique_ptr<ExpressionNode> parseSum();
    std::unique_ptr<ExpressionNode> parseTerm();
    std::unique_ptr<ExpressionNode> parseFactor();
    std::unique_ptr<ExpressionNode> parsePower();
    std::unique_ptr<ExpressionNode> parsePrimary(bool in_target_context = false); // Added flag
    std::unique_ptr<ExpressionNode> parseSlices();
    std::unique_ptr<SliceNode> parseSlice();
    std::unique_ptr<ExpressionNode> parseAtom(bool in_target_context = false); // Added flag


    // Literals and Atoms
    std::unique_ptr<ExpressionNode> parseTupleGroupVariant(bool in_target_context);
    std::unique_ptr<ListLiteralNode> parseListVariant(bool in_target_context);
    std::unique_ptr<ExpressionNode> parseDictSetVariant();
    std::unique_ptr<StringLiteralNode> parseStrings(); // Concatenates adjacent string literals
    std::unique_ptr<BytesLiteralNode> parseBytes();     // For bytes literals
    std::unique_ptr<ListLiteralNode> parseListLiteral(bool in_target_context);

    void parseKvPair(std::vector<std::unique_ptr<ExpressionNode>>& keys, std::vector<std::unique_ptr<ExpressionNode>>& values, int line);

    // Arguments for function calls (CFG: arguments -> args)
    void parseArgumentsForCall(std::vector<std::unique_ptr<ExpressionNode>>& pos_args, std::vector<std::unique_ptr<KeywordArgNode>>& kw_args, int& line);
    std::unique_ptr<KeywordArgNode> parseKeywordItem();

    // Targets for assignment
    std::vector<std::unique_ptr<ExpressionNode>> parseTargets();
    std::unique_ptr<ExpressionNode> parseTarget(); // Corresponds to CFG 'target'
    std::unique_ptr<ExpressionNode> parseTargetAtom();
    std::unique_ptr<ExpressionNode> parseSingleTarget(); // Corresponds to CFG 'single_target'
    std::unique_ptr<ExpressionNode> parseTPrimary();    // Corresponds to CFG 't_primary'

    // Helper for simplified_star_etc in parameters
    void parseSimplifiedStarEtc(ArgumentsNode& args_node_ref);

    void unputToken();

    unique_ptr<StatementNode> parseImportStatement();
};

#endif // PARSER_HPP