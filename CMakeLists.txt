PROJECT (multcher)
CMAKE_MINIMUM_REQUIRED (VERSION 2.6)
INCLUDE (CMakeMacro.cmake)

SET(MULTCHER_VERSION_MAJOR 0)
SET(MULTCHER_VERSION_MINOR 1)
SET(MULTCHER_VERSION_PATCH 0)
SET(MULTCHER_VERSION "${MULTCHER_VERSION_MAJOR}.${MULTCHER_VERSION_MINOR}.${MULTCHER_VERSION_PATCH}")

USE_PACKAGE (expat expat.h)
USE_PACKAGE (coda coda/coda.h)
USE_PACKAGE (curl curl/curl.h)

AUX_SOURCE_DIRECTORY(multcher SRC_MULTCHER)
ADD_LIBRARY(multcher ${SRC_MULTCHER})
TARGET_LINK_LIBRARIES(multcher ${LIB_coda} curl pthread)
SET_TARGET_PROPERTIES(multcher PROPERTIES OUTPUT_NAME multcher)
SET_TARGET_PROPERTIES(multcher PROPERTIES LINKER_LANGUAGE CXX)

INSTALL(FILES multcher/multcher.hpp DESTINATION include/multcher)
INSTALL(TARGETS multcher DESTINATION lib${LIB_SUFFIX})

ADD_EXECUTABLE(test_multcher test_multcher/main.cpp)
TARGET_LINK_LIBRARIES(test_multcher multcher)

ADD_EXECUTABLE(test_parse_robotstxt test_parse_robotstxt/main.cpp multcher/parse_robotstxt.cpp multcher/robotstxt.cpp)
TARGET_LINK_LIBRARIES(test_parse_robotstxt ${LIB_coda})

