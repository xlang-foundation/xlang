# A Package to wrap all Image APIs

cmake_minimum_required (VERSION 3.8)

project ("xlang_image")

include_directories("../Api")
include_directories(".")

FILE(GLOB API_SRC "../Api/value.cpp")
FILE(GLOB Main_SRC "*.cpp")
if (WIN32)
FILE(GLOB OS_SRC "win32/*.cpp")
else()
FILE(GLOB OS_SRC "dummy/*.cpp")
endif()
add_library(${PROJECT_NAME} 
	SHARED
	${API_SRC}
	${Main_SRC}
	${OS_SRC}
)


if (WIN32)
    target_link_libraries(${PROJECT_NAME} d2d1 Comctl32)
endif()