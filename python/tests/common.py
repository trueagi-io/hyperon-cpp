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

    def __init__(self, name, op):
        GroundedAtom.__init__(self)
        self.name = name
        self.op = op

    def execute(self, args, result):
        a = args.get_content()[1]
        b = args.get_content()[2]
        result.add_atom(ValueAtom(self.op(a.value, b.value)));

    def __eq__(self, other):
        return isinstance(other, BinaryOpAtom) and self.name == self.name

    def __repr__(self):
        return self.name

class SubAtom(BinaryOpAtom):
    def __init__(self):
        BinaryOpAtom.__init__(self, "-", lambda a, b: a - b)

class MulAtom(BinaryOpAtom):
    def __init__(self):
        BinaryOpAtom.__init__(self, "*", lambda a, b: a * b)

class PlusAtom(BinaryOpAtom):
    def __init__(self):
        BinaryOpAtom.__init__(self, "+", lambda a, b: a + b)

class DivAtom(BinaryOpAtom):
    def __init__(self):
        BinaryOpAtom.__init__(self, "/", lambda a, b: a / b)

class EqualAtom(BinaryOpAtom):
    def __init__(self):
        BinaryOpAtom.__init__(self, "==", lambda a, b: a == b)

class GreaterAtom(BinaryOpAtom):
    def __init__(self):
        BinaryOpAtom.__init__(self, ">", lambda a, b: a > b)

class LessAtom(BinaryOpAtom):
    def __init__(self):
        BinaryOpAtom.__init__(self, "<", lambda a, b: a < b)
