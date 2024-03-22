# CMakeList.txt : CMake project for OS Package, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("xlang_os")

include_directories("../Api")
include_directories("../Inc")
FILE(GLOB API_SRC "../Api/value.cpp")
FILE(GLOB Main_SRC "*.cpp")

add_library(${PROJECT_NAME} 
	SHARED
	${API_SRC}
	${Main_SRC}
)

if (WIN32)
else()
    target_link_libraries(${PROJECT_NAME} uuid)
endif()