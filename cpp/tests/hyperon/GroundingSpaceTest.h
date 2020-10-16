#include <cxxtest/TestSuite.h>

#include <hyperon/hyperon.h>
#include <hyperon/common/common.h>

void add_factorial_definition(GroundingSpace& kb) {
    kb.add_atom(E({ S("="), E({ S("if"), TRUE, V("x"), V("y") }), V("x") }));
    kb.add_atom(E({ S("="), E({ S("if"), FALSE, V("x"), V("y") }), V("y") }));
    kb.add_atom(E({ S("="),
                E({ S("fact"), V("n") }),
                E({ S("if"), E({ EQ, Int(0), V("n") }),
                        Int(1),
                        E({ MUL,
                                E({ S("fact"), E({ SUB, V("n"), Int(1) }) }),
                                V("n") }) }) }));
}

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
        kb.add_atom(E({ S(":-"), E({ S("fact"), S("0") }), S("1") }));
        kb.add_atom(E({ S(":-"), E({ S("fact"), V("n") }),
                    E({ S("*"), V("n"), E({ S("-"), V("n"), S("1") }) }) }));
        GroundingSpace pattern;
        pattern.add_atom(E({ S(":-"), E({ S("fact"), S("5") }), V("x") }));
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

    /*
        lamp = dev:kitchen-lamp
        lamp = dev:bedroom-lamp
        call:turn_on lamp
        
        lamp dev:kitchen-lamp = True
        lamp dev:bedroom-lamp = True
        turn_lamp_on = if (lamp $x) (call:turn_on $x) nop
        
        turn_lamps_on = (match (spaces kb) (isa $x lamp) (call:turn_on $x))

        lamp dev:kitchen-lamp
        lamp dev:bedroom-lamp
        turn_lamp_on (lamp $x) = call:turn_on $x
        turn_lamp_on $y
        --> turn_lamp_on $y = $z --> $y:=lamp $x, $z:=call:turn_on $x -->
        --> $x:=dev:kitchen-lamp,dev:bedroom-lamp --> call:turn_on dev:kitchen-lamp,
        --> call:turn_on dev:bedroom-lamp
    */

    void test_interpret_plain_expr() {
        Logger::setLevel(Logger::DEBUG);
        GroundingSpace kb;
        add_factorial_definition(kb);
        GroundingSpace target;
        target.add_atom(E({ S("fact"), Int(5) }));

        AtomPtr result = interpret_until_result(target, kb);

        TS_ASSERT(*Int(120) == *result);
    }

    void test_interpret_plain_expr_twice() {
        GroundingSpace kb;
        add_factorial_definition(kb);
        GroundingSpace target;
        target.add_atom(E({ S("fact"), Int(3) }));
        target.add_atom(E({ S("fact"), Int(5) }));

        AtomPtr result = interpret_until_result(target, kb);
        TS_ASSERT(*Int(120) == *result);

        result = interpret_until_result(target, kb);
        TS_ASSERT(*Int(6) == *result);

    }

    void test_match_variable_in_target() {
        GroundingSpace kb;
        kb.add_atom(E({ S("="), E({ S("isa"), S("Fred"), S("frog") }),
                    S("True") }));
        GroundingSpace target;
        target.add_atom(E({ S("isa"), S("Fred"), V("x") }));

        AtomPtr result = interpret_until_result(target, kb);

        TS_ASSERT(*S("True") == *result);
    }
};
