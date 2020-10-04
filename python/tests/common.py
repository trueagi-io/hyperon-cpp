from hyperon import *

def interpret_until_result(target, kb):
    result = None
    while not result:
        result = target.interpret_step(kb)
    return result

class SpacesAtom(GroundedAtom):

    def __init__(self, spaces):
        GroundedAtom.__init__(self)
        self.spaces = spaces

    def execute(self, args, result):
        content = args.get_content()
        name = content[1].get_symbol();
        result.add_atom(ValueAtom(self.spaces[name]))

    def __eq__(self, other):
        return isinstance(other, SpacesAtom)

    def __repr__(self):
        return "spaces"

        if not name in self.devices:
            self.devices[name] = DeviceAtom(name)
        return self.devices[name]

class MatchAtom(GroundedAtom):

    def __init__(self):
        GroundedAtom.__init__(self)

    def execute(self, args, result):
        content = args.get_content()
        space = content[1].value
        pattern = GroundingSpace()
        pattern.add_atom(content[2])
        templ = GroundingSpace()
        # FIXME: hack to make both quoted and unquoted expression work
        templ_op = content[3].get_children()[0]
        if templ_op.get_type() == Atom.SYMBOL and templ_op.get_symbol() == 'q':
            quoted = content[3].get_children()[1:]
            templ.add_atom(E(*quoted))
        else:
            templ.add_atom(content[3])
        space.match(pattern, templ, result)

    def __eq__(self, other):
        return isinstance(other, MatchAtom)

    def __repr__(self):
        return "match"

