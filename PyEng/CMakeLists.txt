﻿# CMakeList.txt : CMake project for Grus, include source and define
#Import python into X
cmake_minimum_required(VERSION 3.8)

set(PROJECT_NAME "pyeng")
project (${PROJECT_NAME})

find_package (Python3 COMPONENTS Interpreter Development NumPy)

string(REPLACE "numpy/core/include" "" Python3_SITELIB ${Python3_NumPy_INCLUDE_DIRS})

message(STATUS
    "Python: version=${Python3_VERSION} interpreter=${Python3_EXECUTABLE}")
if(NOT Python3_FOUND AND Python3_Development_FOUND)
    #   find_package() will not abort the build if anything's missing.
    string(JOIN "\n" errmsg
        "  Python3 and/or development libs not found."
        "  - Python3_FOUND=${Python3_FOUND}"
        "  - Python3_Development_FOUND=${Python3_Development_FOUND}"
        "  - Python3_INCLUDE_DIRS=${Python3_INCLUDE_DIRS}"
    )
    message(FATAL_ERROR ${errmsg})
else()
    string(JOIN "\n" infoMsg
        "  - Python3_FOUND=${Python3_FOUND}"
        "  - Python3_Development_FOUND=${Python3_Development_FOUND}"
        "  - Python3_INCLUDE_DIRS=${Python3_INCLUDE_DIRS}"
        "  - Python3_NumPy_INCLUDE_DIRS=${Python3_NumPy_INCLUDE_DIRS}"
        "  - Python3_LIBRARY_DIRS=${Python3_LIBRARY_DIRS}"
        "  - Python3_LIBRARIES=${Python3_LIBRARIES}"
        "  - Python3_SITELIB=${Python3_SITELIB}"
        "  - Python3_RUNTIME_LIBRARY_DIRS=${Python3_RUNTIME_LIBRARY_DIRS}"
        "  - _Python3_LIBRARY_DEBUG=${_Python3_LIBRARY_DEBUG}"
    )
    message(STATUS ${infoMsg})
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION")

#set(_Python3_LIBRARY_DEBUG,${_Python3_LIBRARY_RELEASE})
#set(Python3_LIBRARIES,"${_Python3_LIBRARY_RELEASE})
FILE(GLOB SRC *.cpp)
FILE(GLOB Out_SRC ../Ext/Locker.cpp
        ../Core/utility.cpp)


add_library(${PROJECT_NAME} 
	SHARED
    ${Out_SRC}
	${SRC}
)

target_include_directories(${PROJECT_NAME} 
	PUBLIC
    "../Inc"
    "../Ext"
    "../Core"
	"${Python3_INCLUDE_DIRS}"
	"${Python3_NumPy_INCLUDE_DIRS}"
	)
target_link_directories(${PROJECT_NAME} PUBLIC 
	"${Python3_LIBRARY_DIRS}"
	)

if (WIN32)
target_link_libraries(${PROJECT_NAME} PUBLIC 
    "${Python3_LIBRARIES}" Ws2_32
	)
else()
target_link_libraries(${PROJECT_NAME} PUBLIC 
	"${Python3_LIBRARIES}" uuid
	)
endif()