#include "GroundingSpace.h"

#include <map>
#include <memory>
#include <algorithm>
#include <stdexcept>

#include "logger_priv.h"

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

// Match

struct MatchBindings {
    Bindings a_bindings;
    Bindings b_bindings;
};

// FIXME: if variable matched twice it should be checked the second match is
// equal to the first one.
static bool match_atoms(AtomPtr a, AtomPtr b, MatchBindings& match) {
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

static AtomPtr apply_bindings_to_atom(AtomPtr const& atom, Bindings const& bindings) {
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
                    AtomPtr applied = apply_bindings_to_atom(atom, bindings);
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

static Bindings apply_bindings_to_bindings(Bindings from, Bindings to) {
    Bindings result;
    for (auto const& pair : to) {
        AtomPtr applied = apply_bindings_to_atom(pair.second, from);
        result[pair.first] = applied;
    }
    return result;
}

static void apply_bindings_to_templ(GroundingSpace& target,
        std::vector<AtomPtr> const& templ, Bindings const& bindings) {
    for (auto const& atom : templ) {
        AtomPtr result = apply_bindings_to_atom(atom, bindings);
        clog::debug << __func__ << ": result: " << result->to_string() << std::endl;
        target.add_atom(result);
    }
}

std::vector<Bindings> GroundingSpace::match(AtomPtr pattern) const {
    std::vector<Bindings> result;
    clog::debug << __func__ << ": pattern: " << pattern->to_string() << std::endl;
    for (auto const& match : get_content()) {
        MatchBindings bindings;
        if (!match_atoms(match, pattern, bindings)) {
            continue;
        }
        bindings.b_bindings = apply_bindings_to_bindings(bindings.a_bindings,
                bindings.b_bindings);
        result.emplace_back(bindings.b_bindings);
    }
    return result;
}

void GroundingSpace::match(SpaceAPI const& _pattern, SpaceAPI const& _templ, GroundingSpace& target) const {
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
    clog::debug << __func__ << ": pattern: " << pattern.to_string() <<
        ", templ: " << templ.to_string() << std::endl;
    AtomPtr pattern_atom = pattern.content[0];
    std::vector<Bindings> results = match(pattern_atom);
    for (auto const& result : results) {
        apply_bindings_to_templ(target, templ.content, result);
    }
}

// Unify

struct UnificationResult {
    Bindings a_bindings;
    Bindings b_bindings;
    Unifications unifications;
};

bool unify_atoms(AtomPtr a, AtomPtr b, UnificationResult result) {
    // TODO: it is not clear how should we handle the case when a and b are
    // both variables. We can check variable name equality and skip binding. We
    // can add a as binding for b and vice versa.
    if (b->get_type() == Atom::VARIABLE) {
        VariableAtomPtr var_b = std::static_pointer_cast<VariableAtom>(b);
        result.b_bindings[var_b] = a;
        return true;
    }
    switch (a->get_type()) {
    case Atom::SYMBOL:
    case Atom::GROUNDED:
        if (b->get_type() == Atom::SYMBOL || b->get_type() == Atom::GROUNDED) {
            return *a == *b;
        }
        result.unifications.emplace_back(a, b);
        return true;
    case Atom::VARIABLE:
        {
            VariableAtomPtr var_a = std::static_pointer_cast<VariableAtom>(a);
            result.a_bindings[var_a] = b;
            return true;
        }
    case Atom::EXPR:
        if (b->get_type() == Atom::EXPR) {
            ExprAtomPtr expr_a = std::static_pointer_cast<ExprAtom>(a);
            ExprAtomPtr expr_b = std::static_pointer_cast<ExprAtom>(b);
            if (expr_a->get_children().size() != expr_b->get_children().size()) {
                return false;
            }
            for (int i = 0; i < expr_a->get_children().size(); ++i) {
                if (!unify_atoms(expr_a->get_children()[i], expr_b->get_children()[i], result)) {
                    return false;
                }
            }
        } else {
            result.unifications.emplace_back(a, b);
        }
        return true;
    default:
        throw std::logic_error("Not implemented for type: " +
                to_string(a->get_type()));
    }
}

static void apply_bindings_to_unifications(UnificationResult& result) {
    Unifications applied;
    for (const auto& unification : result.unifications) {
        AtomPtr a = apply_bindings_to_atom(unification.a, result.a_bindings);
        AtomPtr b = apply_bindings_to_atom(unification.b, result.b_bindings);
        applied.emplace_back(a, b);
    }
    result.unifications = applied;
}

std::vector<Unifications> GroundingSpace::unify(AtomPtr atom) const {
    std::vector<Unifications> all_unifications;
    for (auto const& candidate : get_content()) {
        UnificationResult result;
        if (!unify_atoms(candidate, atom, result)) {
            continue;
        }
        result.b_bindings = apply_bindings_to_bindings(result.a_bindings,
                result.b_bindings);
        apply_bindings_to_unifications(result);
        all_unifications.push_back(result.unifications);
    }
    return all_unifications; 
}

// Interpret

struct SubExpression {
    static int NO_PARENT;
    SubExpression(ExprAtomPtr expr, int parent_sub_index, int child_index)
        : expr(expr) , parent_sub_index(parent_sub_index), child_index(child_index) { }
    bool has_parent() const { return parent_sub_index != NO_PARENT; }
    ExprAtomPtr expr;
    int parent_sub_index;
    int child_index;
};

int SubExpression::NO_PARENT = -1;

class ExpressionReduction : public GroundedAtom {
public:

    ExpressionReduction(GroundingSpace const& kb, ExprAtomPtr expr)
        : kb(kb) { parse(expr, SubExpression::NO_PARENT, 0); }
    ExpressionReduction(GroundingSpace const& kb, ExprAtomPtr full,
            std::vector<SubExpression> subs)
        : kb(kb), subs(subs) {}

    void execute(GroundingSpace const& args, GroundingSpace& result) const override;

    bool operator==(Atom const& _other) const override {
        ExpressionReduction const* other = dynamic_cast<ExpressionReduction const*>(&_other);
        return other && *full() == *(other->full());
    }

    std::string to_string() const override {
        return "reduction " + full()->to_string();
    }

private:
    void parse(ExprAtomPtr expr, int parent_sub_index, int child_index);
    std::shared_ptr<ExpressionReduction> pop_sub(SubExpression sub, AtomPtr replacement) const;
    ExprAtomPtr full() const { return subs[0].expr; }
    void replace_sub(SubExpression& sub, AtomPtr replacement);

    GroundingSpace const& kb;
    std::vector<SubExpression> subs;
};

void ExpressionReduction::parse(ExprAtomPtr expr, int parent_sub_index, int child_index) {
    int expr_sub_index = subs.size();
    subs.emplace_back(expr, parent_sub_index, child_index);
    auto const& children = expr->get_children();
    for (int i = 0; i < children.size(); ++i) {
        AtomPtr child = children[i];
        if (child->get_type() == Atom::EXPR) {
            parse(std::static_pointer_cast<ExprAtom>(child), expr_sub_index, i);
        }
    }
}

struct ExecutionResult {
    bool success;
    std::vector<AtomPtr> results;
};

static bool is_grounded_expression(ExprAtomPtr expr) {
    return expr->get_children()[0]->get_type() == Atom::GROUNDED;
}

static ExecutionResult execute_grounded_expression(ExprAtomPtr expr) {
    GroundedAtom const* func = static_cast<GroundedAtom const*>(expr->get_children()[0].get());
    // TODO: How should we return results of the execution? At the moment they
    // are put into current atomspace. Should we return new child atomspace
    // instead?
    auto children = expr->get_children();
    // FIXME: temporary hack: if grounded atom has variables don't execute it
    bool has_variables = std::any_of(children.cbegin(), children.cend(),
            [](auto const& child) -> bool { return child->get_type() == Atom::VARIABLE; });
    if (!has_variables) {
        GroundingSpace args(children);
        clog::debug << __func__ << ": args: \"" << args.to_string() << "\"" << std::endl;
        GroundingSpace results;
        func->execute(args, results);
        clog::debug << __func__ << ": results: \"" << results.to_string() << "\"" << std::endl;
        return { true, results.get_content() };
    }
    clog::debug << __func__ << ": skip execution because atom has unbound variables as arguments" << std::endl;
    return { false };
}

static AtomPtr match_plain_nongrounded_expression(GroundingSpace const& kb, ExprAtomPtr expr, AtomPtr templ, GroundingSpace& target) {
    clog::debug << __func__ << ": looking for expression in KB: "
        << expr->to_string() << std::endl;
    std::vector<Bindings> results = kb.match(E({ S("="), expr, V("X") }));
    std::vector<AtomPtr> _templ({ templ });
    for (auto const& result : results) {
        apply_bindings_to_templ(target, _templ, result);
    }
    return results.empty() ? expr : Atom::INVALID;
}

static AtomPtr interpret_full_expression(GroundingSpace const& kb, ExprAtomPtr expr, GroundingSpace& target) {
    if (is_grounded_expression(expr)) {
        ExecutionResult result = execute_grounded_expression(expr);
        if (result.success) {
            for (auto const& result : result.results) {
                target.add_atom(result);
            }
            return Atom::INVALID;
        } else {
            return expr;
        }
    } else {
        return match_plain_nongrounded_expression(kb, expr, V("X"), target);
    }
}

void ExpressionReduction::execute(GroundingSpace const& args, GroundingSpace& target) const {
    SubExpression const& sub = subs.back();
    if (!sub.has_parent()) {
        clog::debug << __func__ << ": full expression: " << sub.expr->to_string() << std::endl;
        AtomPtr non_interpretable = interpret_full_expression(kb, sub.expr, target);
        if (non_interpretable) {
            target.add_atom(non_interpretable);
        }
    } else {
        clog::debug << __func__ << ": sub expression: " << sub.expr->to_string() << std::endl;
        if (is_grounded_expression(sub.expr)) {
            ExecutionResult result = execute_grounded_expression(sub.expr);
            if (result.success) {
                if (result.results.empty()) {
                    throw std::runtime_error("Grounded expression: " +
                            sub.expr->to_string() +
                            " returned nothing while being part of large expression: " +
                            full()->to_string());
                }
                for (auto const& result : result.results) {
                    target.add_atom(E({ pop_sub(sub, result) }));
                }
            } else {
                target.add_atom(E({ pop_sub(sub, Atom::INVALID) }));
            }
        } else {
            GroundingSpace results;
            // FIXME: hack temporary replace expr by variable to form pattern
            subs[sub.parent_sub_index].expr->get_children()[sub.child_index] = V("X");
            AtomPtr non_interpretable = match_plain_nongrounded_expression(kb, sub.expr, full(), results);
            subs[sub.parent_sub_index].expr->get_children()[sub.child_index] = sub.expr;
            if (non_interpretable) {
                target.add_atom(E({ pop_sub(sub, Atom::INVALID) }));
                return;
            }
            for (auto const& result : results.get_content()) {
                ExprAtomPtr expr = std::static_pointer_cast<ExprAtom>(result);
                // FIXME: ineffective we parse expression each time even if
                // no variables were replaced
                target.add_atom(E({std::make_shared<ExpressionReduction>(kb, expr)}));
            }   
        }
    }
}

void ExpressionReduction::replace_sub(SubExpression& sub, AtomPtr replacement) {
    SubExpression& parent_sub = subs[sub.parent_sub_index];
    ExprAtomPtr parent_copy = E(parent_sub.expr->get_children());  
    parent_copy->get_children()[sub.child_index] = replacement;
    if (parent_sub.has_parent()) {
        subs[sub.parent_sub_index].expr->get_children()[sub.child_index] = replacement;
    }
    parent_sub.expr = parent_copy;
}

std::shared_ptr<ExpressionReduction> ExpressionReduction::pop_sub(SubExpression sub, AtomPtr tail) const {
    // TODO: replace copy by reusing array with variable containing size
    std::vector<SubExpression> subs_copy = subs;
    subs_copy.pop_back();
    auto copy = std::make_shared<ExpressionReduction>(kb, full(), subs_copy);
    if (tail) {
        copy->replace_sub(sub, tail);
        if (tail->get_type() == Atom::EXPR) {
            ExprAtomPtr expr = std::static_pointer_cast<ExprAtom>(tail);
            copy->parse(expr, sub.parent_sub_index, sub.child_index);
            return copy;
        }
    }
    return copy;
}

static bool is_plain_expression(ExprAtomPtr expr) {
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
    clog::debug << __func__ << ": next atom to interpret: " << atom->to_string() << std::endl;
    if (atom->get_type() != Atom::EXPR) {
        return atom;
    }

    ExprAtomPtr expr = std::static_pointer_cast<ExprAtom>(atom);
    if (is_grounded_expression(expr)) {
        if (is_plain_expression(expr)) {
            clog::debug << __func__ << ": executing plain grounded expression" << std::endl;
            return interpret_full_expression(kb, expr, *this);
        } else {
            // reduct expression
        }
    } else {
        std::vector<Unifications> unifications = kb.unify(E({ S("="), expr, V("X") }));
        if (unifications.empty()) {
            if (is_plain_expression(expr)) {
                return expr;
            } else {
                // reduct expression
            }
        } else {
            clog::error << __func__ << ": " << "adding unification result" << std::endl; 
            content.push_back(E({std::make_shared<ExpressionReduction>(kb, expr)}));
            return Atom::INVALID;
        }
    }
    
    clog::error << __func__ << ": " << "reducting expression" << std::endl;
    content.push_back(E({std::make_shared<ExpressionReduction>(kb, expr)}));
    return Atom::INVALID;
}

bool GroundingSpace::operator==(SpaceAPI const& _other) const {
    if (_other.get_type() != GroundingSpace::TYPE) {
        return false;
    }
    GroundingSpace const& other = static_cast<GroundingSpace const&>(_other);
    return content == other.content;
}

