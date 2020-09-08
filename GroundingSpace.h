#ifndef GROUNDING_SPACE_H
#define GROUNDING_SPACE_H

#include <initializer_list>
#include <stdexcept>
#include <vector>
#include <memory>

#include "SpaceAPI.h"

// Atom

class Atom;

using AtomPtr = std::shared_ptr<Atom>;

class Atom {
public:
    enum Type {
        SYMBOL,
        GROUNDED,
        EXPR,
        VARIABLE
    };

    static AtomPtr INVALID;

    virtual ~Atom() { }
    virtual Type get_type() const = 0;
    virtual bool operator==(Atom const& other) const = 0;
    virtual bool operator!=(Atom const& other) const { return !(*this == other); }
    virtual std::string to_string() const = 0;
};

std::string to_string(Atom::Type type);
bool operator==(std::vector<AtomPtr> const& a, std::vector<AtomPtr> const& b); 
std::string to_string(std::vector<AtomPtr> const& atoms, std::string delimiter);

class SymbolAtom : public Atom {
public:
    SymbolAtom(std::string symbol) : symbol(symbol) { }
    std::string get_symbol() const { return symbol; }

    Type get_type() const override { return SYMBOL; }
    bool operator==(Atom const& _other) const override { 
        SymbolAtom const* other = dynamic_cast<SymbolAtom const*>(&_other);
        return other && symbol == other->symbol;
    }
    std::string to_string() const override { return symbol; }
private:
    std::string symbol;
};

inline AtomPtr S(std::string symbol) {
    return std::make_shared<SymbolAtom>(symbol);
}

class ExprAtom : public Atom {
public:
    ExprAtom(std::initializer_list<AtomPtr> children) : children(children) { }
    ExprAtom(std::vector<AtomPtr> children) : children(children) { }
    std::vector<AtomPtr>& get_children() { return children; }

    Type get_type() const override { return EXPR; }
    bool operator==(Atom const& _other) const override;
    std::string to_string() const override { return "(" + ::to_string(children, " ") + ")"; }

private:
    std::vector<AtomPtr> children;
};

inline AtomPtr E(std::initializer_list<AtomPtr> children) {
    return std::make_shared<ExprAtom>(children);
}

inline AtomPtr E(std::vector<AtomPtr> children) {
    return std::make_shared<ExprAtom>(children);
}

using ExprAtomPtr = std::shared_ptr<ExprAtom>;

class VariableAtom : public Atom {
public:
    VariableAtom(std::string name) : name(name) { }
    std::string get_name() const { return name; }

    Type get_type() const override { return VARIABLE; }
    bool operator==(Atom const& _other) const override {
        VariableAtom const* other = dynamic_cast<VariableAtom const*>(&_other);
        return other && name == other->name;
    }
    std::string to_string() const override { return "$" + name; }
private:
    std::string name;
};

inline AtomPtr V(std::string name) {
    return std::make_shared<VariableAtom>(name);
}

class GroundedAtom : public Atom {
public:
    virtual ~GroundedAtom() { }
    virtual AtomPtr execute(AtomPtr args) const {
        throw std::runtime_error("Operation is not supported");
    }

    Type get_type() const override { return GROUNDED; }
};

using GroundedAtomPtr = std::shared_ptr<GroundedAtom>;

template <typename T>
class ValueAtom : public GroundedAtom {
public:
    ValueAtom(T value) : value(value) { }
    virtual ~ValueAtom() { }
    bool operator==(Atom const& _other) const override { 
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

    void add_native(const SpaceAPI* other) override {
        throw std::logic_error("Method is not implemented");
    }

    std::string get_type() const override { return TYPE; }

    void add_atom(AtomPtr atom) {
        content.push_back(atom);
    }

    // TODO: Which operations should we add into SpaceAPI to make
    // interpret_step space implementation agnostic?
    AtomPtr interpret_step(SpaceAPI const& kb);

    bool operator==(SpaceAPI const& space) const;
    bool operator!=(SpaceAPI const& other) const { return !(*this == other); }
    std::string to_string() const { return ::to_string(content, "\n"); }

private:

    std::vector<AtomPtr> content;
};

#endif // GROUNDING_SPACE_H
