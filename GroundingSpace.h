#ifndef GROUNDING_SPACE_H
#define GROUNDING_SPACE_H

#include <initializer_list>
#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <regex>

#include "SpaceAPI.h"

// Expression

class Expr;

using ExprPtr = std::shared_ptr<Expr>;

class Expr {
public:
    enum Type {
        SYMBOL,
        GROUNDED,
        COMPOSITE,
        VARIABLE
    };

    static ExprPtr INVALID;

    virtual ~Expr() { }
    virtual Type get_type() const = 0;
    virtual bool operator==(Expr const& other) const = 0;
    virtual bool operator!=(Expr const& other) const { return !(*this == other); }
    virtual std::string to_string() const = 0;
};

std::string to_string(Expr::Type type);
bool operator==(std::vector<ExprPtr> const& a, std::vector<ExprPtr> const& b); 
std::string to_string(std::vector<ExprPtr> const& exprs, std::string delimiter);

class SymbolExpr : public Expr {
public:
    SymbolExpr(std::string symbol) : symbol(symbol) { }
    std::string get_symbol() const { return symbol; }

    Type get_type() const { return SYMBOL; }
    bool operator==(Expr const& _other) const { 
        SymbolExpr const* other = dynamic_cast<SymbolExpr const*>(&_other);
        return other && symbol == other->symbol;
    }
    std::string to_string() const { return symbol; }
private:
    std::string symbol;
};

inline ExprPtr S(std::string symbol) {
    return std::make_shared<SymbolExpr>(symbol);
}

class CompositeExpr : public Expr {
public:
    CompositeExpr(std::initializer_list<ExprPtr> children) : children(children) { }
    CompositeExpr(std::vector<ExprPtr> children) : children(children) { }
    std::vector<ExprPtr>& get_children() { return children; }

    Type get_type() const { return COMPOSITE; }
    bool operator==(Expr const& _other) const;
    std::string to_string() const { return "(" + ::to_string(children, " ") + ")"; }

private:
    std::vector<ExprPtr> children;
};

inline ExprPtr C(std::initializer_list<ExprPtr> children) {
    return std::make_shared<CompositeExpr>(children);
}

inline ExprPtr C(std::vector<ExprPtr> children) {
    return std::make_shared<CompositeExpr>(children);
}

using CompositeExprPtr = std::shared_ptr<CompositeExpr>;

class VariableExpr : public Expr {
public:
    VariableExpr(std::string name) : name(name) { }
    std::string get_name() const { return name; }

    Type get_type() const { return VARIABLE; }
    bool operator==(Expr const& _other) const {
        VariableExpr const* other = dynamic_cast<VariableExpr const*>(&_other);
        return other && name == other->name;
    }
    std::string to_string() const { return "$" + name; }
private:
    std::string name;
};

inline ExprPtr V(std::string name) {
    return std::make_shared<VariableExpr>(name);
}

class GroundedExpr : public Expr {
public:
    virtual ~GroundedExpr() { }
    virtual ExprPtr execute(ExprPtr args) const {
        throw std::runtime_error("Operation not supported");
    }

    Type get_type() const { return GROUNDED; }
};

using GroundedExprPtr = std::shared_ptr<GroundedExpr>;

template <typename T>
class ValueAtom : public GroundedExpr {
public:
    ValueAtom(T value) : value(value) {}
    virtual ~ValueAtom() { }
    bool operator==(Expr const& _other) const { 
        // TODO: should it be replaced by type checking?
        ValueAtom const* other = dynamic_cast<ValueAtom const*>(&_other);
        return other && other->value == value;
    }
    T get() const { return value; }
private:
    T value;
};

// Space

class GroundingSpace : public SpaceAPI {
public:

    static std::string TYPE;

    virtual ~GroundingSpace() { }

    void add_native(const SpaceAPI* other) {
        throw std::logic_error("Method is not implemented");
    }

    std::string get_type() const { return TYPE; }

    void add_expr(ExprPtr expr) {
        content.push_back(expr);
    }

    // TODO: Which operations should we add into SpaceAPI to make
    // interpret_step space implementation agnostic?
    ExprPtr interpret_step(SpaceAPI const& kb);

    bool operator==(SpaceAPI const& space) const;
    bool operator!=(SpaceAPI const& other) const { return !(*this == other); }
    std::string to_string() const { return ::to_string(content, "\n"); }

private:

    std::vector<ExprPtr> content;
};

// Text space

class TextSpace : public SpaceAPI {
public:

    static std::string TYPE;

    using GroundedExprConstr = std::function<GroundedExprPtr(std::string)>;
    using GroundedTypeDescr = std::pair<std::regex, GroundedExprConstr>;

    virtual ~TextSpace() { }

    void add_to(SpaceAPI& space) const;

    void add_native(const SpaceAPI* other) {
        throw std::logic_error("Method is not implemented");
    }

    std::string get_type() const { return TYPE; }

    void add_string(std::string str_expr) {
        code.push_back(str_expr);
    }

    void register_grounded_type(std::regex regex,
            GroundedExprConstr constructor) {
        grounded_types.push_back(GroundedTypeDescr(regex, constructor));
    }

    GroundedExprPtr find_grounded_type(std::string token) const;

private:
    struct ParseResult {
        ExprPtr expr;
        bool is_eof;
    };

    ParseResult  recursive_parse(char const* text, char const*& pos) const;
    void parse(std::string text, std::function<void(ExprPtr)> add) const;

    std::vector<std::string> code; 
    std::vector<GroundedTypeDescr> grounded_types;
};

#endif // GROUNDING_SPACE_H
