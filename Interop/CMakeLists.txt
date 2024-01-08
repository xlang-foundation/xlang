cmake_minimum_required (VERSION 3.8)

project ("xlang_interop")

include_directories("../Api")

FILE(GLOB API_SRC "../Api/value.cpp" "../Api/xload.cpp")
FILE(GLOB Main_SRC "*.cpp")

add_library(${PROJECT_NAME} 
	SHARED
	${API_SRC}
	${Main_SRC}
)


# add_subdirectory ("dotnet")
