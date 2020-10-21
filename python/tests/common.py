from hyperon import *

def interpret_until_result(target, kb):
    result = None
    while not result:
        result = target.interpret_step(kb)
    return result

class SpacesAtom(GroundedAtom):

    def __init__(self, spaces):
        GroundedAtom.__init__(self)
        self.spaces = spaces

    def execute(self, args, result):
        content = args.get_content()
        name = content[1].get_symbol();
        result.add_atom(ValueAtom(self.spaces[name]))

    def __eq__(self, other):
        return isinstance(other, SpacesAtom)

    def __repr__(self):
        return "spaces"

class MatchAtom(GroundedAtom):

    def __init__(self):
        GroundedAtom.__init__(self)

    def execute(self, args, result):
        content = args.get_content()
        space = content[1].value
        pattern = GroundingSpace()
        pattern.add_atom(content[2])
        templ = GroundingSpace()
        # FIXME: hack to make both quoted and unquoted expression work
        templ_op = content[3].get_children()[0]
        if templ_op.get_type() == Atom.SYMBOL and templ_op.get_symbol() == 'q':
            quoted = content[3].get_children()[1:]
            templ.add_atom(E(*quoted))
        else:
            templ.add_atom(content[3])
        space.match(pattern, templ, result)

    def __eq__(self, other):
        return isinstance(other, MatchAtom)

    def __repr__(self):
        return "match"

class BinaryOpAtom(GroundedAtom):

    def __init__(self, op):
        GroundedAtom.__init__(self)
        self.op = op;

    def execute(self, args, result):
        a = args.get_content()[1]
        b = args.get_content()[2]
        result.add_atom(ValueAtom(self.do(a.value, b.value)));

    def __eq__(self, other):
        return isinstance(other, BinaryOpAtom) and self.op == other.op

    def __repr__(self):
        return self.op

class SubAtom(BinaryOpAtom):

    def __init__(self):
        BinaryOpAtom.__init__(self, "-")

    def do(self, a, b):
        return a - b

class MulAtom(BinaryOpAtom):

    def __init__(self):
        BinaryOpAtom.__init__(self, "*")

    def do(self, a, b):
        return a * b

class EqualAtom(BinaryOpAtom):

    def __init__(self):
        BinaryOpAtom.__init__(self, "==")

    def do(self, a, b):
        return a == b
