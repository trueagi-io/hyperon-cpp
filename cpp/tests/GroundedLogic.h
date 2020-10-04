#ifndef GROUNDED_LOGIC_H
#define GROUNDED_LOGIC_H

#include <hyperon/GroundingSpace.h>

class BoolAtom : public ValueAtom<bool> {
public:
    BoolAtom(bool value) : ValueAtom(value) {}
    virtual ~BoolAtom() {}
    std::string to_string() const override { return get() ? "true" : "false"; }
};

extern const AtomPtr TRUE;
extern const AtomPtr FALSE;
inline const AtomPtr Bool(bool value) { return value ? TRUE : FALSE; }

class EqAtom;
extern const AtomPtr EQ;

class IfAtom;
extern const AtomPtr IF;

#endif /* GROUNDED_LOGIC_H */
