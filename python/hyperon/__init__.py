from hyperonpy import (
        Atom,
        S,
        V,
        C as _C,
        GroundedAtom,
        GroundingSpace,
        TextSpace)

def C(*args):
    return _C(list(args))

class ValueAtom(GroundedAtom):

    def __init__(self, value):
        GroundedAtom.__init__(self)
        self.value = value

    def __eq__(self, other):
        if isinstance(other, ValueAtom):
            return self.value == other.value
        return False

    def __repr__(self):
        return repr(self.value)
