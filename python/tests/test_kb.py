import unittest
import re

from hyperon import *
from common import Atomese, AtomspaceAtom

class KbTest(unittest.TestCase):

    def test_use_self(self):
        target = GroundingSpace()

        atomese = Atomese()
        atomese.add_atom("parser", ValueAtom(Parser()))
        atomese.add_atom("self", AtomspaceAtom(target, "self"))

        target = atomese.parse("(call:parse parser '(+ 1 2)' self)", target)

        target.interpret_step(GroundingSpace())

        expected = atomese.parse("(+ 1 2)")
        self.assertEqual(target, expected)

class Parser:

    def parse(self, text, kb):
        atomese = Atomese()
        atomese.parse(text.value, kb.value)

if __name__ == "__main__":
    unittest.main()
