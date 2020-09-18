#include "GroundingSpace.h"

#include <map>
#include <memory>
//#include <iostream>

// Atom

AtomPtr Atom::INVALID = std::shared_ptr<Atom>(nullptr);

std::string to_string(Atom::Type type) {
    static std::string names[] = { "S", "G", "E", "V" };
    return names[type];
}

bool operator==(std::vector<AtomPtr> const& a, std::vector<AtomPtr> const& b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (int i = 0; i < a.size(); ++i) {
        if (*a.at(i) != *b.at(i)) {
            return false;
        }
    }
    return true;
}

std::string to_string(std::vector<AtomPtr> const& atoms, std::string delimiter) {
    std::string str = "";
    for (auto it = atoms.begin(); it != atoms.end(); ++it) {
        str += (it == atoms.begin() ? "" : delimiter) + (*it)->to_string();
    }
    return str;
}

bool ExprAtom::operator==(Atom const& _other) const { 
    if (_other.get_type() != EXPR) {
        return false;
    }
    ExprAtom const& other = static_cast<ExprAtom const&>(_other);
    return children == other.children;
}

// Grounding space

std::string GroundingSpace::TYPE = "GroundingSpace";

struct PlainExprResult {
    bool found;
    ExprAtomPtr parent;
    int child_index;
    ExprAtomPtr plain;
    bool has_parent() { return child_index != -1; }
};

PlainExprResult find_plain_sub_expr(AtomPtr atom) {
    //std::clog << "find_plain_sub_expr: " << atom->to_string() << std::endl;
    if (atom->get_type() != Atom::EXPR) {
        return { false };
    }
    ExprAtomPtr expr = std::static_pointer_cast<ExprAtom>(atom);
    auto const& children = expr->get_children();
    for (int i = 0; i < children.size(); ++i) {
        // If sub expr contains a variable it is not evaluatable
        if (children[i]->get_type() == Atom::VARIABLE) {
            return { false };
        }
        PlainExprResult plain = find_plain_sub_expr(children[i]);
        if (plain.found) {
            if (plain.has_parent()) {
                return plain;
            } else {
                return { true, expr, i, plain.plain };
            }
        }
    }
    return { true, expr, -1, expr };
}

void GroundingSpace::interpret_step(SpaceAPI const& _kb) {
    if (_kb.get_type() != GroundingSpace::TYPE) {
        throw std::runtime_error("Only " + GroundingSpace::TYPE +
                " knowledge bases are supported");
    }
    GroundingSpace const& kb = static_cast<GroundingSpace const&>(_kb);

    AtomPtr atom = content.back();
    if (atom->get_type() != Atom::EXPR) {
        content.pop_back();
        content.push_back(atom);
        return;
    }

    PlainExprResult plain_expr_result = find_plain_sub_expr(atom);
    ExprAtomPtr plain_expr = plain_expr_result.plain;
    //std::clog << "plain_expr found: " << plain_expr->to_string() << std::endl;
    GroundingSpace result;
    AtomPtr op = plain_expr->get_children()[0];
    if (op->get_type() == Atom::GROUNDED) {
        GroundedAtom const* func = static_cast<GroundedAtom const*>(op.get());
        // TODO: How should we return results of the execution? At the moment they
        // are put into current atomspace. Should we return new child atomspace
        // instead?
        GroundingSpace args(plain_expr->get_children());
        func->execute(&args, &result);
    } else {
        throw std::logic_error("This case is not implemented yet, plain_expr: " +
                plain_expr->to_string());
    }

    if (!plain_expr_result.has_parent()) {
        content.pop_back();
        for (auto & atom : result.get_content()) {
            content.push_back(atom);
        }
        return;
    } else {
        // FIXME: implement by duplicating root of the plain_expr_result, and
        // replacing plain_expr by each item of content and push it back to the
        // content collection.
        if (result.get_content().size() != 1) {
            throw std::logic_error("This case is not implemented yet");
        }
        plain_expr_result.parent->get_children()[plain_expr_result.child_index] = result.get_content()[0];
        return;
    }
}

class LessVariableAtomPtr {
public:
    bool operator()(VariableAtomPtr const& a, VariableAtomPtr const& b) const {
        return a->get_name() < b->get_name();
    }
};

using Bindings = std::map<VariableAtomPtr, AtomPtr, LessVariableAtomPtr>;

struct MatchResult {
    Bindings a_bindings;
    Bindings b_bindings;
};

