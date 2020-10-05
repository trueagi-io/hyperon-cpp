ADD_LIBRARY(hyperon SHARED GroundingSpace.cpp TextSpace.cpp logger.cpp)

INSTALL(TARGETS
    hyperon
    DESTINATION "lib")

INSTALL(FILES
    SpaceAPI.h
    GroundingSpace.h
    TextSpace.h
    logger.h
    hyperon.h
    DESTINATION "include/hyperon")

ADD_SUBDIRECTORY(common)
