#include "GroundedArithmetic.h"

template<typename T>
class BinaryOpAtom : public GroundedAtom {
public:
    BinaryOpAtom(std::string symbol) : symbol(symbol) { }
    virtual ~BinaryOpAtom() { }

    void execute(GroundingSpace const& args, GroundingSpace& result) const override {
        AtomPtr const& _a = args.get_content()[1];
        AtomPtr const& _b = args.get_content()[2];
        T const* a = dynamic_cast<T const*>(_a.get());
        T const* b = dynamic_cast<T const*>(_b.get());
        if (!a || !b) {
            throw std::runtime_error("Cannot cast parameters to operation type, a: " +
                    _a->to_string() + ", b: " + _b->to_string());
        }
        std::shared_ptr<T> c(operator()(a, b));
        result.add_atom(c);
    }
    virtual T* operator() (T const* a, T const* b) const = 0;
    bool operator==(Atom const& _other) const override { 
        return this == &_other;
    }
    std::string to_string() const override { return symbol; }
private:
    std::string symbol;
};

class NumBinaryOpAtom : public BinaryOpAtom<NumAtom> {
public:
    NumBinaryOpAtom(std::string op) : BinaryOpAtom(op) { }
    virtual ~NumBinaryOpAtom() {}
    NumAtom* operator() (NumAtom const* a, NumAtom const* b) const override {
        if (a->get().type == NumValue::FLOAT || b->get().type == NumValue::FLOAT) {
            return new NumAtom(operator()(a->get().get<float>(), b->get().get<float>()));
        } else {
            return new NumAtom(operator()(a->get().get<int>(), b->get().get<int>()));
        }
    }
    virtual int operator() (int a, int b) const = 0;
    virtual float operator() (float a, float b) const = 0;
};

class MulAtom : public NumBinaryOpAtom {
public:
    MulAtom() : NumBinaryOpAtom("*") { }
    int operator() (int a, int b) const override { return a * b; }
    float operator() (float a, float b) const override { return a * b; }
};

class SubAtom : public NumBinaryOpAtom {
public:
    SubAtom() : NumBinaryOpAtom("-") { }
    int operator() (int a, int b) const override { return a - b; }
    float operator() (float a, float b) const override { return a - b; }
};

class PlusAtom : public NumBinaryOpAtom {
public:
    PlusAtom() : NumBinaryOpAtom("+") { }
    int operator() (int a, int b) const override { return a + b; }
    float operator() (float a, float b) const override { return a + b; }
};

class DivAtom : public NumBinaryOpAtom {
public:
    DivAtom() : NumBinaryOpAtom("/") { }
    int operator() (int a, int b) const override { return a / b; }
    float operator() (float a, float b) const override { return a / b; }
};

const GroundedAtomPtr MUL = std::make_shared<MulAtom>();
const GroundedAtomPtr SUB = std::make_shared<SubAtom>();
const GroundedAtomPtr ADD = std::make_shared<PlusAtom>();
const GroundedAtomPtr DIV = std::make_shared<DivAtom>();

class ConcatAtom : public BinaryOpAtom<StringAtom> {
public:
    ConcatAtom() : BinaryOpAtom("++") {}
    virtual StringAtom* operator() (StringAtom const* a, StringAtom const* b) const override {
        return new StringAtom(a->get() + b->get());
    }
};

const GroundedAtomPtr CONCAT = std::make_shared<ConcatAtom>();

