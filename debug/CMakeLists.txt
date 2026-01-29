add_executable(lexer_dbg
    gdb_debug.c
)

target_link_libraries(lexer_dbg
    lexer
    parser
    executer
    util
)

add_library(syntax_error_stub
    syntax_error_stub.c
)
