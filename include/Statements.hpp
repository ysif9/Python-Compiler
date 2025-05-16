#ifndef STATEMENTS_HPP
#define STATEMENTS_HPP

#include "ASTNode.hpp"
#include "Token.hpp"
#include <vector>
#include <string>
#include <memory>
#include <utility>

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
    std::vector<std::unique_ptr<ExpressionNode>> targets;
    std::unique_ptr<ExpressionNode> value;

    AssignmentStatementNode(int line, std::vector<std::unique_ptr<ExpressionNode>> tgts_expr, std::unique_ptr<ExpressionNode> val_expr)
            : StatementNode(line), targets(std::move(tgts_expr)), value(std::move(val_expr)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "AssignmentStatementNode"; }
};

// REMOVED: class AnnAssignNode

// For augmented assignment: target op= value (e.g. x += 1)
class AugAssignNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> target;
    Token op; // The augmented assignment operator (e.g., TK_PLUS_ASSIGN)
    std::unique_ptr<ExpressionNode> value;

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
    std::vector<std::pair<std::unique_ptr<ExpressionNode>, std::unique_ptr<BlockNode>>> elif_blocks;
    std::unique_ptr<BlockNode> else_block; // Optional

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
    std::unique_ptr<ExpressionNode> target;
    std::unique_ptr<ExpressionNode> iterable;
    std::unique_ptr<BlockNode> body;
    std::unique_ptr<BlockNode> else_block;

    ForStatementNode(int line, std::unique_ptr<ExpressionNode> target_expr, std::unique_ptr<ExpressionNode> iter_expr,
                     std::unique_ptr<BlockNode> body_block, /* Removed async_val */
                     std::unique_ptr<BlockNode> else_blk = nullptr)
            : StatementNode(line), target(std::move(target_expr)), iterable(std::move(iter_expr)),
              body(std::move(body_block)), else_block(std::move(else_blk)) {} // Removed is_async init

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
    std::unique_ptr<ExpressionNode> value; // Optional

    ReturnStatementNode(int line, std::unique_ptr<ExpressionNode> val_expr = nullptr)
            : StatementNode(line), value(std::move(val_expr)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ReturnStatementNode"; }
};

class RaiseStatementNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> exception;
    std::unique_ptr<ExpressionNode> cause;

    RaiseStatementNode(int line, std::unique_ptr<ExpressionNode> exc_expr = nullptr, std::unique_ptr<ExpressionNode> cause_expr = nullptr)
            : StatementNode(line), exception(std::move(exc_expr)), cause(std::move(cause_expr)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "RaiseStatementNode"; }
};

// --- Definitions (Function, Class) ---
class FunctionDefinitionNode : public StatementNode {
public:
    std::unique_ptr<IdentifierNode> name;
    std::unique_ptr<ArgumentsNode> arguments_spec;
    std::unique_ptr<BlockNode> body;

    FunctionDefinitionNode(int line, std::unique_ptr<IdentifierNode> func_name,
                           std::unique_ptr<ArgumentsNode> args_spec,
                           std::unique_ptr<BlockNode> func_body
            /* Removed decorators, ret_ann, async_val */)
            : StatementNode(line), name(std::move(func_name)), arguments_spec(std::move(args_spec)),
              body(std::move(func_body)) {} /* Removed init for decorators, return_annotation, is_async */

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "FunctionDefinitionNode"; }
};

class ClassDefinitionNode : public StatementNode {
public:
    std::unique_ptr<IdentifierNode> name;
    std::vector<std::unique_ptr<ExpressionNode>> base_classes;
    std::vector<std::unique_ptr<KeywordArgNode>> keywords;
    std::unique_ptr<BlockNode> body;

    ClassDefinitionNode(int line, std::unique_ptr<IdentifierNode> class_name,
                        std::vector<std::unique_ptr<ExpressionNode>> bases,
                        std::vector<std::unique_ptr<KeywordArgNode>> class_keywords,
                        std::unique_ptr<BlockNode> class_body
            /* Removed decorators */)
            : StatementNode(line), name(std::move(class_name)), base_classes(std::move(bases)),
              keywords(std::move(class_keywords)), body(std::move(class_body)) {} /* Removed init for decorators */

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ClassDefinitionNode"; }
};

// --- Import Statements ---
class NamedImportNode : public ASTNode {
public:
    std::string module_path_str;
    std::unique_ptr<IdentifierNode> alias;

    NamedImportNode(int line, std::string path_str, std::unique_ptr<IdentifierNode> alias_node = nullptr)
            : ASTNode(line), module_path_str(std::move(path_str)), alias(std::move(alias_node)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "NamedImportNode"; }
};

class ImportStatementNode : public StatementNode { // 'import a, b as c'
public:
    std::vector<std::unique_ptr<NamedImportNode>> names;

    ImportStatementNode(int line, std::vector<std::unique_ptr<NamedImportNode>> import_names)
            : StatementNode(line), names(std::move(import_names)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ImportStatementNode"; }
};

class ImportNameNode : public ASTNode {
public:
    std::string name_str;
    std::unique_ptr<IdentifierNode> alias;

    ImportNameNode(int line, std::string imported_name_str, std::unique_ptr<IdentifierNode> alias_node = nullptr)
            : ASTNode(line), name_str(std::move(imported_name_str)), alias(std::move(alias_node)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ImportNameNode"; }
};

class ImportFromStatementNode : public StatementNode {
public:
    int level;
    std::string module_str;
    std::vector<std::unique_ptr<ImportNameNode>> names;
    bool import_star;

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
    std::vector<std::unique_ptr<IdentifierNode>> names;

    GlobalStatementNode(int line, std::vector<std::unique_ptr<IdentifierNode>> global_names)
            : StatementNode(line), names(std::move(global_names)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "GlobalStatementNode"; }
};

class NonlocalStatementNode : public StatementNode {
public:
    std::vector<std::unique_ptr<IdentifierNode>> names;

    NonlocalStatementNode(int line, std::vector<std::unique_ptr<IdentifierNode>> nonlocal_names)
            : StatementNode(line), names(std::move(nonlocal_names)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "NonlocalStatementNode"; }
};

// --- Exception Handling ---
class ExceptionHandlerNode : public ASTNode {
public:
    std::unique_ptr<ExpressionNode> type;
    std::unique_ptr<IdentifierNode> name;
    std::unique_ptr<BlockNode> body;
    bool is_star_handler; // Kept: CFG has distinct except_star_block

    ExceptionHandlerNode(int line, std::unique_ptr<BlockNode> handler_body,
                         std::unique_ptr<ExpressionNode> exc_type = nullptr,
                         std::unique_ptr<IdentifierNode> exc_name = nullptr,
                         bool star_handler = false)
            : ASTNode(line), type(std::move(exc_type)), name(std::move(exc_name)),
              body(std::move(handler_body)), is_star_handler(star_handler) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ExceptionHandlerNode"; }
};

class TryStatementNode : public StatementNode {
public:
    std::unique_ptr<BlockNode> try_block;
    std::vector<std::unique_ptr<ExceptionHandlerNode>> handlers;
    std::unique_ptr<BlockNode> else_block;
    std::unique_ptr<BlockNode> finally_block;

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