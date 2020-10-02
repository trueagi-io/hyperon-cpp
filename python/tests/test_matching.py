import unittest
import re

from hyperon import *
from common import inteprep_until_result

class MatchingTest(unittest.TestCase):

    def setUp(self):
        self.devices = {}
        self.spaces = {}

    def test_interpreter_grounded_python(self):
        target = GroundingSpace()
        target.add_atom(E(PlusAtom(), ValueAtom(1), ValueAtom(2)))

        result = inteprep_until_result(target, GroundingSpace())

        self.assertEqual(result, ValueAtom(3))

    def test_interpreter_groundede_text_python(self):
        text_kb = TextSpace()
        text_kb.register_token("\\d+(\\.\\d+)?", lambda s : ValueAtom(float(s)))
        text_kb.register_token("\\+", lambda s : PlusAtom())
        text_kb.add_string("(+ 2.0 1.0)")
        target = GroundingSpace()
        target.add_from_space(text_kb)

        result = inteprep_until_result(target, GroundingSpace())

        self.assertEqual(result, ValueAtom(3))

    def test_simple_matching_python(self):
        Logger.setLevel(Logger.TRACE)
        kb = GroundingSpace()
        kb.add_atom(E(S("isa"), self._get_device("bedroom-lamp"), S("lamp")))
        kb.add_atom(E(S("isa"), self._get_device("kitchen-lamp"), S("lamp")))

        target = GroundingSpace()
        target.add_atom(E(MatchAtom(),
            ValueAtom(kb),
            E(S("isa"), V("x"), S("lamp")),
            E(CallAtom("turn_on"), V("x"))))

        result = inteprep_until_result(target, GroundingSpace())

        self.assertTrue(self._get_device("kitchen-lamp").is_on)
        self.assertTrue(self._get_device("bedroom-lamp").is_on)

    def test_simple_matching_atomese(self):
        kb = self._atomese('kb', '''
            (isa dev:kitchen-lamp lamp)
            (isa dev:bedroom-lamp lamp)
        ''')
        target = self._atomese('target', '''
            (match (spaces kb) (isa $x lamp) (call:turn_on $x))
        ''')

        result = inteprep_until_result(target, GroundingSpace())

        self.assertTrue(self._get_device("kitchen-lamp").is_on)
        self.assertTrue(self._get_device("bedroom-lamp").is_on)

    def test_nested_matching(self):
        kb = self._atomese('kb', '''
            (isa Fred frog)
            (isa frog green)
        ''')
        target = self._atomese('target', '''
            (match (spaces kb) (isa $x $y)
                (q match (spaces kb) (isa $y $z) (isa $x $z)))
        ''')

        actual = inteprep_until_result(target, kb)

        self.assertEqual(actual, E(S('isa'), S('Fred'), S('green')))

    def _get_device(self, name):
        if not name in self.devices:
            self.devices[name] = DeviceAtom(name)
        return self.devices[name]

    def _atomese(self, name, program):
        kb = GroundingSpace()
        text = TextSpace()
        text.register_token("spaces", lambda token: SpacesAtom(self.spaces))
        text.register_token("match", lambda token: MatchAtom())
        text.register_token("dev:\\S+", lambda token: self._get_device(token[4:]))
        text.register_token("call:\\S+", lambda token: CallAtom(token[5:]))
        text.add_string(program)
        kb.add_from_space(text)
        self.spaces[name] = kb
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

class DeviceAtom(GroundedAtom):

    def __init__(self, name):
        GroundedAtom.__init__(self)
        self.name = name
        self.is_on = False

    def turn_on(self):
        self.is_on = True
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

