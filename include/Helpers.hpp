#ifndef HELPERS_HPP
#define HELPERS_HPP

#include "ASTNode.hpp"

class IdentifierNode; // From Expressions.hpp
class ExpressionNode; // From Expressions.hpp


// Represents 'name = value' in function calls, class definitions, etc.
class KeywordArgNode : public ASTNode {
public:
    std::unique_ptr<IdentifierNode> arg_name;
    std::unique_ptr<ExpressionNode> value;

    KeywordArgNode(int line, std::unique_ptr<IdentifierNode> name, std::unique_ptr<ExpressionNode> val)
            : ASTNode(line), arg_name(std::move(name)), value(std::move(val)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "KeywordArgNode"; }
};


#endif // HELPERS_HPP