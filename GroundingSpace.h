#ifndef GROUNDING_SPACE_H
#define GROUNDING_SPACE_H

#include <initializer_list>
#include <stdexcept>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>

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
    virtual std::string to_string() const = 0;
};

std::string to_string(Expr::Type type) {
    static std::string names[] = { "S", "G", "C", "V" };
    return names[type];
}

ExprPtr Expr::INVALID = std::shared_ptr<Expr>(nullptr);

class SymbolExpr : public Expr {
public:
    SymbolExpr(std::string symbol) : symbol(symbol) { }
    Type get_type() const { return SYMBOL; }
    std::string get_symbol() const { return symbol; }
    virtual bool operator==(Expr const& _other) const { 
        SymbolExpr const* other = dynamic_cast<SymbolExpr const*>(&_other);
        return other && symbol == other->symbol;
    }
    std::string to_string() const { return ::to_string(get_type()) + "(" + symbol + ")"; }
private:
    std::string symbol;
};

class GroundedExpr : public SymbolExpr {
public:
    GroundedExpr(std::string symbol) : SymbolExpr(symbol) { }
    Type get_type() const { return GROUNDED; }
};

ExprPtr G(std::string symbol) {
    return std::make_shared<GroundedExpr>(symbol);
}

class CompositeExpr : public Expr {
public:
    CompositeExpr(std::initializer_list<ExprPtr> children) : children(children) { }
    Type get_type() const { return COMPOSITE; }
    std::vector<ExprPtr>& get_children() { return children; }
    bool operator==(Expr const& _other) const;
    std::string to_string() const;
private:
    std::vector<ExprPtr> children;
};

bool CompositeExpr::operator==(Expr const& _other) const { 
    if (_other.get_type() != COMPOSITE) {
        return false;
    }
    CompositeExpr const& other = static_cast<CompositeExpr const&>(_other);
    return children == other.children;
}

std::string CompositeExpr::to_string() const {
    std::string str = "";
    for (auto const& child : children) {
        str += str + ", " + child->to_string();
    }
    return str;
}

ExprPtr C(std::initializer_list<ExprPtr> children) {
    return std::make_shared<CompositeExpr>(children);
}

using CompositeExprPtr = std::shared_ptr<CompositeExpr>;

// Groundings

class Grounding {
public:
    enum Type {
        VALUE,
        FUNC
    };

    virtual ~Grounding() { }
    virtual Type get_type() const = 0;
};

// Grounding is immutable but can be replaced by another grounding
using GroundingPtr = std::shared_ptr<const Grounding>;

std::string to_string(Grounding::Type type) {
    static std::string names[] = { "VALUE", "FUNC" };
    return names[type];
}

// Space

class GroundingSpace : public SpaceAPI {
public:

    static std::string TYPE;

    virtual ~GroundingSpace() { }

    virtual void add_native(const SpaceAPI* other) {
        throw std::logic_error("Method is not implemented");
    }

    virtual std::string get_type() const {
        return TYPE;
    }

    void add_grounded_symbol(std::string symbol, GroundingPtr value) {
        grounding_by_symbol[symbol] = value;
    }

    GroundingPtr get_grounding(std::string symbol) const {
        return grounding_by_symbol.at(symbol);
    }

    void add_expr(ExprPtr expr) {
        content.push_back(expr);
    }

    // TODO: How should we return results of the execution? At the moment they
    // are put into current atomspace. Should we return new child atomspace
    // instead?
    ExprPtr execute(CompositeExprPtr expr);

    // TODO: Which operations should we add into SpaceAPI to make
    // interpret_step space implementation agnostic?
    ExprPtr interpret_step();

private:

    std::map<std::string, GroundingPtr> grounding_by_symbol;
    std::vector<ExprPtr> content;
};

std::string GroundingSpace::TYPE = "GroundingSpace";

struct PlainExprResult {
    bool found;
    CompositeExprPtr parent;
    int child_index;
    CompositeExprPtr plain;
};

PlainExprResult find_plain_sub_expr(ExprPtr expr) {
    if (expr->get_type() != Expr::COMPOSITE) {
        return { false };
    }
    CompositeExprPtr composite = std::static_pointer_cast<CompositeExpr>(expr);
    auto const& children = composite->get_children();
    for (int i = 0; i < children.size(); ++i) {
        PlainExprResult plain = find_plain_sub_expr(children[i]);
        if (plain.found) {
            if (plain.child_index != -1) {
                return plain;
            } else {
                return { true, composite, i, plain.plain };
            }
        }
    }
    return { true, composite, -1, composite };
}

ExprPtr GroundingSpace::interpret_step() {
    ExprPtr expr = content.back();
    if (expr->get_type() != Expr::COMPOSITE) {
        content.pop_back();
        return expr;
    }

    PlainExprResult plainExprResult = find_plain_sub_expr(expr);
    CompositeExprPtr plain_expr = plainExprResult.plain;
    ExprPtr result = Expr::INVALID;
    ExprPtr op = plain_expr->get_children()[0];
    if (op->get_type() == Expr::GROUNDED) {
         result = execute(plain_expr);
    }

    if (plainExprResult.child_index == -1) {
        content.pop_back();
        content.push_back(result);
        return result;
    } else {
        plainExprResult.parent->get_children()[plainExprResult.child_index] = result;
        return plainExprResult.parent;
    }
}

class Value : public Grounding {
public:
    virtual ~Value() { }
    Type get_type() const { return VALUE; }
};

class Func : public Grounding {
public:
    virtual ExprPtr execute(GroundingSpace& space, ExprPtr args) const = 0;
    Type get_type() const { return FUNC; }
};

ExprPtr GroundingSpace::execute(CompositeExprPtr expr) {
    SymbolExpr const* op = static_cast<SymbolExpr const*>(expr->get_children()[0].get());
    GroundingPtr grounding = grounding_by_symbol.at(op->get_symbol());
    switch (grounding->get_type()) {
    case Grounding::VALUE:
        throw std::runtime_error("Cannot execute grounded value");
    case Grounding::FUNC: {
        Func const* func = static_cast<Func const*>(grounding.get());
        return func->execute(*this, expr);
    }
    default:
        throw std::logic_error("Unexpected Grounding::Type value: " + to_string(grounding->get_type()));
    }
}

#endif // GROUNDING_SPACE_H
