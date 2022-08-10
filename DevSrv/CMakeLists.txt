# CMakeList.txt : CMake project for DevOps Server
#
cmake_minimum_required (VERSION 3.8)

project ("xdev")

include_directories("../Inc")
include_directories("../Ext")
include_directories("../Http")

FILE(GLOB Main_SRC "*.cpp")
FILE(GLOB Dev_SRC "../Ext/gthread.cpp" 
		"../Ext/ipc.cpp"
		"../Ext/Locker.cpp"
		"../Ext/wait.cpp"
		"../Core/utility.cpp")

if (WIN32)
	add_executable (${PROJECT_NAME} WIN32 
		${Main_SRC} ${Dev_SRC})
else()
	add_executable (${PROJECT_NAME} ${Main_SRC}
		${Dev_SRC})
    target_link_libraries(${PROJECT_NAME} pthread dl)
endif()

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()
