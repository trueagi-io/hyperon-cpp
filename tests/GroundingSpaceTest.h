#include <cxxtest/TestSuite.h>

#include "GroundingSpace.h"

class GroundingSpaceTest : public CxxTest::TestSuite {
public:

    void test_expr_atom_to_string() {
        AtomPtr atom = E({S("="), V("a"), S("0")});
        TS_ASSERT_EQUALS(atom->to_string(), "(= $a 0)");
    }

    void test_expr_atom_equals() {
        AtomPtr atom = E({S("="), V("a"), S("0")});
        TS_ASSERT(*atom == *E({S("="), V("a"), S("0")}));
    }

};
