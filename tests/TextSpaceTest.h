#include "GroundingSpace.h"

class TextSpaceTest : public CxxTest::TestSuite {
public:

    void test_parse_variable() {
        ExprPtr result;
        parse("$a", [&result](ExprPtr value) { result = value; });
        TS_ASSERT(*result == *V("a"));
    }

    void test_parse_plain_expression() {
        ExprPtr result;
        parse("(= $a 1.0)", [&result](ExprPtr value) { result = value; });
        TS_ASSERT(*result == *C({S("="), V("a"), S("1.0")}));
    }

    void test_parse_nested_expression() {
        ExprPtr result;
        parse("(= $a (+ 1.0 2.0))", [&result](ExprPtr value) { result = value; });
        TS_ASSERT(*result == *C({ S("="), V("a"), C({ S("+"), S("1.0"), S("2.0") }) }));
    }

};
