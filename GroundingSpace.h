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
    virtual bool operator!=(Expr const& other) const { return !(*this == other); }
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

ExprPtr S(std::string symbol) {
    return std::make_shared<SymbolExpr>(symbol);
}

class CompositeExpr : public Expr {
public:
    CompositeExpr(std::initializer_list<ExprPtr> children) : children(children) { }
    CompositeExpr(std::vector<ExprPtr> children) : children(children) { }
    std::vector<ExprPtr>& get_children() { return children; }

    Type get_type() const { return COMPOSITE; }
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
    if (children.size() != other.children.size()) {
        return false;
    }
    for (int i = 0; i < children.size(); ++i) {
        if (*children.at(i) != *other.children.at(i)) {
            return false;
        }
    }
    return true;
}

std::string CompositeExpr::to_string() const {
    std::string str = "(";
    for (auto it = children.begin(); it != children.end(); ++it) {
        str += (it == children.begin() ? "" : " ") + (*it)->to_string();
    }
    return str + ")";
}

ExprPtr C(std::initializer_list<ExprPtr> children) {
    return std::make_shared<CompositeExpr>(children);
}

ExprPtr C(std::vector<ExprPtr> children) {
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

ExprPtr V(std::string name) {
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
    ExprPtr interpret_step();

private:

    std::vector<ExprPtr> content;
};

std::string GroundingSpace::TYPE = "GroundingSpace";

struct PlainExprResult {
    bool found;
    CompositeExprPtr parent;
    int child_index;
    CompositeExprPtr plain;
    bool has_parent() { return child_index != -1; }
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
            if (plain.has_parent()) {
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
        GroundedExpr const* func = static_cast<GroundedExpr const*>(op.get());
        // TODO: How should we return results of the execution? At the moment they
        // are put into current atomspace. Should we return new child atomspace
        // instead?
        result = func->execute(plain_expr);
    }

    if (!plainExprResult.has_parent()) {
        content.pop_back();
        content.push_back(result);
        return result;
    } else {
        plainExprResult.parent->get_children()[plainExprResult.child_index] = result;
        return plainExprResult.parent;
    }
}

#endif // GROUNDING_SPACE_H
