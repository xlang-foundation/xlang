# CMakeList.txt : CMake project for X-Lang, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("xlang")

include_directories("Api")
include_directories("Inc")
include_directories("utils")
include_directories("Ast")
include_directories("Parse")
include_directories("Serialize")
include_directories("Ext")
include_directories("Core")
include_directories("Http")
include_directories("Text")
include_directories("Main")

FILE(GLOB Api_SRC "Api/*.cpp")
FILE(GLOB Utils_SRC "utils/*.cpp")
FILE(GLOB Core_SRC "Core/*.cpp")
FILE(GLOB Ast_SRC "Ast/*.cpp")
FILE(GLOB Serialize_SRC "Serialize/*.cpp")
FILE(GLOB Parse_SRC "Parse/*.cpp")
FILE(GLOB Ext_SRC "Ext/*.cpp")
FILE(GLOB Text_SRC "Text/*.cpp")
FILE(GLOB Main_SRC "X.cpp" "Main/*.cpp")

add_executable (${PROJECT_NAME}
	${Api_SRC}
	${Utils_SRC}
	${Core_SRC}
	${Ast_SRC}
	${Parse_SRC}
	${Serialize_SRC}
	${Ext_SRC}
	${Text_SRC}
	${Main_SRC}
)

if (WIN32)
	#Inheritance via dominance warning
	target_compile_options(${PROJECT_NAME} PRIVATE "/wd4250;")
else()
    target_link_libraries(${PROJECT_NAME} pthread dl)
endif()

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()

add_subdirectory("os")
add_subdirectory("http")
add_subdirectory("PyEng")
add_subdirectory("DevSrv")
