# CMakeList.txt : CMake project for http Package, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("xlang_devsrv")

include_directories("../Api")
include_directories("../Http")
include_directories("../utils")
include_directories("../Inc")

FILE(GLOB API_SRC "../Api/value.cpp")
FILE(GLOB Main_SRC "*.cpp" 
	"../utils/gthread.cpp" 
	"../utils/wait.cpp" 
	"../utils/Locker.cpp"
	"../utils/utility.cpp")

add_library(${PROJECT_NAME} 
	SHARED
	${API_SRC}
	${Main_SRC}
)
if (WIN32)
else()
    target_link_libraries(${PROJECT_NAME} pthread dl)
endif()
