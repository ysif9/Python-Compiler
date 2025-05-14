#ifndef STATEMENTS_HPP
#define STATEMENTS_HPP

#include "ASTNode.hpp"
#include "Token.hpp"    // For Token in AssignmentStatementNode, etc.
#include <vector>
#include <string>
#include <memory>
#include <utility> // For std::pair in IfStatementNode elif_blocks

// Forward declarations from other AST files
#include "Expressions.hpp" // For ExpressionNode, IdentifierNode
#include "UtilNodes.hpp"   // For ParameterNode, TypeHintNode

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
    std::unique_ptr<ExpressionNode> target;
    Token op_token;
    std::unique_ptr<ExpressionNode> value;

    AssignmentStatementNode(int line, std::unique_ptr<ExpressionNode> tgt_expr, Token op, std::unique_ptr<ExpressionNode> val_expr)
            : StatementNode(line), target(std::move(tgt_expr)), op_token(op), value(std::move(val_expr)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "AssignmentStatementNode"; }
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

class DelStatementNode : public StatementNode {
public:
    std::vector<std::unique_ptr<ExpressionNode>> targets;

    DelStatementNode(int line, std::vector<std::unique_ptr<ExpressionNode>> tgts)
            : StatementNode(line), targets(std::move(tgts)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "DelStatementNode"; }
};

class AssertStatementNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> test;
    std::unique_ptr<ExpressionNode> message; // Optional message

    AssertStatementNode(int line, std::unique_ptr<ExpressionNode> test_expr, std::unique_ptr<ExpressionNode> msg_expr = nullptr)
            : StatementNode(line), test(std::move(test_expr)), message(std::move(msg_expr)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "AssertStatementNode"; }
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
    std::unique_ptr<BlockNode> else_block;

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
                     std::unique_ptr<BlockNode> body_block, std::unique_ptr<BlockNode> else_blk = nullptr)
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
    std::unique_ptr<ExpressionNode> value; // Optional

    ReturnStatementNode(int line, std::unique_ptr<ExpressionNode> val_expr = nullptr)
            : StatementNode(line), value(std::move(val_expr)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ReturnStatementNode"; }
};

class RaiseStatementNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> exception; // Optional (e.g., raise)
    std::unique_ptr<ExpressionNode> cause;     // Optional (e.g., raise X from Y)

    RaiseStatementNode(int line, std::unique_ptr<ExpressionNode> exc_expr = nullptr, std::unique_ptr<ExpressionNode> cause_expr = nullptr)
            : StatementNode(line), exception(std::move(exc_expr)), cause(std::move(cause_expr)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "RaiseStatementNode"; }
};

// --- Definitions (Function, Class) ---
class FunctionDefinitionNode : public StatementNode {
public:
    std::unique_ptr<IdentifierNode> name;
    std::vector<std::unique_ptr<ParameterNode>> parameters;
    std::unique_ptr<BlockNode> body;
    std::unique_ptr<TypeHintNode> return_type_hint;
    std::vector<std::unique_ptr<ExpressionNode>> decorators;

    FunctionDefinitionNode(int line, std::unique_ptr<IdentifierNode> func_name,
                           std::vector<std::unique_ptr<ParameterNode>> params,
                           std::unique_ptr<BlockNode> func_body,
                           std::unique_ptr<TypeHintNode> ret_type_hint = nullptr,
                           std::vector<std::unique_ptr<ExpressionNode>> decs = {})
            : StatementNode(line), name(std::move(func_name)), parameters(std::move(params)),
              body(std::move(func_body)), return_type_hint(std::move(ret_type_hint)),
              decorators(std::move(decs)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "FunctionDefinitionNode"; }
};

class ClassDefinitionNode : public StatementNode {
public:
    std::unique_ptr<IdentifierNode> name;
    std::vector<std::unique_ptr<ExpressionNode>> base_classes;
    std::unique_ptr<BlockNode> body;
    std::vector<std::unique_ptr<ExpressionNode>> decorators;

    ClassDefinitionNode(int line, std::unique_ptr<IdentifierNode> class_name,
                        std::vector<std::unique_ptr<ExpressionNode>> bases,
                        std::unique_ptr<BlockNode> class_body,
                        std::vector<std::unique_ptr<ExpressionNode>> decs = {})
            : StatementNode(line), name(std::move(class_name)), base_classes(std::move(bases)),
              body(std::move(class_body)), decorators(std::move(decs)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ClassDefinitionNode"; }
};

// --- Import Statements ---
// Helper for 'import foo as bar'
class NamedImportNode : public ASTNode { // Not a statement itself
public:
    std::unique_ptr<IdentifierNode> module_name; // or dotted name interpreted as identifier sequence by parser
    std::unique_ptr<IdentifierNode> alias;      // Optional

