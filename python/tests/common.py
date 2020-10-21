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

class UnaryOpAtom(GroundedAtom):

    def __init__(self, name, op):
        GroundedAtom.__init__(self)
        self.name = name
        self.op = op

    def execute(self, args, result):
        a = args.get_content()[1]
        result.add_atom(ValueAtom(self.op(a.value)));

    def __eq__(self, other):
        return isinstance(other, BinaryOpAtom) and self.name == self.name

    def __repr__(self):
        return self.name

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

class AddAtom(BinaryOpAtom):
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

class OrAtom(BinaryOpAtom):
    def __init__(self):
        BinaryOpAtom.__init__(self, "or", lambda a, b: a or b)

class AndAtom(BinaryOpAtom):
    def __init__(self):
        BinaryOpAtom.__init__(self, "and", lambda a, b: a and b)

class NotAtom(BinaryOpAtom):
    def __init__(self):
        BinaryOpAtom.__init__(self, "not", lambda a: not a)

class SampleTextSpace(TextSpace):

    def __init__(self):
        TextSpace.__init__(self)
        self.register_token("\+", lambda token: AddAtom())
        self.register_token("-", lambda token: SubAtom())
        self.register_token("\*", lambda token: MulAtom())
        self.register_token("\/", lambda token: DivAtom())
        self.register_token("==", lambda token: EqualAtom())
        self.register_token("<", lambda token: LessAtom())
        self.register_token(">", lambda token: GreaterAtom())
        self.register_token("or", lambda token: OrAtom())
        self.register_token("and", lambda token: AndAtom())
        self.register_token("not", lambda token: NotAtom())
        self.register_token("\\d+(.\\d+)", lambda token: ValueAtom(float(token)))
        self.register_token("\\d+", lambda token: ValueAtom(int(token)))
        self.register_token("'[^']*'", lambda token: ValueAtom(str(token)))
        self.register_token("True|False", lambda token: ValueAtom(token == 'True'))

def atomese(program):
    kb = GroundingSpace()
    text = SampleTextSpace()
    text.add_string(program)
    kb.add_from_space(text)
    return kb
