#include <memory>
#include <iostream>

#include "GroundingSpace.h"

class FloatAtom : public ValueAtom<float> {
public:
    FloatAtom(float value) : ValueAtom(value) {}
    virtual ~FloatAtom() {}
    std::string to_string() const { return std::to_string(get()) + "f"; }
};

class PlusAtom : public GroundedExpr {
public:
    virtual ~PlusAtom() { }
    ExprPtr execute(ExprPtr _args) const {
        CompositeExprPtr args = std::dynamic_pointer_cast<CompositeExpr>(_args);
        FloatAtom const* a = dynamic_cast<FloatAtom const*>(args->get_children()[1].get());
        FloatAtom const* b = dynamic_cast<FloatAtom const*>(args->get_children()[2].get());
        float c = a->get() + b->get();
        return std::make_shared<FloatAtom>(c);
    }
    bool operator==(Expr const& _other) const { 
        return dynamic_cast<PlusAtom const*>(&_other);
    }
    std::string to_string() const { return "+"; }
};

class GroundedSymbolTest : public CxxTest::TestSuite {
public:

    void test_simple_grounded_operation() {
        GroundingSpace empty_kb;
        GroundingSpace targets;
        targets.add_expr(C({std::make_shared<PlusAtom>(), std::make_shared<FloatAtom>(1), std::make_shared<FloatAtom>(2)}));
        ExprPtr result = targets.interpret_step(empty_kb);
        TS_ASSERT(*result == FloatAtom(3));
    }

    void test_adding_new_grounded_types() {
        TextSpace text_kb;
        text_kb.register_grounded_type(std::regex("\\d+(\\.\\d+)?"),
                [] (std::string str) -> GroundedExprPtr {
                    return std::make_shared<FloatAtom>(std::stof(str));    
                });
        text_kb.register_grounded_type(std::regex("\\+"),
                [] (std::string str) -> GroundedExprPtr {
                    return std::make_shared<PlusAtom>();    
                });
        text_kb.add_string("(+ 2.0 1.0)");
        GroundingSpace targets;
        targets.add_from_space(text_kb);

        GroundingSpace empty_kb;
        ExprPtr result = targets.interpret_step(empty_kb);
        
        TS_ASSERT(*result == FloatAtom(3));
    }

};
