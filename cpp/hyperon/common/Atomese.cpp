#include "Atomese.h"

#include "GroundedArithmetic.h"
#include "GroundedLogic.h"

void Atomese::parse(std::string program, GroundingSpace& kb) const {
    TextSpace parser;
    init_parser(parser);
    parser.add_string(program);
    kb.add_from_space(parser);
}

static void register_token_string_regex(TextSpace& parser, std::string regex, TextSpace::AtomConstr constr) {
    parser.register_token(std::regex(regex), constr);
}

static void register_token_without_params(TextSpace& parser, std::string regex, AtomPtr atom) {
    register_token_string_regex(parser, regex, [atom](std::string) -> AtomPtr { return atom; });
}

void Atomese::init_parser(TextSpace& parser) const {
    register_token_without_params(parser, "\\+", ADD);
    register_token_without_params(parser, "\\-", SUB);
    register_token_without_params(parser, "\\*", MUL);
    register_token_without_params(parser, "\\/", DIV);
    register_token_without_params(parser, "==", EQ);
    register_token_string_regex(parser, "\\d+(\\.\\d+)", [](std::string token) -> AtomPtr{
                return Float(::atof(token.c_str()));
            });
    register_token_string_regex(parser, "\\d+", [](std::string token) -> AtomPtr{
                return Int(::atoi(token.c_str()));
            });
}
