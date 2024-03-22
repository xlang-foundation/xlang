# CMakeList.txt : CMake project for http Package, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("xlang_http")

add_compile_definitions(CPPHTTPLIB_OPENSSL_SUPPORT)

include_directories("../Api")
if (WIN32)
	include_directories("depends/windows/include")
	link_directories("depends/windows/lib")
endif()

FILE(GLOB API_SRC "../Api/value.cpp")
FILE(GLOB Main_SRC "*.cpp")

add_library(${PROJECT_NAME} 
	SHARED
	${API_SRC}
	${Main_SRC}
 "cypher.cpp")

if (WIN32)
    target_link_libraries(${PROJECT_NAME} libcrypto_static libssl_static)
else()
    target_link_libraries(${PROJECT_NAME} ssl)
endif()
