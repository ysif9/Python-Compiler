#ifndef STATEMENTS_HPP
#define STATEMENTS_HPP

#include "ASTNode.hpp"
#include "Token.hpp"
#include <vector>
#include <string>
#include <memory>
#include <utility> // For std::pair in IfStatementNode

// Forward declarations from other AST files
#include "Expressions.hpp" // For ExpressionNode, IdentifierNode, etc.
#include "UtilNodes.hpp"   // For ArgumentsNode, ParameterNode
#include "Helpers.hpp"     // For KeywordArgNode


// --- Base Statement Node ---
class StatementNode : public ASTNode {
public:
    StatementNode(int line) : ASTNode(line) {}
};

// --- Structural Nodes ---
class BlockNode : public ASTNode {
public:
    std::vector<std::unique_ptr<StatementNode>> statements;

    BlockNode(int line, std::vector<std::unique_ptr<StatementNode>> stmts)
            : ASTNode(line), statements(std::move(stmts)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "BlockNode"; }
};

class ProgramNode : public ASTNode {
public:
    std::vector<std::unique_ptr<StatementNode>> statements;

    ProgramNode(int line, std::vector<std::unique_ptr<StatementNode>> stmts)
            : ASTNode(line), statements(std::move(stmts)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ProgramNode"; }
};

// --- Basic Statements ---

class AssignmentStatementNode : public StatementNode {
public:
    std::vector<std::unique_ptr<ExpressionNode>> targets; // CFG 'targets' can be multiple
    std::unique_ptr<ExpressionNode> value; // CFG 'expressions' (RHS), parser usually forms a single expr (e.g. tuple)

    AssignmentStatementNode(int line, std::vector<std::unique_ptr<ExpressionNode>> tgts_expr, std::unique_ptr<ExpressionNode> val_expr)
            : StatementNode(line), targets(std::move(tgts_expr)), value(std::move(val_expr)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "AssignmentStatementNode"; }
};

// For augmented assignment: target op= value (e.g. x += 1)
class AugAssignNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> target; // CFG 'single_target'
    Token op; // The augmented assignment operator (e.g., TK_PLUS_ASSIGN)
    std::unique_ptr<ExpressionNode> value; // CFG 'expressions'

    AugAssignNode(int line, std::unique_ptr<ExpressionNode> tgt, Token op_token, std::unique_ptr<ExpressionNode> val)
            : StatementNode(line), target(std::move(tgt)), op(op_token), value(std::move(val)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "AugAssignNode"; }
};


class ExpressionStatementNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> expression;

    ExpressionStatementNode(int line, std::unique_ptr<ExpressionNode> expr)
            : StatementNode(line), expression(std::move(expr)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ExpressionStatementNode"; }
};

class PassStatementNode : public StatementNode {
public:
    PassStatementNode(int line) : StatementNode(line) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "PassStatementNode"; }
};

// --- Control Flow Statements ---
class IfStatementNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> condition;
    std::unique_ptr<BlockNode> then_block;
    std::vector<std::pair<std::unique_ptr<ExpressionNode>, std::unique_ptr<BlockNode>>> elif_blocks; // For TK_ELIF
    std::unique_ptr<BlockNode> else_block; // Optional (TK_ELSE)

    IfStatementNode(int line, std::unique_ptr<ExpressionNode> cond_expr, std::unique_ptr<BlockNode> then_blk,
                    std::vector<std::pair<std::unique_ptr<ExpressionNode>, std::unique_ptr<BlockNode>>> elif_blks = {},
                    std::unique_ptr<BlockNode> else_blk = nullptr)
            : StatementNode(line), condition(std::move(cond_expr)), then_block(std::move(then_blk)),
              elif_blocks(std::move(elif_blks)), else_block(std::move(else_blk)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "IfStatementNode"; }
};

class WhileStatementNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> condition;
    std::unique_ptr<BlockNode> body;
    std::unique_ptr<BlockNode> else_block; // Optional 'else' clause for while loop

    WhileStatementNode(int line, std::unique_ptr<ExpressionNode> cond_expr, std::unique_ptr<BlockNode> body_block,
                       std::unique_ptr<BlockNode> else_blk = nullptr)
            : StatementNode(line), condition(std::move(cond_expr)), body(std::move(body_block)),
              else_block(std::move(else_blk)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "WhileStatementNode"; }
};

class ForStatementNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> target; // CFG 'targets', parser wraps multiple into one (e.g. TupleLiteralNode)
    std::unique_ptr<ExpressionNode> iterable; // CFG 'expressions', parser wraps multiple into one
    std::unique_ptr<BlockNode> body;
    std::unique_ptr<BlockNode> else_block; // Optional

    ForStatementNode(int line, std::unique_ptr<ExpressionNode> target_expr, std::unique_ptr<ExpressionNode> iter_expr,
                     std::unique_ptr<BlockNode> body_block,
                     std::unique_ptr<BlockNode> else_blk = nullptr)
            : StatementNode(line), target(std::move(target_expr)), iterable(std::move(iter_expr)),
              body(std::move(body_block)), else_block(std::move(else_blk)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ForStatementNode"; }
};

class BreakStatementNode : public StatementNode {
public:
    BreakStatementNode(int line) : StatementNode(line) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "BreakStatementNode"; }
};

class ContinueStatementNode : public StatementNode {
public:
    ContinueStatementNode(int line) : StatementNode(line) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ContinueStatementNode"; }
};

class ReturnStatementNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> value; // Optional, from CFG 'expressions_opt'

    ReturnStatementNode(int line, std::unique_ptr<ExpressionNode> val_expr = nullptr)
            : StatementNode(line), value(std::move(val_expr)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ReturnStatementNode"; }
};

class RaiseStatementNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> exception; // Optional (TK_RAISE expression ...)
    std::unique_ptr<ExpressionNode> cause;     // Optional (TK_FROM expression)

    RaiseStatementNode(int line, std::unique_ptr<ExpressionNode> exc_expr = nullptr, std::unique_ptr<ExpressionNode> cause_expr = nullptr)
            : StatementNode(line), exception(std::move(exc_expr)), cause(std::move(cause_expr)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "RaiseStatementNode"; }
};

// --- Definitions (Function, Class) ---
class FunctionDefinitionNode : public StatementNode {
public:
    std::unique_ptr<IdentifierNode> name;
    std::unique_ptr<ArgumentsNode> arguments_spec; // From CFG 'params_opt'
    std::unique_ptr<BlockNode> body;
    // Decorators, return annotations, async removed as per CFG

    FunctionDefinitionNode(int line, std::unique_ptr<IdentifierNode> func_name,
                           std::unique_ptr<ArgumentsNode> args_spec,
                           std::unique_ptr<BlockNode> func_body)
            : StatementNode(line), name(std::move(func_name)), arguments_spec(std::move(args_spec)),
              body(std::move(func_body)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "FunctionDefinitionNode"; }
};

class ClassDefinitionNode : public StatementNode {
public:
    std::unique_ptr<IdentifierNode> name;
    std::vector<std::unique_ptr<ExpressionNode>> base_classes; // From CFG 'class_arguments_opt' -> 'arguments_opt' -> positional
    std::vector<std::unique_ptr<KeywordArgNode>> keywords;     // From CFG 'class_arguments_opt' -> 'arguments_opt' -> keywords
    std::unique_ptr<BlockNode> body;
    // Decorators removed as per CFG

    ClassDefinitionNode(int line, std::unique_ptr<IdentifierNode> class_name,
                        std::vector<std::unique_ptr<ExpressionNode>> bases,
                        std::vector<std::unique_ptr<KeywordArgNode>> class_keywords,
                        std::unique_ptr<BlockNode> class_body)
            : StatementNode(line), name(std::move(class_name)), base_classes(std::move(bases)),
              keywords(std::move(class_keywords)), body(std::move(class_body)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ClassDefinitionNode"; }
};

// --- Import Statements ---
// Helper for 'import a as b' parts
class NamedImportNode : public ASTNode { // Corresponds to CFG 'dotted_as_name'
public:
    std::string module_path_str; // The 'dotted_name' part
    std::unique_ptr<IdentifierNode> alias; // Optional 'AS TK_IDENTIFIER' part

    NamedImportNode(int line, std::string path_str, std::unique_ptr<IdentifierNode> alias_node = nullptr)
            : ASTNode(line), module_path_str(std::move(path_str)), alias(std::move(alias_node)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "NamedImportNode"; }
};

class ImportStatementNode : public StatementNode { // 'import a, b as c' (CFG 'import_name')
public:
    std::vector<std::unique_ptr<NamedImportNode>> names;

