#ifndef ATOMESE_H
#define ATOMESE_H

#include <string>

#include <hyperon/GroundingSpace.h>
#include <hyperon/TextSpace.h>

class Atomese {
public:
    void parse(std::string program, GroundingSpace& kb) const;

private:
    void init_parser(TextSpace& parser) const;
};

#endif /* ATOMESE_H */
