#ifndef UTILNODES_HPP
#define UTILNODES_HPP

#include "ASTNode.hpp"
#include <string>
#include <vector>
#include <memory>
#include "Expressions.hpp"

// Forward declarations
class ExpressionNode;

class TypeHintNode : public ASTNode {
public:
    std::string name;
    std::vector<std::unique_ptr<TypeHintNode>> generic_arguments;

    TypeHintNode(int line, std::string name_val, std::vector<std::unique_ptr<TypeHintNode>> generic_args = {})
            : ASTNode(line), name(std::move(name_val)), generic_arguments(std::move(generic_args)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "TypeHintNode"; }
};

class ParameterNode : public ASTNode {
public:
    std::string name;
    std::unique_ptr<TypeHintNode> type_hint;
    std::unique_ptr<ExpressionNode> default_value;

    ParameterNode(int line, std::string param_name,
                  std::unique_ptr<TypeHintNode> th = nullptr,
                  std::unique_ptr<ExpressionNode> def_val = nullptr)
            : ASTNode(line), name(std::move(param_name)), type_hint(std::move(th)), default_value(std::move(def_val)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ParameterNode"; }
};


#endif // UTILNODES_HPP