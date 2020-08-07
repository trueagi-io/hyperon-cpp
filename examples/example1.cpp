#include "TextSpace.h"
#include "MySpace.h"
#include "SimpleSpace.h"

// from space to space
int main()
{
    SimpleSpace ss;
    ss.add_e(E(E(":-"), E(E("frog"), E("Sam")), E("T")));
    ss.add_e(E(E(":-"), E(E("shiny"), V("x")), E(E("green"), V("x"))));
    ss.add_e(E(E(":-"), E(E("green"), V("x")), E(E("frog"), V("x"))));
    ss.add_e(E(E(":-"), E(V("f"), E("Sam")), E("Sam")));
    TextSpace ts;
    ts.add_from_space(ss);
    std::cout << "TextSpace content:" << ts.get_code() << std::endl;
    MySpace ms;
    // Why can't we fill MySpace directly? We can, but SimpleSpace provides just right interface:
    // it knows how to represent expressions of our simple language in Atomspace.
    std::cout << "MySpace content:\n";
    ms.add_from_space(ss);
    ms.print_content();
    std::cout << std::endl;

    TextSpace ftext;
    ftext.add_string("(:- (f 0) 1)");
    ftext.add_string("(:- (f $n) (* $n (f (- $n 1))))");
    std::cout << "Initial TextSpace content:" << ftext.get_code() << "\n\n";
    MySpace fms;
    std::cout << "MySpace content:\n";
    fms.add_from_space(ftext);
    fms.print_content();
    SimpleSpace prog;
    prog.add_from_space(ftext);
    std::cout << prog.to_string();
}

/* Output:
TextSpace content:
(:- (frog Sam) T)
(:- (shiny $x) (green $x))
(:- (green $x) (frog $x))
(:- ($f Sam) Sam)

MySpace content:
(ListLink (ConceptNode ":-") (ListLink (ConceptNode "frog") (ConceptNode "Sam")) (ConceptNode "T"))
(ListLink (ConceptNode ":-") (ListLink (ConceptNode "shiny") (VariableNode "x")) (ListLink (ConceptNode "green") (VariableNode "x")))
(ListLink (ConceptNode ":-") (ListLink (ConceptNode "green") (VariableNode "x")) (ListLink (ConceptNode "frog") (VariableNode "x")))
(ListLink (ConceptNode ":-") (ListLink (VariableNode "f") (ConceptNode "Sam")) (ConceptNode "Sam"))

Initial TextSpace content:
(:- (f 0) 1)
(:- (f $n) (* $n (f (- $n 1))))

MySpace content:
(ListLink (ConceptNode ":-") (ListLink (ConceptNode "f") (ConceptNode "0")) (ConceptNode "1"))
(ListLink (ConceptNode ":-") (ListLink (ConceptNode "f") (VariableNode "n")) (ListLink (ConceptNode "*") (VariableNode "n") (ListLink (ConceptNode "f") (ListLink (ConceptNode "-") (VariableNode "n") (ConceptNode "1")))))

terminate called after throwing an instance of 'std::runtime_error'
  what():  Don't know how to add TextSpace to SimpleSpace
*/
