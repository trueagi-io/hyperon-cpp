import unittest

from hyperonpy import *

class ApiTest(unittest.TestCase):

    def test_symbol_equals(self):
        self.assertEqual(S("a"), S("a"))
        self.assertNotEqual(S("a"), S("b"))

    def test_symbol_str(self):
        self.assertEqual(str(S("a")), "a")

    def test_symbol_type(self):
        self.assertEqual(S("a").get_type(), Expr.SYMBOL)

    def test_variable_equals(self):
        self.assertEqual(V("x"), V("x"))
        self.assertNotEqual(V("x"), V("y"))

    def test_variable_str(self):
        self.assertEqual(str(V("x")), "$x")

    def test_variable_type(self):
        self.assertEqual(V("a").get_type(), Expr.VARIABLE)

    def test_grounded_equals(self):
        self.assertEqual(FloatAtom(1.0), FloatAtom(1.0))
        self.assertNotEqual(FloatAtom(1.0), FloatAtom(2.0))

    def test_grounded_str(self):
        self.assertEqual(str(FloatAtom(1.0)), "1.0")

    def test_grounded_type(self):
        self.assertEqual(FloatAtom(1.0).get_type(), Expr.GROUNDED)

    def test_grounded_execute_default(self):
        with self.assertRaises(RuntimeError) as e:
            FloatAtom(1.0).execute(None)
        self.assertEqual(str(e.exception), "Operation is not supported")

    def test_grounded_execute(self):
        self.assertEqual(X2Atom().execute(FloatAtom(1.0)), FloatAtom(2.0))

class FloatAtom(GroundedAtom):

    def __init__(self, value):
        GroundedAtom.__init__(self)
        self.value = value

    def __eq__(self, other):
        if isinstance(other, FloatAtom):
            return self.value == other.value
        return False

    def __repr__(self):
        return repr(self.value)

class X2Atom(GroundedAtom):

    def __init__(self):
        GroundedAtom.__init__(self)

    def execute(self, expr):
        return FloatAtom(2 * expr.value);

    def __eq__(self, other):
        return isinstance(other, GroundedFunc)

    def __repr__(self):
        return "foo"