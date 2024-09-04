# CMakeList.txt : CMake project for Grus, include source and define
#Import python into X
cmake_minimum_required(VERSION 3.8)

set(PROJECT_NAME "pyeng")
project (${PROJECT_NAME})

find_package (Python3 COMPONENTS Interpreter Development NumPy)

string(REPLACE "numpy/core/include" "" Python3_SITELIB ${Python3_NumPy_INCLUDE_DIRS})

message(STATUS
    "Python: version=${Python3_VERSION} interpreter=${Python3_EXECUTABLE}")
if(NOT Python3_FOUND OR NOT Python3_Development_FOUND OR NOT Python3_NumPy_INCLUDE_DIRS)
    #   find_package() will not abort the build if anything's missing.
    string(JOIN "\n" errmsg
        "  Python3 and/or development libs not found."
        "  - Python3_FOUND=${Python3_FOUND}"
        "  - Python3_Development_FOUND=${Python3_Development_FOUND}"
        "  - Python3_INCLUDE_DIRS=${Python3_INCLUDE_DIRS}"
    )
    message(WARNING  ${errmsg})
else()
    string(JOIN "\n" infoMsg
        "  - Python3_FOUND=${Python3_FOUND}"
        "  - Python3_Development_FOUND=${Python3_Development_FOUND}"
        "  - Python3_INCLUDE_DIRS=${Python3_INCLUDE_DIRS}"
        "  - Python3_NumPy_INCLUDE_DIRS=${Python3_NumPy_INCLUDE_DIRS}"
        "  - Python3_LIBRARY_DIRS=${Python3_LIBRARY_DIRS}"
        "  - Python3_LIBRARIES=${Python3_LIBRARIES}"
        "  - Python3_SITELIB=${Python3_SITELIB}"
        "  - Python3_RUNTIME_LIBRARY_DIRS=${Python3_RUNTIME_LIBRARY_DIRS}"
        "  - _Python3_LIBRARY_DEBUG=${_Python3_LIBRARY_DEBUG}"
    )
    message(STATUS ${infoMsg})

    set(REL_TAG "optimized")
    set(DBG_TAG "debug")

    string(FIND "${Python3_LIBRARIES}" "${REL_TAG}" pos_1)

    string(LENGTH "${REL_TAG}" len_1)
    string(FIND "${Python3_LIBRARIES}" "${DBG_TAG}" pos_2)
    MATH(EXPR len ${pos_2}-${len_1}) 
    IF(${pos_1} GREATER_EQUAL 0)
        string(SUBSTRING "${Python3_LIBRARIES}" ${len_1} ${len} Python3_LIBRARIES)
        message(STATUS ${pos_1} " " ${pos_2} " " ${Python3_LIBRARIES})
    ENDIF()

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION")

    #set(_Python3_LIBRARY_DEBUG,${_Python3_LIBRARY_RELEASE})
    #set(Python3_LIBRARIES,"${_Python3_LIBRARY_RELEASE})
    FILE(GLOB SRC *.cpp)
    FILE(GLOB Out_SRC ../utils/utility.cpp)
    file(GLOB Xlang_Api_SRC "../Api/*.cpp")

    add_library(${PROJECT_NAME} 
	    SHARED
        ${Out_SRC}
        ${Xlang_Api_SRC}
	    ${SRC})

    target_include_directories(${PROJECT_NAME} 
	    PUBLIC
        "../Inc"
        "../utils"
	    "${Python3_INCLUDE_DIRS}"
	    "${Python3_NumPy_INCLUDE_DIRS}"
	    )
    target_link_directories(${PROJECT_NAME} PUBLIC 
	    "\"${Python3_LIBRARY_DIRS}\""
	    )

    if (WIN32)
    target_link_libraries(${PROJECT_NAME} PUBLIC 
        "${Python3_LIBRARIES}" Ws2_32
	    )

    # Custom post-build step for Windows
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> "${Python3_SITELIB}/xlang.pyd"
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PDB_FILE:${PROJECT_NAME}> "${Python3_SITELIB}/xlang.pdb"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${Python3_SITELIB}/xlang"
    )

    # Find all xlang_*.dll and xlang_*.pdb files
    file(GLOB XLANG_DLLS "${CMAKE_BINARY_DIR}/bin/xlang_*.dll")
    file(GLOB XLANG_PDBS "${CMAKE_BINARY_DIR}/bin/xlang_*.pdb")

    # Copy each DLL and PDB file individually
    foreach(file ${XLANG_DLLS} ${XLANG_PDBS})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${file} "${Python3_SITELIB}/xlang"
        )
    endforeach()

    else()
    target_link_libraries(${PROJECT_NAME} PUBLIC 
        "${Python3_LIBRARIES}"
	    )
    endif()

endif()

