#include "GroundingSpace.h"

#include <map>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <functional>

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

bool add_binding(Bindings& bindings, AtomPtr _var, AtomPtr value) {
    VariableAtomPtr var = std::static_pointer_cast<VariableAtom>(_var);
    auto cur = bindings.find(var);
    if (cur != bindings.end()) {
        return *(cur->second) == *value;
    } 
    bindings[var] = value;
    return true;
}

static bool match_atoms(AtomPtr a, AtomPtr b, MatchBindings& match) {
    // TODO: it is not clear how should we handle the case when a and b are
    // both variables. We can check variable name equality and skip binding. We
    // can add a as binding for b and vice versa.
    if (b->get_type() == Atom::VARIABLE) {
        return add_binding(match.b_bindings, b, a);
    }
    switch (a->get_type()) {
        case Atom::SYMBOL:
        case Atom::GROUNDED:
            return *a == *b;
        case Atom::VARIABLE:
            return add_binding(match.a_bindings, a, b);
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
        LOG_DEBUG << "result: " << result->to_string() << std::endl;
        target.add_atom(result);
    }
}

std::vector<Bindings> GroundingSpace::match(AtomPtr pattern) const {
    std::vector<Bindings> result;
    LOG_DEBUG << "pattern: " << pattern->to_string() << std::endl;
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
    LOG_DEBUG << "pattern: " << pattern.to_string() <<
        ", templ: " << templ.to_string() << std::endl;
    AtomPtr pattern_atom = pattern.content[0];
    std::vector<Bindings> results = match(pattern_atom);
    for (auto const& result : results) {
        apply_bindings_to_templ(target, templ.content, result);
    }
}

// Unify

