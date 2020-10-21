import unittest
import re

from hyperon import *
from common import interpret_until_result, Atomese

def interpret_and_print_results(target, kb):
    while True:
        next = interpret_until_result(target, kb)
        if next == S('eos'):
            break
        print(next)

class MatchingTest(unittest.TestCase):

    def test_show_all_color_names(self):
        atomese = Atomese()

        kb = atomese.parse('''
            (isa red color)
            (isa green color)
            (isa blue color)
        ''')

        atomese.add_atom("kb", ValueAtom(kb))
        target = atomese.parse('(match kb (isa $color color) $color)')

        interpret_and_print_results(target, kb)

    def test_create_semantic_triple(self):
        atomese = Atomese()

        kb = atomese.parse('''
            (obj make pottery)
            (from make clay)
        ''')

        atomese.add_atom("kb", ValueAtom(kb))
        target = atomese.parse('''
            (match kb (obj $verb $var0)
                (q match kb (from $verb $var1) (make_from $var0 $var1)))
        ''')

        interpret_and_print_results(target, kb)

    def test_grounded_arithmetics(self):
        atomese = Atomese()

        kb = atomese.parse('''
            (= (foo $a $b) (* (+ $a $b) (+ $a $b)))
            (= (foo $a $b) (* (+ $a $b) (+ $a $b)))
        ''')

        target = atomese.parse('''
            (foo 3 4)
            (+ 'Hello ' 'world')
        ''')

        interpret_and_print_results(target, kb)

if __name__ == "__main__":
    unittest.main()
