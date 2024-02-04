# CMakeList.txt : CMake project for algorithm Package, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("xlang_algorithm")

include_directories("../ThirdParty")
include_directories("../Api")

FILE(GLOB API_SRC "../Api/value.cpp")
FILE(GLOB Main_SRC "*.cpp")


add_library(${PROJECT_NAME} 
	SHARED
	${API_SRC}
	${Main_SRC}
	${sqlite_src}
)

