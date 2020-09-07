from hyperonpy import (
        Atom,
        S,
        V,
        GroundedAtom,
        cCompositeAtom as _cCompositeAtom,
        cGroundingSpace as _cGroundingSpace,
        cTextSpace as _cTextSpace)

def C(*args):
    return _CompositeAtom(*args)

class _CompositeAtom(_cCompositeAtom):

    def __init__(self, *args):
        _cCompositeAtom.__init__(self, list(args))
        self.children = list(args);

