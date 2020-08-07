#ifndef SPACE_API_H
#define SPACE_API_H

#include <string>
#include <stdexcept>

// In fact, all spaces are just grounded object nodes with certain interfaces.
// In a more complete design, we should have GroundedObjectNode and SpaceAPI inherited from it,
// because SpaceAPI add some concrete requirements.
// get_type can be a part of GroundedObjectNode, or it can be a part of Atomese type system
// (that is, an object of a certain type will have a corresponding [grounded] type in Atomese)
class SpaceAPI {
public:
    virtual void add_to(SpaceAPI& graph) const {
        // this is the primary function for overriding, because it's easier to traverse its own content
        // and add it to another container using its interface than to traverse a non-trivial container,
        // which implementation details can be hidden
        if (get_type() == graph.get_type()) {
            graph.add_native(this);
        } else {
            throw std::runtime_error( "Don't know how to add " + get_type() + " to " + graph.get_type() );
        }
    }
    virtual void add_from_space(const SpaceAPI& graph) {
        // some classes would like to override this function as well,
        // so they are able to convert both from and to foremly implemented fixed classes
        graph.add_to(*this);
    }
    // Is it a must-have function? Basically, do we want any container to form a monoid? Maybe, yes, but merging two Atomspaces is not so trivial...
    virtual void add_native(const SpaceAPI* pGraph) = 0;
    virtual SpaceAPI* match_to(SpaceAPI& space) {
        // There are different possible approaches. One of them would be to have (natural) transformations between different types
        // of spaces, and to have match_native for the corresponding space type. Then, if there is a transform from the space type
        // of the query to the space type of KB, then we can simply run match_native (and add_native instead of implementing add_to).
        // Thus, we naturally decompose match_to and add_to into composition of transform and match_native/add_native.
        // There are few possible drawbacks, though. One of them is excessive wrapping and conversion. We may not need to convert
        // query to the KB container format to process it (the same is more obvious for adding data to spaces). Another situational
        // drawback is that if some space type supports queries of specific structure, it can implement efficient pattern matching for
        // them. Of course, having one pattern matching per space type is good, because pattern matching is not trivial (in contrast,
        // add_to can be simpler to introduce than add_native + transform). But generic pattern matching of general containers like
        // Atomspace, especially if these are 3rd-party containers unaware of Atomese[2.0], can do not precisely what we want.
        // Ultimately, SpaceAPI could provide graph traversal API, which would be enough for implementing a container-agnostic very
        // general pattern matching, and only one such pattern matching would be enough, but most likely it will be inefficient
        // (especially in the case of distributed graph dbs), and thus we most likely need container-specific pattern matchers
        // (or at least some intermediate-level functions - not only simple graph traversals, but joints, etc.).
        // Thus, match_to can be not necessarily an implementation of the pattern matching itself (although it can), but an interface
        // to the internal pattern matching of the existing container, which correctly uses the structure of queries. Moreover,
        // we usually want the results of the pattern matching to be in the same format as the query, while match_native of the
        // transformed query will give us the KB format results, which will typically require one more transformation.
        // One more option would be to specify matching not between two containers, but matching the query against the container
        // (this would also be useful for add_native function: most containers implement their own insertion functions accepting
        // not spaces, but more basic elements like Handle for AtomSpace, string for TextSpace, and E for SimpleSpace -- this
        // could be unified in API...). Right now, such matching can be implemented as non-virtual methods in spaces themselves,
        // and these methods can be called by match_to methods by converting queries to Handle, etc.
        // In fact, all these options might be needed and combined in different ways, but here we limit ourselves to match_to.
        // 
        throw std::runtime_error( "match_to is not implemented for " + get_type() );
        return NULL;
    }
    virtual ~SpaceAPI() {}
    virtual std::string get_type() const { return "SpaceAPI"; } // better make it NULL, so it should be overrided???
};

#endif // SPACE_API_H
