#include <cxxtest/TestSuite.h>

#include <hyperon/hyperon.h>
#include <hyperon/common/common.h>

void add_factorial_definition(GroundingSpace& kb) {
    kb.add_atom(E({ S("="), E({ S("if"), TRUE, V("then"), V("else") }), V("then") }));
    kb.add_atom(E({ S("="), E({ S("if"), FALSE, V("then"), V("else") }), V("else") }));
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

    void test_interpret_plain_expr() {
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

    void test_not_reduct_ifmatch_arguments_before_matching() {
        Logger::setLevel(Logger::DEBUG);
        Atomese atomese;
        GroundingSpace kb, target;
        atomese.parse("(= (inc Z) (S Z))", kb);
        atomese.parse("(= (inc (S $x)) (S (inc $x)))", kb);
        atomese.parse("(inc Z)", target);

        AtomPtr result;
        int steps = 0;
        do {
            result = target.interpret_step(kb);
            ++steps;
        } while (result == Atom::INVALID && steps < 100);

        if (result != Atom::INVALID) {
            TS_ASSERT(*E({ S("S"), S("Z") }) == *result);
        } else {
            TS_FAIL("Could not get result in 100 steps");
        }
    }

    void test_match_variables_in_unified_expressions() {
        Atomese atomese;
        GroundingSpace kb, target;
        atomese.parse("(= (len nil) 0)", kb);
        atomese.parse("(= (len (:: $x $xs)) (+ 1 (len $xs)))", kb);
        atomese.parse("(len (:: 1 (:: 2 (:: 3 nil))))", target);

        AtomPtr result = interpret_until_result(target, kb);

        TS_ASSERT(*Int(3) == *result);
    }
};
