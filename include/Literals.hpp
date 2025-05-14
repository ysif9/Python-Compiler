#ifndef LITERALS_HPP
#define LITERALS_HPP

#include "Expressions.hpp" // Literals are expressions
#include <string>

// Concrete Literal Nodes (all inherit from ExpressionNode)

class NumberLiteralNode : public ExpressionNode {
public:
    std::string value; // Store as string to preserve original format (e.g., "1.0", "1e3", "123")

    NumberLiteralNode(int line, std::string val_str)
            : ExpressionNode(line), value(std::move(val_str)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "NumberLiteralNode"; }
};

class StringLiteralNode : public ExpressionNode {
public:
    std::string value; // The actual string content (e.g., after unescaping quotes)

    StringLiteralNode(int line, std::string val_str)
            : ExpressionNode(line), value(std::move(val_str)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "StringLiteralNode"; }
};

class BooleanLiteralNode : public ExpressionNode {
public:
    bool value;

    BooleanLiteralNode(int line, bool val)
            : ExpressionNode(line), value(val) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "BooleanLiteralNode"; }
};

class NoneLiteralNode : public ExpressionNode {
public:
    NoneLiteralNode(int line) : ExpressionNode(line) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "NoneLiteralNode"; }
};

class ComplexLiteralNode : public ExpressionNode {
public:
    std::string value; // e.g., "3+5j", "1j"; parsing to actual complex type is later

    ComplexLiteralNode(int line, std::string val_str)
            : ExpressionNode(line), value(std::move(val_str)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ComplexLiteralNode"; }
};

class BytesLiteralNode : public ExpressionNode {
public:
    std::string value; // The content of the bytes literal, usually raw or escaped

    BytesLiteralNode(int line, std::string val_str)
            : ExpressionNode(line), value(std::move(val_str)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "BytesLiteralNode"; }
};


#endif // LITERALS_HPP