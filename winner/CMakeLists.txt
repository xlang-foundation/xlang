# A Package to wrap all windows APIs

cmake_minimum_required (VERSION 3.8)

project ("xlang_win")

include_directories("../Api")

FILE(GLOB API_SRC "../Api/value.cpp")
FILE(GLOB Main_SRC "*.cpp")

add_library(${PROJECT_NAME} 
	SHARED
	${API_SRC}
	${Main_SRC}
)


if (WIN32)
    target_link_libraries(${PROJECT_NAME} d2d1)
endif()