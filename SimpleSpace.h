#ifndef SIMPLE_SPACE_H
#define SIMPLE_SPACE_H

#include <memory>
#include <string>
#include <vector>

#include "TextSpace.h"

struct VBIND {
    std::string vname;
    const void* binding;
};
struct MATCH {
    bool bSuccess;
    std::vector<VBIND> query;
    std::vector<VBIND> kb;
};

// a simplistic tree structure for expressions
class Expr;
typedef std::shared_ptr<Expr> ExprPtr;

class Expr {
friend class SimpleSpace;
public:
    Expr(): symb(""), bVariable(false) {}
    Expr(const std::string &s, bool bVar = false): symb(s), bVariable(bVar) {}
    /*Expr(const Expr& e1) { // interpreted as a copy-constructor
        Expr e = new Expr();
        *e = e1;
        children.push_back(e);
    }*/
    Expr(const ExprPtr& e1, const ExprPtr& e2) {
        children.push_back(e1);
        children.push_back(e2);
    }
    Expr(const ExprPtr& e1, const ExprPtr& e2, const ExprPtr& e3) {
        children.push_back(e1);
        children.push_back(e2);
        children.push_back(e3);
    }
    bool isVar() const { return bVariable; }
    virtual bool isGrounded() const { return false; }
    bool isExpr() const { return children.size() > 0; }
    bool isPlainExpr() const {
        if(!isExpr()) return false;
        for(auto it = children.begin(); it != children.end(); ++it) {
            if((*it)->isExpr()) return false;
        }
        return true;
    }
    const std::vector<ExprPtr>& get_children() const { return children; }
    const std::string& get_symb() const { return symb; }
    std::string to_string() const {
        std::string s = "";
        if(isExpr()) {
            s += "(";
            for(int i = 0; i < children.size(); i++) {
                s += children[i]->to_string();
                if(i != children.size() - 1) s += " ";
            }
            s += ")";
        } else {
            if(isVar()) s += "$";
            s += get_symb();
        }
        return s;
    }
    
protected:
    bool bVariable;
    std::vector<ExprPtr> children;
    std::string symb;
};

ExprPtr E() { return std::make_shared<Expr>(); }
ExprPtr E(std::string symb, bool isVar = false) { return std::make_shared<Expr>(symb, isVar); }
ExprPtr E(const ExprPtr& e1, const ExprPtr& e2) { return std::make_shared<Expr>(e1, e2); }
ExprPtr E(const ExprPtr& e1, const ExprPtr& e2, const ExprPtr& e3) { return std::make_shared<Expr>(e1, e2, e3); }
ExprPtr V(const std::string &s) { return std::make_shared<Expr>(s, true); }


