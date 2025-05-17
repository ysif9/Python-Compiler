#include "DOTGenerator.hpp"
#include "Expressions.hpp"
#include "Literals.hpp"
#include "Statements.hpp"
#include "Helpers.hpp" // Assuming UtilNodes.hpp might be included via here or directly if needed for other types

#include <iostream>
#include <sstream>

DOTGenerator::DOTGenerator() : nodeIdCounter(0), currentNodeParentId(""), currentEdgeLabel("") {}

void DOTGenerator::generate(ASTNode* root, const std::string& filename) {
    outFile.open(filename);
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for DOT generation." << std::endl;
        return;
    }

    nodeIdCounter = 0;
    visitedNodeIds.clear();
    currentNodeParentId = "";
    currentEdgeLabel = ""; // Reset for each generation

    outFile << "digraph AST {" << std::endl;
    outFile << "  node [shape=box, style=filled, fillcolor=lightblue];" << std::endl;

    if (root) {
        root->accept(this); // currentEdgeLabel is "" for the root
    }

    outFile << "}" << std::endl;
    outFile.close();
}

std::string DOTGenerator::escapeDotString(const std::string& s) {
    std::string escaped;
    escaped.reserve(s.length());
    for (char c : s) {
        switch (c) {
            case '"':  escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\n': escaped += "\\n";  break;
            case '\r': /* ignore */       break;
            case '\t': escaped += "\\t";  break;
            case '<':  escaped += "\\<";  break;
            case '>':  escaped += "\\>";  break;
            case '{':  escaped += "\\{";  break;
            case '}':  escaped += "\\}";  break;
            default:   escaped += c;      break;
        }
    }
    return escaped;
}

std::string DOTGenerator::getDotNodeId(ASTNode* node, const std::string& labelDetails) {
    if (node && visitedNodeIds.count(node)) {
        return visitedNodeIds[node];
    }

    std::ostringstream ss_node_id;
    ss_node_id << "node" << nodeIdCounter++;
    std::string nodeId = ss_node_id.str();

    if (node) {
        visitedNodeIds[node] = nodeId;
    }

    std::ostringstream label_ss;
    if (node) {
        label_ss << escapeDotString(node->getNodeName());
        if (!labelDetails.empty()) {
            label_ss << "\\n(" << escapeDotString(labelDetails) << ")";
        }
        label_ss << "\\n(line " << node->line << ")";
    } else {
        label_ss << (!labelDetails.empty() ? escapeDotString(labelDetails) : "ConceptualNode");
    }

    outFile << "  \"" << nodeId << "\" [label=\"" << label_ss.str() << "\"];" << std::endl;
    return nodeId;
}

void DOTGenerator::linkToParent(const std::string& childId) {
    if (!currentNodeParentId.empty() && !childId.empty() && currentNodeParentId != childId) { // Prevent self-loops
        outFile << "  \"" << currentNodeParentId << "\" -> \"" << childId << "\"";
        if (!currentEdgeLabel.empty()) {
            outFile << " [label=\"" << escapeDotString(currentEdgeLabel) << "\"]";
        }
        outFile << ";" << std::endl;
    }
}

// --- Visit Methods Implementation (Refactored) ---

