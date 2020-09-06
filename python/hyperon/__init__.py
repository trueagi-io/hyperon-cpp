from hyperonpy import *

def C(*args):
    return CompositeAtom(*args)

class CompositeAtom(cCompositeAtom):

    def __init__(self, *args):
        cCompositeAtom.__init__(self, list(args))
        self.children = list(args);


