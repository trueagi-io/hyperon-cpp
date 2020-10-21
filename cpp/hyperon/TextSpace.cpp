#include <regex>
#include <stdexcept>

#include "TextSpace.h"

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

AtomPtr TextSpace::find_token(char const*& text) const {
    for (auto const& pair : tokens) {
        std::cmatch match;
        if (std::regex_search(text, match, pair.first, std::regex_constants::match_continuous)) {
            text += match.length();
            return pair.second(match.str());
        }
    }
    return Atom::INVALID;
}

struct TextSpace::ParseResult {
    AtomPtr atom;
    bool is_eof;
};

TextSpace::ParseResult  TextSpace::recursive_parse(char const* text, char const*& pos) const {
    skip_space(pos);
    switch (*pos) {
        case '$':
            ++pos;
            return { V(next_token(pos)), false };
        case '(':
            {
                ++pos;
                std::vector<AtomPtr> children;
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
                    children.push_back(result.atom);
                }
                AtomPtr atom = E(children);
                return { atom, false };
            }
        case '\0':
            return { Atom::INVALID, true };
        default:
            {
                AtomPtr atom = find_token(pos);
                if (atom) {
                    return { atom, false };
                } else {
                    std::string token = next_token(pos);
                    return { S(token), false };
                }
            }
    };
}

void TextSpace::parse(std::string text, std::function<void(AtomPtr)> add) const {
    char const* c_str = text.c_str();
    char const* pos = c_str;
    while (true) {
        ParseResult result = recursive_parse(c_str, pos);
        if (result.is_eof) {
            break;
        }
        add(result.atom);
    }
}

void TextSpace::add_to(SpaceAPI& _space) const {
    if (_space.get_type() == GroundingSpace::TYPE) {
        GroundingSpace& space = static_cast<GroundingSpace&>(_space);
        for (auto const& str_atom : code) {
            parse(str_atom, [&space] (AtomPtr atom) -> void { space.add_atom(atom); });
        }
    } else {
        SpaceAPI::add_to(_space);
    }
}
