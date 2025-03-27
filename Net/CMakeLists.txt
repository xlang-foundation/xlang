# CMakeList.txt : CMake project for http Package, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("xlang_net")

include_directories("../ThirdParty")
include_directories("../Inc")
include_directories("../Api")
include_directories("../utils")

if(APPLE)
	find_package(OpenSSL REQUIRED)
	include_directories(${OPENSSL_INCLUDE_DIR})
	link_directories(${OPENSSL_LIB_DIR})
    link_libraries(${OPENSSL_LIBRARIES})
endif()

if (WIN32)
	include_directories("../ThirdParty/OpenSSL/Win64/include")
	link_directories("../ThirdParty/OpenSSL/Win64/lib")
endif()

FILE(GLOB API_SRC "../Api/value.cpp")
FILE(GLOB Utils_SRC "../utils/*.cpp")
FILE(GLOB Main_SRC "*.cpp")

add_library(${PROJECT_NAME} 
	SHARED
	${API_SRC}
	${Utils_SRC}
	${Main_SRC}
)
