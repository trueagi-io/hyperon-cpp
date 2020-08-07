#ifndef MY_SPACE_H
#define MY_SPACE_H

#include <string>

#include "SpaceAPI.h"

#include <opencog/atomspace/AtomSpace.h>
//#include <opencog/atoms/execution/ExecutionOutputLink.h>
#include <opencog/atoms/grounded/LibraryManager.h>
#include <opencog/atoms/base/Node.h>
#include <opencog/atoms/base/Link.h>
//#include <opencog/rule-engine/backwardchainer/BackwardChainer.h>
//#include <opencog/truthvalue/SimpleTruthValue.h>
//#include <opencog/atoms/truthvalue/SimpleTruthValue.h>

using namespace opencog;

class MySpace: public SpaceAPI {
public:
    MySpace() {
        as = new AtomSpace();
        root = as->add_node(CONCEPT_NODE, "@root");
    }

    Handle add_node(Type t, const std::string& name) { return as->xadd_node(t, name); }
    Handle add_link(Type t, const HandleSeq& outgoing) { return as->xadd_link(t, outgoing); }
    Handle get_root() const { return root; }
    
    void add_native(const SpaceAPI* pGraph) override {
        throw std::runtime_error( "add_native is not implemented for " + get_type() );
    }
    std::string get_type() const override { return "MySpace"; }

    void print_content() {
        IncomingSet members = root->getIncomingSet();
        for(auto it = members.begin(); it != members.end(); ++it) {
            // unwrapping from "Member ... @root"
            std::cout << (*it)->getOutgoingSet()[0]->to_short_string() << std::endl;
        }
    }

private:
    AtomSpace* as;
    Handle root;
};


#endif // MY_SPACE_H
