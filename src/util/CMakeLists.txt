add_library(util
    vec.c
    str.c
    error.c
)

target_link_libraries(util
    project_headers
)
