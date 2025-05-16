#include "DOTGenerator.hpp"
#include "Expressions.hpp"
#include "Literals.hpp"
#include "Statements.hpp"
#include "Helpers.hpp"

#include <iostream> // For potential error reporting
#include <sstream>  // For std::ostringstream

DOTGenerator::DOTGenerator() : nodeIdCounter(0), currentNodeParentId("") {}

void DOTGenerator::generate(ASTNode* root, const std::string& filename) {
    outFile.open(filename);
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for DOT generation." << std::endl;
        return;
    }

    nodeIdCounter = 0; // Reset for each generation
    visitedNodeIds.clear(); // Clear cache for each generation
    currentNodeParentId = ""; // Reset parent ID

    outFile << "digraph AST {" << std::endl;
    outFile << "  node [shape=box, style=filled, fillcolor=lightblue];" << std::endl;

    if (root) {
        root->accept(this);
    }

    outFile << "}" << std::endl;
    outFile.close();
}

std::string DOTGenerator::escapeDotString(const std::string& s) {
    std::string escaped;
    escaped.reserve(s.length()); // Pre-allocate memory
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

// Robust getDotNodeId for nullptr safety
std::string DOTGenerator::getDotNodeId(ASTNode* node, const std::string& labelDetails) {
    if (node && visitedNodeIds.count(node)) {
        return visitedNodeIds[node];
    }

    std::ostringstream ss_node_id;
    ss_node_id << "node" << nodeIdCounter++;
    std::string nodeId = ss_node_id.str();

    if (node) { // Only add to map if node is not null
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
        // For nullptr (conceptual) nodes
        label_ss << (!labelDetails.empty() ? escapeDotString(labelDetails) : "ConceptualNode");
        // Optionally, add a comment or fixed line number for conceptual nodes
        // label_ss << "\\n(conceptual)";
    }

    outFile << "  \"" << nodeId << "\" [label=\"" << label_ss.str() << "\"];" << std::endl;
    return nodeId;
}


void DOTGenerator::linkToParent(const std::string& childId, const std::string& edgeLabel) {
    if (!currentNodeParentId.empty() && !childId.empty()) {
        outFile << "  \"" << currentNodeParentId << "\" -> \"" << childId << "\"";
        if (!edgeLabel.empty()) {
            outFile << " [label=\"" << escapeDotString(edgeLabel) << "\"]";
        }
        outFile << ";" << std::endl;
    }
}

// --- Visit Methods Implementation ---

void DOTGenerator::visit(ProgramNode* node) {
    std::string selfId = getDotNodeId(node);
    // ProgramNode is root, no linkToParent for itself.
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    for (size_t i = 0; i < node->statements.size(); ++i) {
        if (node->statements[i]) {
            node->statements[i]->accept(this);
        }
    }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(NumberLiteralNode* node) {
    std::ostringstream details_ss;
    details_ss << "type: " << (node->type == NumberLiteralNode::Type::INTEGER ? "int" : "float");
    details_ss << ", value: " << node->value_str;
    std::string selfId = getDotNodeId(node, details_ss.str());
    linkToParent(selfId);
}

void DOTGenerator::visit(StringLiteralNode* node) {
    std::ostringstream details_ss;
    details_ss << "value: " << node->value; // Assuming node->value is already string-safe for ostream
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

void DOTGenerator::visit(ListLiteralNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    for (size_t i = 0; i < node->elements.size(); ++i) {
        if (node->elements[i]) {
            std::string elemId = getDotNodeId(node->elements[i].get());
            linkToParent(elemId, "elem[" + std::to_string(i) + "]");

            std::string tempParent = currentNodeParentId;
            currentNodeParentId = elemId;
            node->elements[i]->accept(this);
            currentNodeParentId = tempParent;
        }
    }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(TupleLiteralNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    for (size_t i = 0; i < node->elements.size(); ++i) {
        if (node->elements[i]) {
            std::string elemId = getDotNodeId(node->elements[i].get());
            linkToParent(elemId, "elem[" + std::to_string(i) + "]");

            std::string tempParent = currentNodeParentId;
            currentNodeParentId = elemId;
            node->elements[i]->accept(this);
            currentNodeParentId = tempParent;
        }
    }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(DictLiteralNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    for (size_t i = 0; i < node->keys.size(); ++i) {
        std::ostringstream pair_label_ss;
        pair_label_ss << "pair[" << i << "]";
        std::string pairNodeId = getDotNodeId(nullptr, "key-value pair");
        linkToParent(pairNodeId, pair_label_ss.str());

        std::string tempParentForPair = currentNodeParentId;
        currentNodeParentId = pairNodeId;

        if (node->keys[i]) {
            std::string keyId = getDotNodeId(node->keys[i].get());
            linkToParent(keyId, "key");
            std::string tempGrandParent = currentNodeParentId;
            currentNodeParentId = keyId;
            node->keys[i]->accept(this);
            currentNodeParentId = tempGrandParent;
        }
        if (i < node->values.size() && node->values[i]) {
            std::string valId = getDotNodeId(node->values[i].get());
            linkToParent(valId, "value");
            std::string tempGrandParent = currentNodeParentId;
            currentNodeParentId = valId;
            node->values[i]->accept(this);
            currentNodeParentId = tempGrandParent;
        }
        currentNodeParentId = tempParentForPair;
    }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(SetLiteralNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    for (size_t i = 0; i < node->elements.size(); ++i) {
        if (node->elements[i]) {
            std::string elemId = getDotNodeId(node->elements[i].get());
            linkToParent(elemId, "elem[" + std::to_string(i) + "]");
            std::string tempParent = currentNodeParentId;
            currentNodeParentId = elemId;
            node->elements[i]->accept(this);
            currentNodeParentId = tempParent;
        }
    }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(IdentifierNode* node) {
    std::string selfId = getDotNodeId(node, "name: " + node->name);
    linkToParent(selfId);
}

void DOTGenerator::visit(BinaryOpNode* node) {
    std::string selfId = getDotNodeId(node, "op: " + node->op.lexeme);
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    if (node->left) {
        std::string leftId = getDotNodeId(node->left.get());
        linkToParent(leftId, "left");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = leftId;
        node->left->accept(this);
        currentNodeParentId = tempParent;
    }
    if (node->right) {
        std::string rightId = getDotNodeId(node->right.get());
        linkToParent(rightId, "right");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = rightId;
        node->right->accept(this);
        currentNodeParentId = tempParent;
    }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(UnaryOpNode* node) {
    std::string selfId = getDotNodeId(node, "op: " + node->op.lexeme);
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    if (node->operand) {
        std::string operandId = getDotNodeId(node->operand.get());
        linkToParent(operandId, "operand");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = operandId;
        node->operand->accept(this);
        currentNodeParentId = tempParent;
    }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(FunctionCallNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;

    if (node->callee) {
        std::string calleeId = getDotNodeId(node->callee.get());
        linkToParent(calleeId, "callee");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = calleeId;
        node->callee->accept(this);
        currentNodeParentId = tempParent;
    }
    for (size_t i = 0; i < node->args.size(); ++i) {
        if (node->args[i]) {
            std::string argId = getDotNodeId(node->args[i].get());
            linkToParent(argId, "arg[" + std::to_string(i) + "]");
            std::string tempParent = currentNodeParentId;
            currentNodeParentId = argId;
            node->args[i]->accept(this);
            currentNodeParentId = tempParent;
        }
    }
    for (size_t i = 0; i < node->keywords.size(); ++i) {
        if (node->keywords[i]) {
            std::string kwId = getDotNodeId(node->keywords[i].get());
            linkToParent(kwId, "kwarg[" + std::to_string(i) + "]");
            std::string tempParent = currentNodeParentId;
            currentNodeParentId = kwId;
            node->keywords[i]->accept(this);
            currentNodeParentId = tempParent;
        }
    }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(AttributeAccessNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    if (node->object) {
        std::string objId = getDotNodeId(node->object.get());
        linkToParent(objId, "object");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = objId;
        node->object->accept(this);
        currentNodeParentId = tempParent;
    }
    if (node->attribute_name) {
        std::string attrId = getDotNodeId(node->attribute_name.get());
        linkToParent(attrId, "attribute");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = attrId;
        node->attribute_name->accept(this);
        currentNodeParentId = tempParent;
    }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(SubscriptionNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    if (node->object) {
        std::string objId = getDotNodeId(node->object.get());
        linkToParent(objId, "object");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = objId;
        node->object->accept(this);
        currentNodeParentId = tempParent;
    }
    if (node->slice_or_index) {
        std::string sliceId = getDotNodeId(node->slice_or_index.get());
        linkToParent(sliceId, "slice/index");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = sliceId;
        node->slice_or_index->accept(this);
        currentNodeParentId = tempParent;
    }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(IfExpNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    if (node->body) {
        std::string bodyId = getDotNodeId(node->body.get());
        linkToParent(bodyId, "body (true_val)");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = bodyId;
        node->body->accept(this);
        currentNodeParentId = tempParent;
    }
    if (node->condition) {
        std::string condId = getDotNodeId(node->condition.get());
        linkToParent(condId, "condition");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = condId;
        node->condition->accept(this);
        currentNodeParentId = tempParent;
    }
    if (node->orelse) {
        std::string orelseId = getDotNodeId(node->orelse.get());
        linkToParent(orelseId, "orelse (false_val)");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = orelseId;
        node->orelse->accept(this);
        currentNodeParentId = tempParent;
    }
    currentNodeParentId = oldParentId;
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
    currentNodeParentId = selfId;

    if (node->left) {
        std::string leftId = getDotNodeId(node->left.get());
        linkToParent(leftId, "left");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = leftId;
        node->left->accept(this);
        currentNodeParentId = tempParent;
    }
    for (size_t i = 0; i < node->comparators.size(); ++i) {
        if (node->comparators[i]) {
            std::string compId = getDotNodeId(node->comparators[i].get());
            linkToParent(compId, "comp[" + std::to_string(i) + "]");
            std::string tempParent = currentNodeParentId;
            currentNodeParentId = compId;
            node->comparators[i]->accept(this);
            currentNodeParentId = tempParent;
        }
    }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(SliceNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    if (node->lower) {
        std::string lowerId = getDotNodeId(node->lower.get());
        linkToParent(lowerId, "lower");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = lowerId;
        node->lower->accept(this);
        currentNodeParentId = tempParent;
    }
    if (node->upper) {
        std::string upperId = getDotNodeId(node->upper.get());
        linkToParent(upperId, "upper");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = upperId;
        node->upper->accept(this);
        currentNodeParentId = tempParent;
    }
    if (node->step) {
        std::string stepId = getDotNodeId(node->step.get());
        linkToParent(stepId, "step");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = stepId;
        node->step->accept(this);
        currentNodeParentId = tempParent;
    }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(BlockNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    for (const auto& stmt : node->statements) {
        if (stmt) {
            std::string stmtId = getDotNodeId(stmt.get());
            linkToParent(stmtId);
            std::string tempParent = currentNodeParentId;
            currentNodeParentId = stmtId;
            stmt->accept(this);
            currentNodeParentId = tempParent;
        }
    }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(AssignmentStatementNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    for (size_t i = 0; i < node->targets.size(); ++i) {
        if (node->targets[i]) {
            std::string targetId = getDotNodeId(node->targets[i].get());
            linkToParent(targetId, "target["+std::to_string(i)+"]");
            std::string tempParent = currentNodeParentId;
            currentNodeParentId = targetId;
            node->targets[i]->accept(this);
            currentNodeParentId = tempParent;
        }
    }
    if (node->value) {
        std::string valueId = getDotNodeId(node->value.get());
        linkToParent(valueId, "value");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = valueId;
        node->value->accept(this);
        currentNodeParentId = tempParent;
    }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(ExpressionStatementNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    if (node->expression) {
        std::string exprId = getDotNodeId(node->expression.get());
        linkToParent(exprId, "expr");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = exprId;
        node->expression->accept(this);
        currentNodeParentId = tempParent;
    }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(IfStatementNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;

    if (node->condition) {
        std::string condId = getDotNodeId(node->condition.get());
        linkToParent(condId, "condition");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = condId;
        node->condition->accept(this);
        currentNodeParentId = tempParent;
    }
    if (node->then_block) {
        std::string thenId = getDotNodeId(node->then_block.get());
        linkToParent(thenId, "then_block");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = thenId;
        node->then_block->accept(this);
        currentNodeParentId = tempParent;
    }

    for(size_t i=0; i < node->elif_blocks.size(); ++i) {
        const auto& elif_pair = node->elif_blocks[i];
        std::string elifNodeId = getDotNodeId(nullptr, "elif_clause");
        linkToParent(elifNodeId, "elif["+std::to_string(i)+"]");

        std::string tempParentIf = currentNodeParentId;
        currentNodeParentId = elifNodeId;
        if(elif_pair.first) { // condition
            std::string elifCondId = getDotNodeId(elif_pair.first.get());
            linkToParent(elifCondId, "condition");
            std::string tempParentElif = currentNodeParentId;
            currentNodeParentId = elifCondId;
            elif_pair.first->accept(this);
            currentNodeParentId = tempParentElif;
        }
        if(elif_pair.second) { // block
            std::string elifBlockId = getDotNodeId(elif_pair.second.get());
            linkToParent(elifBlockId, "block");
            std::string tempParentElif = currentNodeParentId;
            currentNodeParentId = elifBlockId;
            elif_pair.second->accept(this);
            currentNodeParentId = tempParentElif;
        }
        currentNodeParentId = tempParentIf;
    }

    if (node->else_block) {
        std::string elseId = getDotNodeId(node->else_block.get());
        linkToParent(elseId, "else_block");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = elseId;
        node->else_block->accept(this);
        currentNodeParentId = tempParent;
    }

    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(WhileStatementNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    if (node->condition) {
        std::string condId = getDotNodeId(node->condition.get());
        linkToParent(condId, "condition");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = condId;
        node->condition->accept(this);
        currentNodeParentId = tempParent;
    }
    if (node->body) {
        std::string bodyId = getDotNodeId(node->body.get());
        linkToParent(bodyId, "body");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = bodyId;
        node->body->accept(this);
        currentNodeParentId = tempParent;
    }
    if (node->else_block) {
        std::string elseId = getDotNodeId(node->else_block.get());
        linkToParent(elseId, "else_block");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = elseId;
        node->else_block->accept(this);
        currentNodeParentId = tempParent;
    }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(ForStatementNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    if (node->target) {
        std::string targetId = getDotNodeId(node->target.get());
        linkToParent(targetId, "target");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = targetId;
        node->target->accept(this);
        currentNodeParentId = tempParent;
    }
    if (node->iterable) {
        std::string iterId = getDotNodeId(node->iterable.get());
        linkToParent(iterId, "iterable");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = iterId;
        node->iterable->accept(this);
        currentNodeParentId = tempParent;
    }
    if (node->body) {
        std::string bodyId = getDotNodeId(node->body.get());
        linkToParent(bodyId, "body");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = bodyId;
        node->body->accept(this);
        currentNodeParentId = tempParent;
    }
    if (node->else_block) {
        std::string elseId = getDotNodeId(node->else_block.get());
        linkToParent(elseId, "else_block");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = elseId;
        node->else_block->accept(this);
        currentNodeParentId = tempParent;
    }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(FunctionDefinitionNode* node) {
    std::string selfId = getDotNodeId(node, "name: " + (node->name ? node->name->name : "<?>"));
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    if (node->name) {
        std::string nameId = getDotNodeId(node->name.get());
        linkToParent(nameId, "name_node");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = nameId;
        node->name->accept(this);
        currentNodeParentId = tempParent;
    }
    if (node->arguments_spec) {
        std::string argsId = getDotNodeId(node->arguments_spec.get());
        linkToParent(argsId, "arguments");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = argsId;
        node->arguments_spec->accept(this);
        currentNodeParentId = tempParent;
    }
    if (node->body) {
        std::string bodyId = getDotNodeId(node->body.get());
        linkToParent(bodyId, "body");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = bodyId;
        node->body->accept(this);
        currentNodeParentId = tempParent;
    }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(ClassDefinitionNode* node) {
    std::string selfId = getDotNodeId(node, "name: " + (node->name ? node->name->name : "<?>"));
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    if (node->name) {
        std::string nameId = getDotNodeId(node->name.get());
        linkToParent(nameId, "name_node");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = nameId;
        node->name->accept(this);
        currentNodeParentId = tempParent;
    }
    for (size_t i = 0; i < node->base_classes.size(); ++i) {
        if (node->base_classes[i]) {
            std::string baseId = getDotNodeId(node->base_classes[i].get());
            linkToParent(baseId, "base[" + std::to_string(i) + "]");
            std::string tempParent = currentNodeParentId;
            currentNodeParentId = baseId;
            node->base_classes[i]->accept(this);
            currentNodeParentId = tempParent;
        }
    }
    for (size_t i = 0; i < node->keywords.size(); ++i) {
        if (node->keywords[i]) {
            std::string kwId = getDotNodeId(node->keywords[i].get());
            linkToParent(kwId, "keyword[" + std::to_string(i) + "]");
            std::string tempParent = currentNodeParentId;
            currentNodeParentId = kwId;
            node->keywords[i]->accept(this);
            currentNodeParentId = tempParent;
        }
    }
    if (node->body) {
        std::string bodyId = getDotNodeId(node->body.get());
        linkToParent(bodyId, "body");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = bodyId;
        node->body->accept(this);
        currentNodeParentId = tempParent;
    }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(ReturnStatementNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    if (node->value) {
        std::string valId = getDotNodeId(node->value.get());
        linkToParent(valId, "value");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = valId;
        node->value->accept(this);
        currentNodeParentId = tempParent;
    }
    currentNodeParentId = oldParentId;
}

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
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    for (size_t i=0; i < node->names.size(); ++i) {
        if (node->names[i]) {
            std::string nameNodeId = getDotNodeId(node->names[i].get());
            linkToParent(nameNodeId, "name["+std::to_string(i)+"]");
            std::string tempParent = currentNodeParentId;
            currentNodeParentId = nameNodeId;
            node->names[i]->accept(this);
            currentNodeParentId = tempParent;
        }
    }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(ImportFromStatementNode* node) {
    std::ostringstream details_ss;
    details_ss << "level: " << node->level;
    if (!node->module_str.empty()) details_ss << ", module: " << node->module_str;
    if (node->import_star) details_ss << ", imports *";

    std::string selfId = getDotNodeId(node, details_ss.str());
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    for (size_t i=0; i < node->names.size(); ++i) {
        if (node->names[i]) {
            std::string nameNodeId = getDotNodeId(node->names[i].get());
            linkToParent(nameNodeId, "name["+std::to_string(i)+"]");
            std::string tempParent = currentNodeParentId;
            currentNodeParentId = nameNodeId;
            node->names[i]->accept(this);
            currentNodeParentId = tempParent;
        }
    }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(GlobalStatementNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    for (size_t i=0; i < node->names.size(); ++i) {
        if (node->names[i]) {
            std::string nameId = getDotNodeId(node->names[i].get());
            linkToParent(nameId, "name["+std::to_string(i)+"]");
            std::string tempParent = currentNodeParentId;
            currentNodeParentId = nameId;
            node->names[i]->accept(this);
            currentNodeParentId = tempParent;
        }
    }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(NonlocalStatementNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    for (size_t i=0; i < node->names.size(); ++i) {
        if (node->names[i]) {
            std::string nameId = getDotNodeId(node->names[i].get());
            linkToParent(nameId, "name["+std::to_string(i)+"]");
            std::string tempParent = currentNodeParentId;
            currentNodeParentId = nameId;
            node->names[i]->accept(this);
            currentNodeParentId = tempParent;
        }
    }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(TryStatementNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    if (node->try_block) {
        std::string tryId = getDotNodeId(node->try_block.get());
        linkToParent(tryId, "try_block");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = tryId;
        node->try_block->accept(this);
        currentNodeParentId = tempParent;
    }
    for (size_t i=0; i < node->handlers.size(); ++i) {
        if (node->handlers[i]) {
            std::string handlerId = getDotNodeId(node->handlers[i].get());
            linkToParent(handlerId, "handler["+std::to_string(i)+"]");
            std::string tempParent = currentNodeParentId;
            currentNodeParentId = handlerId;
            node->handlers[i]->accept(this);
            currentNodeParentId = tempParent;
        }
    }
    if (node->else_block) {
        std::string elseId = getDotNodeId(node->else_block.get());
        linkToParent(elseId, "else_block");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = elseId;
        node->else_block->accept(this);
        currentNodeParentId = tempParent;
    }
    if (node->finally_block) {
        std::string finallyId = getDotNodeId(node->finally_block.get());
        linkToParent(finallyId, "finally_block");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = finallyId;
        node->finally_block->accept(this);
        currentNodeParentId = tempParent;
    }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(RaiseStatementNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    if (node->exception) {
        std::string excId = getDotNodeId(node->exception.get());
        linkToParent(excId, "exception");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = excId;
        node->exception->accept(this);
        currentNodeParentId = tempParent;
    }
    if (node->cause) {
        std::string causeId = getDotNodeId(node->cause.get());
        linkToParent(causeId, "cause");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = causeId;
        node->cause->accept(this);
        currentNodeParentId = tempParent;
    }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(AugAssignNode* node) {
    std::string selfId = getDotNodeId(node, "op: " + node->op.lexeme);
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    if (node->target) {
        std::string targetId = getDotNodeId(node->target.get());
        linkToParent(targetId, "target");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = targetId;
        node->target->accept(this);
        currentNodeParentId = tempParent;
    }
    if (node->value) {
        std::string valueId = getDotNodeId(node->value.get());
        linkToParent(valueId, "value");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = valueId;
        node->value->accept(this);
        currentNodeParentId = tempParent;
    }
    currentNodeParentId = oldParentId;
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
    currentNodeParentId = selfId;
    if (node->default_value) {
        std::string defaultId = getDotNodeId(node->default_value.get());
        linkToParent(defaultId, "default_value");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = defaultId;
        node->default_value->accept(this);
        currentNodeParentId = tempParent;
    }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(ArgumentsNode* node) {
    std::string selfId = getDotNodeId(node);
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    for (size_t i = 0; i < node->args.size(); ++i) {
        if (node->args[i]) {
            std::string argId = getDotNodeId(node->args[i].get());
            linkToParent(argId, "param["+std::to_string(i)+"]");
            std::string tempParent = currentNodeParentId;
            currentNodeParentId = argId;
            node->args[i]->accept(this);
            currentNodeParentId = tempParent;
        }
    }
    if (node->vararg) {
        std::string varargId = getDotNodeId(node->vararg.get());
        linkToParent(varargId, "vararg (*args)");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = varargId;
        node->vararg->accept(this);
        currentNodeParentId = tempParent;
    }
    if (node->kwarg) {
        std::string kwargId = getDotNodeId(node->kwarg.get());
        linkToParent(kwargId, "kwarg (**kwargs)");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = kwargId;
        node->kwarg->accept(this);
        currentNodeParentId = tempParent;
    }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(KeywordArgNode* node) {
    std::string selfId = getDotNodeId(node, "name: " + (node->arg_name ? node->arg_name->name : "<?>"));
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    if (node->arg_name) {
        std::string nameId = getDotNodeId(node->arg_name.get());
        linkToParent(nameId, "arg_name_node");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = nameId;
        node->arg_name->accept(this);
        currentNodeParentId = tempParent;
    }
    if (node->value) {
        std::string valueId = getDotNodeId(node->value.get());
        linkToParent(valueId, "value");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = valueId;
        node->value->accept(this);
        currentNodeParentId = tempParent;
    }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(NamedImportNode* node) {
    std::string details = "path: " + node->module_path_str;
    if (node->alias) details += ", as: " + node->alias->name;
    std::string selfId = getDotNodeId(node, details);
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    if (node->alias) {
        // The alias is an IdentifierNode. Its details are already part of the NamedImportNode's label.
        // If IdentifierNode itself had complex children or required its own separate box for more detail,
        // you might get its ID and call accept on it.
        // However, IdentifierNode's visit method in this generator only creates a label and links to parent.
        // To show it as a distinct child node graphically (even if simple):
        // std::string aliasId = getDotNodeId(node->alias.get());
        // linkToParent(aliasId, "alias_node");
        // node->alias->accept(this); // This would draw the IdentifierNode box and link it
    }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(ImportNameNode* node) {
    std::string details = "name: " + node->name_str;
    if (node->alias) details += ", as: " + node->alias->name;
    std::string selfId = getDotNodeId(node, details);
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    // Similar to NamedImportNode, alias details are in the label.
    // If separate node box for alias is desired:
    // if (node->alias) {
    //     std::string aliasId = getDotNodeId(node->alias.get());
    //     linkToParent(aliasId, "alias_node");
    //     node->alias->accept(this);
    // }
    currentNodeParentId = oldParentId;
}

void DOTGenerator::visit(ExceptionHandlerNode* node) {
    std::ostringstream details_ss;
    if (node->type) details_ss << "type: (see child)";
    if (node->name) details_ss << (node->type ? ", " : "") << "as: " << node->name->name;

    std::string selfId = getDotNodeId(node, details_ss.str());
    linkToParent(selfId);
    std::string oldParentId = currentNodeParentId;
    currentNodeParentId = selfId;
    if (node->type) {
        std::string typeId = getDotNodeId(node->type.get());
        linkToParent(typeId, "type_expr");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = typeId;
        node->type->accept(this);
        currentNodeParentId = tempParent;
    }
    if (node->name) {
        std::string nameId = getDotNodeId(node->name.get());
        linkToParent(nameId, "name_node (as)");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = nameId;
        node->name->accept(this);
        currentNodeParentId = tempParent;
    }
    if (node->body) {
        std::string bodyId = getDotNodeId(node->body.get());
        linkToParent(bodyId, "body");
        std::string tempParent = currentNodeParentId;
        currentNodeParentId = bodyId;
        node->body->accept(this);
        currentNodeParentId = tempParent;
    }
    currentNodeParentId = oldParentId;
}