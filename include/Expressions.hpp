#ifndef EXPRESSIONS_HPP
#define EXPRESSIONS_HPP

#include "ASTNode.hpp"
#include "Token.hpp" // Assuming Token.hpp is in the parent directory
#include <vector>
#include <string>
#include <memory>
#include <utility> // For std::pair in DictLiteralNode

// Base class for all expression nodes
class ExpressionNode : public ASTNode {
public:
    ExpressionNode(int line) : ASTNode(line) {}
};

// Concrete Expression Nodes

class IdentifierNode : public ExpressionNode {
public:
    std::string name;

    IdentifierNode(int line, std::string name_val)
            : ExpressionNode(line), name(std::move(name_val)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "IdentifierNode"; }
};

class BinaryOpNode : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> left;
    Token op;
    std::unique_ptr<ExpressionNode> right;

    BinaryOpNode(int line, std::unique_ptr<ExpressionNode> l, Token op_token, std::unique_ptr<ExpressionNode> r)
            : ExpressionNode(line), left(std::move(l)), op(op_token), right(std::move(r)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "BinaryOpNode"; }
};

class UnaryOpNode : public ExpressionNode {
public:
    Token op;
    std::unique_ptr<ExpressionNode> operand;

    UnaryOpNode(int line, Token op_token, std::unique_ptr<ExpressionNode> o)
            : ExpressionNode(line), op(op_token), operand(std::move(o)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "UnaryOpNode"; }
};

class FunctionCallNode : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> callee;
    std::vector<std::unique_ptr<ExpressionNode>> arguments;

    FunctionCallNode(int line, std::unique_ptr<ExpressionNode> callee_expr, std::vector<std::unique_ptr<ExpressionNode>> args)
            : ExpressionNode(line), callee(std::move(callee_expr)), arguments(std::move(args)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "FunctionCallNode"; }
};

class AttributeAccessNode : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> object;
    std::unique_ptr<IdentifierNode> attribute;

    AttributeAccessNode(int line, std::unique_ptr<ExpressionNode> obj_expr, std::unique_ptr<IdentifierNode> attr_ident)
            : ExpressionNode(line), object(std::move(obj_expr)), attribute(std::move(attr_ident)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "AttributeAccessNode"; }
};

class SubscriptionNode : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> object;
    std::unique_ptr<ExpressionNode> index;

    SubscriptionNode(int line, std::unique_ptr<ExpressionNode> obj_expr, std::unique_ptr<ExpressionNode> index_expr)
            : ExpressionNode(line), object(std::move(obj_expr)), index(std::move(index_expr)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "SubscriptionNode"; }
};

class ListLiteralNode : public ExpressionNode {
public:
    std::vector<std::unique_ptr<ExpressionNode>> elements;

    ListLiteralNode(int line, std::vector<std::unique_ptr<ExpressionNode>> elems)
            : ExpressionNode(line), elements(std::move(elems)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ListLiteralNode"; }
};

class TupleLiteralNode : public ExpressionNode {
public:
    std::vector<std::unique_ptr<ExpressionNode>> elements;

    TupleLiteralNode(int line, std::vector<std::unique_ptr<ExpressionNode>> elems)
            : ExpressionNode(line), elements(std::move(elems)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "TupleLiteralNode"; }
};

class DictLiteralNode : public ExpressionNode {
public:
    std::vector<std::pair<std::unique_ptr<ExpressionNode>, std::unique_ptr<ExpressionNode>>> pairs;

    DictLiteralNode(int line, std::vector<std::pair<std::unique_ptr<ExpressionNode>, std::unique_ptr<ExpressionNode>>> kv_pairs)
            : ExpressionNode(line), pairs(std::move(kv_pairs)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "DictLiteralNode"; }
};

class SetLiteralNode : public ExpressionNode {
public:
    std::vector<std::unique_ptr<ExpressionNode>> elements;

    SetLiteralNode(int line, std::vector<std::unique_ptr<ExpressionNode>> elems)
            : ExpressionNode(line), elements(std::move(elems)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "SetLiteralNode"; }
};

class AssignmentExpressionNode : public ExpressionNode { // Walrus operator :=
public:
    std::unique_ptr<IdentifierNode> target;
    std::unique_ptr<ExpressionNode> value;

    AssignmentExpressionNode(int line, std::unique_ptr<IdentifierNode> target_ident, std::unique_ptr<ExpressionNode> val_expr)
            : ExpressionNode(line), target(std::move(target_ident)), value(std::move(val_expr)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "AssignmentExpressionNode"; }
};


#endif // EXPRESSIONS_HPP