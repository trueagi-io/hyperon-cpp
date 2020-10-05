ADD_LIBRARY(hyperon_common SHARED GroundedArithmetic.cpp GroundedLogic.cpp
    Interpret.cpp)

INSTALL(TARGETS
    hyperon_common
    DESTINATION "lib")

INSTALL(FILES
    common.h
    GroundedArithmetic.h
    GroundedLogic.h
    Interpret.h
    DESTINATION "include/hyperon/common")