    ImportStatementNode(int line, std::vector<std::unique_ptr<NamedImportNode>> import_names)
            : StatementNode(line), names(std::move(import_names)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ImportStatementNode"; }
};

// Helper for 'from .mod import x as y' parts
class ImportNameNode : public ASTNode { // Corresponds to CFG 'import_from_as_name'
public:
    std::string name_str; // The 'TK_IDENTIFIER' being imported
    std::unique_ptr<IdentifierNode> alias; // Optional 'AS TK_IDENTIFIER' part

    ImportNameNode(int line, std::string imported_name_str, std::unique_ptr<IdentifierNode> alias_node = nullptr)
            : ASTNode(line), name_str(std::move(imported_name_str)), alias(std::move(alias_node)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ImportNameNode"; }
};

class ImportFromStatementNode : public StatementNode { // (CFG 'import_from')
public:
    int level; // Number of leading dots (from 'dot_or_ellipsis_star' or 'dot_or_ellipsis_plus')
    std::string module_str; // Optional module name after dots (from 'dotted_name')
    std::vector<std::unique_ptr<ImportNameNode>> names; // Specific names imported
    bool import_star; // True if 'TK_MULTIPLY' is used

    ImportFromStatementNode(int line, int lvl, std::string mod_str,
                            std::vector<std::unique_ptr<ImportNameNode>> import_names, bool star = false)
            : StatementNode(line), level(lvl), module_str(std::move(mod_str)),
              names(std::move(import_names)), import_star(star) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ImportFromStatementNode"; }
};

// --- Scope Statements ---
class GlobalStatementNode : public StatementNode {
public:
    std::vector<std::unique_ptr<IdentifierNode>> names; // From CFG 'name_comma_list'

    GlobalStatementNode(int line, std::vector<std::unique_ptr<IdentifierNode>> global_names)
            : StatementNode(line), names(std::move(global_names)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "GlobalStatementNode"; }
};

class NonlocalStatementNode : public StatementNode {
public:
    std::vector<std::unique_ptr<IdentifierNode>> names; // From CFG 'name_comma_list'

    NonlocalStatementNode(int line, std::vector<std::unique_ptr<IdentifierNode>> nonlocal_names)
            : StatementNode(line), names(std::move(nonlocal_names)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "NonlocalStatementNode"; }
};

// --- Exception Handling ---
class ExceptionHandlerNode : public ASTNode { // Corresponds to CFG 'except_block'
public:
    std::unique_ptr<ExpressionNode> type; // Optional: 'TK_EXCEPT expression'
    std::unique_ptr<IdentifierNode> name; // Optional: 'TK_AS TK_IDENTIFIER'
    std::unique_ptr<BlockNode> body;
    // is_star_handler removed as 'except*' is not in the provided CFG

    ExceptionHandlerNode(int line, std::unique_ptr<BlockNode> handler_body,
                         std::unique_ptr<ExpressionNode> exc_type = nullptr,
                         std::unique_ptr<IdentifierNode> exc_name = nullptr)
            : ASTNode(line), type(std::move(exc_type)), name(std::move(exc_name)),
              body(std::move(handler_body)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ExceptionHandlerNode"; }
};

class TryStatementNode : public StatementNode { // Corresponds to CFG 'try_stmt'
public:
    std::unique_ptr<BlockNode> try_block;
    std::vector<std::unique_ptr<ExceptionHandlerNode>> handlers; // From 'except_block_plus'
    std::unique_ptr<BlockNode> else_block;    // Optional, from 'else_block_opt'
    std::unique_ptr<BlockNode> finally_block; // Optional, from 'finally_block' or 'finally_block_opt'

    TryStatementNode(int line, std::unique_ptr<BlockNode> try_b,
                     std::vector<std::unique_ptr<ExceptionHandlerNode>> ex_handlers,
                     std::unique_ptr<BlockNode> else_b = nullptr,
                     std::unique_ptr<BlockNode> finally_b = nullptr)
            : StatementNode(line), try_block(std::move(try_b)), handlers(std::move(ex_handlers)),
              else_block(std::move(else_b)), finally_block(std::move(finally_b)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "TryStatementNode"; }
};
#endif // STATEMENTS_HPP