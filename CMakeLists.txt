# CMakeList.txt : CMake project for LitePy, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("XPython")

FILE(GLOB Core_SRC "Core/*.h" "Core/*.cpp")

add_executable (XPython 
	"XPython.cpp" 
	"XPython.h"
	${Core_SRC}
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET XPython PROPERTY CXX_STANDARD 20)
endif()


