#include <opencog/atoms/atom_types/atom_names.h>

#include "TextSpace.h"
#include "ClassicSpace.h"
#include "SimpleSpace.h"

using namespace opencog;

ExprPtr plus(ExprPtr args) {
    int a = std::stoi(args->get_children()[1]->get_symb());
    int b = std::stoi(args->get_children()[2]->get_symb());
    return E(std::to_string(a + b));
}

extern "C" Handle* plus_as(AtomSpace* as, Handle* args) {
    return nullptr;
}

class GroundedSymbolTest : public CxxTest::TestSuite {
public:

    void test_plus() {
        ClassicSpace kb;
        SimpleSpace program;
        program.add_e(E(G("+", &plus), E("1"), E("2")));
        program.interpret_step(&kb);
        TS_ASSERT_EQUALS(program.to_string(), "3\n");
    }

    void test_grounded_schema_node_in_text() {
        TextSpace text;
        text.add_string("(:- (+ $a $b) (&lib:plus_as $a $b))");

        ClassicSpace kb;
        kb.add_from_space(text);
        
        std::cout << "result: " << kb.get_root()->to_string() << std::endl;
        TS_ASSERT_EQUALS(kb.get_root(),
                List(Concept(":-"),
                    List(Concept("+"), Variable("a"), Variable("b")),
                    List(GroundedSchema("lib:plus_as"), Variable("a"), Variable("b"))
                    ));

    }
};
