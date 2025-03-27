# CMakeList.txt : CMake project for X-Lang, include source and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.8)
project ("xlang")
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/Api)
configure_file("${PROJECT_SOURCE_DIR}/Api/value.h" "${CMAKE_BINARY_DIR}/bin/Api/value.h")
configure_file("${PROJECT_SOURCE_DIR}/Api/value.cpp" "${CMAKE_BINARY_DIR}/bin/Api/value.cpp")
configure_file("${PROJECT_SOURCE_DIR}/Api/xhost.h" "${CMAKE_BINARY_DIR}/bin/Api/xhost.h")
configure_file("${PROJECT_SOURCE_DIR}/Api/xlang.h" "${CMAKE_BINARY_DIR}/bin/Api/xlang.h")
configure_file("${PROJECT_SOURCE_DIR}/Api/xload.h" "${CMAKE_BINARY_DIR}/bin/Api/xload.h")
configure_file("${PROJECT_SOURCE_DIR}/Api/xload.cpp" "${CMAKE_BINARY_DIR}/bin/Api/xload.cpp")
configure_file("${PROJECT_SOURCE_DIR}/Api/xlstream.h" "${CMAKE_BINARY_DIR}/bin/Api/xlstream.h")
configure_file("${PROJECT_SOURCE_DIR}/Api/xpackage.h" "${CMAKE_BINARY_DIR}/bin/Api/xpackage.h")
configure_file("${PROJECT_SOURCE_DIR}/Api/xport.h" "${CMAKE_BINARY_DIR}/bin/Api/xport.h")
configure_file("${PROJECT_SOURCE_DIR}/Api/xproxy.h" "${CMAKE_BINARY_DIR}/bin/Api/xproxy.h")


# To enable Link Time Optimization (LTO) for Release builds.
if(MSVC)
    # Enable LTO for MSVC
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GL")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG")
    set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS_RELEASE} /LTCG")
    set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} /LTCG")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # Enable LTO for GCC
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
endif()

# Set static runtime linking for MSVC compiler
if(MSVC)
    # Enable static linking of the Microsoft Visual C++ runtime libraries
    foreach(flag_var
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        if(${flag_var} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endif()
    endforeach()
endif()



set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories("Api")

FILE(GLOB Api_SRC "Api/*.cpp")
FILE(GLOB AppEntry_SRC "X.cpp" "cli.cpp")



add_executable (${PROJECT_NAME}
	${Api_SRC}
	${AppEntry_SRC}
)
if(NOT (WIN32))
target_link_libraries(${PROJECT_NAME} pthread dl)
endif()
add_subdirectory("Main")
add_subdirectory("os")
add_subdirectory("Http")
add_subdirectory("PyEng")
add_subdirectory("DevSrv")
if(WIN32)
add_subdirectory("winner")
endif()
add_subdirectory("sqlite")
add_subdirectory("Image")
add_subdirectory("WebCore")
add_subdirectory("Net")
add_subdirectory("all")
add_subdirectory("Interop")
add_subdirectory("algorithm")
if(NOT (BARE_METAL))
add_subdirectory("BareLink")
add_subdirectory("Yaml")
endif()

