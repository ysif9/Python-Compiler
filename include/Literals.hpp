#ifndef LITERALS_HPP
#define LITERALS_HPP

#include "Expressions.hpp" // For ExpressionNode base
#include <string>
#include <variant> // For NumberLiteralNode if used

class NumberLiteralNode : public ExpressionNode {
public:
    // Store value as string to preserve original format (e.g. hex, leading zeros if necessary for some tools)
    // Or use a variant if direct numeric value is preferred: std::variant<long long, double> value;
    std::string value_str;
    enum class Type { INTEGER, FLOAT } type; // Simplified

    NumberLiteralNode(int line, std::string val_str, Type t)
            : ExpressionNode(line), value_str(std::move(val_str)), type(t) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "NumberLiteralNode"; }
};

class StringLiteralNode : public ExpressionNode {
public:
    // For concatenated strings "a" "b", lexer/parser should join them
    // This holds the final string value
    std::string value;
    // Add prefix, is_raw, is_bytes, is_unicode flags if needed, but grammar simplified.
    // Current grammar has TK_STRING and TK_BYTES tokens.

    StringLiteralNode(int line, std::string val)
            : ExpressionNode(line), value(std::move(val)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "StringLiteralNode"; }
};

class BytesLiteralNode : public ExpressionNode {
public:
    std::string value; // Represents the content of bytes literal, prefixes b'' handled by lexer.

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
    // Storing as strings as precision and original form might matter
    std::string real_part_str;
    std::string imag_part_str;
    // Or: double real; double imag;

    ComplexLiteralNode(int line, std::string real_str, std::string imag_str)
            : ExpressionNode(line), real_part_str(std::move(real_str)), imag_part_str(std::move(imag_str)) {}

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
    std::string getNodeName() const override { return "ComplexLiteralNode"; }
};


#endif // LITERALS_HPP