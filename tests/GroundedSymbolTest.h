#include "MySpace.h"
#include "SimpleSpace.h"

ExprPtr plus(ExprPtr args) {
    int a = std::stoi(args->get_children()[1]->get_symb());
    int b = std::stoi(args->get_children()[2]->get_symb());
    return E(std::to_string(a + b));
}

class GroundedSymbolTest : public CxxTest::TestSuite {
public:


    void test_plus() {
        MySpace kb;
        SimpleSpace program;
        program.add_e(E(G("+", &plus), E("1"), E("2")));
        program.interpret_step(&kb);
        TS_ASSERT_EQUALS(program.to_string(), "3\n")
    }
};
