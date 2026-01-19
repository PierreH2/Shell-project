# src/CMakeLists.txt

add_subdirectory(lexer)
add_subdirectory(parser)
add_subdirectory(executer)
add_subdirectory(util)
add_subdirectory(cli)

add_executable(42sh
    main.c
)

target_link_libraries(42sh
    lexer
    parser
    executer
    util
    cli
)

add_library(project_headers INTERFACE)

target_include_directories(project_headers INTERFACE
    ${PROJECT_SOURCE_DIR}/src
)