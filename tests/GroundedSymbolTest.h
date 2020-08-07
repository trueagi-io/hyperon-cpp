#include "MySpace.h"
#include "SimpleSpace.h"

class GroundedSymbolTest : public CxxTest::TestSuite {
public:

    void test_plus() {
        MySpace ms;
        SimpleSpace space;
        space.add_e(E(E("+"), E("1"), E("2")));
        space.interpret_step(&ms);
        TS_ASSERT_EQUALS(space.to_string(), "3\n")
    }
};
