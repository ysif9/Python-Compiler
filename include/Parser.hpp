#ifndef PARSER_HPP
#define PARSER_HPP

#include "tempAST.hpp"
#include "Token.hpp"
#include "Lexer.hpp"
#include <vector>
#include <string>
#include <memory>
#include <utility>

using namespace std;

class Parser {
public:
    Parser(Lexer& lexer_instance);
    shared_ptr<AstNode> parse();
    const vector<string>& getErrors() const;

    void generateDotNode(shared_ptr<AstNode> node, string &output, int &nodeId, vector<pair<int, int>> &edges);

    string nodeTypeToString(NodeType type);

    string generateDot(shared_ptr<AstNode> root);

    void saveDotFile(const string &dotContent, const string &filename);
    bool had_error;
private:
    Lexer& lexer_ref;
    vector<Token> tokens;
    size_t current_pos;

    vector<string> errors_list;

    Token& peek(int offset = 0);
    Token& previous();
    bool isAtEnd(int offset = 0);
    Token advance();
    bool check(TokenType type) const;
    bool check(const vector<TokenType>& types) const;
    bool match(TokenType type);
    Token consume(TokenType type, const string& message);
    void reportError(const Token& token, const string& message);
    void synchronize();
    void unputToken();

    shared_ptr<AstNode> parseFile();
    vector<shared_ptr<AstNode>> parseStatementsOpt();
    vector<shared_ptr<AstNode>> parseStatements();
    shared_ptr<AstNode> parseStatement();
    vector<shared_ptr<AstNode>> parseSimpleStmts();
    shared_ptr<AstNode> parseSimpleStmt();
    shared_ptr<AstNode> parseCompoundStmt();
    Token parseAugassign();

    shared_ptr<AstNode> parseReturnStmt();
    shared_ptr<AstNode> parseExpressionsOpt();
    shared_ptr<AstNode> parseExpressions();
    shared_ptr<AstNode> parseImportStatement();
    shared_ptr<AstNode> parseExpression();
    shared_ptr<AstNode> parseDisjunction();
    shared_ptr<AstNode> parseConjunction();
    shared_ptr<AstNode> parseInversion();
    shared_ptr<AstNode> parseComparison();
    shared_ptr<AstNode> parseBitwiseOr();
    shared_ptr<AstNode> parseBitwiseXor();
    shared_ptr<AstNode> parseBitwiseAnd();
    shared_ptr<AstNode> parseShiftExpr();
    shared_ptr<AstNode> parseSum();
    shared_ptr<AstNode> parseTerm();
    shared_ptr<AstNode> parseFactor();
    shared_ptr<AstNode> parsePower();
    shared_ptr<AstNode> parsePrimary(bool in_target_context);
    shared_ptr<AstNode> parseAtom(bool in_target_context);
    shared_ptr<AstNode> parseStrings();
    shared_ptr<AstNode> parseTupleGroupVariant(bool in_target_context);
    shared_ptr<AstNode> parseListVariant(bool in_target_context);
    shared_ptr<AstNode> parseListLiteral(bool in_target_context);
    shared_ptr<AstNode> parseBlock();
    shared_ptr<AstNode> parseFunctionDef();
    shared_ptr<AstNode> parseParamsOpt(int& line_start);
    shared_ptr<AstNode> parseRaiseStmt();
    shared_ptr<AstNode> parseGlobalStmt();
    shared_ptr<AstNode> parseNonlocalStmt();
    shared_ptr<AstNode> parseIfStmt();
    shared_ptr<AstNode> parseElseBlockOpt();
    shared_ptr<AstNode> parseClassDef();

    shared_ptr<AstNode> parseForStmt();

    shared_ptr<AstNode> parseTryStmt();

    void parseClassArgumentsOpt(vector<shared_ptr<AstNode>>& bases, vector<shared_ptr<AstNode>>& keywords, int& line_start_ref);
    shared_ptr<AstNode> parseParamIdentifier();
    shared_ptr<AstNode> parseDefault();
    shared_ptr<AstNode> parseParamWithDefault();
    shared_ptr<AstNode> parseParamNoDefault();
    shared_ptr<AstNode> parseWhileStmt();
    void parseArgumentsForCall(vector<shared_ptr<AstNode>>& pos_args, vector<shared_ptr<AstNode>>& kw_args, int& line_start_call);
    shared_ptr<AstNode> parseSlices();
    shared_ptr<AstNode> parseDictSetVariant();
    shared_ptr<AstNode> parseParameters(int& line_start);
    vector<shared_ptr<AstNode>> parseNameCommaList(int& line_start);
    shared_ptr<AstNode> parseExceptBlock();
    shared_ptr<AstNode> parseFinallyBlockOpt();
    shared_ptr<AstNode> parseFinallyBlock();
    shared_ptr<AstNode> parseSlice();
    shared_ptr<AstNode> parseBytes();
    void parseKvPair(vector<shared_ptr<AstNode>>& keys, vector<shared_ptr<AstNode>>& values, int line);
    shared_ptr<AstNode> parseKeywordItem();
    vector<shared_ptr<AstNode>> parseTargets();
    shared_ptr<AstNode> parseTarget();
    shared_ptr<AstNode> parseTargetAtom();
    shared_ptr<AstNode> parseSingleTarget();
    shared_ptr<AstNode> parseTPrimary();
    void parseSimplifiedStarEtc(shared_ptr<AstNode> args_node_ref);
};

#endif // PARSER_HPP
