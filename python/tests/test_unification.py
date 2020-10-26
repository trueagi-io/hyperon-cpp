import unittest
import re

from hyperon import *
from common import interpret_until_result, Atomese

class UnificationTest(unittest.TestCase):

    def test_factorial_via_unification(self):
        atomese = Atomese()
        kb = atomese.parse('''
            (= (if True $then $else) $then)
            (= (if False $then $else) $else)
            (= (fact $n) (if (== $n 0) 1 (* (fact (- $n 1)) $n)))
        ''')
        target = atomese.parse('''
            (fact 5)
        ''')

        result = interpret_until_result(target, kb)

        self.assertEqual(result, ValueAtom(120))
