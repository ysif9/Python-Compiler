#ifndef UTILNODES_HPP
#define UTILNODES_HPP

#include "ASTNode.hpp"
#include <string>
#include <vector>
#include <memory>
#include "Expressions.hpp" // Required for ExpressionNode

// Represents a single parameter in a function/lambda definition
class ParameterNode : public ASTNode {
public:
    enum class Kind {
        POSITIONAL_ONLY,
        POSITIONAL_OR_KEYWORD,
        VAR_POSITIONAL,
        KEYWORD_ONLY,
        VAR_KEYWORD
    };

    std::string arg_name;
    Kind kind;

    std::unique_ptr<ExpressionNode> default_value;

    ParameterNode(int line, std::string name_str, Kind k,
            /* Removed annotation */
                  std::unique_ptr<ExpressionNode> def_val = nullptr)
            : ASTNode(line), arg_name(std::move(name_str)), kind(k),
            /* Removed annotation init */ default_value(std::move(def_val)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ParameterNode"; }
};


// Represents the full set of arguments for a function definition
class ArgumentsNode : public ASTNode {
public:
    std::vector<std::unique_ptr<ParameterNode>> posonlyargs;
    std::vector<std::unique_ptr<ParameterNode>> args;
    std::unique_ptr<ParameterNode> vararg;
    std::vector<std::unique_ptr<ParameterNode>> kwonlyargs;
    std::unique_ptr<ParameterNode> kwarg;

    ArgumentsNode(int line,
                  std::vector<std::unique_ptr<ParameterNode>> po_args = {},
                  std::vector<std::unique_ptr<ParameterNode>> reg_args = {},
                  std::unique_ptr<ParameterNode> va_arg = nullptr,
                  std::vector<std::unique_ptr<ParameterNode>> ko_args = {},
                  std::unique_ptr<ParameterNode> kwa_arg = nullptr
    )
            : ASTNode(line),
              posonlyargs(std::move(po_args)),
              args(std::move(reg_args)),
              vararg(std::move(va_arg)),
              kwonlyargs(std::move(ko_args)),
              kwarg(std::move(kwa_arg))
    {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ArgumentsNode"; }
};


#endif // UTILNODES_HPP