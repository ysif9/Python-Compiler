#ifndef STATEMENTS_HPP
#define STATEMENTS_HPP

#include "ASTNode.hpp"
#include "Token.hpp"
#include <vector>
#include <string>
#include <memory>
#include <utility>         // For std::pair in IfStatementNode elif_blocks

// Forward declarations from other AST files
#include "Expressions.hpp" // For ExpressionNode, IdentifierNode, etc.
#include "UtilNodes.hpp"   // For ArgumentsNode, ParameterNode, TypeHintNode
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

// For simple assignment: targets = value
class AssignmentStatementNode : public StatementNode {
public:
    // Python allows multiple targets: a, b = value or a = b = value (chained)
    // Grammar: star_targets_eq_plus yield_expr_or_star_expressions -> (star_targets '=')* star_expressions
    // For a = b = c, it's one AST.Assign(targets=[a,b], value=c)
    // For a,b = c,d it's AST.Assign(targets=[Tuple(a,b)], value=Tuple(c,d)) or similar.
    // Let targets be a list, parser forms tuples for complex LHS if needed.
    std::vector<std::unique_ptr<ExpressionNode>> targets;
    std::unique_ptr<ExpressionNode> value;

    AssignmentStatementNode(int line, std::vector<std::unique_ptr<ExpressionNode>> tgts_expr, std::unique_ptr<ExpressionNode> val_expr)
            : StatementNode(line), targets(std::move(tgts_expr)), value(std::move(val_expr)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "AssignmentStatementNode"; }
};

// For annotated assignment: target: annotation [= value]
class AnnAssignNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> target;      // Must be Name, Attribute, or Subscript
    std::unique_ptr<ExpressionNode> annotation;
    std::unique_ptr<ExpressionNode> value;       // Optional
    bool simple; // True if target is a simple Name and not in parens (for var annotations in class/module scope without assignment)

    AnnAssignNode(int line, std::unique_ptr<ExpressionNode> tgt, std::unique_ptr<ExpressionNode> ann,
                  std::unique_ptr<ExpressionNode> val = nullptr, bool is_simple = false)
            : StatementNode(line), target(std::move(tgt)), annotation(std::move(ann)),
              value(std::move(val)), simple(is_simple) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "AnnAssignNode"; }
};

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
    // elif branches: list of (condition, block) pairs
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
    std::unique_ptr<ExpressionNode> target; // Loop variable(s) (can be tuple, Name, etc.)
    std::unique_ptr<ExpressionNode> iterable; // Expression to iterate over
    std::unique_ptr<BlockNode> body;
    std::unique_ptr<BlockNode> else_block; // Optional 'else' clause for for loop
    bool is_async;

    ForStatementNode(int line, std::unique_ptr<ExpressionNode> target_expr, std::unique_ptr<ExpressionNode> iter_expr,
                     std::unique_ptr<BlockNode> body_block, bool async_val = false,
                     std::unique_ptr<BlockNode> else_blk = nullptr)
            : StatementNode(line), target(std::move(target_expr)), iterable(std::move(iter_expr)),
              body(std::move(body_block)), else_block(std::move(else_blk)), is_async(async_val) {}

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
    std::unique_ptr<ExpressionNode> value; // Optional (nullptr for 'return' without value)

    ReturnStatementNode(int line, std::unique_ptr<ExpressionNode> val_expr = nullptr)
            : StatementNode(line), value(std::move(val_expr)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ReturnStatementNode"; }
};

class RaiseStatementNode : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> exception; // Optional (e.g., bare 'raise')
    std::unique_ptr<ExpressionNode> cause;     // Optional (for 'raise X from Y')

    RaiseStatementNode(int line, std::unique_ptr<ExpressionNode> exc_expr = nullptr, std::unique_ptr<ExpressionNode> cause_expr = nullptr)
            : StatementNode(line), exception(std::move(exc_expr)), cause(std::move(cause_expr)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "RaiseStatementNode"; }
};

// --- Definitions (Function, Class) ---
class FunctionDefinitionNode : public StatementNode {
public:
    std::unique_ptr<IdentifierNode> name;
    std::unique_ptr<ArgumentsNode> arguments_spec; // From UtilNodes.hpp, captures all param details
    std::unique_ptr<BlockNode> body;
    std::vector<std::unique_ptr<ExpressionNode>> decorators;
    std::unique_ptr<ExpressionNode> return_annotation; // Optional, type hint for return
    bool is_async;

    FunctionDefinitionNode(int line, std::unique_ptr<IdentifierNode> func_name,
                           std::unique_ptr<ArgumentsNode> args_spec,
                           std::unique_ptr<BlockNode> func_body,
                           std::vector<std::unique_ptr<ExpressionNode>> decs = {},
                           std::unique_ptr<ExpressionNode> ret_ann = nullptr,
                           bool async_val = false)
            : StatementNode(line), name(std::move(func_name)), arguments_spec(std::move(args_spec)),
              body(std::move(func_body)), decorators(std::move(decs)),
              return_annotation(std::move(ret_ann)), is_async(async_val) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "FunctionDefinitionNode"; }
};

