#include <memory>
#include <opencog/atoms/atom_types/atom_names.h>

#include "GroundingSpace.h"

class FloatValue : public Value {
public:
    FloatValue(float value) : value(value) {}
    virtual ~FloatValue() { }
    float get() const { return value; }
private:
    float value;
};

class Plus : public Func {
public:
    virtual ~Plus() { }
    virtual ExprPtr execute(GroundingSpace& space, ExprPtr _args) const {
        CompositeExprPtr args = std::dynamic_pointer_cast<CompositeExpr>(_args);
        SymbolExpr const* a_symbol = dynamic_cast<SymbolExpr const*>(args->get_children()[1].get());
        SymbolExpr const* b_symbol = dynamic_cast<SymbolExpr const*>(args->get_children()[2].get());
        float a = dynamic_cast<FloatValue const*>(space.get_grounding(a_symbol->get_symbol()).get())->get();
        float b = dynamic_cast<FloatValue const*>(space.get_grounding(b_symbol->get_symbol()).get())->get();
        float c = a + b;
        std::string result_symbol = std::to_string(c);
        GroundingPtr result_grounding = std::make_shared<FloatValue>(c);
        space.add_grounded_symbol(result_symbol, result_grounding);
        ExprPtr result = G(result_symbol);
        space.add_expr(result);
        return result;
    }
};

class GroundedSymbolTest : public CxxTest::TestSuite {
public:

    void test_plus() {
        GroundingSpace kb;
        kb.add_grounded_symbol("+", std::make_shared<Plus>());
        kb.add_grounded_symbol("1", std::make_shared<FloatValue>(1));
        kb.add_grounded_symbol("2", std::make_shared<FloatValue>(2));
        kb.add_expr(C({G("+"), G("1"), G("2")}));
        ExprPtr result = kb.interpret_step();
        std::cout << "result: " << result->to_string() << std::endl;
        TS_ASSERT(*result == *G("3.000000"));
    }

};
