# CMakeList.txt : CMake project for http Package, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("xlang_devsrv")

include_directories("../Api")
include_directories("../Http")
include_directories("../utils")

FILE(GLOB API_SRC "../Api/value.cpp")
FILE(GLOB Main_SRC "*.cpp" 
	"../utils/gthread.cpp" 
	"../utils/wait.cpp" 
	"../utils/Locker.cpp")

add_library(${PROJECT_NAME} 
	SHARED
	${API_SRC}
	${Main_SRC}
)

