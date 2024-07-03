# CMakeList.txt : CMake project for BareLink Package, include source and define
# project specific logic here.
# use this module to connect with bare-metal device like pico, stm32, etc.

cmake_minimum_required (VERSION 3.8)

project ("xlang_barelink")

include_directories("../Api")
include_directories("../Inc")

FILE(GLOB API_SRC "../Api/value.cpp")
FILE(GLOB Main_SRC "*.cpp")

add_library(${PROJECT_NAME} 
	SHARED
	${API_SRC}
	${Main_SRC}
)

