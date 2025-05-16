#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>
#include <memory>
#include "Token.hpp"

enum class NodeType {
    Module, FunctionDef, Assign, Expr, BinOp, Name, Num, Str, If, While, For, Return,
    Pass, Raise, Global, Nonlocal, Del, Assert, Break, Continue, Import, ImportFrom, ClassDef, Decorator, Stmt,AugAssign,Params
,Param,Try,Except,Finally,Slice,List,Dict,Bytes,Tuple,Set,DictItem,Args,KwArg,IfExp,UnaryOp,Constant,Subscript,Attribute,Call
};

class AstNode {
public:
    NodeType type;
    Token token;
    vector<shared_ptr<AstNode>> children;
    string value;

    AstNode(NodeType type, const Token& token) : type(type), token(token) {}
};
#endif // AST_HPP