add_executable(test
        test.c
        )

# pull in common dependencies
target_link_libraries(test pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(test)