MATCH match2_top_down(const ExprPtr& graph, Handle h) {
    MATCH m;
    m.bSuccess = false;
    Atom* pA = h.atom_ptr();
    if(graph->isExpr() && pA->is_link()) {
        const std::vector<ExprPtr>& ch1 = graph->get_children();
        const HandleSeq& ch2 = pA->getOutgoingSet();
        //
        if(ch1.size() != ch2.size()) { return m; }
        m.bSuccess = true;
        for(int n = 0; n < ch1.size(); n++) {
            MATCH mc = match2_top_down(ch1[n], ch2[n]);
            if(!mc.bSuccess) {
                m.bSuccess = false;
                return m;
            }
            // TODO: better unification of variable bindings
            for(int i = 0; i < mc.query.size(); i++) {
                bool bFound = false;
                for(int j = 0; j < m.query.size(); j++) {
                    if(mc.query[i].vname == m.query[j].vname) {
                        bFound = true;
                        if(mc.query[i].binding != m.query[j].binding) {
                            m.bSuccess = false;
                        }
                    }
                }
                if(!m.bSuccess) return m;
                if(!bFound) { m.query.push_back(mc.query[i]); }
            }
            // little copypast
            for(int i = 0; i < mc.kb.size(); i++) {
                bool bFound = false;
                for(int j = 0; j < m.kb.size(); j++) {
                    if(mc.kb[i].vname == m.kb[j].vname) {
                        bFound = true;
                        // TODO: doesn't work this way because no deduplication is done...
                        if(mc.kb[i].binding != m.kb[j].binding) {
                            m.bSuccess = false;
                        }
                    }
                }
                if(!m.bSuccess) return m;
                if(!bFound) m.kb.push_back(mc.kb[i]);
            }
        }
        return m;
    }
    if(!graph->isExpr() && pA->is_node()) {
        if(!graph->isVar() && pA->get_type() != VARIABLE_NODE) {
            //std::cout << "comparing " << graph.get_symb() << " " << pA->get_name() << " " << (graph.get_symb() == pA->get_name()) << "\n";
            m.bSuccess = graph->get_symb() == pA->get_name();
            return m;
        }
    }
    if(!graph->isVar() && pA->get_type() != VARIABLE_NODE) {
        return m;
    }
    if(graph->isVar()) {
        // we ground query variable in kb subgraph even if the latter is variable by itself
        // TODO: or we should do the opposite: we should prioretize grounding of kb variables (or should 
        // keep symmetric groundings, but more commonly we'd like all kb variables be bound in contract
        // [maybe] to query variables... well, not necessarily if we want to do partial application), e.g.
        // kb: fact n :- n * fact(n-1)
        // query: fact x :- y
        // should give us n<-x first, and y<-x*fact(x-1) in the result
        // although we can just say that query can be matched against this kb entry with x==n and y==n*fact(n-1),
        // but this will not give us the desirable substitution -- just a match...
        // Partial application example:
        // kb: 'sum x y :- x + y'
        // query: 'sum 2 x :- y' --> 'sum 2 x :- 2 + x' (2->x, x->y, y->2+x)
        // ... well, we do ground y from kb to x from query here
        // TODO: in fact, what we want here is just matching of subgraphs
        // we can match
        // 'frog $a :- and $y (croaks Sam)'
        // 'frog $x :- and (green $x) (croaks $x)'
        // if we wish getting matches $a==$x, $y==(green $x), Sam==$x, which should be further checked for consistency
        // whether we will allow such queries or not and what to do with their results will depend on the language interpreter...
        VBIND vb;
        vb.vname = graph->get_symb();
        vb.binding = h.atom_ptr();
        m.query.push_back(vb);
        //std::cout << "-->Bound: " << vb.vname << " to " << h->to_short_string() << std::endl;
    } else {
        VBIND vb;
        vb.vname = pA->get_name();
        vb.binding = &graph;
        m.kb.push_back(vb);
        //std::cout << "<--Bound: " << vb.vname << " to [Some Expr]" << std::endl;
    }
    m.bSuccess = true;
    return m;
}

