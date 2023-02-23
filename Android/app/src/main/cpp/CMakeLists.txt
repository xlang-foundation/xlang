# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.22.1)

# Declares and names the project.
# todo: change the name to xlang_eng to keep same as in other os build
# xlang was the original name when generated this android studio project
# maybe some where it keep this name, so if changed,
# debug can't find the natvie module's symbols
project("xlang")
set(CMAKE_CXX_STANDARD 17)

add_compile_definitions(XLANG_ENG_DBG=1)

set(XLANG_ROOT "../../../../..")

include_directories("${XLANG_ROOT}/Api")
include_directories("${XLANG_ROOT}/Inc")
include_directories("${XLANG_ROOT}/utils")
include_directories("${XLANG_ROOT}/Ast")
include_directories("${XLANG_ROOT}/Parse")
include_directories("${XLANG_ROOT}/Serialize")
include_directories("${XLANG_ROOT}/rpc")
include_directories("${XLANG_ROOT}/Thread")
include_directories("${XLANG_ROOT}/Ext")
include_directories("${XLANG_ROOT}/Core")
include_directories("${XLANG_ROOT}/Text")
include_directories("${XLANG_ROOT}/Scripts")
include_directories("${XLANG_ROOT}/Main")

FILE(GLOB Api_SRC "${XLANG_ROOT}/Api/value.cpp")
FILE(GLOB Utils_SRC "${XLANG_ROOT}/utils/*.cpp")
FILE(GLOB Core_SRC "${XLANG_ROOT}/Core/*.cpp")
FILE(GLOB Ast_SRC "${XLANG_ROOT}/Ast/*.cpp")
FILE(GLOB Serialize_SRC "${XLANG_ROOT}/Serialize/*.cpp")
FILE(GLOB Parse_SRC "${XLANG_ROOT}/Parse/*.cpp")
FILE(GLOB Thread_SRC "${XLANG_ROOT}/Thread/*.cpp")
FILE(GLOB Ext_SRC "${XLANG_ROOT}/Ext/*.cpp")
FILE(GLOB Text_SRC "${XLANG_ROOT}/Text/*.cpp")
FILE(GLOB Rpc_SRC "${XLANG_ROOT}/rpc/*.cpp")
FILE(GLOB Main_SRC "${XLANG_ROOT}/Main/*.cpp")
FILE(GLOB Android_SRC "*.cpp")

add_library(${PROJECT_NAME} SHARED
        ${Api_SRC}
        ${Utils_SRC}
        ${Core_SRC}
        ${Ast_SRC}
        ${Parse_SRC}
        ${Serialize_SRC}
        ${Thread_SRC}
        ${Ext_SRC}
        ${Text_SRC}
        ${Rpc_SRC}
        ${Main_SRC}
        ${Android_SRC}
        )


# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
        ${PROJECT_NAME}

        # Links the target library to the log library
        # included in the NDK.
        ${log-lib})