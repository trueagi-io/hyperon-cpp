import unittest
import re

from hyperon import *
from common import interpret_until_result, Atomese, AddAtom

class MatchingTest(unittest.TestCase):

    def setUp(self):
        self.atomese = Atomese()
        self.atomese.add_token("dev:\\S+", lambda token: self._get_device(token[4:]))

    def test_interpret_grounded_symbol(self):
        target = GroundingSpace()
        target.add_atom(E(AddAtom(), ValueAtom(1), ValueAtom(2)))

        result = interpret_until_result(target, GroundingSpace())

        self.assertEqual(result, ValueAtom(3))

    def test_interprete_grounded_symbol_atomese(self):
        text_kb = TextSpace()
        text_kb.register_token("\\d+(\\.\\d+)?", lambda s : ValueAtom(float(s)))
        text_kb.register_token("\\+", lambda s : AddAtom())
        text_kb.add_string("(+ 2.0 1.0)")
        target = GroundingSpace()
        target.add_from_space(text_kb)

        result = interpret_until_result(target, GroundingSpace())

        self.assertEqual(result, ValueAtom(3))

    def test_nested_matching(self):
        Logger.setLevel(Logger.TRACE)
        kb = self.atomese.parse('''
            (isa Fred frog)
            (isa frog green)
        ''')
        self.atomese.add_atom("kb", ValueAtom(kb))
        target = self.atomese.parse('''
            (match kb (isa $x $y)
                (q match kb (isa $y $z) (isa $x $z)))
        ''')

        actual = interpret_until_result(target, kb)

        self.assertEqual(actual, E(S('isa'), S('Fred'), S('green')))

    def test_match_variable_in_target(self):
        kb = self.atomese.parse('''
            (= (isa Fred frog) True)
        ''')
        target = self.atomese.parse('''
            (isa Fred $x)
        ''')

        actual = interpret_until_result(target, kb)

        self.assertEqual(actual, ValueAtom(True))

    def test_match_single_symbol_in_interpret(self):
        kb = GroundingSpace()
        kb.add_atom(E(S("="), E(S("f")), S("g")))
        target = GroundingSpace()
        target.add_atom(E(S("f")))

        actual = interpret_until_result(target, kb)

        self.assertEqual(actual, S('g'))
