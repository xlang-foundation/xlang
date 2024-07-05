add_executable(test
        test.cpp
        )

# target_compile_options(test PRIVATE -frtti)
set_source_files_properties(test.cpp PROPERTIES COMPILE_OPTIONS "-frtti")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti")
# target_compile_options(test PRIVATE -frtti)

# Optionally, explicitly enable RTTI globally
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti")
# message(STATUS "C++ compile flags: ${CMAKE_CXX_FLAGS}")

# pull in common dependencies
target_link_libraries(test pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(test)
