import unittest
import re

from hyperon import *
from common import interpret_until_result, SubAtom, MulAtom, EqualAtom

class UnificationTest(unittest.TestCase):

    def test_factorial_via_unification(self):
        Logger.setLevel(Logger.DEBUG)
        kb = self._atomese('kb', '''
            (= (if True $then $else) $then)
            (= (if False $then $else) $else)
            (= (fact $n) (if (== $n 0) 1 (* (fact (- $n 1)) $n)))
        ''')
        target = self._atomese('target', '''
            (fact 5)
        ''')

        result = interpret_until_result(target, kb)

        self.assertEqual(result, ValueAtom(120))

    def _atomese(self, name, program):
        kb = GroundingSpace()
        text = TextSpace()
        text.register_token("-", lambda token: SubAtom())
        text.register_token("\*", lambda token: MulAtom())
        text.register_token("==", lambda token: EqualAtom())
        text.register_token("\\d+", lambda token: ValueAtom(int(token)))
        text.register_token("True|False", lambda token: ValueAtom(token == 'True'))
        text.add_string(program)
        kb.add_from_space(text)
        return kb
