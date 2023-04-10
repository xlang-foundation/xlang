# CMakeList.txt : CMake project for X-Lang, include source and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.8)
project ("xlang")

include_directories("Api")

FILE(GLOB Api_SRC "Api/*.cpp")
FILE(GLOB AppEntry_SRC "X.cpp" "cli.cpp")


add_executable (${PROJECT_NAME}
	${Api_SRC}
	${AppEntry_SRC}
)
if(NOT (WIN32))
target_link_libraries(${PROJECT_NAME} pthread dl)
endif()
add_subdirectory("Main")
add_subdirectory("os")
add_subdirectory("Http")
add_subdirectory("PyEng")
add_subdirectory("DevSrv")
if(WIN32)
add_subdirectory("winner")
endif()
add_subdirectory("sqlite")
add_subdirectory("Image")

