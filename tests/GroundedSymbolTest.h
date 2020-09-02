#include <cxxtest/TestSuite.h>
#include <memory>
#include <iostream>

#include "GroundingSpace.h"
#include "TextSpace.h"

class FloatAtom : public ValueAtom<float> {
public:
    FloatAtom(float value) : ValueAtom(value) {}
    virtual ~FloatAtom() {}
    std::string to_string() const { return std::to_string(get()) + "f"; }
};

class StringAtom : public ValueAtom<std::string> {
public:
    StringAtom(std::string value) : ValueAtom(value) {}
    virtual ~StringAtom() {}
    std::string to_string() const { return "\"" + get() + "\"s"; }
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
        text_kb.register_token(std::regex("\\d+(\\.\\d+)?"),
                [] (std::string str) -> GroundedExprPtr {
                    return std::make_shared<FloatAtom>(std::stof(str));    
                });
        text_kb.register_token(std::regex("\\+"),
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

    // FIXME: test is not passed yet
    void _test_two_grounded_types() {
        TextSpace text_kb;
        text_kb.register_token(std::regex("\\d+(\\.\\d+)?"),
                [] (std::string str) -> GroundedExprPtr {
                    return std::make_shared<FloatAtom>(std::stof(str));    
                });
        text_kb.register_token(std::regex("\"[^\"]*\""),
                [] (std::string str) -> GroundedExprPtr {
                    return std::make_shared<StringAtom>(str.substr(1, str.size() - 2));    
                });
        text_kb.register_token(std::regex("\\+"),
                [] (std::string str) -> GroundedExprPtr {
                    return std::make_shared<PlusAtom>();    
                });
        text_kb.add_string("(= $a 1.0)");
        text_kb.add_string("(= $x (+ $a 2.0))");
        text_kb.add_string("(= $b \"1\")");
        text_kb.add_string("(= $y (+ $b \"2\"))");
        GroundingSpace kb;
        kb.add_from_space(text_kb);

        GroundingSpace targets;
        targets.add_expr(V("x"));
        targets.add_expr(V("y"));
        ExprPtr y = targets.interpret_step(kb);
        TS_ASSERT(*y == StringAtom("12"));
        ExprPtr x = targets.interpret_step(kb);
        TS_ASSERT(*x == FloatAtom(3));
    }
};
