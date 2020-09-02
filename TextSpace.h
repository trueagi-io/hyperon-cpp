#ifndef TEXT_SPACE_H
#define TEXT_SPACE_H

#include <vector>
#include <functional>
#include <regex>

#include "SpaceAPI.h"
#include "GroundingSpace.h"

// Text space

class TextSpace : public SpaceAPI {
public:

    static std::string TYPE;

    using ExprConstr = std::function<ExprPtr(std::string)>;
    using TokenDescr = std::pair<std::regex, ExprConstr>;

    virtual ~TextSpace() { }

    void add_to(SpaceAPI& space) const;

    void add_native(const SpaceAPI* other) {
        throw std::logic_error("Method is not implemented");
    }

    std::string get_type() const { return TYPE; }

    void add_string(std::string str_expr) {
        code.push_back(str_expr);
    }

    void register_token(std::regex regex,
            ExprConstr constructor) {
        tokens.push_back(TokenDescr(regex, constructor));
    }

    ExprPtr find_token(std::string token) const;

private:
    struct ParseResult {
        ExprPtr expr;
        bool is_eof;
    };

    ParseResult  recursive_parse(char const* text, char const*& pos) const;
    void parse(std::string text, std::function<void(ExprPtr)> add) const;

    std::vector<std::string> code; 
    std::vector<TokenDescr> tokens;
};

#endif /* TEXT_SPACE_H */
