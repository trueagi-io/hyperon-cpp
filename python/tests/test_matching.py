import unittest
import re

from hyperon import *

class MatchingTest(unittest.TestCase):

    @unittest.skip("not implemented yet")
    def test_simple_matching_python(self):
        kb = GroundingSpace()
        kb.add_expr(C(S("isa"), DeviceAtom("bedroom-lamp"), S("lamp")))
        kb.add_expr(C(S("isa"), DeviceAtom("kitchen-lamp"), S("lamp")))

        target = GroundingSpace()
        target.add_expr(C(S(":-"),
            C(S("isa"), V("x"), S("lamp")),
            C(CallAtom("turn_on"), V("x"))))

        target.interpret_step(kb)
        target.interpret_step(kb)

    @unittest.skip("not implemented yet")
    def test_simple_matching_atomese(self):
        kb = atomese('''
            (isa dev:kitchen-lamp lamp)
            (isa dev:bedroom-lamp lamp)
        ''')
        target = atomese('''
            (:- (isa $x lamp) (call:turn_on $x))
        ''')
        target.interpret_step(kb)
        target.interpret_step(kb)

def atomese(program):
    kb = GroundingSpace()
    text = TextSpace()
    text.register_token(re.compile("dev:\\S+"),
            lambda token: DeviceAtom(token[4:]))
    text.register_token(re.compile("call:\\S+"),
            lambda token: CallAtom(token[5:]))
    kb.add_from_space(text)
    return kb

class DeviceAtom(GroundedAtom):

    def __init__(self, name):
        GroundedAtom.__init__(self)
        self.name = name

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

    def execute(self, device):
        method = getattr(device, self.method_name)
        method()

    def __eq__(self, other):
        if isinstance(other, CallAtom):
            return self.method_name == other.method_name
        return False

    def __repr__(self):
        return "call:" + self.method_name

