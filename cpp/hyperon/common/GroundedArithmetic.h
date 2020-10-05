#ifndef GROUNDED_ARITHMETIC_H
#define GROUNDED_ARITHMETIC_H

#include <hyperon/GroundingSpace.h>

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

inline auto Int(int x) { return std::make_shared<NumAtom>(x); }
inline auto Float(float x) { return std::make_shared<NumAtom>(x); }

extern const GroundedAtomPtr SUB;
extern const GroundedAtomPtr MUL;
extern const GroundedAtomPtr ADD;
extern const GroundedAtomPtr DIV;

class StringAtom : public ValueAtom<std::string> {
public:
    StringAtom(std::string value) : ValueAtom(value) {}
    virtual ~StringAtom() {}
    std::string to_string() const override { return "\"" + get() + "\""; }
};

inline auto String(std::string str) { return std::make_shared<StringAtom>(str); }

extern const GroundedAtomPtr CONCAT;

#endif /* GROUNDED_ARITHMETIC_H */
