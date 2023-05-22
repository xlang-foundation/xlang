# CMakeList.txt : CMake project for X-Lang, build all in one , include source and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.8)
project ("xlang_all")

include_directories("../Api")
include_directories("../Inc")
include_directories("../Main")
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
include_directories("../Tensor")


FILE(GLOB Api_SRC "../Api/*.cpp")
FILE(GLOB Utils_SRC "../utils/*.cpp")
FILE(GLOB Core_SRC "../Core/*.cpp")
FILE(GLOB Ast_SRC "../Ast/*.cpp")
FILE(GLOB Serialize_SRC "../Serialize/*.cpp")
FILE(GLOB Parse_SRC "../Parse/*.cpp")
FILE(GLOB Thread_SRC "../Thread/*.cpp")
FILE(GLOB Ext_SRC "../Ext/*.cpp")
FILE(GLOB Text_SRC "../Text/*.cpp")
FILE(GLOB Rpc_SRC "../rpc/*.cpp")
FILE(GLOB Nn_SRC "../Tensor/*.cpp")
FILE(GLOB Main_SRC "../Main/AddScripts.cpp" "../Main/Hosting.cpp" "../Main/xhost_impl.cpp")

FILE(GLOB AppEntry_SRC "../X.cpp" "../cli.cpp")


add_executable (${PROJECT_NAME}
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
	${AppEntry_SRC}
)
if(NOT (WIN32))
target_link_libraries(${PROJECT_NAME} pthread dl)
endif()

