#ifndef GROUNDED_ARITHMETIC_H
#define GROUNDED_ARITHMETIC_H

#include <hyperon/GroundingSpace.h>
#include <stdexcept>
#include <string>

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

template<> inline int NumValue::get<int>() {
    if (type == INT) {
        return value.i;
    } else {
        throw std::runtime_error("Converting float to int will lost precision");
    }
}

template<> inline float NumValue::get<float>() {
    if (type == INT) {
        return value.i;
    } else {
        return value.f;
    }
}

inline bool operator==(NumValue a, NumValue b) {
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
    static std::shared_ptr<MulAtom> INSTANCE;
    MulAtom() : NumBinaryOpAtom("*") { }
    int operator() (int a, int b) const override { return a * b; }
    float operator() (float a, float b) const override { return a * b; }
};

class SubAtom : public NumBinaryOpAtom {
public:
    static std::shared_ptr<SubAtom> INSTANCE;
    SubAtom() : NumBinaryOpAtom("-") { }
    int operator() (int a, int b) const override { return a - b; }
    float operator() (float a, float b) const override { return a - b; }
};

class PlusAtom : public NumBinaryOpAtom {
public:
    static std::shared_ptr<PlusAtom> INSTANCE;
    PlusAtom() : NumBinaryOpAtom("+") { }
    int operator() (int a, int b) const override { return a + b; }
    float operator() (float a, float b) const override { return a + b; }
};

class DivAtom : public NumBinaryOpAtom {
public:
    static std::shared_ptr<DivAtom> INSTANCE;
    DivAtom() : NumBinaryOpAtom("/") { }
    int operator() (int a, int b) const override { return a / b; }
    float operator() (float a, float b) const override { return a / b; }
};

inline std::shared_ptr<SubAtom> Sub() { return SubAtom::INSTANCE; }
inline std::shared_ptr<MulAtom> Mul() { return MulAtom::INSTANCE; }
inline std::shared_ptr<PlusAtom> Plus() { return PlusAtom::INSTANCE; }
inline std::shared_ptr<DivAtom> Div() { return DivAtom::INSTANCE; }
inline std::shared_ptr<NumAtom> Int(int x) { return std::make_shared<NumAtom>(x); }
inline std::shared_ptr<NumAtom> Float(float x) { return std::make_shared<NumAtom>(x); }

class StringAtom : public ValueAtom<std::string> {
public:
    StringAtom(std::string value) : ValueAtom(value) {}
    virtual ~StringAtom() {}
    std::string to_string() const override { return "\"" + get() + "\""; }
};


class ConcatAtom : public BinaryOpAtom<StringAtom> {
public:
    static std::shared_ptr<ConcatAtom> INSTANCE;
    ConcatAtom() : BinaryOpAtom("++") {}
    virtual StringAtom* operator() (StringAtom const* a, StringAtom const* b) const override {
        return new StringAtom(a->get() + b->get());
    }
};

inline std::shared_ptr<ConcatAtom> Concat() { return ConcatAtom::INSTANCE; };
inline std::shared_ptr<StringAtom> String(std::string str) { return std::make_shared<StringAtom>(str); }

#endif /* GROUNDED_ARITHMETIC_H */
