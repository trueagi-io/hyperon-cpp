from hyperonpy import (
        Atom,
        S,
        V,
        E as _E,
        GroundedAtom,
        GroundingSpace,
        TextSpace,
        Logger,
        IFMATCH)

def E(*args):
    return _E(list(args))

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

