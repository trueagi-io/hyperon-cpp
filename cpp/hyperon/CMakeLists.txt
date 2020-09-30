ADD_LIBRARY(hyperon SHARED GroundingSpace.cpp TextSpace.cpp)

INSTALL(TARGETS
    hyperon
    DESTINATION "lib")

INSTALL(FILES
    SpaceAPI.h
    GroundingSpace.h
    TextSpace.h
    DESTINATION "include/hyperon")
