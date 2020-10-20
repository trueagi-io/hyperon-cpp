ADD_LIBRARY(hyperon_common SHARED GroundedArithmetic.cpp GroundedLogic.cpp
    Interpret.cpp)
TARGET_LINK_LIBRARIES(hyperon_common PRIVATE hyperon)

INSTALL(TARGETS
    hyperon_common
    DESTINATION "lib")

INSTALL(FILES
    common.h
    GroundedArithmetic.h
    GroundedLogic.h
    Interpret.h
    DESTINATION "include/hyperon/common")
