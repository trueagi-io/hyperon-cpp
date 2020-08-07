#include "TextSpace.h"
#include "MySpace.h"
#include "SimpleSpace.h"

// Two-way variable grounding
int main()
{
    SimpleSpace ss;
    ss.add_e(E(E(":-"), E(E("frog"), E("Sam")), E("T")));
    ss.add_e(E(E(":-"), E(E("shiny"), V("x")), E(E("green"), V("x"))));
    ss.add_e(E(E(":-"), E(E("green"), V("x")), E(E("frog"), V("x"))));
    ss.add_e(E(E(":-"), E(V("f"), E("Sam")), E("Sam")));
    MySpace ms;
    ms.add_from_space(ss);
    SpaceAPI* match_res = SimpleSpace(E(E(":-"), E(E("frog"), E("Sam")), V("x"))).match_to(ms);
    TextSpace ts_match;
    ts_match.add_from_space(*match_res);
    std::cout << ts_match.get_code() << std::endl;

    TextSpace ftext;
    ftext.add_string("(:- (f 0) 1)");
    ftext.add_string("(:- (f $n) (* $n (f (- $n 1))))");
    MySpace fms;
    fms.add_from_space(ftext);
    SimpleSpace(E(E(":-"), E(E("f"), E("2")), V("x"))).match_to(fms);
}

/*
KB:
(:- (frog Sam) T)
(:- (shiny $x) (green $x))
(:- (green $x) (frog $x))
(:- ($f Sam) Sam)
QUERY:
(:- (frog Sam) $x)

DEBUG info:
Matched: (ListLink (ConceptNode ":-") (ListLink (ConceptNode "frog") (ConceptNode "Sam")) (ConceptNode "T"))
x --> (ConceptNode "T")
Substitution: (:- (frog Sam) T)

Matched: (ListLink (ConceptNode ":-") (ListLink (VariableNode "f") (ConceptNode "Sam")) (ConceptNode "Sam"))
x --> (ConceptNode "Sam")
frog <-- f
Substitution: (:- (frog Sam) Sam)

Result:
(:- (frog Sam) T)
(:- (frog Sam) Sam)

------------------

KB:
(:- (f 0) 1)
(:- (f $n) (* $n (f (- $n 1))))
QUERY:
(:- (f 2) $x)

Matched: (ListLink (ConceptNode ":-") (ListLink (ConceptNode "f") (VariableNode "n")) (ListLink (ConceptNode "*") (VariableNode "n") (ListLink (ConceptNode "f") (ListLink (ConceptNode "-") (VariableNode "n") (ConceptNode "1")))))
x --> (ListLink (ConceptNode "*") (VariableNode "n") (ListLink (ConceptNode "f") (ListLink (ConceptNode "-") (VariableNode "n") (ConceptNode "1"))))
2 <-- n
Substitution: (:- (f 2) (* 2 (f (- 2 1))))

*/