// FIXME: if variable matched twice it should be checked the second match is
// equal to the first one.
static bool match_atoms(AtomPtr a, AtomPtr b, MatchResult& match) {
    // TODO: it is not clear how should we handle the case when a and b are
    // both variables. We can check variable name equality and skip binding. We
    // can add a as binding for b and vice versa.
    if (b->get_type() == Atom::VARIABLE) {
        VariableAtomPtr var = std::static_pointer_cast<VariableAtom>(b);
        match.b_bindings[var] = a;
        return true;
    }
    switch (a->get_type()) {
        case Atom::SYMBOL:
        case Atom::GROUNDED:
            return *a == *b;
        case Atom::VARIABLE:
            {
                VariableAtomPtr var = std::static_pointer_cast<VariableAtom>(a);
                match.a_bindings[var] = b;
                return true;
            }
        case Atom::EXPR:
            {
                if (b->get_type() != Atom::EXPR) {
                    return false;
                }
                std::vector<AtomPtr>& childrenA = std::static_pointer_cast<ExprAtom>(a)->get_children();
                std::vector<AtomPtr>& childrenB = std::static_pointer_cast<ExprAtom>(b)->get_children();
                if (childrenA.size() != childrenB.size()) {
                    return false;
                }
                for (int i = 0; i < childrenA.size(); ++i) {
                    if (!match_atoms(childrenA[i], childrenB[i], match)) {
                        return false;
                    }
                }
                return true;
            }
        default:
            throw std::logic_error("Not implemented for type: " +
                    to_string(a->get_type()));
    }
}

static AtomPtr apply_match_to_atom(AtomPtr const& atom, Bindings const& bindings) {
    switch (atom->get_type()) {
        case Atom::SYMBOL:
        case Atom::GROUNDED:
            return atom;
        case Atom::VARIABLE:
            {
                VariableAtomPtr var = std::static_pointer_cast<VariableAtom>(atom);
                auto const& pair = bindings.find(var);
                if (pair != bindings.end()) {
                    return pair->second;
                } else {
                    return atom;
                }
            }
        case Atom::EXPR:
            {
                ExprAtomPtr expr = std::static_pointer_cast<ExprAtom>(atom);
                std::vector<AtomPtr> children;
                for (auto const& atom : expr->get_children()) {
                    AtomPtr applied = apply_match_to_atom(atom, bindings);
                    children.push_back(applied);
                }
                AtomPtr grounded_expr = E(children);
                return grounded_expr;
            }
        default:
            throw std::logic_error("Not implemented for type: " +
                    to_string(atom->get_type()));
    }
}

static void apply_a_to_b_bindings(MatchResult& match) {
    Bindings b_bindings;
    for (auto const& pair : match.b_bindings) {
        AtomPtr applied = apply_match_to_atom(pair.second, match.a_bindings);
        b_bindings[pair.first] = applied;
    }
    match.b_bindings = b_bindings;
}

static void apply_match_to_templ(GroundingSpace& result,
        std::vector<AtomPtr> const& templ, MatchResult const& match) {
    for (auto const& atom : templ) {
        result.add_atom(apply_match_to_atom(atom, match.b_bindings));
    }
}

void GroundingSpace::match(SpaceAPI const& _pattern, SpaceAPI const& _templ, GroundingSpace& result) const {
    if (_pattern.get_type() != GroundingSpace::TYPE) {
        throw std::runtime_error("_pattern is expected to be GroundingSpace");
    }
    GroundingSpace const& pattern = static_cast<GroundingSpace const&>(_pattern);
    if (_templ.get_type() != GroundingSpace::TYPE) {
        throw std::runtime_error("_templ is expected to be GroundingSpace");
    }
    GroundingSpace const& templ = static_cast<GroundingSpace const&>(_templ);
    if (pattern.content.size() != 1) {
        throw std::logic_error("_pattern with more than one clause is not supported");
    }
    AtomPtr pattern_atom = pattern.content[0];
    for (auto const& kb_atom : content) {
        MatchResult match_result;
        if (!match_atoms(kb_atom, pattern_atom, match_result)) {
            continue;
        }
        apply_a_to_b_bindings(match_result);
        apply_match_to_templ(result, templ.content, match_result);
    }
}

bool GroundingSpace::operator==(SpaceAPI const& _other) const {
    if (_other.get_type() != GroundingSpace::TYPE) {
        return false;
    }
    GroundingSpace const& other = static_cast<GroundingSpace const&>(_other);
    return content == other.content;
}

