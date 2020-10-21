#include <cxxtest/TestSuite.h>
#include <memory>
#include <iostream>

#include <hyperon/hyperon.h>
#include <hyperon/common/common.h>

class GroundedSymbolTest : public CxxTest::TestSuite {
public:

    void test_plus_int() {
        GroundingSpace targets;
        targets.add_atom(E({ADD, Int(1), Int(2)}));

        AtomPtr result = interpret_until_result(targets, GroundingSpace());

        TS_ASSERT(*result == *Int(3));
    }

    void test_plus_float_in_text_space() {
        TextSpace text_kb;
        text_kb.register_token(std::regex("\\d+(\\.\\d+)?"),
                [] (std::string str) -> GroundedAtomPtr { return Float(std::stof(str)); });
        text_kb.register_token(std::regex("\\+"),
                [] (std::string str) -> GroundedAtomPtr { return ADD; });
        text_kb.add_string("(+ 2.0 1.0)");
        GroundingSpace targets;
        targets.add_from_space(text_kb);

        AtomPtr result = interpret_until_result(targets, GroundingSpace());
        
        TS_ASSERT(*result == *Float(3.0));
    }

    void test_float_and_string_in_text_space() {
        TextSpace text_kb;
        text_kb.register_token(std::regex("\\d+(\\.\\d+)?"),
                [] (std::string str) -> GroundedAtomPtr { return Float(std::stof(str)); });
        text_kb.register_token(std::regex("\"[^\"]*\""),
                [] (std::string str) -> GroundedAtomPtr {
                    return String(str.substr(1, str.size() - 2));    
                });
        text_kb.register_token(std::regex("\\+\\+"),
                [] (std::string str) -> GroundedAtomPtr {
                    return CONCAT;
                });
        text_kb.register_token(std::regex("\\+"),
                [] (std::string str) -> GroundedAtomPtr {
                    return ADD;    
                });
        text_kb.add_string("(= (a) 1.0)");
        text_kb.add_string("(= (x) (+ (a) 2.0))");
        text_kb.add_string("(= (b) \"1\")");
        text_kb.add_string("(= (y) (++ (b) \"2\"))");
        GroundingSpace kb;
        kb.add_from_space(text_kb);
        GroundingSpace targets;
        targets.add_atom(E({ S("x") }));
        targets.add_atom(E({ S("y") }));

        AtomPtr result = interpret_until_result(targets, kb);
        TS_ASSERT(*result == *String("12"));
        result = interpret_until_result(targets, kb);
        TS_ASSERT(*result == *Float(3.0))
    }
};
