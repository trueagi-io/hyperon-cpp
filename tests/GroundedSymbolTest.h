#include <memory>
#include <opencog/atoms/atom_types/atom_names.h>

#include "GroundingSpace.h"

class FloatValue : public GroundedExpr {
public:
    FloatValue(float value) : value(value) {}
    virtual ~FloatValue() { }
    virtual bool operator==(Expr const& _other) const { 
        // TODO: it should be replaced by types?
        FloatValue const* other = dynamic_cast<FloatValue const*>(&_other);
        return other && other->value == value;
    }
    virtual std::string to_string() const { return std::to_string(value); }
    float get() const { return value; }
private:
    float value;
};

class Plus : public GroundedExpr {
public:
    virtual ~Plus() { }
    virtual ExprPtr execute(ExprPtr _args) const {
        CompositeExprPtr args = std::dynamic_pointer_cast<CompositeExpr>(_args);
        FloatValue const* a = dynamic_cast<FloatValue const*>(args->get_children()[1].get());
        FloatValue const* b = dynamic_cast<FloatValue const*>(args->get_children()[2].get());
        float c = a->get() + b->get();
        return std::make_shared<FloatValue>(c);
    }
    virtual bool operator==(Expr const& _other) const { 
        return dynamic_cast<Plus const*>(&_other);
    }
    virtual std::string to_string() const { return "+"; }
};

class GroundedSymbolTest : public CxxTest::TestSuite {
public:

    void test_plus() {
        GroundingSpace kb;
        kb.add_expr(C({std::make_shared<Plus>(), std::make_shared<FloatValue>(1), std::make_shared<FloatValue>(2)}));
        ExprPtr result = kb.interpret_step();
        std::cout << "result: " << result->to_string() << std::endl;
        TS_ASSERT(*result == *std::make_shared<FloatValue>(3));
    }

};