class ClassDefinitionNode : public StatementNode {
public:
    std::unique_ptr<IdentifierNode> name;
    std::vector<std::unique_ptr<ExpressionNode>> base_classes; // List of base class expressions
    std::vector<std::unique_ptr<KeywordArgNode>> keywords; // For metaclass, etc. (e.g. metaclass=MyMeta)
    std::unique_ptr<BlockNode> body;
    std::vector<std::unique_ptr<ExpressionNode>> decorators;

    ClassDefinitionNode(int line, std::unique_ptr<IdentifierNode> class_name,
                        std::vector<std::unique_ptr<ExpressionNode>> bases,
                        std::vector<std::unique_ptr<KeywordArgNode>> class_keywords,
                        std::unique_ptr<BlockNode> class_body,
                        std::vector<std::unique_ptr<ExpressionNode>> decs = {})
            : StatementNode(line), name(std::move(class_name)), base_classes(std::move(bases)),
              keywords(std::move(class_keywords)), body(std::move(class_body)), decorators(std::move(decs)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ClassDefinitionNode"; }
};

// --- Import Statements ---
// Helper for 'import foo as bar' or 'foo.bar as baz'
class NamedImportNode : public ASTNode { // Not a statement itself; part of ImportStatement
public:
    // Dotted name like 'module.submodule' will be parsed into an ExpressionNode (e.g. AttrAccess or special DottedNameNode)
    // Python AST stores 'name' as string (e.g., "a.b.c")
    // Let's use string for simplicity, parser converts dotted Identifiers to string
    std::string module_path_str;
    std::unique_ptr<IdentifierNode> alias;      // Optional (nullptr if no 'as alias')

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

// Helper for 'from foo import bar as baz'
class ImportNameNode : public ASTNode { // Not a statement itself; part of ImportFromStatement
public:
    std::string name_str; // Name being imported (e.g., "bar")
    std::unique_ptr<IdentifierNode> alias; // Optional (nullptr if no 'as alias')

    ImportNameNode(int line, std::string imported_name_str, std::unique_ptr<IdentifierNode> alias_node = nullptr)
            : ASTNode(line), name_str(std::move(imported_name_str)), alias(std::move(alias_node)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ImportNameNode"; }
};

class ImportFromStatementNode : public StatementNode { // 'from .module import name as alias, *'
public:
    int level; // Number of leading dots for relative import (0 for absolute)
    // Module name; optional for 'from . import foo'. Use string for simplicity (parser handles dotted names).
    std::string module_str; // Can be empty for "from . import ..."
    std::vector<std::unique_ptr<ImportNameNode>> names; // List of names to import; empty if import_star is true
    bool import_star; // True if 'from module import *'

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
// Helper for 'except Foo as bar:' or 'except* Foo as bar:'
class ExceptionHandlerNode : public ASTNode { // Not a statement itself; part of TryStatement
public:
    std::unique_ptr<ExpressionNode> type; // Optional (nullptr for bare 'except:')
    std::unique_ptr<IdentifierNode> name; // Optional (for 'as e')
    std::unique_ptr<BlockNode> body;
    bool is_star_handler; // True for 'except*' from Python 3.11+ (grammar has except_star_block)

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
    std::unique_ptr<BlockNode> else_block;    // Optional
    std::unique_ptr<BlockNode> finally_block; // Optional
    // bool is_try_star; // Python 3.11 TryStar - handled by is_star_handler on ExceptionHandlerNode for now

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
class WithItemNode : public ASTNode { // Not a statement itself; part of WithStatement
public:
    std::unique_ptr<ExpressionNode> context_expr;
    std::unique_ptr<ExpressionNode> as_target;   // Optional (for 'as target'), can be Name, Tuple, etc.

    WithItemNode(int line, std::unique_ptr<ExpressionNode> ctx_expr, std::unique_ptr<ExpressionNode> target_expr = nullptr)
            : ASTNode(line), context_expr(std::move(ctx_expr)), as_target(std::move(target_expr)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "WithItemNode"; }
};

class WithStatementNode : public StatementNode {
public:
    std::vector<std::unique_ptr<WithItemNode>> items;
    std::unique_ptr<BlockNode> body;
    bool is_async;

    WithStatementNode(int line, std::vector<std::unique_ptr<WithItemNode>> with_items,
                      std::unique_ptr<BlockNode> with_body, bool async_val = false)
            : StatementNode(line), items(std::move(with_items)), body(std::move(with_body)), is_async(async_val) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "WithStatementNode"; }
};


#endif // STATEMENTS_HPP