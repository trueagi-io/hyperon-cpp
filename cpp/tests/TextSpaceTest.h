#include <cxxtest/TestSuite.h>

#include <hyperon/GroundingSpace.h>
#include <hyperon/TextSpace.h>

class FloatAtom : public ValueAtom<float> {
public:
    FloatAtom(float value) : ValueAtom(value) {}
    virtual ~FloatAtom() {}
    std::string to_string() const override { return std::to_string(get()); }
};

class TextSpaceTest : public CxxTest::TestSuite {
public:

    void test_parse_variable() {
        TextSpace text;
        text.add_string("$a");

        GroundingSpace space;
        space.add_from_space(text);

        GroundingSpace expected;
        expected.add_atom(V("a"));
        TS_ASSERT_EQUALS(space, expected);
    }

    void test_parse_plain_atom() {
        TextSpace text;
        text.add_string("(= $a 1.0)");

        GroundingSpace space;
        space.add_from_space(text);

        GroundingSpace expected;
        expected.add_atom(E({ S("="), V("a"), S("1.0") }));
        TS_ASSERT_EQUALS(space, expected);
    }

    void test_parse_nested_atom() {
        TextSpace text;
        text.add_string("(= $a (+ 1.0 2.0))");

        GroundingSpace space;
        space.add_from_space(text);

        GroundingSpace expected;
        expected.add_atom(E({ S("="), V("a"), E({ S("+"), S("1.0"), S("2.0") }) }));
        TS_ASSERT_EQUALS(space, expected);
    }

    void test_parse_grounded_atom() {
        TextSpace text;
        text.register_token(std::regex("\\d+(\\.\\d+)?"),
                [] (std::string str) -> GroundedAtomPtr {
                    return std::make_shared<FloatAtom>(std::stof(str));    
                });
        text.add_string("(+ 1.0 2.0)");
        
        GroundingSpace space;
        space.add_from_space(text);

        GroundingSpace expected;
        expected.add_atom(E({ S("+"), std::make_shared<FloatAtom>(1.0), std::make_shared<FloatAtom>(2.0) }));
        TS_ASSERT_EQUALS(space, expected);
    }
};
