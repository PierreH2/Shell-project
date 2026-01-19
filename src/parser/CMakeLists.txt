add_library(parser
    parser.c
    ast.c
)

target_link_libraries(parser
    project_headers
)