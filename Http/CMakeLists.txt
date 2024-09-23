# CMakeList.txt : CMake project for http Package, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("xlang_http")

add_compile_definitions(CPPHTTPLIB_OPENSSL_SUPPORT)

if(APPLE)
	find_package(OpenSSL REQUIRED)
	include_directories(${OPENSSL_INCLUDE_DIR})
	link_directories(${OPENSSL_LIB_DIR})
    link_libraries(${OPENSSL_LIBRARIES})
endif()

include_directories("../Api")
include_directories("../Inc")
if (WIN32)
	include_directories("depends/windows/include")
	link_directories("depends/windows/lib/x64")

    # Extract libcrypto_static.zip
    set(LIBCRYPTO_ZIP "${CMAKE_CURRENT_SOURCE_DIR}/depends/windows/lib/x64/libcrypto_static.zip")
    set(LIBCRYPTO_LIB "${CMAKE_BINARY_DIR}/lib/libcrypto_static.lib")
    file(ARCHIVE_EXTRACT INPUT ${LIBCRYPTO_ZIP} DESTINATION ${CMAKE_BINARY_DIR}/lib)

    # Add the extracted library to the link path
    link_directories(${CMAKE_BINARY_DIR}/lib)
endif()

FILE(GLOB API_SRC "../Api/value.cpp")
FILE(GLOB Main_SRC "*.cpp")

add_library(${PROJECT_NAME} 
	SHARED
	${API_SRC}
	${Main_SRC})

if (WIN32)
    target_link_libraries(${PROJECT_NAME} libcrypto_static libssl_static)
elseif(APPLE)
else()
    target_link_libraries(${PROJECT_NAME} ssl)
endif()