// FIXME: depth - is a hack for implementing unification with (= a b)
// correctly; it should not be implemented here but on the caller level to keep
// unify_atoms code clean
bool unify_atoms(AtomPtr a, AtomPtr b, UnificationResult& result, int depth=0) {
    // TODO: it is not clear how should we handle the case when a and b are
    // both variables. We can check variable name equality and skip binding. We
    // can add a as binding for b and vice versa.
    if (b->get_type() == Atom::VARIABLE) {
        // FIXME: hardcoding V("X") below is a hack to make work matching for
        // (= (plus Z $y) $y) and (= (plus Z $n) $X), otherwise $y cannot be
        // bound to $n and $X at same time, but bounding it to $X doesn't make
        // sense anyway
        if (a->get_type() == Atom::VARIABLE && *b != *V("X")) {
            return add_binding(result.a_bindings, a, b)
                && add_binding(result.b_bindings, b, a);
        } else {
            return add_binding(result.b_bindings, b, a);
        }
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
        return add_binding(result.a_bindings, a, b);
    case Atom::EXPR:
        if (b->get_type() == Atom::EXPR) {
            ExprAtomPtr expr_a = std::static_pointer_cast<ExprAtom>(a);
            ExprAtomPtr expr_b = std::static_pointer_cast<ExprAtom>(b);
            if (expr_a->get_children().size() != expr_b->get_children().size()) {
                if (depth == 1) {
                    return false;
                }
                result.unifications.emplace_back(a, b);
                return true;
            }
            for (int i = 0; i < expr_a->get_children().size(); ++i) {
                if (!unify_atoms(expr_a->get_children()[i], expr_b->get_children()[i], result, depth+1)) {
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

std::vector<UnificationResult> GroundingSpace::unify(AtomPtr atom) const {
    LOG_DEBUG << "match and unify atom: " << atom->to_string() << std::endl;
    std::vector<UnificationResult> all_unifications;
    for (auto const& candidate : get_content()) {
        UnificationResult result;
        if (!unify_atoms(candidate, atom, result)) {
            LOG_TRACE << "candidate: " << candidate->to_string() << ": fail" << std::endl;
            continue;
        }
        LOG_DEBUG << "candidate: " << candidate->to_string() << ": ok" << std::endl;
        result.b_bindings = apply_bindings_to_bindings(result.a_bindings,
                result.b_bindings);
        apply_bindings_to_unifications(result);
        all_unifications.push_back(result);
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
    GroundingSpace args(children);
    LOG_DEBUG << "args: \"" << args.to_string() << "\"" << std::endl;
    GroundingSpace results;
    try {
        func->execute(args, results);
    } catch (...) {
        // FIXME: we should print the error here, but for doing this we need to
        // add new type for error; this is the case for
        // IllegalArgumentExpression analogue
        LOG_DEBUG << "error while executing expression" << std::endl;
        return { true, std::vector<AtomPtr>() };
    }
    LOG_DEBUG << "results: \"" << results.to_string() << "\"" << std::endl;
    return { true, results.get_content() };
}

static AtomPtr match_plain_nongrounded_expression(GroundingSpace const& kb, ExprAtomPtr expr, AtomPtr templ, GroundingSpace& target) {
    LOG_DEBUG << "looking for expression in KB: " << expr->to_string() << std::endl;
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
        LOG_DEBUG << "full expression: " << sub.expr->to_string() << std::endl;
        AtomPtr non_interpretable = interpret_full_expression(kb, sub.expr, target);
        if (non_interpretable) {
            target.add_atom(non_interpretable);
        }
    } else {
        LOG_DEBUG << "sub expression: " << sub.expr->to_string() << std::endl;
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

class IfMatchAtom : public GroundedAtom {
public:
    IfMatchAtom() {}
    virtual ~IfMatchAtom() {}

    void execute(GroundingSpace const& args, GroundingSpace& result) const override {
        AtomPtr a = args.get_content()[1];
        AtomPtr b = args.get_content()[2];
        MatchBindings match;
        if (match_atoms(a, b, match)) {
            AtomPtr c = args.get_content()[3];
            c = apply_bindings_to_atom(c, match.a_bindings);
            c = apply_bindings_to_atom(c, match.b_bindings);
            result.add_atom(c);
        }
    }

    bool operator==(Atom const& other) const override { return this == &other; }
    std::string to_string() const override { return "ifmatch"; }
};

const GroundedAtomPtr IFMATCH = std::make_shared<IfMatchAtom>();

const SymbolAtomPtr REDUCT = S("reduct");
// FIXME: make AT symbol more unique
const SymbolAtomPtr AT = S("@");

static bool find_next_expr(std::vector<AtomPtr>::iterator& it,
        std::vector<AtomPtr>::const_iterator end) {
    while (it != end) {
        if ((*it)->get_type() == Atom::EXPR) {
            return true;
        }
        it++;
    }
    return false;
}

static AtomPtr reduct_first_arg(ExprAtomPtr expr) {
    std::vector<AtomPtr> children = expr->get_children();
    auto it = children.begin();
    if (!find_next_expr(it, children.end())) {
        throw std::runtime_error("Could not find first expression argument");
    }
    AtomPtr arg = *it;
    *it = AT;
    return E({REDUCT, arg, E(children)});
}

static AtomPtr reduct_next_arg(ExprAtomPtr expr, AtomPtr value) {
    std::vector<AtomPtr> children = expr->get_children();
    auto it = children.begin();
    bool ifmatch = *it == IFMATCH;
    while (it != children.end()) {
        if (*it == AT) {
            *it = value;
            break;
        }
        it++;
    }
    if (it == expr->get_children().end()) {
        throw std::runtime_error("Could not find placeholder to replace by value");
    }
    it++;
    if (find_next_expr(it, children.end()) && (!ifmatch || it <= (children.begin() + 2))) {
        AtomPtr arg = *it;
        *it = AT;
        return E({REDUCT, arg, E(children)});
    } else {
        return E({REDUCT, E(children)});
    }
}

static AtomPtr generate_if_eq_recursively(Unifications::const_reverse_iterator i,
        Unifications::const_reverse_iterator end, AtomPtr expr) {
    if (i == end) {
        return expr;
    }
    return E({IFMATCH, i->a, i->b, generate_if_eq_recursively(i + 1, end, expr)});
}

static AtomPtr unification_result_to_expr(UnificationResult const& unification_result,
        VariableAtomPtr var) {
    auto expr = unification_result.b_bindings.at(var);
    auto it = unification_result.unifications.crbegin();
    return generate_if_eq_recursively(it, unification_result.unifications.crend(), expr);
}

static AtomPtr interpret_expr_step(GroundingSpace const& kb,
    AtomPtr atom, bool reducted, std::function<void(AtomPtr, Bindings const*)> callback) {
    LOG_DEBUG << "interpreting atom: " << atom->to_string() << std::endl;
    if (atom->get_type() != Atom::EXPR) {
        return atom;
    }
    ExprAtomPtr expr = std::static_pointer_cast<ExprAtom>(atom);
    AtomPtr op = expr->get_children()[0];
    if (op == REDUCT) {
        AtomPtr sub_expr = expr->get_children()[1];
        if (expr->get_children().size() < 3) {
            LOG_DEBUG << "interpreting expression after reduction" << std::endl;
            return interpret_expr_step(kb, sub_expr,
                    true, [&callback](AtomPtr result, Bindings const* bindings) -> void {
                        callback(result, bindings);
                    });
        } else {
            LOG_DEBUG << "interpret sub expression" << std::endl;
            ExprAtomPtr full_expr = std::static_pointer_cast<ExprAtom>(expr->get_children()[2]);
            AtomPtr result = interpret_expr_step(kb, sub_expr,
                    false, [&callback, &full_expr](AtomPtr result, Bindings const* bindings) -> void {
                        AtomPtr applied = full_expr;
                        if (bindings) {
                            LOG_DEBUG << "apply bindings to full_expr" << std::endl;
                            applied = apply_bindings_to_atom(full_expr, *bindings);
                        }
                        callback(E({ REDUCT, result, applied }), bindings);
                    });
            if (result) {
                LOG_DEBUG << "sub expression is not interpretable" << std::endl;
                callback(reduct_next_arg(full_expr, result), nullptr);
            }
            return Atom::INVALID;
        }
    } else if (is_grounded_expression(expr)) {
        LOG_DEBUG << "executing grounded expression" << std::endl;
        if (is_plain_expression(expr) || reducted) {
            LOG_DEBUG << "executing " << (reducted ? "reducted" : "plain") <<
                " grounded expression" << std::endl;
            ExecutionResult result = execute_grounded_expression(expr);
            if (result.success) {
                for (auto const& result : result.results) {
                    LOG_DEBUG << "execution result: " << result->to_string() << std::endl;
                    callback(result, nullptr);
                }
                return Atom::INVALID;
            } else {
                LOG_DEBUG << "cannot execute expression" << std::endl;
                return expr;
            }
        } else {
            LOG_DEBUG << "reducting expression" << std::endl;
            callback(reduct_first_arg(expr), nullptr);
            return Atom::INVALID;
        }
    } else {
        LOG_DEBUG << "interpreting symbolic expression" << std::endl;
        // FIXME: replace V("X") by UniqueVar
        VariableAtomPtr var = V("X");
        std::vector<UnificationResult> results = kb.unify(E({S("="), expr, var}));
        if (results.empty()) {
            LOG_DEBUG << "unification is not found" << std::endl;
            if (is_plain_expression(expr) || reducted) {
                LOG_DEBUG << (reducted ? "reducted" : "plain") <<
                    " symbolic expression is not interpretable" << std::endl;
                return expr;
            } else {
                LOG_DEBUG << "reducting expression" << std::endl;
                callback(reduct_first_arg(expr), nullptr);
                return Atom::INVALID;
            }
        } else {
            LOG_DEBUG << "adding unification results" << std::endl; 
            for (auto const& result : results) {
                auto expr = result.b_bindings.find(var);
                // TODO: the situation when (= ... $X) matched symbol not
                // expression
                if (expr != result.b_bindings.end()) {
                    callback(unification_result_to_expr(result, var), &result.b_bindings);
                }
            }
            return Atom::INVALID;
        }
    }
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
    LOG_DEBUG << "next atom: " << atom->to_string() << std::endl;
    return interpret_expr_step(kb, atom, false, [this](AtomPtr result, Bindings const* bindings) -> void {
                LOG_DEBUG << "push atom: " << result->to_string() << std::endl;
                this->content.push_back(result);
            });
}

bool GroundingSpace::operator==(SpaceAPI const& _other) const {
    if (_other.get_type() != GroundingSpace::TYPE) {
        return false;
    }
    GroundingSpace const& other = static_cast<GroundingSpace const&>(_other);
    return content == other.content;
}

