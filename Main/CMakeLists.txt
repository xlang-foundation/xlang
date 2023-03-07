# CMakeList.txt : CMake project for X-Lang-Engine, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("xlang_eng")

add_compile_definitions(XLANG_ENG_DBG=1)

include_directories("../Api")
include_directories("../Inc")
include_directories("../utils")
include_directories("../Ast")
include_directories("../Parse")
include_directories("../Serialize")
include_directories("../rpc")
include_directories("../Thread")
include_directories("../Ext")
include_directories("../Core")
include_directories("../Text")
include_directories("../Scripts")
include_directories("../nn")
include_directories(".")

FILE(GLOB Api_SRC "../Api/value.cpp")
FILE(GLOB Utils_SRC "../utils/*.cpp")
FILE(GLOB Core_SRC "../Core/*.cpp")
FILE(GLOB Ast_SRC "../Ast/*.cpp")
FILE(GLOB Serialize_SRC "../Serialize/*.cpp")
FILE(GLOB Parse_SRC "../Parse/*.cpp")
FILE(GLOB Thread_SRC "../Thread/*.cpp")
FILE(GLOB Ext_SRC "../Ext/*.cpp")
FILE(GLOB Text_SRC "../Text/*.cpp")
FILE(GLOB Rpc_SRC "../rpc/*.cpp")
FILE(GLOB Nn_SRC "../nn/*.cpp")
FILE(GLOB Main_SRC "*.cpp")

add_library(${PROJECT_NAME} SHARED 
	${Api_SRC}
	${Utils_SRC}
	${Core_SRC}
	${Ast_SRC}
	${Parse_SRC}
	${Serialize_SRC}
	${Thread_SRC}
	${Ext_SRC}
	${Text_SRC}
	${Rpc_SRC}
	${Nn_SRC}
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

#[[
add_library(${PROJECT_NAME}.mini SHARED 
	${Api_SRC}
	${Utils_SRC}
	${Core_SRC}
	${Ast_SRC}
	${Parse_SRC}
	${Main_SRC}
)

if (WIN32)
	#Inheritance via dominance warning
	target_compile_options(${PROJECT_NAME}.mini PRIVATE "/wd4250;")
else()
    target_link_libraries(${PROJECT_NAME}.mini pthread dl)
endif()

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${PROJECT_NAME}.mini PROPERTY CXX_STANDARD 20)
endif()
]]

