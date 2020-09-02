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

// Text space

std::string TextSpace::TYPE = "TextSpace";

static void skip_space(char const*& text) {
    while (*text && std::isspace(*text)) {
        ++text;
    }
}

static std::string next_token(char const*& text) {
    char const* start = text;
    // TODO: this doesn't work for string in quotes with spaces inside them,
    // to fix it we should made TokenDescr more complex and use list of token
    // descriptions to build a parser
    while (*text && !std::isspace(*text) && *text != '(' && *text != ')') {
        ++text;
    }
    return std::string(start, text);
}

static std::string show_position(char const* text, char const* pos) {
    return std::string(text, pos) + ">" + std::string(1, *pos) + "<" +
            (*pos ? std::string(pos + 1) : "");
}

static void parse_error(char const* text, char const* pos, std::string message) {
    throw std::runtime_error(message + "\n" + show_position(text, pos));
}

ExprPtr TextSpace::find_token(std::string token) const {
    for (auto const& pair : tokens) {
        if (std::regex_match(token, pair.first)) {
            return pair.second(token);
        }
    }
    return Expr::INVALID;
}

TextSpace::ParseResult  TextSpace::recursive_parse(char const* text, char const*& pos) const {
    skip_space(pos);
    switch (*pos) {
        case '$':
            ++pos;
            return { V(next_token(pos)), false };
        case '(':
            {
                ++pos;
                std::vector<ExprPtr> children;
                while (true) {
                    skip_space(pos);
                    if (*pos == ')') {
                        ++pos;
                        break;
                    }
                    ParseResult result = recursive_parse(text, pos);
                    if (result.is_eof) {
                        parse_error(text, pos, "Unexpected end of expression");
                    }
                    children.push_back(result.expr);
                }
                ExprPtr expr = C(children);
                return { expr, false };
            }
        case '\0':
            return { Expr::INVALID, true };
        default:
            {
                std::string token = next_token(pos);
                ExprPtr expr = find_token(token);
                if (expr) {
                    return { expr, false };
                } else {
                    return { S(token), false };
                }
            }
    };
}

void TextSpace::parse(std::string text, std::function<void(ExprPtr)> add) const {
    char const* c_str = text.c_str();
    char const* pos = c_str;
    while (true) {
        ParseResult result = recursive_parse(c_str, pos);
        if (result.is_eof) {
            break;
        }
        add(result.expr);
    }
}

void TextSpace::add_to(SpaceAPI& _space) const {
    if (_space.get_type() == GroundingSpace::TYPE) {
        GroundingSpace& space = static_cast<GroundingSpace&>(_space);
        for (auto const& str_expr : code) {
            parse(str_expr, [&space] (ExprPtr expr) -> void { space.add_expr(expr); });
        }
    } else {
        SpaceAPI::add_to(_space);
    }
}