#include <algorithm>
class SimpleSpace: public SpaceAPI {
protected:
    ExprPtr atom2e_subs(Atom* pA, const std::vector<VBIND>& kbb) {
        if(pA->is_node()) {
            if(pA->get_type() != VARIABLE_NODE) {
                return E(pA->get_name());
            } else {
                for(int i = 0; i < kbb.size(); i++) {
                    if(kbb[i].vname == pA->get_name()) {
                        return *(ExprPtr*)(kbb[i].binding);
                    }
                }
                throw std::runtime_error( "unbound kb variable " + pA->get_name() );
            }
        } else {
            ExprPtr e = E();
            const HandleSeq& ch = pA->getOutgoingSet();
            for(int i = 0; i < ch.size(); i++) {
                e->children.push_back(atom2e_subs(ch[i], kbb));
            }
            return e;
        }
    }
    ExprPtr atom2e_subs(Handle graph, const std::vector<VBIND>& kbb) {
        return atom2e_subs(graph.atom_ptr(), kbb);
    }

public:
    SimpleSpace() {}
    SimpleSpace(const ExprPtr& graph) {
        content.push_back(graph);
    }
    void add_e(const ExprPtr& graph) {
        content.push_back(graph);
    }
    std::string to_string() const {
        std::string s = "";
        for(auto it = content.begin(); it != content.end(); it++) {
            s += (*it)->to_string() + "\n";
        }
        return s;
    }
    void add_native(const SpaceAPI* pGraph) override {
        std::vector<ExprPtr> to_add = ((SimpleSpace*)pGraph)->content;
        // we may want to deduplicate...
        std::copy(to_add.begin(), to_add.end(), std::back_inserter(content));
        //for(int i = 0; i < to_add.size(); i++) {
        //    content.push_back(to_add[i]);
        //}
    }
    std::string get_type() const override { return "SimpleSpace"; }
    void add_to(SpaceAPI& graph) const override {
        if(graph.get_type() == "TextSpace") {
            ((TextSpace&)graph).add_string(to_string());
        } else
        if(graph.get_type() == "MySpace") {
            MySpace& ms = (MySpace&)graph;
            for(auto it = content.begin(); it != content.end(); ++it) {
                HandleSeq hs;
                hs.push_back(add_atoms_rec(ms, *it));
                hs.push_back(ms.get_root());
                ms.add_link(MEMBER_LINK, hs);
            }
        } else SpaceAPI::add_to(graph);
    }
    SpaceAPI* match_to(SpaceAPI& space) override {
        // Well, it is a concrete pattern matching for Atomspace... It could be made as complex as the real Pattern Matching...
        // It may seem that we should implement it in MySpace. This would require some additional transformation and wrapping,
        // but maybe it's not a large evil. On the other hand, we separated the container itself (Atomspace with its
        // specific deduplication and incoming/outgoing set representation) and pattern matching over it.
        // Maybe more explicit separation would be better?..
        if (space.get_type() != "MySpace") {
            return SpaceAPI::match_to(space);
        }
        SimpleSpace* pRes = new SimpleSpace();
        MySpace& ms = (MySpace&)space;
        // How should we treat the space as query?
        // Is it just a set of queries to be processed independently like here?
        // Or should it be treated as a holistic graph to be matched at once?
        // Should we consider variables in queries to be related or not?
        for(auto pe = content.begin(); pe != content.end(); ++pe) {
            // TODO: check if there is root?
            IncomingSet members = ms.get_root()->getIncomingSet();
            for(auto it = members.begin(); it != members.end(); ++it) {
                // unwrapping from "Member ... @root"
                MATCH res = match2_top_down(*pe, (*it)->getOutgoingSet()[0]);
                if(res.bSuccess) {
                    std::cout << "Matched: " << (*it)->getOutgoingSet()[0]->to_short_string() << "\n";
                    for(int i = 0; i < res.query.size(); i++) {
                        std::cout << res.query[i].vname << " --> " << ((Atom*)res.query[i].binding)->to_short_string() << std::endl;
                    }
                    for(int i = 0; i < res.kb.size(); i++) {
                        std::cout << (*(ExprPtr*)res.kb[i].binding)->get_symb() <<  " <-- " << res.kb[i].vname << std::endl;
                    }
                    ExprPtr e = atom2e_subs((*it)->getOutgoingSet()[0], res.kb);
                    std::cout << "Substitution: " << e->to_string() << std::endl;
                    std::cout << std::endl;
                    pRes->add_e(e);
                }
            }
        }
        return pRes;
    }
    
