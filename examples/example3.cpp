#include "TextSpace.h"
#include "MySpace.h"
#include "SimpleSpace.h"

// Interpreting on top of pattern matching
int main()
{
    TextSpace ftext;
    ftext.add_string("(:- (f 0) 1)");
    ftext.add_string("(:- (f $n) (* $n (f (- $n 1))))");
    MySpace fms;
    fms.add_from_space(ftext);
    SimpleSpace prog(E(E("f"), E("2")));
    // Just (f 2) -- not (:- (f 2) x)
    // It's interpreter that knows how to evaluate (f 2) by searching for such x that (:- (f 2) x) can be matched
    // In particular, it searches not just for (:- (f 2) x), but for (MemberLink (:- (f 2) x) ConceptNode("@root"))
    // Indeed, to be a proper container, atomspace should distinguish if there is a graph placed directly into it,
    // or it is only a subgraph of another graph... Here, we have choice either to search only for root graphs
    // or for subgraphs located in any other graph
    prog.interpret_step(&fms);
    prog.interpret_step(&fms);
    prog.interpret_step(&fms);
    prog.interpret_step(&fms);
    prog.interpret_step(&fms);
    prog.interpret_step(&fms);
    prog.interpret_step(&fms);
    prog.interpret_step(&fms);
    // '*' and '-' are grounded operations, which are processed not in the pattern matcher, but in the interpreter
    // although we may want to have them in the "pattern matching language" to make execution of certain queries in distributed atomspaces fast
    // one thing to note -- we see a strict distinction between "pattern matching language" and "Atomese language"
    // there should be special syntax for both (e.g. AndLink and PatternMatcherAndLink, etc...)
}

/*
Output:
First plain expr: (f 2)
New Target: (* 2 (f (- 2 1)))
First plain expr: (- 2 1)
Modified target: (* 2 (f 1))
First plain expr: (f 1)
Modified target: (* 2 (* 1 (f (- 1 1))))
First plain expr: (- 1 1)
Modified target: (* 2 (* 1 (f 0)))
First plain expr: (f 0)
Modified target: (* 2 (* 1 1))
First plain expr: (* 1 1)
Modified target: (* 2 1)
First plain expr: (* 2 1)
New Target: 2
Result of evaluation: 2

*/
