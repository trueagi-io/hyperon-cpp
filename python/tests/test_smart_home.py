import unittest
import re

from hyperon import *
from common import interpret_until_result, SpacesAtom, MatchAtom

class SmartHomeTest(unittest.TestCase):

    def setUp(self):
        self.devices = {}
        self.spaces = {}

    def test_turn_lamps_on_via_grounded_match(self):
        kb = GroundingSpace()
        kb.add_atom(E(S("isa"), self._get_device("bedroom-lamp"), S("lamp")))
        kb.add_atom(E(S("isa"), self._get_device("kitchen-lamp"), S("lamp")))

        target = GroundingSpace()
        target.add_atom(E(MatchAtom(),
            ValueAtom(kb),
            E(S("isa"), V("x"), S("lamp")),
            E(CallAtom("turn_on"), V("x"))))

        interpret_until_result(target, GroundingSpace())

        self.assertTrue(self._get_device("kitchen-lamp").is_on)
        self.assertTrue(self._get_device("bedroom-lamp").is_on)

    def test_turn_lamps_on_via_grounded_match_atomese(self):
        kb = self._atomese('kb', '''
            (isa dev:kitchen-lamp lamp)
            (isa dev:bedroom-lamp lamp)
        ''')
        target = self._atomese('target', '''
            (match (spaces kb) (isa $x lamp) (call:turn_on $x))
        ''')

        interpret_until_result(target, GroundingSpace())

        self.assertTrue(self._get_device("kitchen-lamp").is_on)
        self.assertTrue(self._get_device("bedroom-lamp").is_on)

    def test_turn_lamps_on_via_interpreter_matching(self):
        kb = self._atomese('kb', '''
            (= (lamp) dev:kitchen-lamp)
            (= (lamp) dev:bedroom-lamp)
        ''')
        target = self._atomese('target', '''
            (call:turn_on (lamp))
        ''')

        interpret_until_result(target, kb)

        self.assertTrue(self._get_device("kitchen-lamp").is_on)
        self.assertTrue(self._get_device("bedroom-lamp").is_on)

    # FIXME: works incorrectly because if is not lazy and
    # lamps are turned on even if condition is True
    def _test_turn_lamps_on_via_condition_and_matching(self):
        kb = self._atomese('kb', '''
            (= (lamp dev:kitchen-lamp)  True)
            (= (lamp dev:bedroom-lamp)  True)
            (= (lamp dev:toilet)  False)
            (= (turn_lamp_on) (if (lamp $x) (call:turn_on $x) nop))
        ''')
        target = self._atomese('target', '''
            (turn_lamp_on)
        ''')

        interpret_until_result(target, kb)

        self.assertTrue(self._get_device("kitchen-lamp").is_on)
        self.assertTrue(self._get_device("bedroom-lamp").is_on)
        self.assertFalse(self._get_device("toilet").is_on)


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
        text.register_token("True|False", lambda token: ValueAtom(token == 'True'))
        text.register_token("if", lambda token: IfAtom())
        text.add_string(program)
        kb.add_from_space(text)
        self.spaces[name] = kb
        return kb


class DeviceAtom(GroundedAtom):

    def __init__(self, name):
        GroundedAtom.__init__(self)
        self.name = name
        self.is_on = False

    def turn_on(self):
        self.is_on = True
        print(self.name + " light is on")

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

class IfAtom(GroundedAtom):

    def __init__(self):
        GroundedAtom.__init__(self)

    def execute(self, args, result):
        condition = args.get_content()[1]
        if_true = args.get_content()[2]
        if_false = args.get_content()[3]
        if (condition.value):
            result.add_atom(if_true)
        else:
            result.add_atom(if_false)

    def __eq__(self, other):
        return isinstance(other, IfAtom)

    def __repr__(self):
        return "if"
