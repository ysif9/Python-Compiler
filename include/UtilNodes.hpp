#ifndef UTILNODES_HPP
#define UTILNODES_HPP

#include "ASTNode.hpp"
#include <string>
#include <vector>
#include <memory>
#include "Expressions.hpp"

// Forward declarations
class ExpressionNode;
class IdentifierNode; // If ParameterNode name is IdentifierNode

// TypeHintNode simply holds an expression representing the type hint.
class TypeHintNode : public ASTNode { // Technically, this wraps an expression.
public:
    std::unique_ptr<ExpressionNode> type_expression;

    TypeHintNode(int line, std::unique_ptr<ExpressionNode> type_expr)
            : ASTNode(line), type_expression(std::move(type_expr)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "TypeHintNode"; }
};

// Represents a single parameter in a function/lambda definition
class ParameterNode : public ASTNode {
public:
    // Represents the kind of parameter (positional-only, *args, **kwargs, etc.)
    enum class Kind {
        POSITIONAL_ONLY,         // Before /
        POSITIONAL_OR_KEYWORD,   // Normal argument
        VAR_POSITIONAL,          // *args
        KEYWORD_ONLY,            // After *args or *
        VAR_KEYWORD              // **kwargs
    };

    std::string arg_name; // Name of the parameter (e.g. "x"). For *args, it's "args".
    Kind kind;
    std::unique_ptr<ExpressionNode> annotation; // Type hint (optional)
    std::unique_ptr<ExpressionNode> default_value; // Default value (optional)

    ParameterNode(int line, std::string name_str, Kind k,
                  std::unique_ptr<ExpressionNode> ann = nullptr,
                  std::unique_ptr<ExpressionNode> def_val = nullptr)
            : ASTNode(line), arg_name(std::move(name_str)), kind(k),
              annotation(std::move(ann)), default_value(std::move(def_val)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ParameterNode"; }
};


// Represents the full set of arguments for a function definition (similar to Python's ast.arguments)
class ArgumentsNode : public ASTNode {
public:
    // For parameters before /
    std::vector<std::unique_ptr<ParameterNode>> posonlyargs;
    // For regular positional or keyword parameters
    std::vector<std::unique_ptr<ParameterNode>> args;
    // For *args type parameter (e.g., ParameterNode with kind=VAR_POSITIONAL)
    std::unique_ptr<ParameterNode> vararg; // Can be nullptr if no *args
    // For keyword-only parameters (after *args or *)
    std::vector<std::unique_ptr<ParameterNode>> kwonlyargs;
    // Default values for keyword-only parameters; must be parallel to kwonlyargs, nullptr if no default
    // (Note: ParameterNode itself also has default_value, this might be redundant or specific for Python's ast model)
    // Python AST: kw_defaults is a list of expr (value only).
    // We can keep default_value on ParameterNode for kwonlyargs too.
    // std::vector<std::unique_ptr<ExpressionNode>> kw_defaults; (Alternative to ParameterNode::default_value for kwonlyargs)

    // For **kwargs type parameter (e.g., ParameterNode with kind=VAR_KEYWORD)
    std::unique_ptr<ParameterNode> kwarg; // Can be nullptr if no **kwargs

    // Default values for regular args (last N args); parallel to the tail of `args` list.
    // Python AST: defaults is a list of expr (value only).
    // Again, ParameterNode::default_value makes this somewhat redundant from AST perspective.
    // std::vector<std::unique_ptr<ExpressionNode>> defaults; (Alternative for args)


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