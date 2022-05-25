# CMakeList.txt : CMake project for LitePy, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("XPython")

# Add source to this project's executable.
add_executable (XPython 
	"XPython.cpp" 
	"XPython.h" "Core/pycore.h" "Core/pycore.cpp" "Tools/lex.h" "Tools/lex.cpp" "Core/exp.h" "Core/exp.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET XPython PROPERTY CXX_STANDARD 20)
endif()


