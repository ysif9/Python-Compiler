#ifndef EXPRESSIONS_HPP
#define EXPRESSIONS_HPP

#include "ASTNode.hpp"
#include "Token.hpp" // For Token struct if used directly (e.g. BinaryOpNode)
#include "Literals.hpp" // Include new literals file
#include "Helpers.hpp"  // For KeywordArgNode if used in expressions (e.g. Dict items implicitly)
#include "UtilNodes.hpp" // For ArgumentsNode (in Lambda)

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

// Represents *expr or **expr
class StarExprNode : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> value;
    enum class Kind { SINGLE_STAR, DOUBLE_STAR } kind; // Differentiates *args from **kwargs type usage

    StarExprNode(int line, std::unique_ptr<ExpressionNode> val, Kind k)
            : ExpressionNode(line), value(std::move(val)), kind(k) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "StarExprNode"; }
};


class FunctionCallNode : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> callee;
    // Arguments can be regular expressions or StarExprNode (for *iterable)
    std::vector<std::unique_ptr<ExpressionNode>> args;
    // Keyword arguments. For **dict, a KeywordArgNode with name=nullptr and value=dict_expr is used,
    // or more distinctly, its value expression itself could be a StarExprNode(kind=DOUBLE_STAR)
    // Python's AST: keywords is a list of `keyword = (identifier? arg, expr value)`.
    // So a KeywordArgNode whose `name` is null could signify `**expr`.
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
    // Index can be a simple expression, a SliceNode, or even a tuple of these (parser creates TupleLiteralNode for index)
    std::unique_ptr<ExpressionNode> slice_or_index;

    SubscriptionNode(int line, std::unique_ptr<ExpressionNode> obj_expr, std::unique_ptr<ExpressionNode> index_expr)
            : ExpressionNode(line), object(std::move(obj_expr)), slice_or_index(std::move(index_expr)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "SubscriptionNode"; }
};

// Collection Literals
class ListLiteralNode : public ExpressionNode {
public:
    // Elements can include StarExprNode for unpacking
    std::vector<std::unique_ptr<ExpressionNode>> elements;

    ListLiteralNode(int line, std::vector<std::unique_ptr<ExpressionNode>> elems)
            : ExpressionNode(line), elements(std::move(elems)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ListLiteralNode"; }
};

class TupleLiteralNode : public ExpressionNode {
public:
    // Elements can include StarExprNode for unpacking
    std::vector<std::unique_ptr<ExpressionNode>> elements;

    TupleLiteralNode(int line, std::vector<std::unique_ptr<ExpressionNode>> elems)
            : ExpressionNode(line), elements(std::move(elems)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "TupleLiteralNode"; }
};


class DictElementNode : public ASTNode { // Helper base for DictLiteralNode elements
public:
    DictElementNode(int line) : ASTNode(line) {}
};

class KeyValuePairNode : public DictElementNode {
public:
    std::unique_ptr<ExpressionNode> key;
    std::unique_ptr<ExpressionNode> value;

    KeyValuePairNode(int line, std::unique_ptr<ExpressionNode> k, std::unique_ptr<ExpressionNode> v)
            : DictElementNode(line), key(std::move(k)), value(std::move(v)) {}

    // This node itself might not be visited if DictLiteralNode handles iteration. If it is:
    // void accept(ASTVisitor* visitor) override { visitor->visit(this); } // Requires visitor method
    std::string getNodeName() const override { return "KeyValuePairNode"; }
};

class DictUnpackNode : public DictElementNode { // For **expr in dicts
public:
    std::unique_ptr<ExpressionNode> value_to_unpack; // This should be the expr after **

    DictUnpackNode(int line, std::unique_ptr<ExpressionNode> val)
            : DictElementNode(line), value_to_unpack(std::move(val)) {}

    // void accept(ASTVisitor* visitor) override { visitor->visit(this); } // Requires visitor method
    std::string getNodeName() const override { return "DictUnpackNode"; }
};


class DictLiteralNode : public ExpressionNode {
public:
    // Python AST: keys and values are separate lists. For key=None, it's a dict expansion.
    // Here, `keys` can contain nullptr for `**expr`, with corresponding `values` being the expr.
    std::vector<std::unique_ptr<ExpressionNode>> keys; // nullptr for dict unpacking `**expr`
    std::vector<std::unique_ptr<ExpressionNode>> values;

    DictLiteralNode(int line, std::vector<std::unique_ptr<ExpressionNode>> dict_keys,
                    std::vector<std::unique_ptr<ExpressionNode>> dict_values)
            : ExpressionNode(line), keys(std::move(dict_keys)), values(std::move(dict_values)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "DictLiteralNode"; }
};

class SetLiteralNode : public ExpressionNode {
public:
    // Elements can include StarExprNode for unpacking
    std::vector<std::unique_ptr<ExpressionNode>> elements;

