#include "GroundedLogic.h"

const std::shared_ptr<BoolAtom> TRUE = std::shared_ptr<BoolAtom>(new BoolAtom(true));
const std::shared_ptr<BoolAtom> FALSE = std::shared_ptr<BoolAtom>(new BoolAtom(false));

class EqAtom : public GroundedAtom {
public:
    virtual ~EqAtom() { }

    void execute(GroundingSpace const& args, GroundingSpace& result) const override {
        AtomPtr const& a = args.get_content()[1];
        AtomPtr const& b = args.get_content()[2];
        result.add_atom(Bool(*a == *b));
    }
    bool operator==(Atom const& _other) const override { 
        return this == &_other;
    }
    std::string to_string() const override { return "=="; }
};

const GroundedAtomPtr EQ = std::shared_ptr<EqAtom>(new EqAtom());

class IfAtom : public GroundedAtom {
public:
    virtual ~IfAtom() {}
    void execute(GroundingSpace const& args, GroundingSpace& result) const override {
        AtomPtr _condition = args.get_content()[1];
        AtomPtr if_true = args.get_content()[2];
        AtomPtr if_false = args.get_content().size() > 3 ? args.get_content()[3] : nullptr;
        BoolAtom const* condition = dynamic_cast<BoolAtom*>(_condition.get());
        if (!condition) {
            throw new std::runtime_error("Cannot cast condition to bool, condition: " +
                    _condition->to_string());
        }
        if (condition) {
            result.add_atom(if_true);
        } else if (if_false) {
            result.add_atom(if_false);
        }
    }
    bool operator==(Atom const& _other) const override {
        return this == &_other;
    }
    std::string to_string() const override { return "if"; }
};

const GroundedAtomPtr IF = std::shared_ptr<IfAtom>(new IfAtom());
