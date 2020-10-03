#include <cxxtest/TestSuite.h>
#include <memory>
#include <iostream>

#include <hyperon/GroundingSpace.h>
#include <hyperon/TextSpace.h>

#include "common.h"

class StringAtom : public ValueAtom<std::string> {
public:
    StringAtom(std::string value) : ValueAtom(value) {}
    virtual ~StringAtom() {}
    std::string to_string() const override { return "\"" + get() + "\""; }
};

std::shared_ptr<StringAtom> String(std::string str) { return std::make_shared<StringAtom>(str); }

class GroundedSymbolTest : public CxxTest::TestSuite {
public:

    void test_simple_grounded_operation() {
        GroundingSpace targets;
        targets.add_atom(E({Plus(), Int(1), Int(2)}));

        AtomPtr result = interpret_until_result(targets, GroundingSpace());

        TS_ASSERT(*result == *Int(3));
    }

    void test_adding_new_grounded_types() {
        TextSpace text_kb;
        text_kb.register_token(std::regex("\\d+(\\.\\d+)?"),
                [] (std::string str) -> GroundedAtomPtr { return Float(std::stof(str)); });
        text_kb.register_token(std::regex("\\+"),
                [] (std::string str) -> GroundedAtomPtr { return Plus(); });
        text_kb.add_string("(+ 2.0 1.0)");
        GroundingSpace targets;
        targets.add_from_space(text_kb);

        AtomPtr result = interpret_until_result(targets, GroundingSpace());
        
        TS_ASSERT(*result == *Float(3.0));
    }

    // FIXME: test is not passed yet
    void _test_two_grounded_types() {
        TextSpace text_kb;
        text_kb.register_token(std::regex("\\d+(\\.\\d+)?"),
                [] (std::string str) -> GroundedAtomPtr { return Float(std::stof(str)); });
        text_kb.register_token(std::regex("\"[^\"]*\""),
                [] (std::string str) -> GroundedAtomPtr {
                    return String(str.substr(1, str.size() - 2));    
                });
        text_kb.register_token(std::regex("\\+"),
                [] (std::string str) -> GroundedAtomPtr {
                    return std::make_shared<PlusAtom>();    
                });
        text_kb.add_string("(= $a 1.0)");
        text_kb.add_string("(= $x (+ $a 2.0))");
        text_kb.add_string("(= $b \"1\")");
        text_kb.add_string("(= $y (+ $b \"2\"))");
        GroundingSpace kb;
        kb.add_from_space(text_kb);

        GroundingSpace targets;
        targets.add_atom(V("x"));
        targets.add_atom(V("y"));
        targets.interpret_step(kb);
        targets.interpret_step(kb);
        TS_ASSERT(targets == GroundingSpace({ Float(3.0), std::make_shared<StringAtom>("12") }));
    }
};
