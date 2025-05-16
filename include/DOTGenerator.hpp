#pragma once

#include "ASTNode.hpp" // For ASTVisitor and ASTNode base
#include "UtilNodes.hpp" // <<<< ADDED for ParameterNode::Kind

#include <fstream>
#include <string>
#include <sstream>       // For node ID generation and string building
#include <unordered_map> // To store generated node IDs


class DOTGenerator : public ASTVisitor {
public:
    DOTGenerator();
    void generate(ASTNode* root, const std::string& filename);

    // Visit methods (declarations are the same as before)
    void visit(NumberLiteralNode* node) override;
    void visit(StringLiteralNode* node) override;
    void visit(BooleanLiteralNode* node) override;
    void visit(NoneLiteralNode* node) override;
    void visit(ComplexLiteralNode* node) override;
    void visit(BytesLiteralNode* node) override;
    void visit(ListLiteralNode* node) override;
    void visit(TupleLiteralNode* node) override;
    void visit(DictLiteralNode* node) override;
    void visit(SetLiteralNode* node) override;
    void visit(IdentifierNode* node) override;
    void visit(BinaryOpNode* node) override;
    void visit(UnaryOpNode* node) override;
    void visit(FunctionCallNode* node) override;
    void visit(AttributeAccessNode* node) override;
    void visit(SubscriptionNode* node) override;
    void visit(IfExpNode* node) override;
    void visit(ComparisonNode* node) override;
    void visit(SliceNode* node) override;
    void visit(ProgramNode* node) override;
    void visit(BlockNode* node) override;
    void visit(AssignmentStatementNode* node) override;
    void visit(ExpressionStatementNode* node) override;
    void visit(IfStatementNode* node) override;
    void visit(WhileStatementNode* node) override;
    void visit(ForStatementNode* node) override;
    void visit(FunctionDefinitionNode* node) override;
    void visit(ClassDefinitionNode* node) override;
    void visit(ReturnStatementNode* node) override;
    void visit(PassStatementNode* node) override;
    void visit(BreakStatementNode* node) override;
    void visit(ContinueStatementNode* node) override;
    void visit(ImportStatementNode* node) override;
    void visit(ImportFromStatementNode* node) override;
    void visit(GlobalStatementNode* node) override;
    void visit(NonlocalStatementNode* node) override;
    void visit(TryStatementNode* node) override;
    void visit(RaiseStatementNode* node) override;
    void visit(AugAssignNode* node) override;
    void visit(ParameterNode* node) override;
    void visit(ArgumentsNode* node) override;
    void visit(KeywordArgNode* node) override;
    void visit(NamedImportNode* node) override;
    void visit(ImportNameNode* node) override;
    void visit(ExceptionHandlerNode* node) override;

private:
    std::ofstream outFile;
    int nodeIdCounter;
    std::string currentNodeParentId;
    std::unordered_map<ASTNode*, std::string> visitedNodeIds;

    std::string getDotNodeId(ASTNode* node, const std::string& labelDetails = "");
    void linkToParent(const std::string& childId, const std::string& edgeLabel = "");
    std::string escapeDotString(const std::string& s);
    std::string paramKindToString(ParameterNode::Kind kind); // Declaration is now valid
};