    NamedImportNode(int line, std::unique_ptr<IdentifierNode> name_node, std::unique_ptr<IdentifierNode> alias_node = nullptr)
            : ASTNode(line), module_name(std::move(name_node)), alias(std::move(alias_node)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "NamedImportNode"; }
};

class ImportStatementNode : public StatementNode {
public:
    std::vector<std::unique_ptr<NamedImportNode>> names;

    ImportStatementNode(int line, std::vector<std::unique_ptr<NamedImportNode>> import_names)
            : StatementNode(line), names(std::move(import_names)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ImportStatementNode"; }
};

// Helper for 'from foo import bar as baz'
class ImportNameNode : public ASTNode { // Not a statement itself
public:
    std::unique_ptr<IdentifierNode> name;
    std::unique_ptr<IdentifierNode> alias; // Optional

    ImportNameNode(int line, std::unique_ptr<IdentifierNode> name_node, std::unique_ptr<IdentifierNode> alias_node = nullptr)
            : ASTNode(line), name(std::move(name_node)), alias(std::move(alias_node)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ImportNameNode"; }
};

class ImportFromStatementNode : public StatementNode {
public:
    int level; // Number of leading dots for relative import (0 for absolute)
    std::unique_ptr<IdentifierNode> module; // Optional (e.g., for 'from . import foo', module is nullptr)
    // Could also be a string if parser resolves dotted names.
    std::vector<std::unique_ptr<ImportNameNode>> names; // Empty if 'import *'
    bool import_star; // True if 'from module import *'

    ImportFromStatementNode(int line, int lvl, std::unique_ptr<IdentifierNode> mod_name,
                            std::vector<std::unique_ptr<ImportNameNode>> import_names, bool star = false)
            : StatementNode(line), level(lvl), module(std::move(mod_name)),
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
// Helper for 'except Foo as bar:'
class ExceptionHandlerNode : public ASTNode { // Not a statement itself
public:
    std::unique_ptr<ExpressionNode> type; // Optional (nullptr for bare 'except:')
    std::unique_ptr<IdentifierNode> name; // Optional (for 'as e')
    std::unique_ptr<BlockNode> body;

    ExceptionHandlerNode(int line, std::unique_ptr<BlockNode> handler_body,
                         std::unique_ptr<ExpressionNode> exc_type = nullptr,
                         std::unique_ptr<IdentifierNode> exc_name = nullptr)
            : ASTNode(line), type(std::move(exc_type)), name(std::move(exc_name)), body(std::move(handler_body)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ExceptionHandlerNode"; }
};

class TryStatementNode : public StatementNode {
public:
    std::unique_ptr<BlockNode> try_block;
    std::vector<std::unique_ptr<ExceptionHandlerNode>> handlers;
    std::unique_ptr<BlockNode> else_block;    // Optional
    std::unique_ptr<BlockNode> finally_block; // Optional

    TryStatementNode(int line, std::unique_ptr<BlockNode> try_b,
                     std::vector<std::unique_ptr<ExceptionHandlerNode>> ex_handlers,
                     std::unique_ptr<BlockNode> else_b = nullptr,
                     std::unique_ptr<BlockNode> finally_b = nullptr)
            : StatementNode(line), try_block(std::move(try_b)), handlers(std::move(ex_handlers)),
              else_block(std::move(else_b)), finally_block(std::move(finally_b)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "TryStatementNode"; }
};

// --- Context Managers ---
// Helper for 'with foo as bar:'
class WithItemNode : public ASTNode { // Not a statement itself
public:
    std::unique_ptr<ExpressionNode> context_expr;
    std::unique_ptr<ExpressionNode> as_target;   // Optional (for 'as target')

    WithItemNode(int line, std::unique_ptr<ExpressionNode> ctx_expr, std::unique_ptr<ExpressionNode> target_expr = nullptr)
            : ASTNode(line), context_expr(std::move(ctx_expr)), as_target(std::move(target_expr)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "WithItemNode"; }
};

class WithStatementNode : public StatementNode {
public:
    std::vector<std::unique_ptr<WithItemNode>> items;
    std::unique_ptr<BlockNode> body;

    WithStatementNode(int line, std::vector<std::unique_ptr<WithItemNode>> with_items, std::unique_ptr<BlockNode> with_body)
            : StatementNode(line), items(std::move(with_items)), body(std::move(with_body)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "WithStatementNode"; }
};


#endif // STATEMENTS_HPP