    void interpret_step(SpaceAPI* ltm) {
        MySpace* ms = (MySpace*)ltm;

        ExprPtr& e = content.back();
        if(!e->isExpr()) {
            std::cout << "Result of evaluation: " << e->get_symb() << std::endl;
            content.pop_back();
            // TODO: in fact, every expression should have a type like IO (), so it outputs its result somewhere by itself
            // here we output it "manually" and discard
            return;
        }
        

        // TODO: traversing the tree each time is not too good
        std::vector<int> stack_i;
        std::vector<ExprPtr> stack_e;
        ExprPtr pE = e;
        int idx = 0;
        while(!pE->isPlainExpr()) {
            // TODO: we need to have some standard way to understand if we do call-by-value or call-by-name
            // this is hard-coded override of normal behavior: @repl-subgraph is evaluated before evaluating its children
            // this can be emulated via @quote (to be implemented), but @quote itself needs similar implementation...
            if(pE->isExpr() && !pE->children[0]->isExpr() && pE->children[0]->get_symb() == "@repl-subgraph") { break; }
            if(idx < pE->children.size()) { // works both for leaves and fully examined expressions
                stack_i.push_back(idx);
                stack_e.push_back(pE);
                pE = pE->children[idx];
                idx = 0;
            } else {
                pE = stack_e.back();
                idx = stack_i.back() + 1;
                stack_e.pop_back();
                stack_i.pop_back();
            }
        }
        std::cout << "First plain expr: " << pE->to_string() << std::endl;

        ExprPtr e_res = E("UNDEFINED");
        const std::string& op = pE->children[0]->get_symb();
        if(op == "+" || op == "-" || op == "*" || op == "/") {
            // TODO: very simplistic now; check type (int/double); check if numbers
            int v1 = std::stoi(pE->children[1]->get_symb());
            int v2 = std::stoi(pE->children[2]->get_symb());
            int v = 0;
            if(op == "+") v = v1 + v2;
            if(op == "-") v = v1 - v2;
            if(op == "*") v = v1 * v2;
            if(op == "/") v = v1 / v2;
            e_res = E(std::to_string(v));
        } else
        if(op == "@move") {
            int v1 = std::stoi(pE->children[1]->get_symb());
            int v2 = std::stoi(pE->children[2]->get_symb());
            if(v1 < 0) v1 += content.size();
            if(v2 < 0) v2 += content.size();
            // TODO a subtle issue in the current implementation: if @move is target, we don't want to swap it,
            // and should consider it as already deleted, but we delete it at the end to keep e accessible...
            // Here, we just don't consider @move as an evaluatable subexpression (this may be solvable with types?)
            std::rotate(content.begin() + v1, content.begin() + v1 + 1, content.begin() + v2);
            // TODO: funny enough, @move has wrong type: it doesn't produce e_res, but instead it has a "side effect"
            // here, we simply hard-code it (once again, this may be solvable with proper typing)
            content.pop_back();
            return;
        } else
        if(op == "@repl-subgraph") {
            ExprPtr edup = pE->children[1];
            content.push_back(edup);
            return;
        } else {
            ExprPtr run_e = E(E(":-"), pE, V("|R|"));
            //E(E(":-"), V("$$$"))
            IncomingSet members = ms->get_root()->getIncomingSet();
            for(auto it = members.begin(); it != members.end(); ++it) {
                // unwrapping from "Member ... @root"
                MATCH res = match2_top_down(run_e,(*it)->getOutgoingSet()[0]);
                if(res.bSuccess) {
                    /*for(int i = 0; i < res.query.size(); i++) {
                        std::cout << res.query[i].vname << " --> " << ((Atom*)res.query[i].binding)->to_short_string() << std::endl;
                    }*/
                    //ExprPtr e = atom2e_subs((*it)->getOutgoingSet()[0], res.kb);
                    e_res = atom2e_subs((Atom*)res.query[/*TODO we should search for |R| in fact*/0].binding, res.kb);
                    break; // TODO: we consider only first match, but we need to consider all to enable reasoning
                    // (but in this case we need to somehow resolve that if (f 0), (f n) should not be matched
                    // (they should be not separate entries, but connected via |
                }
            }
        }
        if(stack_e.size() == 0) {
            // removing target (new target will be inserted)
            std::cout << "New Target: " << e_res->to_string() << std::endl;
            content.pop_back();
            content.insert ( content.begin(), e_res );
        } else {
            // maybe, we need to change the position of the modified target as well?..
            pE = stack_e.back();
            idx = stack_i.back();
            pE->children[idx] = e_res;
            std::cout << "Modified target: " << e->to_string() << std::endl;
        }
    }
    
protected:
    Handle add_atoms_rec(MySpace& ms, const ExprPtr& graph) const {
        if (graph->isExpr()) {
            HandleSeq outgoing;
            for (auto e = graph->get_children().begin(); e != graph->get_children().end(); ++e) {
                outgoing.push_back(add_atoms_rec(ms, *e));
            }
            return ms.add_link(LIST_LINK, outgoing);
        } else {
            return ms.add_node( graph->isVar() ? VARIABLE_NODE : CONCEPT_NODE, graph->get_symb() );
        }
    }

private:
    std::vector<ExprPtr> content;
};


