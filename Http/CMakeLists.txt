# CMakeList.txt : CMake project for http Package, include source and define
# project specific logic here.
#

cmake_minimum_required(VERSION 3.8)

project("xlang_http")

add_compile_definitions(CPPHTTPLIB_OPENSSL_SUPPORT)

# Handle OpenSSL detection on Apple
if (APPLE)
    find_package(OpenSSL REQUIRED)
    include_directories(${OPENSSL_INCLUDE_DIR})
    link_directories(${OPENSSL_LIB_DIR})
    link_libraries(${OPENSSL_LIBRARIES})

# Windows-specific configuration
elseif (WIN32)
    include_directories("depends/windows/include")
    link_directories("depends/windows/lib/x64")

    # Extract libcrypto_static.zip
    set(LIBCRYPTO_ZIP "${CMAKE_CURRENT_SOURCE_DIR}/depends/windows/lib/x64/libcrypto_static.zip")
    set(LIBCRYPTO_LIB "${CMAKE_BINARY_DIR}/lib/libcrypto_static.lib")
    file(ARCHIVE_EXTRACT INPUT ${LIBCRYPTO_ZIP} DESTINATION ${CMAKE_BINARY_DIR}/lib)

    # Add the extracted library to the link path
    link_directories(${CMAKE_BINARY_DIR}/lib)


# Non-Windows and Non-Apple configuration for OpenSSL (Linux, etc.)
else()
    # Run 'openssl version' to find the OpenSSL version and root directory
    execute_process(
        COMMAND openssl version -v
        OUTPUT_VARIABLE OPENSSL_VERSION_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Extract the OpenSSL version number
    string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" OPENSSL_VERSION "${OPENSSL_VERSION_OUTPUT}")

    # Print the OpenSSL version for debugging
    message(STATUS "Detected OpenSSL version: ${OPENSSL_VERSION}")

    # Check if OpenSSL version is 3.0.0 or higher
    if (OPENSSL_VERSION VERSION_LESS "3.0.0")
        message(WARNING "OpenSSL version is less than 3.0.0, skipping build for this package.")
        return()  # Exit from CMake without building the project
    else()
        # Run 'openssl version -d' to find the OpenSSL root directory
        execute_process(
            COMMAND openssl version -d
            OUTPUT_VARIABLE OPENSSL_ROOT_OUTPUT
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        # Print the OpenSSL output for debugging
        message(STATUS "OpenSSL version output: ${OPENSSL_ROOT_OUTPUT}")

        # Extract the directory from the output
        string(REGEX REPLACE ".*\"([^\"]+)\".*" "\\1" OPENSSL_ROOT_DIR "${OPENSSL_ROOT_OUTPUT}")

        # Print the extracted OpenSSL root directory
        message(STATUS "Extracted OpenSSL Root Directory: ${OPENSSL_ROOT_DIR}")

        # Include and link the directories based on the discovered OpenSSL path
        include_directories(${OPENSSL_ROOT_DIR}/include)
        link_directories(${OPENSSL_ROOT_DIR}/lib)

    endif()
endif()

# General project configuration
include_directories("../Api")
include_directories("../Inc")

# Source files
FILE(GLOB API_SRC "../Api/value.cpp")
FILE(GLOB Main_SRC "*.cpp")

add_library(${PROJECT_NAME} 
    SHARED
    ${API_SRC}
    ${Main_SRC})

# Link libraries based on the platform
if (WIN32)
    target_link_libraries(${PROJECT_NAME} libcrypto_static libssl_static)
elseif(APPLE)
    target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES})
else()
    target_link_libraries(${PROJECT_NAME} ssl crypto)
endif()
