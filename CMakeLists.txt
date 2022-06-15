# CMakeList.txt : CMake project for LitePy, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("X")

include_directories("Inc")

FILE(GLOB Core_SRC "Core/*.h" "Core/*.cpp")
FILE(GLOB Ext_SRC "Ext/*.cpp")

add_executable (X 
	"X.cpp" 
	"X.h"
	${Core_SRC}
	${Ext_SRC}
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET X PROPERTY CXX_STANDARD 20)
endif()


