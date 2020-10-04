#ifndef GROUNDED_LOGIC_H
#define GROUNDED_LOGIC_H

#include <hyperon/GroundingSpace.h>

class BoolAtom : public ValueAtom<bool> {
public:
    BoolAtom(bool value) : ValueAtom(value) {}
    virtual ~BoolAtom() {}
    std::string to_string() const override { return get() ? "true" : "false"; }
};

extern const std::shared_ptr<BoolAtom> TRUE;
extern const std::shared_ptr<BoolAtom> FALSE;
inline auto Bool(bool value) { return value ? TRUE : FALSE; }

extern const GroundedAtomPtr EQ;
extern const GroundedAtomPtr IF;

#endif /* GROUNDED_LOGIC_H */
