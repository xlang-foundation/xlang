# CMakeList.txt : CMake project for X-Lang, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("xlang")

include_directories("Api")

FILE(GLOB Api_SRC "Api/*.cpp")
FILE(GLOB AppEntry_SRC "X.cpp")


add_executable (${PROJECT_NAME}
	${Api_SRC}
	${AppEntry_SRC}
)

add_subdirectory("Main")
add_subdirectory("os")
add_subdirectory("http")
add_subdirectory("PyEng")
add_subdirectory("DevSrv")
