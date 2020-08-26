#ifndef TEXT_SPACE_H
#define TEXT_SPACE_H

#include <string>
#include "SpaceAPI.h"
#include "ClassicSpace.h"

class TextSpace: public SpaceAPI {
public:
    TextSpace(std::string s = ""): code(s) { /*Syntax checking?*/ }
    void add_string(const std::string& s) {
        code += "\n" + s;
    }
    void add_native(const SpaceAPI* pGraph) override {
        add_string(((const TextSpace*)pGraph)->code);
    }
    void add_to(SpaceAPI& graph) const override;
    std::string get_type() const override { return "TextSpace"; }
    std::string get_code() { return code; }
private:
    std::string code;
};

Handle recursive_parse(std::string s, ClassicSpace& ms);

void TextSpace::add_to(SpaceAPI& graph) const
{
    if(graph.get_type() == "ClassicSpace") {
        ClassicSpace& ms = (ClassicSpace&)graph;
        std::istringstream f(code);
        std::string s;
        while (getline(f, s)) {
            //std::cout << "|" << s << "|" << std::endl;
            Handle h = recursive_parse(s, ms);
            if(h == Handle::UNDEFINED) continue;
            HandleSeq hs;
            hs.push_back(h);
            hs.push_back(ms.get_root());
            ms.add_link(MEMBER_LINK, hs);
        }
    } else SpaceAPI::add_to(graph);
}

Handle recursive_parse(std::string s, ClassicSpace& ms)
{
    int r = s.length()-1;
    // not too much syntax checking...
    for(int l = 0; l < s.length(); l++) {
        if(s[l] == ' ' || s[l] == '\t' || s[l] == '\n') continue;
        for(r; r >= l; r--) {
            if(s[r] == ' ' || s[r] == '\t' || s[r] == '\n') continue;
            if(s[l] == '(' && s[r] == ')') {
                HandleSeq outgoing;
                l++;
                int state = s[l] == ' ' ? 1 : 0, cnt = 0;
                for(int l1 = l; l1 < r; l1++) {
                    if(s[l1] == '(') cnt++;
                    if(s[l1] == ')') cnt--;
                    if(s[l1] == ' ' || s[l1] == '\t' || s[l1] == '\n') {
                        if(state != 0 || cnt > 0) continue;
                        //std::cout << "Parsing: " << s.substr(l, l1-l) << std::endl;
                        outgoing.push_back(recursive_parse(s.substr(l, l1-l), ms));
                        l = l1;
                        state = 1;
                    } else {
                        if(state == 0 || cnt > 1) continue;
                        state = 0;
                        l = l1;
                    }
                }
                if(state == 0)
                    outgoing.push_back(recursive_parse(s.substr(l, r-l), ms));
                return ms.add_link(LIST_LINK, outgoing);
            } else {
                if(s[l] == '$') {
                    l++;
                    return ms.add_node(VARIABLE_NODE, s.substr(l, r-l+1));
                } else 
                if(s[l] == '&') {
                    l++;
                    return ms.add_node(GROUNDED_SCHEMA_NODE, s.substr(l, r-l+1));
                } else {
                    //std::cout << "Token: " << s.substr(l, r-l+1) << std::endl;
                    return ms.add_node(CONCEPT_NODE, s.substr(l, r-l+1));
                }
            }
        }
    }
    return Handle::UNDEFINED;
}

#endif // TEXT_SPACE_H
