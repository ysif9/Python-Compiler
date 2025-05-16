#ifndef ASTNODE_HPP
#define ASTNODE_HPP

#include <string>
#include <vector>
#include <memory> // For std::unique_ptr

// Forward declaration for ASTVisitor, used by ASTNode
class ASTVisitor;

// Base class for all AST nodes
class ASTNode {
public:
    int line;

    ASTNode(int line) : line(line) {}
    virtual ~ASTNode() = default;

    // Method for the Visitor pattern
    virtual void accept(ASTVisitor* visitor) = 0;

    // Helper method for debugging or DOT generation
    virtual std::string getNodeName() const = 0;
};

// Forward declarations of all concrete AST node types for the ASTVisitor interface.

// Literals
class NumberLiteralNode;
class StringLiteralNode;
class BooleanLiteralNode;
class NoneLiteralNode;
class ComplexLiteralNode;
class BytesLiteralNode;

// Collection Literals
class ListLiteralNode;
class TupleLiteralNode;
class DictLiteralNode;
class SetLiteralNode;

// Expressions
class ExpressionNode;
class IdentifierNode;
class BinaryOpNode;
class UnaryOpNode;
class FunctionCallNode;
class AttributeAccessNode;
class SubscriptionNode;
class IfExpNode;                // Ternary if expression
class ComparisonNode;           // For chained comparisons
class SliceNode;                // For array/list slicing

// Statements
class StatementNode;
class ProgramNode;
class BlockNode;
class AssignmentStatementNode;
class ExpressionStatementNode;
class IfStatementNode;
class WhileStatementNode;
class ForStatementNode;
class FunctionDefinitionNode;
class ClassDefinitionNode;
class ReturnStatementNode;
class PassStatementNode;
class BreakStatementNode;
class ContinueStatementNode;
class ImportStatementNode;
class ImportFromStatementNode;
class GlobalStatementNode;
class NonlocalStatementNode;
class TryStatementNode;
class RaiseStatementNode;
class AugAssignNode;            // Augmented assignment


// Utility and Helper Nodes
class ParameterNode;
class ArgumentsNode;            // For function arguments structure
class KeywordArgNode;           // For name=value pairs (e.g., function calls)
class NamedImportNode;
class ImportNameNode;
class ExceptionHandlerNode;


// ASTVisitor interface: declares a visit method for each concrete AST node type.
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    // Visit methods for Literals
    virtual void visit(NumberLiteralNode* node) = 0;
    virtual void visit(StringLiteralNode* node) = 0;
    virtual void visit(BooleanLiteralNode* node) = 0;
    virtual void visit(NoneLiteralNode* node) = 0;
    virtual void visit(ComplexLiteralNode* node) = 0;
    virtual void visit(BytesLiteralNode* node) = 0;

    // Collection Literals (are expressions)
    virtual void visit(ListLiteralNode* node) = 0;
    virtual void visit(TupleLiteralNode* node) = 0;
    virtual void visit(DictLiteralNode* node) = 0;
    virtual void visit(SetLiteralNode* node) = 0;

    // Visit methods for other Expressions
    virtual void visit(IdentifierNode* node) = 0;
    virtual void visit(BinaryOpNode* node) = 0;
    virtual void visit(UnaryOpNode* node) = 0;
    virtual void visit(FunctionCallNode* node) = 0;
    virtual void visit(AttributeAccessNode* node) = 0;
    virtual void visit(SubscriptionNode* node) = 0;
    virtual void visit(IfExpNode* node) = 0;
    virtual void visit(ComparisonNode* node) = 0;
    virtual void visit(SliceNode* node) = 0;

    // Visit methods for Statements
    virtual void visit(ProgramNode* node) = 0;
    virtual void visit(BlockNode* node) = 0;
    virtual void visit(AssignmentStatementNode* node) = 0;
    virtual void visit(ExpressionStatementNode* node) = 0;
    virtual void visit(IfStatementNode* node) = 0;
    virtual void visit(WhileStatementNode* node) = 0;
    virtual void visit(ForStatementNode* node) = 0;
    virtual void visit(FunctionDefinitionNode* node) = 0;
    virtual void visit(ClassDefinitionNode* node) = 0;
    virtual void visit(ReturnStatementNode* node) = 0;
    virtual void visit(PassStatementNode* node) = 0;
    virtual void visit(BreakStatementNode* node) = 0;
    virtual void visit(ContinueStatementNode* node) = 0;
    virtual void visit(ImportStatementNode* node) = 0;
    virtual void visit(ImportFromStatementNode* node) = 0;
    virtual void visit(GlobalStatementNode* node) = 0;
    virtual void visit(NonlocalStatementNode* node) = 0;
    virtual void visit(TryStatementNode* node) = 0;
    virtual void visit(RaiseStatementNode* node) = 0;
    virtual void visit(AugAssignNode* node) = 0;


    // Visit methods for Utility and Helper Nodes
    virtual void visit(ParameterNode* node) = 0;
    virtual void visit(ArgumentsNode* node) = 0;
    virtual void visit(KeywordArgNode* node) = 0;
    virtual void visit(NamedImportNode* node) = 0;
    virtual void visit(ImportNameNode* node) = 0;
    virtual void visit(ExceptionHandlerNode* node) = 0;
};

#endif // ASTNODE_HPP