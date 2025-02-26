# CMakeList.txt : CMake project for OS Package, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)
project ("xlang_sqlite")

# Force static linking of runtime libraries
if(MSVC)
    # Disable specific default libraries
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /NODEFAULTLIB:MSVCRT /NODEFAULTLIB:MSVCRTD")
endif()

include_directories("../Api")
FILE(GLOB API_SRC "../Api/value.cpp")
FILE(GLOB Main_SRC "*.cpp")
FILE(GLOB sqlite_src sqlite/sqlite3.c)

# Create the library
add_library(${PROJECT_NAME} 
    SHARED
    ${API_SRC}
    ${Main_SRC}
    ${sqlite_src}
)

# Force static runtime for all source files
if(MSVC)
    # Set static compile flags for all source files
    target_compile_options(${PROJECT_NAME} PRIVATE 
                          /MT         # Use static runtime library
                          /EHsc       # Enable C++ exceptions
                          /D_CRT_SECURE_NO_WARNINGS
                          /DSQLITE_THREADSAFE=1)
    
    # Add these specifically for SQLite
    set_source_files_properties(${sqlite_src} PROPERTIES 
                               COMPILE_FLAGS "/MT")

    # Link with specific static libraries
    target_link_libraries(${PROJECT_NAME} PRIVATE
                         # Core static runtime libraries
                         libcmt.lib
                         oldnames.lib  # For legacy C functions
                         
                         # Additional libraries for thread functions
                         libconcrt.lib
                         libvcruntime.lib
                         libucrt.lib
                         Synchronization.lib)
endif()