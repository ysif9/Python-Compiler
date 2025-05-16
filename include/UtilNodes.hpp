#ifndef UTILNODES_HPP
#define UTILNODES_HPP

#include "ASTNode.hpp"
#include <string>
#include <vector>
#include <memory>
// Forward declare ExpressionNode if not fully included, to avoid circular dependencies
// However, ParameterNode uses unique_ptr<ExpressionNode> for default_value, so full definition is better.
#include "Expressions.hpp" // Required for ExpressionNode

// Represents a single parameter in a function/lambda definition
class ParameterNode : public ASTNode {
public:
    enum class Kind {
        // POSITIONAL_ONLY, // Removed as per CFG: "Removed slash_no_default, slash_with_default (positional-only)"
        POSITIONAL_OR_KEYWORD, // Corresponds to 'param' in CFG
        VAR_POSITIONAL,        // Corresponds to 'TK_MULTIPLY param_no_default' (*args)
        // KEYWORD_ONLY,       // Removed as per CFG: "Simplified to remove keyword-only parameters without *args name"
        // and no explicit syntax like '*, kwarg' in the simplified CFG's 'parameters' rule.
        VAR_KEYWORD            // Corresponds to 'TK_POWER param_no_default' (**kwargs)
    };

    std::string arg_name; // From 'param: TK_IDENTIFIER'
    Kind kind;
    std::unique_ptr<ExpressionNode> default_value; // From 'default: TK_ASSIGN expression'

    ParameterNode(int line, std::string name_str, Kind k,
                  std::unique_ptr<ExpressionNode> def_val = nullptr)
            : ASTNode(line), arg_name(std::move(name_str)), kind(k),
              default_value(std::move(def_val)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ParameterNode"; }
};


// Represents the full set of arguments for a function definition (CFG 'params' -> 'parameters')
class ArgumentsNode : public ASTNode {
public:
    // std::vector<std::unique_ptr<ParameterNode>> posonlyargs; // Removed, CFG simplified positional-only
    std::vector<std::unique_ptr<ParameterNode>> args;       // Regular parameters (param_no_default, param_with_default)
    std::unique_ptr<ParameterNode> vararg;                  // *args (from simplified_star_etc -> TK_MULTIPLY)
    // std::vector<std::unique_ptr<ParameterNode>> kwonlyargs; // Removed, CFG simplified keyword-only syntax
    std::unique_ptr<ParameterNode> kwarg;                   // **kwargs (from simplified_star_etc -> kwds -> TK_POWER)

    ArgumentsNode(int line,
            // std::vector<std::unique_ptr<ParameterNode>> po_args = {}, // Removed
                  std::vector<std::unique_ptr<ParameterNode>> reg_args = {},
                  std::unique_ptr<ParameterNode> va_arg = nullptr,
            // std::vector<std::unique_ptr<ParameterNode>> ko_args = {}, // Removed
                  std::unique_ptr<ParameterNode> kwa_arg = nullptr
    )
            : ASTNode(line),
            // posonlyargs(std::move(po_args)), // Removed
              args(std::move(reg_args)),
              vararg(std::move(va_arg)),
            // kwonlyargs(std::move(ko_args)), // Removed
              kwarg(std::move(kwa_arg))
    {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ArgumentsNode"; }
};


#endif // UTILNODES_HPP