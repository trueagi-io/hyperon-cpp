#include "common.h"

AtomPtr interpret_until_result(GroundingSpace& target, GroundingSpace const& kb) {
    AtomPtr result;
    do {
        result = target.interpret_step(kb);
    } while (result == Atom::INVALID);
    return result;
}
