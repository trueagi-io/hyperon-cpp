#include <memory>
#include <opencog/atoms/atom_types/atom_names.h>

#include "GroundingSpace.h"

class FloatAtom : public GroundedExpr {
public:
    FloatAtom(float value) : value(value) {}
    virtual ~FloatAtom() { }
    virtual bool operator==(Expr const& _other) const { 
        // TODO: it should be replaced by types?
        FloatAtom const* other = dynamic_cast<FloatAtom const*>(&_other);
        return other && other->value == value;
    }
    virtual std::string to_string() const { return std::to_string(value); }
    float get() const { return value; }
private:
    float value;
};

class PlusAtom : public GroundedExpr {
public:
    virtual ~PlusAtom() { }
    virtual ExprPtr execute(ExprPtr _args) const {
        CompositeExprPtr args = std::dynamic_pointer_cast<CompositeExpr>(_args);
        FloatAtom const* a = dynamic_cast<FloatAtom const*>(args->get_children()[1].get());
        FloatAtom const* b = dynamic_cast<FloatAtom const*>(args->get_children()[2].get());
        float c = a->get() + b->get();
        return std::make_shared<FloatAtom>(c);
    }
    virtual bool operator==(Expr const& _other) const { 
        return dynamic_cast<PlusAtom const*>(&_other);
    }
    virtual std::string to_string() const { return "+"; }
};

class GroundedSymbolTest : public CxxTest::TestSuite {
public:

    void test_plus() {
        GroundingSpace kb;
        kb.add_expr(C({std::make_shared<PlusAtom>(), std::make_shared<FloatAtom>(1), std::make_shared<FloatAtom>(2)}));
        ExprPtr result = kb.interpret_step();
        std::cout << "result: " << result->to_string() << std::endl;
        TS_ASSERT(*result == *std::make_shared<FloatAtom>(3));
    }

};
