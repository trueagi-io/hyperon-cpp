import unittest
import re

from hyperon import *
from common import interpret_until_result, Atomese

class MatchingTest(unittest.TestCase):

    def test_matching(self):
        atomese = Atomese()

        kb = atomese.parse('''
            (isa red color)
            (isa green color)
            (isa blue color)
        ''')

        atomese.add_atom("kb", ValueAtom(kb))
        target = atomese.parse('''(match kb (isa $color color) $color)''')

        while True:
            next = interpret_until_result(target, kb)
            if next == S('eos'):
                break
            print("result:", next)

if __name__ == "__main__":
    unittest.main()
