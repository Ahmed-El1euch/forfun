add_library(fungcc_core
    frontend/lexer.c
    frontend/parser.c
    frontend/ast.c
    backend/codegen.c
)

target_include_directories(fungcc_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_features(fungcc_core PRIVATE c_std_17)

add_executable(fungcc_driver
    driver/main.c
)

target_link_libraries(fungcc_driver
    PRIVATE
        fungcc_core
)
