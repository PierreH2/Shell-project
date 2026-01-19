find_package(PkgConfig REQUIRED)
pkg_check_modules(CRITERION REQUIRED criterion)

# Chemin vers les headers du projet
set(PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/src)

# ---------- Lexer tests ----------
add_executable(lexer_tests
    test_lexer.c
)

target_include_directories(lexer_tests PRIVATE
    ${CRITERION_INCLUDE_DIRS}
    ${PROJECT_INCLUDE_DIR}
)

target_link_libraries(lexer_tests
    lexer
    util
    project_headers
    syntax_error_stub
    ${CRITERION_LIBRARIES}
)

add_test(NAME lexer_tests COMMAND lexer_tests)

# ---------- Parser tests ----------
add_executable(parser_tests
    test_parser.c
)

target_include_directories(parser_tests PRIVATE
    ${CRITERION_INCLUDE_DIRS}
    ${PROJECT_INCLUDE_DIR}
)

target_link_libraries(parser_tests
    parser
    lexer
    util
    project_headers
    syntax_error_stub
    ${CRITERION_LIBRARIES}
)

add_test(NAME parser_tests COMMAND parser_tests)

# ---------- Executer tests ----------
add_executable(executer_tests
    test_executer.c
)

target_include_directories(executer_tests PRIVATE
    ${CRITERION_INCLUDE_DIRS}
    ${PROJECT_INCLUDE_DIR}
)

target_link_libraries(executer_tests
    executer
    parser
    lexer
    util
    project_headers
    syntax_error_stub
    ${CRITERION_LIBRARIES}
)

add_test(NAME executer_tests COMMAND executer_tests)

# ---------- Full shell tests (E2E) ----------
add_executable(shell_tests
    test_e2e.c
)

target_include_directories(shell_tests PRIVATE
    ${CRITERION_INCLUDE_DIRS}
    ${PROJECT_INCLUDE_DIR}
)

target_link_libraries(shell_tests
    executer
    parser
    lexer
    util
    project_headers
    syntax_error_stub
    ${CRITERION_LIBRARIES}
)

add_test(NAME shell_tests COMMAND shell_tests)
