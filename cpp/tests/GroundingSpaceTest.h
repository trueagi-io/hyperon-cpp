#include <cxxtest/TestSuite.h>

#include <hyperon/GroundingSpace.h>
#include <hyperon/logger.h>

#include "common.h"

template<typename T>
class BinaryOpAtom : public GroundedAtom {
public:
    BinaryOpAtom(std::string symbol) : symbol(symbol) { }
    virtual ~BinaryOpAtom() { }

    void execute(GroundingSpace const& args, GroundingSpace& result) const override {
        T const* a = dynamic_cast<T const*>(args.get_content()[1].get());
        T const* b = dynamic_cast<T const*>(args.get_content()[2].get());
        std::shared_ptr<T> c(operator()(a, b));
        result.add_atom(c);
    }
    virtual T* operator() (T const* a, T const* b) const = 0;

    bool operator==(Atom const& _other) const override { 
        return dynamic_cast<T const*>(&_other);
    }
    std::string to_string() const override { return symbol; }
private:
    std::string symbol;
};

class IntAtom : public ValueAtom<int> {
public:
    IntAtom(int value) : ValueAtom(value) {}
    virtual ~IntAtom() {}
    std::string to_string() const override { return std::to_string(get()); }
};

class MulAtom : public BinaryOpAtom<IntAtom> {
public:
    MulAtom() : BinaryOpAtom("*") { }
    virtual IntAtom* operator() (IntAtom const* a, IntAtom const* b) const override {
        return new IntAtom(a->get() * b->get());
    }
};

class SubAtom : public BinaryOpAtom<IntAtom> {
public:
    SubAtom() : BinaryOpAtom("-") { }
    virtual IntAtom* operator() (IntAtom const* a, IntAtom const* b) const override {
        return new IntAtom(a->get() - b->get());
    }
};

std::shared_ptr<SubAtom> NewSubAtom() { return std::make_shared<SubAtom>(); }
std::shared_ptr<MulAtom> NewMulAtom() { return std::make_shared<MulAtom>(); }
std::shared_ptr<IntAtom> NewIntAtom(int x) { return std::make_shared<IntAtom>(x); }

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

    void test_interpret_plain_expr() {
        Logger::setLevel(Logger::TRACE);
        GroundingSpace kb;
        kb.add_atom(E({ S("="), E({ S("f"), NewIntAtom(0) }), NewIntAtom(1) }));
        kb.add_atom(E({ S("="),
                        E({ S("f"), V("n") }),
                        E({ NewMulAtom(),
                            E({ S("f"), E({ NewSubAtom(), V("n"), NewIntAtom(1) }) }),
                            V("n") }) }));
        GroundingSpace target;
        target.add_atom(E({ S("f"), NewIntAtom(5) }));

        AtomPtr result = interpret_until_result(target, kb);

        TS_ASSERT(*NewIntAtom(120) == *result);
    }

    void test_interpret_plain_expr_twice() {
        Logger::setLevel(Logger::TRACE);
        GroundingSpace kb;
        kb.add_atom(E({ S("="), E({ S("f"), NewIntAtom(0) }), NewIntAtom(1) }));
        kb.add_atom(E({ S("="),
                        E({ S("f"), V("n") }),
                        E({ NewMulAtom(),
                            E({ S("f"), E({ NewSubAtom(), V("n"), NewIntAtom(1) }) }),
                            V("n") }) }));
        GroundingSpace target;
        target.add_atom(E({ S("f"), NewIntAtom(3) }));
        target.add_atom(E({ S("f"), NewIntAtom(5) }));

        AtomPtr result = interpret_until_result(target, kb);
        TS_ASSERT(*NewIntAtom(120) == *result);
        result = interpret_until_result(target, kb);
        TS_ASSERT(*NewIntAtom(6) == *result);

    }

};
