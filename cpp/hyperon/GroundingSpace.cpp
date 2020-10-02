#include "GroundingSpace.h"

#include <map>
#include <memory>
#include <algorithm>

#include "logger.h"

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

struct SubExpression {
    SubExpression(ExprAtomPtr expr, ExprAtomPtr parent, int index)
        : expr(expr) , parent(parent), index(index) { }
    ExprAtomPtr expr;
    ExprAtomPtr parent;
    int index;
};

class ExpressionSimplifier : public GroundedAtom {
public:

    ExpressionSimplifier(GroundingSpace const& kb, ExprAtomPtr expr)
        : kb(kb), full(expr) { parse(expr, nullptr, 0); }
    ExpressionSimplifier(GroundingSpace const& kb, ExprAtomPtr full,
            std::vector<SubExpression> subs)
        : kb(kb), full(full), subs(subs) {}

    void execute(GroundingSpace const& args, GroundingSpace& result) const override;

    bool operator==(Atom const& _other) const override {
        ExpressionSimplifier const* other = dynamic_cast<ExpressionSimplifier const*>(&_other);
        return other && *full == *(other->full);
    }

    std::string to_string() const override {
        return "simplify " + full->to_string();
    }

private:
    void parse(ExprAtomPtr expr, ExprAtomPtr parent, int index);
    std::shared_ptr<ExpressionSimplifier> pop_sub(SubExpression sub, AtomPtr replacement) const;

    GroundingSpace const& kb;
    ExprAtomPtr full;
    std::vector<SubExpression> subs;
};

void ExpressionSimplifier::parse(ExprAtomPtr expr, ExprAtomPtr parent, int index) {
    subs.emplace_back(expr, parent, index);
    auto const& children = expr->get_children();
    for (int i = 0; i < children.size(); ++i) {
        AtomPtr child = children[i];
        if (child->get_type() == Atom::EXPR) {
            parse(std::dynamic_pointer_cast<ExprAtom>(child), expr, i);
        }
    }
}

static bool interpret_plain_expression(GroundingSpace const& kb, ExprAtomPtr expr, GroundingSpace& result) {
    AtomPtr op = expr->get_children()[0];
    if (op->get_type() == Atom::GROUNDED) {
        GroundedAtom const* func = static_cast<GroundedAtom const*>(op.get());
        // TODO: How should we return results of the execution? At the moment they
        // are put into current atomspace. Should we return new child atomspace
        // instead?
        auto children = expr->get_children();
        // FIXME: temporary hack: if grounded atom has variables don't execute it
        bool has_variables = std::any_of(children.cbegin(), children.cend(),
                [](auto const& child) -> bool { return child->get_type() == Atom::VARIABLE; });
        if (!has_variables) {
            GroundingSpace args(children);
            clog::trace << "interpret_plain_expression(): executing atom, args: \""
                << args.to_string() << "\"" << std::endl;
            func->execute(args, result);
            clog::trace << "interpret_plain_expression(): executing atom, result: \""
                << result.to_string() << "\"" << std::endl;
            return true;
        }
        return false;
    } else {
        clog::debug << "interpret_plain_expression(): looking for expression in KB: "
            << expr->to_string() << std::endl;
        GroundingSpace pattern({ E({ S("="), expr, V("X") }) });
        GroundingSpace templ({ V("X") });
        GroundingSpace tmp;
        kb.match(pattern, templ, tmp);
        clog::trace << "interpret_plain_expression(): matching result: "<< tmp.to_string() << std::endl;
        for (auto const& item : tmp.get_content()) {
            result.add_atom(item);
        }
        return !tmp.get_content().empty();
    }
}

void ExpressionSimplifier::execute(GroundingSpace const& args, GroundingSpace& result) const {
    SubExpression const& sub = subs.back();
    if (!sub.parent) {
        clog::debug << "ExpressionSimplifier.execute(): full expression: " << sub.expr->to_string() << std::endl;
        if (!interpret_plain_expression(kb, sub.expr, result)) {
            result.add_atom(sub.expr);
        }
    } else {
        clog::debug << "ExpressionSimplifier.execute(): sub expression: " << sub.expr->to_string() << std::endl;
        GroundingSpace tmp;
        bool success = interpret_plain_expression(kb, sub.expr, tmp);
        if (!success) {
            tmp.add_atom(sub.expr);
        }
        // FIXME: implement by duplicating root of the plain_expr_result, and
        // replacing plain_expr by each item of content and push it back to the
        // content collection.
        if (tmp.get_content().size() == 0) {
            throw std::logic_error("This case is not implemented yet: "
                    "no results");
        }
        if (success) {
            AtomPtr replacement = tmp.get_content()[0];
            sub.parent->get_children()[sub.index] = replacement;
            result.add_atom(E({ pop_sub(sub, replacement) }));
        } else {
            result.add_atom(E({ pop_sub(sub, Atom::INVALID) }));
        }
    }
}

std::shared_ptr<ExpressionSimplifier> ExpressionSimplifier::pop_sub(SubExpression sub, AtomPtr tail) const {
    // TODO: replace copy by reusing array with variable containing size
    std::vector<SubExpression> copy = subs;
    copy.pop_back();
    if (tail && tail->get_type() == Atom::EXPR) {
        ExprAtomPtr expr = std::dynamic_pointer_cast<ExprAtom>(tail);
        auto ptr = std::make_shared<ExpressionSimplifier>(kb, full, copy);
        ptr->parse(expr, sub.parent, sub.index);
        return ptr;
    } else {
        return std::make_shared<ExpressionSimplifier>(kb, full, copy);
    }
}

static bool is_plain(ExprAtomPtr expr) {
    for (auto const& child : expr->get_children()) {
        if (child->get_type() == Atom::EXPR) {
            return false;
        }
    }
    return true;
}

AtomPtr GroundingSpace::interpret_step(SpaceAPI const& _kb) {
    if (_kb.get_type() != GroundingSpace::TYPE) {
        throw std::runtime_error("Only " + GroundingSpace::TYPE +
                " knowledge bases are supported");
    }
    GroundingSpace const& kb = static_cast<GroundingSpace const&>(_kb);

    if (content.empty()) {
        return S("eos");
    }

    AtomPtr atom = content.back();
    content.pop_back();
    clog::debug << "interpret_step(): atom on top: " << atom->to_string() << std::endl;
    if (atom->get_type() != Atom::EXPR) {
        return atom;
    }

    ExprAtomPtr expr = std::dynamic_pointer_cast<ExprAtom>(atom);
    if (is_plain(expr)) {
        clog::trace << "interpret_step(): handle plain expression" << std::endl;
        bool success = interpret_plain_expression(kb, expr, *this);
        // FIXME: if it is an expression which cannot be simplified this method
        // returns ExpressionSimplifier expression
        return success ? Atom::INVALID : atom;
    } else {
        clog::trace << "interpret_step(): prepare to simplify expression" << std::endl;
        content.push_back(E({std::make_shared<ExpressionSimplifier>(kb, expr)}));
        return Atom::INVALID;
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

static void apply_match_to_templ(GroundingSpace& results,
        std::vector<AtomPtr> const& templ, MatchResult const& match) {
    for (auto const& atom : templ) {
        AtomPtr result = apply_match_to_atom(atom, match.b_bindings);
        clog::trace << "apply_match_to_templ(): result: " << result->to_string() << std::endl;
        results.add_atom(result);
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
    clog::debug << "match: pattern: " << pattern.to_string() <<
        ", templ: " << templ.to_string() << std::endl;
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

