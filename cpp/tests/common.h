#ifndef COMMON_H
#define COMMON_H

#include <hyperon/GroundingSpace.h>
#include <stdexcept>
#include <string>

template<typename T>
class BinaryOpAtom : public GroundedAtom {
public:
    BinaryOpAtom(std::string symbol) : symbol(symbol) { }
    virtual ~BinaryOpAtom() { }

    void execute(GroundingSpace const& args, GroundingSpace& result) const override {
        T const* a = dynamic_cast<T const*>(args.get_content()[1].get());
        T const* b = dynamic_cast<T const*>(args.get_content()[2].get());
        std::shared_ptr<T> c(operator()(a, b));
        result.add_atom(c);
    }
    virtual T* operator() (T const* a, T const* b) const = 0;

    bool operator==(Atom const& _other) const override { 
        return dynamic_cast<T const*>(&_other);
    }
    std::string to_string() const override { return symbol; }
private:
    std::string symbol;
};

struct NumValue {
    enum Type {
        INT,
        FLOAT
    } type;
    union {
        int i;
        float f;
    } value;
    std::string to_string() const { return type == INT ? std::to_string(value.i) : std::to_string(value.f); }
    template<typename T> T get();
};

template<> int NumValue::get<int>() {
    if (type == INT) {
        return value.i;
    } else {
        throw std::runtime_error("Converting float to int will lost precision");
    }
}

template<> float NumValue::get<float>() {
    if (type == INT) {
        return value.i;
    } else {
        return value.f;
    }
}

bool operator==(NumValue a, NumValue b) {
    return a.type == b.type && 
        a.type == NumValue::FLOAT ? a.value.f == b.value.f : a.value.i == b.value.i;
}

class NumAtom : public ValueAtom<NumValue> {
public:
    NumAtom(int value) : ValueAtom({ NumValue::INT, { .i = value, } }) {}
    NumAtom(float value) : ValueAtom({ NumValue::FLOAT, { .f = value } }) {}
    virtual ~NumAtom() {}
    std::string to_string() const override { return ValueAtom::get().to_string(); }
};

class NumBinaryOpAtom : public BinaryOpAtom<NumAtom> {
public:
    NumBinaryOpAtom(std::string op) : BinaryOpAtom(op) { }
    virtual ~NumBinaryOpAtom() {}
    virtual NumAtom* operator() (NumAtom const* a, NumAtom const* b) const override {
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

std::shared_ptr<SubAtom> Sub() { return std::make_shared<SubAtom>(); }
std::shared_ptr<MulAtom> Mul() { return std::make_shared<MulAtom>(); }
std::shared_ptr<PlusAtom> Plus() { return std::make_shared<PlusAtom>(); }
std::shared_ptr<NumAtom> Int(int x) { return std::make_shared<NumAtom>(x); }
std::shared_ptr<NumAtom> Float(float x) { return std::make_shared<NumAtom>(x); }

class StringAtom : public ValueAtom<std::string> {
public:
    StringAtom(std::string value) : ValueAtom(value) {}
    virtual ~StringAtom() {}
    std::string to_string() const override { return "\"" + get() + "\""; }
};


class ConcatAtom : public BinaryOpAtom<StringAtom> {
public:
    ConcatAtom() : BinaryOpAtom("++") {}
    virtual StringAtom* operator() (StringAtom const* a, StringAtom const* b) const override {
        return new StringAtom(a->get() + b->get());
    }
};

std::shared_ptr<ConcatAtom> Concat() { return std::make_shared<ConcatAtom>(); };
std::shared_ptr<StringAtom> String(std::string str) { return std::make_shared<StringAtom>(str); }

AtomPtr interpret_until_result(GroundingSpace& target, GroundingSpace const& kb) {
    AtomPtr result;
    do {
        result = target.interpret_step(kb);
    } while (result == Atom::INVALID);
    return result;
}

#endif /* COMMON_H */
