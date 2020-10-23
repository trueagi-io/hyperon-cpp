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

    def __init__(self, inventory):
        GroundedAtom.__init__(self)
        self.inventory = inventory

    def execute(self, args, result):
        obj = args.get_content()[1]
        return obj in self.inventory

    def __eq__(self, other):
        return isinstance(other, InInventoryAtom)

    def __repr__(self):
        return "in-inventory"

class CraftAtom(GroundedAtom):

    def __init__(self, inventory):
        GroundedAtom.__init__(self)
        self.inventory = inventory

    def execute(self, args, result):
        obj = args.get_content()[1]
        where = args.get_content()[2]
        comp = args.get_content()[3:]
        print(str(obj) + " crafted in " + str(where) + " from " + str(comp))
        self.inventory.append(obj)

    def __eq__(self, other):
        return isinstance(other, CraftAtom)

    def __repr__(self):
        return "craft"

class MinecraftTest(unittest.TestCase):

    def test_minecraft_planning(self):
        Logger.setLevel(Logger.DEBUG)
        atomese = Atomese()
        inventory = [S('inventory'), S('hands')]
        atomese.add_token("in-inventory", lambda _: InInventoryAtom(inventory))
        atomese.add_token("Craft", lambda _: CraftAtom(inventory))

        kb = atomese.parse('''
            (= (if True $then $else) $then)
            (= (if False $then $else) $else)

            (= (wood) (spruce-wood))
            (= (spruce-wood) (mine spruce-tree hand))

            (= (four-planks) (craft four-planks inventory (wood)))
            (= (pack $n planks) (if (> $n 0) (allof (four-planks) (pack (- $n 4) planks)) nop))

            (= (crafting-table) (craft crafting-table inventory  (pack 4 planks)))

            (= (stick) (craft stick inventory (pack 2 planks)))
            (= (pack $n sticks) (if (> $n 0) (allof (stick) (pack (- $n 1) sticks)) nop))

            (= (wooden-pickaxe) (craft wooden-pickaxe
                           (crafting-table) (allof (pack 3 planks) (pack 2 sticks))))

            (= (cobblestone) (mine cobble-ore (wooden-pickaxe)))
            (= (pack $n cobblestones) (if (> $n 0) (allof (cobblestone) (pack (- $n 1) cobblestones)) nop))

            (= (stone-pickaxe) (craft stone-pickaxe (crafting-table)
                           (allof (pack 3 cobblestones) (pack 2 sticks))))
        ''')

        target = atomese.parse('(wooden-pickaxe)')

        interpret_and_print_results(target, kb)

    @unittest.skip("not ready yet")
    def test_minecraft_planning_with_abstractions(self):
        atomese = Atomese()

        atomese.add_token("in-inventory", lambda _: InInventoryAtom())

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
            (= (crafting-table made-from (pack 4 plank)) True)
            (= (crafting-table made-at inventory) True)

            (= (can-be-made inventory) False)
            (= (can-be-mined inventory) False)


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
