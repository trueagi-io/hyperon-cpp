#include "GroundingSpace.h"

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
    if (atom->get_type() != Atom::EXPR) {
        return { false };
    }
    ExprAtomPtr expr = std::static_pointer_cast<ExprAtom>(atom);
    auto const& children = expr->get_children();
    for (int i = 0; i < children.size(); ++i) {
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

AtomPtr GroundingSpace::interpret_step(SpaceAPI const& _kb) {
    if (_kb.get_type() != GroundingSpace::TYPE) {
        throw std::runtime_error("Only " + GroundingSpace::TYPE +
                " knowledge bases are supported");
    }
    GroundingSpace const& kb = static_cast<GroundingSpace const&>(_kb);

    AtomPtr atom = content.back();
    if (atom->get_type() != Atom::EXPR) {
        content.pop_back();
        return atom;
    }

    PlainExprResult plain_expr_result = find_plain_sub_expr(atom);
    ExprAtomPtr plain_expr = plain_expr_result.plain;
    AtomPtr result = Atom::INVALID;
    AtomPtr op = plain_expr->get_children()[0];
    if (op->get_type() == Atom::GROUNDED) {
        GroundedAtom const* func = static_cast<GroundedAtom const*>(op.get());
        // TODO: How should we return results of the execution? At the moment they
        // are put into current atomspace. Should we return new child atomspace
        // instead?
        result = func->execute(plain_expr);
    }

    if (!plain_expr_result.has_parent()) {
        content.pop_back();
        content.push_back(result);
        return result;
    } else {
        plain_expr_result.parent->get_children()[plain_expr_result.child_index] = result;
        return plain_expr_result.parent;
    }
}

bool GroundingSpace::operator==(SpaceAPI const& _other) const {
    if (_other.get_type() != GroundingSpace::TYPE) {
        return false;
    }
    GroundingSpace const& other = static_cast<GroundingSpace const&>(_other);
    return content == other.content;
}

