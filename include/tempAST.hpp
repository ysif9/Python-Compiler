#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>
#include <memory>
#include "Token.hpp"

enum class NodeType {
    ProgramNode, StatementNode, ExpressionNode, PassStatementNode, AssignmentStatementNode, AugAssignNode, ExpressionStatementNode, ReturnStatementNode,
    TupleLiteralNode, IdentifierNode, NamedImportNode, ImportStatementNode,
    IfExpNode, BinaryOpNode, UnaryOpNode, ComparisonNode, AttributeAccessNode, KeywordArgNode, FunctionCallNode,
    SubscriptionNode, BooleanLiteralNode, NoneLiteralNode, NumberLiteralNode, ComplexLiteralNode, StringLiteralNode,ListLiteralNode,BlockNode
,FunctionDefinitionNode,ArgumentsNode,RaiseStatementNode,GlobalStatementNode,
    NonlocalStatementNode,IfStatementNode,ClassDefinitionNode,ForStatementNode,TryStatementNode,ExceptionHandlerNode,
    ParameterNode,WhileStatementNode,DictLiteralNode,SliceNode,BytesLiteralNode,SetLiteralNode,BreakStatementNode,ContinueStatementNode
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