# A Package to wrap all Image APIs

cmake_minimum_required (VERSION 3.8)

project ("xlang_image")

include_directories("../Api")
include_directories(".")

FILE(GLOB API_SRC "../Api/value.cpp")
FILE(GLOB Main_SRC "*.cpp")

FILE(GLOB OS_SRC "turbojpeg/*.cpp")

if(APPLE)
find_path(TURBOJPEG_INCLUDE_DIR NAMES turbojpeg.h PATHS /usr/local/opt/jpeg-turbo/include)
find_library(TURBOJPEG_LIBRARY NAMES turbojpeg PATHS /usr/local/opt/jpeg-turbo/lib)
else()
link_directories("turbojpeg/lib")
endif()

add_library(${PROJECT_NAME} 
	SHARED
	${API_SRC}
	${Main_SRC}
	${OS_SRC}
)
if(WIN32)
target_link_libraries(${PROJECT_NAME} "turbojpeg-static-vc64")
elseif(APPLE)
target_link_libraries(${PROJECT_NAME} ${TURBOJPEG_LIBRARY})
else()
target_link_libraries(${PROJECT_NAME} PUBLIC "libturbojpeg-linux-${CMAKE_SYSTEM_PROCESSOR}.a")
endif()