void DOTGenerator::visit(ProgramNode* node) {
    std::string selfId = getDotNodeId(node);
    // ProgramNode is root, no linkToParent(selfId) for itself from an outer parent.
    // currentNodeParentId is "" and currentEdgeLabel is "" at this point.

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;

    currentNodeParentId = selfId;
    for (size_t i = 0; i < node->statements.size(); ++i) {
        if (node->statements[i]) {
            currentEdgeLabel = "stmt[" + std::to_string(i) + "]"; // Or simply "" if no specific label needed
            node->statements[i]->accept(this);
        }
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

// --- Leaf Nodes (Literals, Identifier) ---
// These nodes typically don't have further children to recurse into *in the same way*,
// so they don't need to manage oldParentId/oldEdgeLabel or set currentNodeParentId = selfId for sub-visits.

void DOTGenerator::visit(NumberLiteralNode* node) {
    std::ostringstream details_ss;
    details_ss << "type: " << (node->type == NumberLiteralNode::Type::INTEGER ? "int" : "float");
    details_ss << ", value: " << node->value_str;
    std::string selfId = getDotNodeId(node, details_ss.str());
    linkToParent(selfId);
}

void DOTGenerator::visit(StringLiteralNode* node) {
    std::ostringstream details_ss;
    details_ss << "value: " << node->value;
    std::string selfId = getDotNodeId(node, details_ss.str());
    linkToParent(selfId);
}

void DOTGenerator::visit(BooleanLiteralNode* node) {
    std::string selfId = getDotNodeId(node, node->value ? "True" : "False");
    linkToParent(selfId);
}

void DOTGenerator::visit(NoneLiteralNode* node) {
    std::string selfId = getDotNodeId(node, "None");
    linkToParent(selfId);
}

void DOTGenerator::visit(ComplexLiteralNode* node) {
    std::ostringstream details_ss;
    details_ss << "real: " << node->real_part_str << ", imag: " << node->imag_part_str << "j";
    std::string selfId = getDotNodeId(node, details_ss.str());
    linkToParent(selfId);
}

void DOTGenerator::visit(BytesLiteralNode* node) {
    std::ostringstream details_ss;
    details_ss << "value: " << node->value;
    std::string selfId = getDotNodeId(node, details_ss.str());
    linkToParent(selfId);
}

void DOTGenerator::visit(IdentifierNode* node) {
    std::string selfId = getDotNodeId(node, "name: " + node->name);
    linkToParent(selfId);
}

// --- Nodes with Children ---

void DOTGenerator::visit(ListLiteralNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    for (size_t i = 0; i < node->elements.size(); ++i) {
        if (node->elements[i]) {
            currentEdgeLabel = "elem[" + std::to_string(i) + "]";
            node->elements[i]->accept(this);
        }
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

void DOTGenerator::visit(TupleLiteralNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    for (size_t i = 0; i < node->elements.size(); ++i) {
        if (node->elements[i]) {
            currentEdgeLabel = "elem[" + std::to_string(i) + "]";
            node->elements[i]->accept(this);
        }
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

void DOTGenerator::visit(DictLiteralNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId; // Parent of DictLiteralNode
    std::string oldEdgeLabel = currentEdgeLabel;   // Edge label for DictLiteralNode

    currentNodeParentId = selfId; // DictLiteralNode is now parent for "pair" conceptual nodes

    for (size_t i = 0; i < node->keys.size(); ++i) {
        std::string pairEdgeLabel = "pair[" + std::to_string(i) + "]";

        // Create conceptual pair node
        currentEdgeLabel = pairEdgeLabel; // Label for DictNode -> PairNode edge
        std::string pairNodeId = getDotNodeId(nullptr, "key-value pair"); // Defines conceptual node
        linkToParent(pairNodeId); // Links DictNode to PairNode

        // Key and value are children of pairNodeId
        std::string parentOfKeyVal = currentNodeParentId; // This is still DictNode (selfId)
        std::string edgeLabelOfPair = currentEdgeLabel;   // This is pairEdgeLabel

        currentNodeParentId = pairNodeId; // PairNode is parent for key/value

        if (node->keys[i]) {
            currentEdgeLabel = "key";
            node->keys[i]->accept(this);
        }
        if (i < node->values.size() && node->values[i]) {
            currentEdgeLabel = "value";
            node->values[i]->accept(this);
        }

        currentNodeParentId = parentOfKeyVal; // Restore parent to DictNode
        currentEdgeLabel = edgeLabelOfPair;   // Restore edge label (though it'll be overwritten or restored fully at end)
    }
    currentNodeParentId = oldParentId; // Restore original parent of DictLiteralNode
    currentEdgeLabel = oldEdgeLabel;   // Restore original edge label for DictLiteralNode
}

void DOTGenerator::visit(SetLiteralNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    for (size_t i = 0; i < node->elements.size(); ++i) {
        if (node->elements[i]) {
            currentEdgeLabel = "elem[" + std::to_string(i) + "]";
            node->elements[i]->accept(this);
        }
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

void DOTGenerator::visit(BinaryOpNode* node) {
    std::string selfId = getDotNodeId(node, "op: " + node->op.lexeme);
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    if (node->left) {
        currentEdgeLabel = "left";
        node->left->accept(this);
    }
    if (node->right) {
        currentEdgeLabel = "right";
        node->right->accept(this);
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

void DOTGenerator::visit(UnaryOpNode* node) {
    std::string selfId = getDotNodeId(node, "op: " + node->op.lexeme);
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    if (node->operand) {
        currentEdgeLabel = "operand";
        node->operand->accept(this);
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

void DOTGenerator::visit(FunctionCallNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    if (node->callee) {
        currentEdgeLabel = "callee";
        node->callee->accept(this);
    }
    for (size_t i = 0; i < node->args.size(); ++i) {
        if (node->args[i]) {
            currentEdgeLabel = "arg[" + std::to_string(i) + "]";
            node->args[i]->accept(this);
        }
    }
    for (size_t i = 0; i < node->keywords.size(); ++i) {
        if (node->keywords[i]) {
            currentEdgeLabel = "kwarg[" + std::to_string(i) + "]"; // kwarg itself is a KeywordArgNode
            node->keywords[i]->accept(this);
        }
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

void DOTGenerator::visit(AttributeAccessNode* node) {
    std::string selfId = getDotNodeId(node); // Attribute name is part of IdentifierNode child
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    if (node->object) {
        currentEdgeLabel = "object";
        node->object->accept(this);
    }
    if (node->attribute_name) { // This is an IdentifierNode
        currentEdgeLabel = "attribute";
        node->attribute_name->accept(this);
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

void DOTGenerator::visit(SubscriptionNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    if (node->object) {
        currentEdgeLabel = "object";
        node->object->accept(this);
    }
    if (node->slice_or_index) {
        currentEdgeLabel = "slice/index";
        node->slice_or_index->accept(this);
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

void DOTGenerator::visit(IfExpNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    if (node->body) { // true value
        currentEdgeLabel = "body (true_val)";
        node->body->accept(this);
    }
    if (node->condition) {
        currentEdgeLabel = "condition";
        node->condition->accept(this);
    }
    if (node->orelse) { // false value
        currentEdgeLabel = "orelse (false_val)";
        node->orelse->accept(this);
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

void DOTGenerator::visit(ComparisonNode* node) {
    std::ostringstream ops_ss;
    ops_ss << "ops: ";
    for (size_t i = 0; i < node->ops.size(); ++i) {
        ops_ss << node->ops[i].lexeme << (i < node->ops.size() - 1 ? ", " : "");
    }
    std::string selfId = getDotNodeId(node, ops_ss.str());
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    if (node->left) {
        currentEdgeLabel = "left";
        node->left->accept(this);
    }
    for (size_t i = 0; i < node->comparators.size(); ++i) {
        if (node->comparators[i]) {
            currentEdgeLabel = "comp[" + std::to_string(i) + "]";
            node->comparators[i]->accept(this);
        }
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

void DOTGenerator::visit(SliceNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    if (node->lower) {
        currentEdgeLabel = "lower";
        node->lower->accept(this);
    }
    if (node->upper) {
        currentEdgeLabel = "upper";
        node->upper->accept(this);
    }
    if (node->step) {
        currentEdgeLabel = "step";
        node->step->accept(this);
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

void DOTGenerator::visit(BlockNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    for (size_t i = 0; i < node->statements.size(); ++i) {
        if (node->statements[i]) {
            currentEdgeLabel = "stmt[" + std::to_string(i) + "]";
            node->statements[i]->accept(this);
        }
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

void DOTGenerator::visit(AssignmentStatementNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    for (size_t i = 0; i < node->targets.size(); ++i) {
        if (node->targets[i]) {
            currentEdgeLabel = "target[" + std::to_string(i) + "]";
            node->targets[i]->accept(this);
        }
    }
    if (node->value) {
        currentEdgeLabel = "value";
        node->value->accept(this);
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

void DOTGenerator::visit(ExpressionStatementNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    if (node->expression) {
        currentEdgeLabel = "expr";
        node->expression->accept(this);
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

void DOTGenerator::visit(IfStatementNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    if (node->condition) {
        currentEdgeLabel = "condition";
        node->condition->accept(this);
    }
    if (node->then_block) {
        currentEdgeLabel = "then_block";
        node->then_block->accept(this);
    }

    for(size_t i=0; i < node->elif_blocks.size(); ++i) {
        const auto& elif_pair = node->elif_blocks[i];

        currentEdgeLabel = "elif["+std::to_string(i)+"]";
        std::string elifNodeId = getDotNodeId(nullptr, "elif_clause"); // Conceptual node for elif
        linkToParent(elifNodeId); // Links IfStatementNode to elif_clause conceptual node

        std::string parentOfElifParts = currentNodeParentId; // IfStatementNode
        std::string edgeLabelOfElifClause = currentEdgeLabel; // "elif[i]"

        currentNodeParentId = elifNodeId; // elif_clause is parent for its condition and block

        if(elif_pair.first) { // condition
            currentEdgeLabel = "condition";
            elif_pair.first->accept(this);
        }
        if(elif_pair.second) { // block
            currentEdgeLabel = "block";
            elif_pair.second->accept(this);
        }
        currentNodeParentId = parentOfElifParts; // Restore to IfStatementNode
        currentEdgeLabel = edgeLabelOfElifClause; // Restore "elif[i]"
    }

    if (node->else_block) {
        currentEdgeLabel = "else_block";
        node->else_block->accept(this);
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

void DOTGenerator::visit(WhileStatementNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    if (node->condition) {
        currentEdgeLabel = "condition";
        node->condition->accept(this);
    }
    if (node->body) {
        currentEdgeLabel = "body";
        node->body->accept(this);
    }
    if (node->else_block) {
        currentEdgeLabel = "else_block";
        node->else_block->accept(this);
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

void DOTGenerator::visit(ForStatementNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    if (node->target) {
        currentEdgeLabel = "target";
        node->target->accept(this);
    }
    if (node->iterable) {
        currentEdgeLabel = "iterable";
        node->iterable->accept(this);
    }
    if (node->body) {
        currentEdgeLabel = "body";
        node->body->accept(this);
    }
    if (node->else_block) {
        currentEdgeLabel = "else_block";
        node->else_block->accept(this);
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

void DOTGenerator::visit(FunctionDefinitionNode* node) {
    std::string selfId = getDotNodeId(node, "name: " + (node->name ? node->name->name : "<?>"));
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    if (node->name) { // This is an IdentifierNode
        currentEdgeLabel = "name_node";
        node->name->accept(this);
    }
    if (node->arguments_spec) {
        currentEdgeLabel = "arguments";
        node->arguments_spec->accept(this);
    }
    if (node->body) { // This is a BlockNode
        currentEdgeLabel = "body";
        node->body->accept(this);
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

void DOTGenerator::visit(ClassDefinitionNode* node) {
    std::string selfId = getDotNodeId(node, "name: " + (node->name ? node->name->name : "<?>"));
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    if (node->name) {
        currentEdgeLabel = "name_node";
        node->name->accept(this);
    }
    for (size_t i = 0; i < node->base_classes.size(); ++i) {
        if (node->base_classes[i]) {
            currentEdgeLabel = "base[" + std::to_string(i) + "]";
            node->base_classes[i]->accept(this);
        }
    }
    for (size_t i = 0; i < node->keywords.size(); ++i) { // These are KeywordArgNodes
        if (node->keywords[i]) {
            currentEdgeLabel = "keyword[" + std::to_string(i) + "]";
            node->keywords[i]->accept(this);
        }
    }
    if (node->body) {
        currentEdgeLabel = "body";
        node->body->accept(this);
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

void DOTGenerator::visit(ReturnStatementNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    if (node->value) {
        currentEdgeLabel = "value";
        node->value->accept(this);
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

// Simple statement nodes (leaf-like in terms of DOT structure from their perspective)
void DOTGenerator::visit(PassStatementNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);
}

void DOTGenerator::visit(BreakStatementNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);
}

void DOTGenerator::visit(ContinueStatementNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);
}

void DOTGenerator::visit(ImportStatementNode* node) {
    std::string selfId = getDotNodeId(node); // Names themselves are children
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    for (size_t i=0; i < node->names.size(); ++i) { // Names are NamedImportNode
        if (node->names[i]) {
            currentEdgeLabel = "name["+std::to_string(i)+"]";
            node->names[i]->accept(this);
        }
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

void DOTGenerator::visit(ImportFromStatementNode* node) {
    std::ostringstream details_ss;
    details_ss << "level: " << node->level;
    if (!node->module_str.empty()) details_ss << ", module: " << node->module_str;
    if (node->import_star) details_ss << ", imports *";

    std::string selfId = getDotNodeId(node, details_ss.str());
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    for (size_t i=0; i < node->names.size(); ++i) { // Names are ImportNameNode
        if (node->names[i]) {
            currentEdgeLabel = "name["+std::to_string(i)+"]";
            node->names[i]->accept(this);
        }
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

void DOTGenerator::visit(GlobalStatementNode* node) {
    std::string selfId = getDotNodeId(node); // Names are children (IdentifierNodes)
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    for (size_t i=0; i < node->names.size(); ++i) {
        if (node->names[i]) {
            currentEdgeLabel = "name["+std::to_string(i)+"]";
            node->names[i]->accept(this);
        }
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

void DOTGenerator::visit(NonlocalStatementNode* node) {
    std::string selfId = getDotNodeId(node); // Names are children (IdentifierNodes)
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    for (size_t i=0; i < node->names.size(); ++i) {
        if (node->names[i]) {
            currentEdgeLabel = "name["+std::to_string(i)+"]";
            node->names[i]->accept(this);
        }
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

void DOTGenerator::visit(TryStatementNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    if (node->try_block) {
        currentEdgeLabel = "try_block";
        node->try_block->accept(this);
    }
    for (size_t i=0; i < node->handlers.size(); ++i) { // Handlers are ExceptionHandlerNode
        if (node->handlers[i]) {
            currentEdgeLabel = "handler["+std::to_string(i)+"]";
            node->handlers[i]->accept(this);
        }
    }
    if (node->else_block) {
        currentEdgeLabel = "else_block";
        node->else_block->accept(this);
    }
    if (node->finally_block) {
        currentEdgeLabel = "finally_block";
        node->finally_block->accept(this);
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

void DOTGenerator::visit(RaiseStatementNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    if (node->exception) {
        currentEdgeLabel = "exception";
        node->exception->accept(this);
    }
    if (node->cause) {
        currentEdgeLabel = "cause";
        node->cause->accept(this);
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

void DOTGenerator::visit(AugAssignNode* node) {
    std::string selfId = getDotNodeId(node, "op: " + node->op.lexeme);
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    if (node->target) {
        currentEdgeLabel = "target";
        node->target->accept(this);
    }
    if (node->value) {
        currentEdgeLabel = "value";
        node->value->accept(this);
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

std::string DOTGenerator::paramKindToString(ParameterNode::Kind kind) {
    switch (kind) {
        case ParameterNode::Kind::POSITIONAL_OR_KEYWORD: return "PositionalOrKeyword";
        case ParameterNode::Kind::VAR_POSITIONAL:        return "VarPositional (*)";
        case ParameterNode::Kind::VAR_KEYWORD:           return "VarKeyword (**)";
        default:                                         return "UnknownKind";
    }
}

void DOTGenerator::visit(ParameterNode* node) {
    std::ostringstream details_ss;
    details_ss << "name: " << node->arg_name << ", kind: " << paramKindToString(node->kind);
    std::string selfId = getDotNodeId(node, details_ss.str());
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    if (node->default_value) {
        currentEdgeLabel = "default_value";
        node->default_value->accept(this);
    }
    // Annotation not shown in this example, but if present, would be another child.
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

void DOTGenerator::visit(ArgumentsNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    for (size_t i = 0; i < node->args.size(); ++i) { // These are ParameterNode
        if (node->args[i]) {
            currentEdgeLabel = "param["+std::to_string(i)+"]";
            node->args[i]->accept(this);
        }
    }
    if (node->vararg) { // ParameterNode
        currentEdgeLabel = "vararg (*args)";
        node->vararg->accept(this);
    }
    // kwonlyargs not shown, but would be similar to args
    if (node->kwarg) { // ParameterNode
        currentEdgeLabel = "kwarg (**kwargs)";
        node->kwarg->accept(this);
    }
    // defaults and kw_defaults link ParameterNodes to their default values,
    // which is handled within ParameterNode::visit if it has a default_value.
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

void DOTGenerator::visit(KeywordArgNode* node) {
    // KeywordArgNode's name is an IdentifierNode, value is an ExpressionNode
    std::string selfId = getDotNodeId(node, "name: " + (node->arg_name ? node->arg_name->name : "<?>"));
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    if (node->arg_name) { // This is an IdentifierNode
        currentEdgeLabel = "arg_name_node";
        node->arg_name->accept(this);
    }
    if (node->value) {
        currentEdgeLabel = "value";
        node->value->accept(this);
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}

void DOTGenerator::visit(NamedImportNode* node) {
    // Represents 'module' or 'module.submodule' [as alias]
    std::string details = "path: " + node->module_path_str;
    if (node->alias) details += ", as: " + node->alias->name;
    std::string selfId = getDotNodeId(node, details);
    linkToParent(selfId);

    // The alias (IdentifierNode) is conceptually part of this node's definition,
    // usually not drawn as a separate child unless it had complex structure.
    // If you wanted to show the alias IdentifierNode as a distinct child:
    /*
    if (node->alias) {
        std::string oldParentId = currentNodeParentId;
        std::string oldEdgeLabel = currentEdgeLabel;
        currentNodeParentId = selfId;
        currentEdgeLabel = "alias_node";
        node->alias->accept(this);
        currentNodeParentId = oldParentId;
        currentEdgeLabel = oldEdgeLabel;
    }
    */
}

void DOTGenerator::visit(ImportNameNode* node) {
    // Represents 'name' [as alias] in 'from module import name1 as alias1, name2'
    std::string details = "name: " + node->name_str;
    if (node->alias) details += ", as: " + node->alias->name;
    std::string selfId = getDotNodeId(node, details);
    linkToParent(selfId);

    // Similar to NamedImportNode, alias is part of the definition.
    // If alias were to be a distinct child node:
    /*
    if (node->alias) {
        std::string oldParentId = currentNodeParentId;
        std::string oldEdgeLabel = currentEdgeLabel;
        currentNodeParentId = selfId;
        currentEdgeLabel = "alias_node";
        node->alias->accept(this);
        currentNodeParentId = oldParentId;
        currentEdgeLabel = oldEdgeLabel;
    }
    */
}

void DOTGenerator::visit(ExceptionHandlerNode* node) {
    std::ostringstream details_ss;
    bool has_details = false;
    if (node->type) { details_ss << "type: (see child)"; has_details = true; }
    if (node->name) { details_ss << (has_details ? ", " : "") << "as: " << node->name->name; }

    std::string selfId = getDotNodeId(node, details_ss.str());
    linkToParent(selfId);

    std::string oldParentId = currentNodeParentId;
    std::string oldEdgeLabel = currentEdgeLabel;
    currentNodeParentId = selfId;

    if (node->type) { // ExpressionNode for exception type
        currentEdgeLabel = "type_expr";
        node->type->accept(this);
    }
    if (node->name) { // IdentifierNode for 'as name'
        currentEdgeLabel = "name_node (as)";
        node->name->accept(this);
    }
    if (node->body) { // BlockNode for the handler body
        currentEdgeLabel = "body";
        node->body->accept(this);
    }
    currentNodeParentId = oldParentId;
    currentEdgeLabel = oldEdgeLabel;
}