# CMakeList.txt : CMake project for yaml Package, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("xlang_yaml")

add_compile_definitions(YAML_CPP_STATIC_DEFINE=1)


include_directories("../Api")
include_directories("../Inc")
include_directories("../ThirdParty/yaml-cpp/include/yaml-cpp")
include_directories("../ThirdParty/yaml-cpp/include")


FILE(GLOB Yaml_SRC "../ThirdParty/yaml-cpp/src/*.cpp")

FILE(GLOB API_SRC "../Api/value.cpp")
FILE(GLOB Main_SRC "*.cpp")

add_library(${PROJECT_NAME} 
	SHARED
	${API_SRC}
	${Yaml_SRC}
	${Main_SRC})