#if 0
// beginning of an "efficient" implementation of graph matching
    void match_graph(const ExprPtr& graph) {
        struct MatchTree {
            std::vector<MatchTree> children;
            MatchTree* pParent;
            const Expr* pE;
            Handle atom;
            int status;
        };
        MatchTree root;
        root.pParent = NULL;
        root.pE = &graph;
        root.atom = Handle::UNDEFINED;
        root.status = 0;
        std::vector<MatchTree*> stack;
        std::vector<MatchTree*> focus;
        stack.push_back(&root);
        // first cycle is to find matchable symbols as starting points
        while(!stack.empty()) {
            MatchTree* pNode = stack.back();
            stack.pop_back();
            if(pNode->pE->isExpr()) {
                const std::vector<Expr>& ch = pNode->pE->get_children();
                //std::cout << "In expr: " << pNode->pE->get_children().size() << std::endl;
                for(int i = 0; i < ch.size(); i++) {
                    MatchTree t;
                    t.status = 0;
                    t.pParent = pNode;
                    t.pE = &ch[i];
                    t.atom = Handle::UNDEFINED;
                    pNode->children.push_back(t);
                    //stack.push_back(&pNode->children.back());//works incorrectly
                }
                for(int n = 0; n < pNode->children.size(); n++) {
                    stack.push_back(&pNode->children[n]);
                    //std::cout << pNode->children[n].pE->get_symb() << "/" << pNode->children[n].pE->isExpr() << " | ";
                }
                //std::cout << std::endl;
            } else {
                if(pNode->pE->isVar()) {
                    //std::cout << "Var: " << pNode->pE->get_symb() << "/" << pNode->pE->isExpr() << "\n";
                } else {
                    // immediately parse non-variable symbols
                    Handle hq = createNode(CONCEPT_NODE, pNode->pE->get_symb());
                    Handle hr = as->get_atom(hq);
                    if(hr == Handle::UNDEFINED) {
                        std::cout << "Unmatched: " << hq->to_short_string() << std::endl;
                        pNode->status = -1;
                        //return; // no match
                    } else {
                        std::cout << "Matched: " << hr->to_short_string() << std::endl;
                        pNode->status = 1;
                        pNode->atom = hr;
                        focus.push_back(pNode);
                    }
                }
            }
        }
        bool bIter = true;
        while(bIter) {
            bIter = false;
            for(int n = 0; n < focus.size(); n++) {
                MatchTree* pNode = focus[n];
                if(pNode->pParent == NULL || pNode->pParent->status == 1) {
                    focus.erase(focus.begin()+n, focus.begin()+n+1);
                    n--;
                    continue;
                }
                // if(pNode->pParent->status != 0) { continue; } //parent status == -1 ?
                pNode = pNode->pParent;
                HandleSeq ch;
                bool bSuccess = true;
                for(int i = 0; i < pNode->children.size(); i++) {
                    MatchTree* pChild = &pNode->children[i];
                    if(pChild->status != 1) {
                        bSuccess = false;
                        break;
                    }
                    ch.push_back(pChild->atom);
                }
                if(bSuccess) {
                    Handle hq = createLink(ch, LIST_LINK);
                    Handle hr = as->get_atom(hq);
                    if(hr == Handle::UNDEFINED) {
                        std::cout << "Unmatched: " << hq->to_short_string() << std::endl;
                        pNode->status = -1;
                    } else {
                        std::cout << "Matched: " << hr->to_short_string() << std::endl;
                        pNode->atom = hr;
                        pNode->status = 1;
                        focus.push_back(pNode);
                        bIter = true;
                    }
                }
            }
            if(!bIter && focus.size() > 0) {
                // trying to guess/ground variables
                for(int n = 0; n < focus.size(); n++) {
                    // ... should call match_top_down
                }
            }
        }
        std::cout << root.status << " " << focus.size() << std::endl;
        for(int n = 0; n < focus.size(); n++) {
            std::cout << focus[n]->atom->to_short_string() << std::endl;
        }
        //if(focus.empty())...
    }
#endif

#endif // SIMPLE_SPACE_H
