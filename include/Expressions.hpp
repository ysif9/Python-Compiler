#ifndef EXPRESSIONS_HPP
#define EXPRESSIONS_HPP

#include "ASTNode.hpp"
#include "Token.hpp" // For Token struct if used directly (e.g. BinaryOpNode)
#include "Literals.hpp" // Include new literals file
#include "Helpers.hpp"  // For KeywordArgNode if used in expressions (e.g. Dict items implicitly)
#include "UtilNodes.hpp" // For ArgumentsNode

#include <vector>
#include <string>
#include <memory>
#include <utility> // For std::pair in DictLiteralNode


// Base class for all expression nodes
class ExpressionNode : public ASTNode {
public:
    ExpressionNode(int line) : ASTNode(line) {}
};

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
    Token op; // Token provides TokenType for the operator
    std::unique_ptr<ExpressionNode> right;

    BinaryOpNode(int line, std::unique_ptr<ExpressionNode> l, Token op_token, std::unique_ptr<ExpressionNode> r)
            : ExpressionNode(line), left(std::move(l)), op(op_token), right(std::move(r)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "BinaryOpNode"; }
};

class UnaryOpNode : public ExpressionNode {
public:
    Token op; // Token provides TokenType for the operator
    std::unique_ptr<ExpressionNode> operand;

    UnaryOpNode(int line, Token op_token, std::unique_ptr<ExpressionNode> o)
            : ExpressionNode(line), op(op_token), operand(std::move(o)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "UnaryOpNode"; }
};

class FunctionCallNode : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> callee;
    std::vector<std::unique_ptr<ExpressionNode>> args;
    std::vector<std::unique_ptr<KeywordArgNode>> keywords;

    FunctionCallNode(int line, std::unique_ptr<ExpressionNode> callee_expr,
                     std::vector<std::unique_ptr<ExpressionNode>> call_args,
                     std::vector<std::unique_ptr<KeywordArgNode>> call_keywords)
            : ExpressionNode(line), callee(std::move(callee_expr)),
              args(std::move(call_args)), keywords(std::move(call_keywords)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "FunctionCallNode"; }
};

class AttributeAccessNode : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> object;
    std::unique_ptr<IdentifierNode> attribute_name; // Name of the attribute being accessed

    AttributeAccessNode(int line, std::unique_ptr<ExpressionNode> obj_expr, std::unique_ptr<IdentifierNode> attr_ident)
            : ExpressionNode(line), object(std::move(obj_expr)), attribute_name(std::move(attr_ident)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "AttributeAccessNode"; }
};

// For slicing: start:stop:step
class SliceNode : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> lower;  // Optional
    std::unique_ptr<ExpressionNode> upper;  // Optional
    std::unique_ptr<ExpressionNode> step;   // Optional

    SliceNode(int line, std::unique_ptr<ExpressionNode> l, std::unique_ptr<ExpressionNode> u, std::unique_ptr<ExpressionNode> s)
            : ExpressionNode(line), lower(std::move(l)), upper(std::move(u)), step(std::move(s)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "SliceNode"; }
};

class SubscriptionNode : public ExpressionNode { // obj[index]
public:
    std::unique_ptr<ExpressionNode> object;
    std::unique_ptr<ExpressionNode> slice_or_index; // Can be a SliceNode or any other ExpressionNode

    SubscriptionNode(int line, std::unique_ptr<ExpressionNode> obj_expr, std::unique_ptr<ExpressionNode> index_expr)
            : ExpressionNode(line), object(std::move(obj_expr)), slice_or_index(std::move(index_expr)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "SubscriptionNode"; }
};

// Collection Literals
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
    std::vector<std::unique_ptr<ExpressionNode>> keys;
    std::vector<std::unique_ptr<ExpressionNode>> values; // Parallel vectors for key-value pairs

    DictLiteralNode(int line, std::vector<std::unique_ptr<ExpressionNode>> dict_keys,
                    std::vector<std::unique_ptr<ExpressionNode>> dict_values)
            : ExpressionNode(line), keys(std::move(dict_keys)), values(std::move(dict_values)) {}

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

// Ternary: value_if_true IF condition ELSE value_if_false
class IfExpNode : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> condition;
    std::unique_ptr<ExpressionNode> body; // Value if condition is true
    std::unique_ptr<ExpressionNode> orelse; // Value if condition is false

    IfExpNode(int line, std::unique_ptr<ExpressionNode> cond, std::unique_ptr<ExpressionNode> then_expr, std::unique_ptr<ExpressionNode> else_expr)
            : ExpressionNode(line), condition(std::move(cond)), body(std::move(then_expr)), orelse(std::move(else_expr)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "IfExpNode"; }
};

// For chained comparisons: left op1 comp1 op2 comp2 ...
class ComparisonNode : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> left; // First operand
    std::vector<Token> ops; // List of comparison operator tokens
    std::vector<std::unique_ptr<ExpressionNode>> comparators; // List of subsequent operands

    ComparisonNode(int line, std::unique_ptr<ExpressionNode> l,
                   std::vector<Token> op_tokens,
                   std::vector<std::unique_ptr<ExpressionNode>> comps)
            : ExpressionNode(line), left(std::move(l)), ops(std::move(op_tokens)), comparators(std::move(comps)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ComparisonNode"; }
};

#endif // EXPRESSIONS_HPP