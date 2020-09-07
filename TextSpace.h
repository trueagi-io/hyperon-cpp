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

    using AtomConstr = std::function<AtomPtr(std::string)>;
    using TokenDescr = std::pair<std::regex, AtomConstr>;

    virtual ~TextSpace() { }

    void add_to(SpaceAPI& space) const override;

    void add_native(const SpaceAPI* other) override {
        throw std::logic_error("Method is not implemented");
    }

    std::string get_type() const override { return TYPE; }

    void add_string(std::string str_atom) {
        code.push_back(str_atom);
    }

    // TODO: We could make this method static and allow registering tokens
    // globally, but on the other hand when it is not static we can register
    // separate set of tokens in each TextSpace and allow using different
    // parsers in parallel. Last solution looks more flexible. We could also
    // pass list of tokens into TextSpace constructor.
    void register_token(std::regex regex, AtomConstr constructor) {
        tokens.push_back(TokenDescr(regex, constructor));
    }

private:

    struct ParseResult;

    AtomPtr find_token(std::string token) const;
    ParseResult  recursive_parse(char const* text, char const*& pos) const;
    void parse(std::string text, std::function<void(AtomPtr)> add) const;

    std::vector<std::string> code; 
    std::vector<TokenDescr> tokens;
};

#endif /* TEXT_SPACE_H */
