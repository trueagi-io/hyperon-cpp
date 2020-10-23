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
        kb.add_atom(next)

class InInventoryAtom(GroundedAtom):

    def __init__(self):
        GroundedAtom.__init__(self)

    def execute(self, args, result):
        # TODO: add inventory checking
        obj = args.get_content()[1]
        if obj == S('inventory') or obj == S('hands'):
            result.add_atom(ValueAtom(True))
        else:
            result.add_atom(ValueAtom(False))

    def __eq__(self, other):
        return isinstance(other, InInventoryAtom)

    def __repr__(self):
        return "in-inventory"

class MinecraftTest(unittest.TestCase):

    def test_minecraft_planning(self):
        Logger.setLevel(Logger.DEBUG)
        atomese = Atomese()

        atomese.add_token("in-inventory", lambda _: InInventoryAtom())

            #(= (($kind plank) made-from ($kind wood)) True)
            #(= (($kind plank) made-at inventory) True)

            #(= (($kind wood) mined-using hands) True)
            #(= (($kind wood) mined-using ($kind tree)) True)

            #(= ((spruce tree) exists) True)
            #(= ((oak tree) exists) True)
            #(= ((birch tree) exists) True)
        kb = atomese.parse('''
            (= (diamond mined-using iron-pickaxe) True)
            (= (diamond mined-from diamond-ore) True)

            (= (iron-pickaxe made-from
                (, stick stick iron-ingot iron-ingot iron-ingot)) True)
            (= (iron-pickaxe made-at crafting-table) True)

            (= (stick made-from (if (($kind tree) exists) (, ($kind plank) ($kind plank)))) True)
            (= (stick made-at inventory) True)


            (= (if True $then) $then)

            (= (make $x) (if (and ($x made-from $comp) ($x made-at $tool))
                             (, (get $tool) (get $comp) (do-make $x $tool $comp))))

            (= (mine $x) (if (and ($x mined-using $tool) ($x mined-from $source))
                             (, (get $tool) (find $source) (do-mine $x $source $tool))))

            (= (get $x) (if (not (in-inventory $x)) (mine $x)))
            (= (get $x) (if (not (in-inventory $x)) (make $x)))
        ''')

        target = atomese.parse('(get diamond)')

        interpret_and_print_results(target, kb)

if __name__ == "__main__":
    unittest.main()
