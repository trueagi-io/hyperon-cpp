#ifndef INTERPRET_H
#define INTERPRET_H

#include <hyperon/GroundingSpace.h>

AtomPtr interpret_until_result(GroundingSpace& target, GroundingSpace const& kb);

#endif /* INTERPRET_H */
