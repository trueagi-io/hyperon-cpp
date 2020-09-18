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

    void test_match_function_definition() {
        GroundingSpace kb;
        kb.add_atom(E({ S(":-"), E({ S("f"), S("0") }), S("1") }));
        kb.add_atom(E({ S(":-"), E({ S("f"), V("n") }),
                    E({ S("*"), V("n"), E({ S("-"), V("n"), S("1") }) }) }));
        GroundingSpace pattern;
        pattern.add_atom(E({ S(":-"), E({ S("f"), S("5") }), V("x") }));
        GroundingSpace templ;
        templ.add_atom(V("x"));

        GroundingSpace result;
        kb.match(pattern, templ, result);

        GroundingSpace expected;
        expected.add_atom(E({ S("*"), S("5"), E({ S("-"), S("5"), S("1") }) }));
        TS_ASSERT(expected == result);
    }

    void test_match_data() {
        GroundingSpace kb;
        kb.add_atom(E({ S("isa"), S("kitchen-lamp"), S("lamp") }));
        kb.add_atom(E({ S("isa"), S("bedroom-lamp"), S("lamp") }));
        GroundingSpace pattern;
        pattern.add_atom(E({ S("isa"), V("x"), S("lamp") }));
        GroundingSpace templ;
        templ.add_atom(V("x"));

        GroundingSpace result;
        kb.match(pattern, templ, result);

        GroundingSpace expected;
        expected.add_atom(S("kitchen-lamp"));
        expected.add_atom(S("bedroom-lamp"));
        TS_ASSERT(expected == result);
    }

};
