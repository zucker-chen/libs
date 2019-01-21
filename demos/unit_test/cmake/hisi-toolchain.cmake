# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
#this one not so much
SET(CMAKE_SYSTEM_VERSION 1)

SET(MY_CROSS_COMPILER arm-hisiv300-linux-)
# source env first
SET(CMAKE_C_COMPILER   ${MY_CROSS_COMPILER}gcc)
SET(CMAKE_CXX_COMPILER ${MY_CROSS_COMPILER}g++)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

