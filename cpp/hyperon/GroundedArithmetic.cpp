#include "GroundedArithmetic.h"

std::shared_ptr<MulAtom> MulAtom::INSTANCE = std::make_shared<MulAtom>();
std::shared_ptr<SubAtom> SubAtom::INSTANCE = std::make_shared<SubAtom>();
std::shared_ptr<PlusAtom> PlusAtom::INSTANCE = std::make_shared<PlusAtom>();
std::shared_ptr<DivAtom> DivAtom::INSTANCE = std::make_shared<DivAtom>();
std::shared_ptr<ConcatAtom> ConcatAtom::INSTANCE = std::make_shared<ConcatAtom>();

