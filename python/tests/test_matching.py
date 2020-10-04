import unittest
import re

from hyperon import *
from common import interpret_until_result, SpacesAtom, MatchAtom

class MatchingTest(unittest.TestCase):

    def setUp(self):
        self.spaces = {}

    def test_interpret_grounded_symbol(self):
        target = GroundingSpace()
        target.add_atom(E(PlusAtom(), ValueAtom(1), ValueAtom(2)))

        result = interpret_until_result(target, GroundingSpace())

        self.assertEqual(result, ValueAtom(3))

    def test_interprete_grounded_symbol_atomese(self):
        text_kb = TextSpace()
        text_kb.register_token("\\d+(\\.\\d+)?", lambda s : ValueAtom(float(s)))
        text_kb.register_token("\\+", lambda s : PlusAtom())
        text_kb.add_string("(+ 2.0 1.0)")
        target = GroundingSpace()
        target.add_from_space(text_kb)

        result = interpret_until_result(target, GroundingSpace())

        self.assertEqual(result, ValueAtom(3))

    def test_nested_matching(self):
        kb = self._atomese('kb', '''
            (isa Fred frog)
            (isa frog green)
        ''')
        target = self._atomese('target', '''
            (match (spaces kb) (isa $x $y)
                (q match (spaces kb) (isa $y $z) (isa $x $z)))
        ''')

        actual = interpret_until_result(target, kb)

        self.assertEqual(actual, E(S('isa'), S('Fred'), S('green')))

    def test_match_variable_in_target(self):
        Logger.setLevel(Logger.TRACE)
        kb = self._atomese('kb', '''
            (= (isa Fred frog) True)
        ''')
        target = self._atomese('target', '''
            (isa Fred $x)
        ''')

        actual = interpret_until_result(target, kb)

        self.assertEqual(actual, S('True'))


    def _atomese(self, name, program):
        kb = GroundingSpace()
        text = TextSpace()
        text.register_token("spaces", lambda token: SpacesAtom(self.spaces))
        text.register_token("match", lambda token: MatchAtom())
        text.add_string(program)
        kb.add_from_space(text)
        self.spaces[name] = kb
        return kb

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

