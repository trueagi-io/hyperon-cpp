#include "TextSpace.h"
#include "ClassicSpace.h"
#include "SimpleSpace.h"

// Interpreting 2+1 targets with order control
int main()
{
    TextSpace ftext;
    ftext.add_string("(:- (f 0) 1)");
    ftext.add_string("(:- (f $n) (* $n (f (- $n 1))))");
    ClassicSpace fms;
    fms.add_from_space(ftext);

    SimpleSpace targets;
    targets.add_e(E(E("f"), E("5")));
    targets.add_e(E(E("f"), E("2")));
    std::cout << "===============\n" << targets.to_string() << "===============\n";
    targets.interpret_step(&fms);
    std::cout << "===============\n" << targets.to_string() << "===============\n";
    targets.add_e(E(E("@repl-subgraph"), E(E("@move"), E("0"), E("-1"))));
    std::cout << "===============\n" << targets.to_string() << "===============\n";
    targets.interpret_step(&fms);
    std::cout << "===============\n" << targets.to_string() << "===============\n";
    targets.interpret_step(&fms);
    std::cout << "===============\n" << targets.to_string() << "===============\n";
    targets.interpret_step(&fms);
    targets.interpret_step(&fms);
    targets.interpret_step(&fms);
    targets.interpret_step(&fms);
    targets.interpret_step(&fms);
    targets.interpret_step(&fms);
    targets.interpret_step(&fms);
    targets.interpret_step(&fms);
    std::cout << "===============\n" << targets.to_string() << "===============\n";
    targets.interpret_step(&fms);
    std::cout << "===============\n" << targets.to_string() << "===============\n";
    targets.interpret_step(&fms);
    targets.interpret_step(&fms);
    std::cout << "===============\n" << targets.to_string() << "===============\n";
}

/*
Output:
===============
(f 5)
(f 2)
===============
First plain expr: (f 2)
New Target: (* 2 (f (- 2 1)))
===============
(* 2 (f (- 2 1)))
(f 5)
===============
===============
(* 2 (f (- 2 1)))
(f 5)
(@repl-subgraph (@move 0 -1))
===============
First plain expr: (@repl-subgraph (@move 0 -1))
===============
(* 2 (f (- 2 1)))
(f 5)
(@repl-subgraph (@move 0 -1))
(@move 0 -1)
===============
First plain expr: (@move 0 -1)
===============
(f 5)
(@repl-subgraph (@move 0 -1))
(* 2 (f (- 2 1)))
===============
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
First plain expr: (@repl-subgraph (@move 0 -1))
First plain expr: (@move 0 -1)
===============
(f 5)
(@repl-subgraph (@move 0 -1))
2
===============
Result of evaluation: 2
===============
(f 5)
(@repl-subgraph (@move 0 -1))
===============
First plain expr: (@repl-subgraph (@move 0 -1))
First plain expr: (@move 0 -1)
===============
(@repl-subgraph (@move 0 -1))
(f 5)
===============

*/
