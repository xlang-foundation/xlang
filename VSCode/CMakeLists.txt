project(vscode_extension)

find_program(NPM npm)
find_program(NPX npx)

if(NPM AND NPX)
    # Ensure output directory exists
    if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    endif()

    # Read version from package.json
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/package.json" PACKAGE_JSON_CONTENT)
    string(REGEX MATCH "\"version\":[ \t]*\"([0-9]+\\.[0-9]+\\.[0-9]+)\"" _ ${PACKAGE_JSON_CONTENT})
    set(EXTENSION_VERSION ${CMAKE_MATCH_1})

    if(NOT EXTENSION_VERSION)
        set(EXTENSION_VERSION "0.0.1") # Fallback
        message(WARNING "Could not extract version from package.json")
    endif()

    add_custom_target(vscode_extension ALL
        COMMAND ${NPM} install
        COMMAND ${NPM} run compile
        COMMAND ${NPX} vsce package -o ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/xlang-${EXTENSION_VERSION}.vsix
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Building VSCode extension to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/xlang-${EXTENSION_VERSION}.vsix"
    )
else()
    message(WARNING "npm or npx not found. VSCode extension will not be built.")
endif()
