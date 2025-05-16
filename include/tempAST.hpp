#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>
#include <memory>
#include "Token.hpp"

enum class NodeType {
    Module, FunctionDef, Assign, Expr, BinOp, Name, Num, Str, If, While, For, Return, Stmt, // Add more as needed
};

class AstNode {
public:
    NodeType type;
    Token token; // Associated token for error reporting
    std::vector<std::shared_ptr<AstNode>> children; // Child nodes
    std::string value; // For literals or identifiers
    // Add fields like line number, type info, etc., as needed

    AstNode(NodeType type, const Token& token) : type(type), token(token) {}
};

#endif // AST_HPP