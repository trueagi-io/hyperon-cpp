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

class InInventoryAtom(GroundedAtom):

    def __init__(self):
        GroundedAtom.__init__(self)

    def execute(self, args, result):
        # TODO: add inventory checking
        obj = args.get_content()[1]
        if obj in [S('inventory'), S('hands'), S('crafting-table'), S('stick'),
                  S('iron-ingot'), S('iron-pickaxe')]:
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
            (= (can-be-mined diamond) True)
            (= (can-be-made diamond) False)
            (= (diamond mined-using iron-pickaxe) True)
            (= (diamond mined-from diamond-ore) True)

            (= (can-be-made iron-pickaxe) True)
            (= (can-be-mined iron-pickaxe) False)
            (= (iron-pickaxe made-from
                (, stick stick iron-ingot iron-ingot iron-ingot)) True)
            (= (iron-pickaxe made-at crafting-table) True)

            (= (can-be-made crafting-table) True)
            (= (can-be-mined crafting-table) False)

            (= (stick made-from (if (($kind tree) exists) (, ($kind plank) ($kind plank)))) True)
            (= (stick made-at inventory) True)


            (= (if True $then $else) $then)
            (= (if False $then $else) $else)

            (= (make $x) (if (and ($x made-from $comp) ($x made-at $tool))
                             (, (get $tool) (get $comp) (do-make $x $tool $comp)) nop))

            (= (mine $x) (if (and ($x mined-using $tool) ($x mined-from $source))
                             (, (get $tool) (find $source) (do-mine $x $source $tool)) nop))

            (= (get $x) (if (and (not (in-inventory $x)) (can-be-mined $x)) (mine $x) nop))
            (= (get $x) (if (and (not (in-inventory $x)) (can-be-made $x)) (make $x) nop))
        ''')

        target = atomese.parse('(get diamond)')

        interpret_and_print_results(target, kb)

if __name__ == "__main__":
    unittest.main()
