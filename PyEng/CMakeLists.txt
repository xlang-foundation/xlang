cmake_minimum_required(VERSION 3.16)
project(pyeng_multi LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNPY_NO_DEPRECATED_API=NPY_1_7_API_VERSION")


file(GLOB SRC *.cpp)
file(GLOB Out_SRC ../utils/utility.cpp)
file(GLOB Xlang_Api_SRC "../Api/*.cpp")

# -------------------------------
# Function to build for one Python
# -------------------------------
function(add_pyeng_target target_name py_root py_tag abi_tag)
    if(WIN32)
        set(python_exec "${py_root}/python.exe")
    else()
        set(python_exec "${py_root}/bin/python${py_tag}")
    endif()

    # Ask Python for include and numpy
    execute_process(
        COMMAND ${python_exec} -c "import sysconfig; print(sysconfig.get_paths()['include'])"
        OUTPUT_VARIABLE py_include
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
        COMMAND ${python_exec} -c "import numpy; print(numpy.get_include())"
        OUTPUT_VARIABLE numpy_include
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(WIN32)
        # Query LIBDIR and LIBRARY
        execute_process(
            COMMAND ${python_exec} -c "import sysconfig; print(sysconfig.get_config_var('LIBDIR'))"
            OUTPUT_VARIABLE py_libdir
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        execute_process(
            COMMAND ${python_exec} -c "import sysconfig; print(sysconfig.get_config_var('LIBRARY'))"
            OUTPUT_VARIABLE py_library
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        # Fix: fall back if Python doesn’t report anything
        if("${py_libdir}" STREQUAL "None" OR "${py_libdir}" STREQUAL "")
            set(py_libdir "${py_root}/libs")
        endif()
        if("${py_library}" STREQUAL "None" OR "${py_library}" STREQUAL "")
            set(py_library "python${py_tag}.lib")
        endif()

        # set(py_lib "${py_libdir}/${py_library}")
        set(py_lib "${py_root}/libs/python${py_tag}.lib")
        set(ext ".dll")
        set(extra_libs Ws2_32)
    else()
        execute_process(
            COMMAND ${python_exec} -c "import sysconfig; print(sysconfig.get_config_var('LIBDIR'))"
            OUTPUT_VARIABLE py_libdir
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        execute_process(
            COMMAND ${python_exec} -c "import sysconfig; print(sysconfig.get_config_var('LDLIBRARY'))"
            OUTPUT_VARIABLE py_ldlibrary
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set(py_lib "${py_libdir}/${py_ldlibrary}")
        set(ext ".so")
        set(extra_libs "")
    endif()

    message(STATUS "---- ${target_name} ----")
    message(STATUS "Python exec : ${python_exec}")
    message(STATUS "Include dir : ${py_include}")
    message(STATUS "NumPy inc   : ${numpy_include}")
    message(STATUS "Library     : ${py_lib}")

    add_library(${target_name} SHARED
        ${SRC}
        ${Out_SRC}
        ${Xlang_Api_SRC}
    )

    set_target_properties(${target_name} PROPERTIES
        PREFIX ""
        SUFFIX ${ext}
        OUTPUT_NAME "pyeng.${abi_tag}"
    )

    target_include_directories(${target_name} PUBLIC
        "../Inc"
        "../utils"
        ${py_include}
        ${numpy_include}
    )

    target_link_libraries(${target_name} PUBLIC ${py_lib} ${extra_libs})
endfunction()


# -------------------------------
# Build for each Python version
# -------------------------------

set(useDefaultPython TRUE)

# Python 3.12
if(DEFINED ENV{PY312_ROOT})
    add_pyeng_target(pyeng312
        "$ENV{PY312_ROOT}"
        "312"
        "cp312"
    )
    set(useDefaultPython FALSE)
endif()

# Python 3.13
if(DEFINED ENV{PY313_ROOT})
    add_pyeng_target(pyeng313
        "$ENV{PY313_ROOT}"
        "313"
        "cp313"
    )
    set(useDefaultPython FALSE)
endif()

# Default Python (system one)
if(useDefaultPython)
    set(target_name "pyeng")
    # Prefer shared libpython to avoid linking static .a into a shared object
    set(Python3_USE_STATIC_LIBS OFF)
    find_package(Python3 COMPONENTS Interpreter Development NumPy REQUIRED)

    string(REPLACE "numpy/core/include" "" Python3_SITELIB "${Python3_NumPy_INCLUDE_DIRS}")

    message(STATUS "Python: version=${Python3_VERSION} interpreter=${Python3_EXECUTABLE}")
    message(STATUS "  - Python3_FOUND=${Python3_FOUND}")
    message(STATUS "  - Python3_Development_FOUND=${Python3_Development_FOUND}")
    message(STATUS "  - Python3_INCLUDE_DIRS=${Python3_INCLUDE_DIRS}")
    message(STATUS "  - Python3_NumPy_INCLUDE_DIRS=${Python3_NumPy_INCLUDE_DIRS}")
    message(STATUS "  - Python3_LIBRARIES=${Python3_LIBRARIES}")

    add_library(${target_name} SHARED
        ${SRC}
        ${Out_SRC}
        ${Xlang_Api_SRC}
    )
    # Ensure position-independent code for all objects used in the shared library
    set_property(TARGET ${target_name} PROPERTY POSITION_INDEPENDENT_CODE ON)

    target_include_directories(${target_name} PUBLIC
        "../Inc"
        "../utils"
        "${Python3_INCLUDE_DIRS}"
        "${Python3_NumPy_INCLUDE_DIRS}"
    )

    # Link against the imported target to prefer the shared Python library
    target_link_libraries(${target_name} PUBLIC Python3::Python)

    if(WIN32)
        target_link_libraries(${target_name} PUBLIC Ws2_32)
        configure_file("${PROJECT_SOURCE_DIR}/install_py_xlang.bat"
                       "${CMAKE_BINARY_DIR}/bin/install_py_xlang.bat")
    else()
        configure_file("${PROJECT_SOURCE_DIR}/install_py_xlang.sh"
                       "${CMAKE_BINARY_DIR}/bin/install_py_xlang.sh")
    endif()
endif()
