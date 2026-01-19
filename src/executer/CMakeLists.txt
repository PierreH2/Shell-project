add_library(executer
    executer.c
    builtins.c
)

target_link_libraries(executer
    project_headers
)
