#ifndef HELPERS_HPP
#define HELPERS_HPP

#include "ASTNode.hpp"

class IdentifierNode;
class ExpressionNode;


// Represents 'name = value' in function calls, class definitions, etc.
class KeywordArgNode : public ASTNode {
public:
    // `arg_name` can be nullptr for `**kwargs` representation if keyword node is used for that,
    // but `StarExprNode` is more typical for `**kwargs`. This node is for explicit `name=value`.
    std::unique_ptr<IdentifierNode> arg_name; // The keyword name.
    std::unique_ptr<ExpressionNode> value;    // The expression for the value.

    KeywordArgNode(int line, std::unique_ptr<IdentifierNode> name, std::unique_ptr<ExpressionNode> val)
            : ASTNode(line), arg_name(std::move(name)), value(std::move(val)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "KeywordArgNode"; }
};


#endif // HELPERS_HPP