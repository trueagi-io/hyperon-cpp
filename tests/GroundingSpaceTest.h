#include "GroundingSpace.h"

class GroundingSpaceTest : public CxxTest::TestSuite {
public:

    void test_composite_expr_to_string() {
        ExprPtr expr = C({S("="), V("a"), S("0")});
        TS_ASSERT_EQUALS(expr->to_string(), "(= $a 0)");
    }

    void test_composite_expr_equals() {
        ExprPtr expr = C({S("="), V("a"), S("0")});
        TS_ASSERT(*expr == *C({S("="), V("a"), S("0")}));
    }

};
