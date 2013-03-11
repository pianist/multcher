PROJECT (multcher)
CMAKE_MINIMUM_REQUIRED (VERSION 2.6)
INCLUDE (CMakeMacro.cmake)

USE_PACKAGE (expat expat.h)
USE_PACKAGE (coda coda/coda.h)
USE_PACKAGE (curl curl/curl.h)

AUX_SOURCE_DIRECTORY(multcher SRC_MULTCHER)
ADD_LIBRARY(multcher ${SRC_MULTCHER})
TARGET_LINK_LIBRARIES(multcher ${LIB_coda} curl pthread)


ADD_EXECUTABLE(test_multcher test_multcher/main.cpp)
TARGET_LINK_LIBRARIES(test_multcher multcher)

