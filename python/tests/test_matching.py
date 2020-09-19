import unittest
import re

from hyperon import *

class MatchingTest(unittest.TestCase):

    def test_interpreter_grounded_python(self):
        target = GroundingSpace()
        target.add_atom(E(PlusAtom(), ValueAtom(1), ValueAtom(2)))

        target.interpret_step(GroundingSpace())

        self.assertEqual(target, GroundingSpace([ValueAtom(3)]))

    def test_interpreter_groundede_text_python(self):
        text_kb = TextSpace()
        text_kb.register_token("\\d+(\\.\\d+)?", lambda s : ValueAtom(float(s)))
        text_kb.register_token("\\+", lambda s : PlusAtom())
        text_kb.add_string("(+ 2.0 1.0)")
        target = GroundingSpace()
        target.add_from_space(text_kb)

        target.interpret_step(GroundingSpace())

        expected = GroundingSpace()
        expected.add_atom(ValueAtom(3))
        self.assertEqual(target, expected)

    def test_simple_matching_python(self):
        kb = GroundingSpace()
        kb.add_atom(E(S("isa"), DeviceAtom("bedroom-lamp"), S("lamp")))
        kb.add_atom(E(S("isa"), DeviceAtom("kitchen-lamp"), S("lamp")))

        target = GroundingSpace()
        target.add_atom(E(MatchAtom(),
            ValueAtom(kb),
            E(S("isa"), V("x"), S("lamp")),
            E(CallAtom("turn_on"), V("x"))))

        target.interpret_step(kb)
        target.interpret_step(kb)
        target.interpret_step(kb)

    def test_simple_matching_atomese(self):
        kb = atomese('kb', '''
            (isa dev:kitchen-lamp lamp)
            (isa dev:bedroom-lamp lamp)
        ''')
        target = atomese('target', '''
            (match (spaces kb) (isa $x lamp) (call:turn_on $x))
        ''')
        target.interpret_step(kb)
        target.interpret_step(kb)
        target.interpret_step(kb)
        target.interpret_step(kb)

spaces = {}

def atomese(name, program):
    kb = GroundingSpace()
    text = TextSpace()
    text.register_token("spaces", lambda token: SpacesAtom(spaces))
    text.register_token("match", lambda token: MatchAtom())
    text.register_token("dev:\\S+", lambda token: DeviceAtom(token[4:]))
    text.register_token("call:\\S+", lambda token: CallAtom(token[5:]))
    text.add_string(program)
    kb.add_from_space(text)
    spaces[name] = kb
    return kb

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

class PlusAtom(GroundedAtom):

    def __init__(self):
        GroundedAtom.__init__(self)

    def execute(self, args, result):
        content = args.get_content()
        result.add_atom(ValueAtom(content[1].value + content[2].value))

    def __eq__(self, other):
        return isinstance(other, PlusAtom)

    def __repr__(self):
        return "+"

class MatchAtom(GroundedAtom):

    def __init__(self):
        GroundedAtom.__init__(self)

    def execute(self, args, result):
        content = args.get_content()
        space = content[1].value
        pattern = GroundingSpace()
        pattern.add_atom(content[2])
        templ = GroundingSpace()
        templ.add_atom(content[3])
        space.match(pattern, templ, result)

    def __eq__(self, other):
        return isinstance(other, MatchAtom)

    def __repr__(self):
        return "match"

class DeviceAtom(GroundedAtom):

    def __init__(self, name):
        GroundedAtom.__init__(self)
        self.name = name

    def turn_on(self):
        print("light is on")

    def __eq__(self, other):
        if isinstance(other, DeviceAtom):
            return self.name == other.name
        return False

    def __repr__(self):
        return "dev:" + self.name

class CallAtom(GroundedAtom):

    def __init__(self, method_name):
        GroundedAtom.__init__(self)
        self.method_name = method_name

    def execute(self, args, result):
        obj = args.get_content()[1]
        method = getattr(obj, self.method_name)
        method()

    def __eq__(self, other):
        if isinstance(other, CallAtom):
            return self.method_name == other.method_name
        return False

    def __repr__(self):
        return "call:" + self.method_name

