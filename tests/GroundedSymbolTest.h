#include <cxxtest/TestSuite.h>
#include <memory>
#include <iostream>

#include "GroundingSpace.h"
#include "TextSpace.h"

class FloatAtom : public ValueAtom<float> {
public:
    FloatAtom(float value) : ValueAtom(value) {}
    virtual ~FloatAtom() {}
    std::string to_string() const override { return std::to_string(get()); }
};

class StringAtom : public ValueAtom<std::string> {
public:
    StringAtom(std::string value) : ValueAtom(value) {}
    virtual ~StringAtom() {}
    std::string to_string() const override { return "\"" + get() + "\""; }
};

class PlusAtom : public GroundedAtom {
public:
    virtual ~PlusAtom() { }
    AtomPtr execute(AtomPtr _args) const override {
        ExprAtomPtr args = std::dynamic_pointer_cast<ExprAtom>(_args);
        FloatAtom const* a = dynamic_cast<FloatAtom const*>(args->get_children()[1].get());
        FloatAtom const* b = dynamic_cast<FloatAtom const*>(args->get_children()[2].get());
        float c = a->get() + b->get();
        return std::make_shared<FloatAtom>(c);
    }
    bool operator==(Atom const& _other) const override { 
        return dynamic_cast<PlusAtom const*>(&_other);
    }
    std::string to_string() const override { return "+"; }
};

class GroundedSymbolTest : public CxxTest::TestSuite {
public:

    void test_simple_grounded_operation() {
        GroundingSpace empty_kb;
        GroundingSpace targets;
        targets.add_atom(E({std::make_shared<PlusAtom>(), std::make_shared<FloatAtom>(1), std::make_shared<FloatAtom>(2)}));
        AtomPtr result = targets.interpret_step(empty_kb);
        TS_ASSERT(*result == FloatAtom(3));
    }

    void test_adding_new_grounded_types() {
        TextSpace text_kb;
        text_kb.register_token(std::regex("\\d+(\\.\\d+)?"),
                [] (std::string str) -> GroundedAtomPtr {
                    return std::make_shared<FloatAtom>(std::stof(str));    
                });
        text_kb.register_token(std::regex("\\+"),
                [] (std::string str) -> GroundedAtomPtr {
                    return std::make_shared<PlusAtom>();    
                });
        text_kb.add_string("(+ 2.0 1.0)");
        GroundingSpace targets;
        targets.add_from_space(text_kb);

        GroundingSpace empty_kb;
        AtomPtr result = targets.interpret_step(empty_kb);
        
        TS_ASSERT(*result == FloatAtom(3));
    }

    // FIXME: test is not passed yet
    void _test_two_grounded_types() {
        TextSpace text_kb;
        text_kb.register_token(std::regex("\\d+(\\.\\d+)?"),
                [] (std::string str) -> GroundedAtomPtr {
                    return std::make_shared<FloatAtom>(std::stof(str));    
                });
        text_kb.register_token(std::regex("\"[^\"]*\""),
                [] (std::string str) -> GroundedAtomPtr {
                    return std::make_shared<StringAtom>(str.substr(1, str.size() - 2));    
                });
        text_kb.register_token(std::regex("\\+"),
                [] (std::string str) -> GroundedAtomPtr {
                    return std::make_shared<PlusAtom>();    
                });
        text_kb.add_string("(= $a 1.0)");
        text_kb.add_string("(= $x (+ $a 2.0))");
        text_kb.add_string("(= $b \"1\")");
        text_kb.add_string("(= $y (+ $b \"2\"))");
        GroundingSpace kb;
        kb.add_from_space(text_kb);

        GroundingSpace targets;
        targets.add_atom(V("x"));
        targets.add_atom(V("y"));
        AtomPtr y = targets.interpret_step(kb);
        TS_ASSERT(*y == StringAtom("12"));
        AtomPtr x = targets.interpret_step(kb);
        TS_ASSERT(*x == FloatAtom(3));
    }
};
