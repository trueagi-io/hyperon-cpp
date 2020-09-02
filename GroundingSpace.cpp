#include "GroundingSpace.h"

// Expression

ExprPtr Expr::INVALID = std::shared_ptr<Expr>(nullptr);

std::string to_string(Expr::Type type) {
    static std::string names[] = { "S", "G", "C", "V" };
    return names[type];
}

bool operator==(std::vector<ExprPtr> const& a, std::vector<ExprPtr> const& b) {
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

std::string to_string(std::vector<ExprPtr> const& exprs, std::string delimiter) {
    std::string str = "";
    for (auto it = exprs.begin(); it != exprs.end(); ++it) {
        str += (it == exprs.begin() ? "" : delimiter) + (*it)->to_string();
    }
    return str;
}

bool CompositeExpr::operator==(Expr const& _other) const { 
    if (_other.get_type() != COMPOSITE) {
        return false;
    }
    CompositeExpr const& other = static_cast<CompositeExpr const&>(_other);
    return children == other.children;
}

// Grounding space

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

ExprPtr GroundingSpace::interpret_step(SpaceAPI const& _kb) {
    if (_kb.get_type() != GroundingSpace::TYPE) {
        throw std::runtime_error("Only " + GroundingSpace::TYPE +
                " knowledge bases are supported");
    }
    GroundingSpace const& kb = static_cast<GroundingSpace const&>(_kb);

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

bool GroundingSpace::operator==(SpaceAPI const& _other) const {
    if (_other.get_type() != GroundingSpace::TYPE) {
        return false;
    }
    GroundingSpace const& other = static_cast<GroundingSpace const&>(_other);
    return content == other.content;
}

