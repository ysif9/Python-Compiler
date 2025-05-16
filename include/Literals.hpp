#ifndef LITERALS_HPP
#define LITERALS_HPP

#include "Expressions.hpp"
#include <string>
#include <variant>

class NumberLiteralNode : public ExpressionNode {
public:
    std::string value_str;
    enum class Type { INTEGER, FLOAT } type;

    NumberLiteralNode(int line, std::string val_str, Type t)
            : ExpressionNode(line), value_str(std::move(val_str)), type(t) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "NumberLiteralNode"; }
};

class StringLiteralNode : public ExpressionNode {
public:
    std::string value;

    StringLiteralNode(int line, std::string val)
            : ExpressionNode(line), value(std::move(val)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "StringLiteralNode"; }
};

class BytesLiteralNode : public ExpressionNode {
public:
    std::string value;

    BytesLiteralNode(int line, std::string val)
            : ExpressionNode(line), value(std::move(val)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "BytesLiteralNode"; }
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
    std::string real_part_str;
    std::string imag_part_str;

    ComplexLiteralNode(int line, std::string real_str, std::string imag_str)
            : ExpressionNode(line), real_part_str(std::move(real_str)), imag_part_str(std::move(imag_str)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ComplexLiteralNode"; }
};


#endif // LITERALS_HPP