    SetLiteralNode(int line, std::vector<std::unique_ptr<ExpressionNode>> elems)
            : ExpressionNode(line), elements(std::move(elems)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "SetLiteralNode"; }
};

// Walrus operator :=
class AssignmentExpressionNode : public ExpressionNode {
public:
    std::unique_ptr<IdentifierNode> target; // Must be an identifier
    std::unique_ptr<ExpressionNode> value;

    AssignmentExpressionNode(int line, std::unique_ptr<IdentifierNode> target_ident, std::unique_ptr<ExpressionNode> val_expr)
            : ExpressionNode(line), target(std::move(target_ident)), value(std::move(val_expr)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "AssignmentExpressionNode"; }
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

class LambdaNode : public ExpressionNode {
public:
    std::unique_ptr<ArgumentsNode> arguments_spec; // From UtilNodes.hpp
    std::unique_ptr<ExpressionNode> body;

    LambdaNode(int line, std::unique_ptr<ArgumentsNode> args, std::unique_ptr<ExpressionNode> b)
            : ExpressionNode(line), arguments_spec(std::move(args)), body(std::move(b)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "LambdaNode"; }
};

class YieldExprNode : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> value; // Optional (yield None if nullptr)

    YieldExprNode(int line, std::unique_ptr<ExpressionNode> val = nullptr)
            : ExpressionNode(line), value(std::move(val)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "YieldExprNode"; }
};

class YieldFromNode : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> value; // The expression to yield from

    YieldFromNode(int line, std::unique_ptr<ExpressionNode> val)
            : ExpressionNode(line), value(std::move(val)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "YieldFromNode"; }
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

// Helper for comprehensions: 'for target in iter [if cond1 [if cond2 ...]]' or 'async for ...'
class ComprehensionGeneratorNode : public ASTNode { // Not an expression itself
public:
    std::unique_ptr<ExpressionNode> target;   // LHS of 'in'
    std::unique_ptr<ExpressionNode> iter;     // RHS of 'in'
    std::vector<std::unique_ptr<ExpressionNode>> ifs; // Conditions
    bool is_async;

    ComprehensionGeneratorNode(int line, std::unique_ptr<ExpressionNode> t, std::unique_ptr<ExpressionNode> i,
                               std::vector<std::unique_ptr<ExpressionNode>> conditions, bool async_val)
            : ASTNode(line), target(std::move(t)), iter(std::move(i)), ifs(std::move(conditions)), is_async(async_val) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ComprehensionGeneratorNode"; }
};

class ListCompNode : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> elt; // The expression for each element
    std::vector<std::unique_ptr<ComprehensionGeneratorNode>> generators;

    ListCompNode(int line, std::unique_ptr<ExpressionNode> element_expr,
                 std::vector<std::unique_ptr<ComprehensionGeneratorNode>> gens)
            : ExpressionNode(line), elt(std::move(element_expr)), generators(std::move(gens)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ListCompNode"; }
};

class SetCompNode : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> elt;
    std::vector<std::unique_ptr<ComprehensionGeneratorNode>> generators;

    SetCompNode(int line, std::unique_ptr<ExpressionNode> element_expr,
                std::vector<std::unique_ptr<ComprehensionGeneratorNode>> gens)
            : ExpressionNode(line), elt(std::move(element_expr)), generators(std::move(gens)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "SetCompNode"; }
};

class DictCompNode : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> key;
    std::unique_ptr<ExpressionNode> value;
    std::vector<std::unique_ptr<ComprehensionGeneratorNode>> generators;

    DictCompNode(int line, std::unique_ptr<ExpressionNode> k, std::unique_ptr<ExpressionNode> v,
                 std::vector<std::unique_ptr<ComprehensionGeneratorNode>> gens)
            : ExpressionNode(line), key(std::move(k)), value(std::move(v)), generators(std::move(gens)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "DictCompNode"; }
};

class GeneratorExpNode : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> elt;
    std::vector<std::unique_ptr<ComprehensionGeneratorNode>> generators;

    GeneratorExpNode(int line, std::unique_ptr<ExpressionNode> element_expr,
                     std::vector<std::unique_ptr<ComprehensionGeneratorNode>> gens)
            : ExpressionNode(line), elt(std::move(element_expr)), generators(std::move(gens)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "GeneratorExpNode"; }
};

class AwaitExprNode : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> value; // Expression being awaited

    AwaitExprNode(int line, std::unique_ptr<ExpressionNode> val)
            : ExpressionNode(line), value(std::move(val)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "AwaitExprNode"; }
};


#endif // EXPRESSIONS_HPP