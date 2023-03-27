# A Package to wrap all Image APIs

cmake_minimum_required (VERSION 3.8)

project ("xlang_image")

include_directories("../Api")
include_directories(".")

FILE(GLOB API_SRC "../Api/value.cpp")
FILE(GLOB Main_SRC "*.cpp")

FILE(GLOB OS_SRC "turbojpeg/*.cpp")
link_directories("turbojpeg/lib")

add_library(${PROJECT_NAME} 
	SHARED
	${API_SRC}
	${Main_SRC}
	${OS_SRC}
)

target_link_libraries(${PROJECT_NAME} turbojpeg-static-vc64)
