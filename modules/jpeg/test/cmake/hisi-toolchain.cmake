# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
#this one not so much
SET(CMAKE_SYSTEM_VERSION 1)

SET(MY_CROSS_COMPILER arm-hisiv500-linux-)

if(TRUE)	# FALSE
	# source env first
	SET(CMAKE_C_COMPILER   ${MY_CROSS_COMPILER}gcc)
	SET(CMAKE_CXX_COMPILER ${MY_CROSS_COMPILER}g++)
else()
	SET(MY_TOOL_PATH /home/zucker/Project/4.hisi/tools/toolchain/arm-hisiv500-linux/target)
	SET(MY_TOOL_BIN_PATH ${MY_TOOL_PATH}/bin)

	# specify the cross compiler
	SET(CMAKE_C_COMPILER   ${MY_TOOL_BIN_PATH}/${MY_CROSS_COMPILER}gcc)
	SET(CMAKE_CXX_COMPILER ${MY_TOOL_BIN_PATH}/${MY_CROSS_COMPILER}g++)

	# where is the target environment 
	SET(CMAKE_FIND_ROOT_PATH ${MY_TOOL_PATH}/lib)
endif()

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

