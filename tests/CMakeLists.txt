add_executable(test_lexer
    unit/test_lexer.c
)

add_executable(test_parser
    unit/test_parser.c
)

add_executable(test_codegen
    unit/test_codegen.c
)

foreach(target test_lexer test_parser test_codegen)
    target_link_libraries(${target}
        PRIVATE
            fungcc_core
    )

    target_include_directories(${target}
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
    )

    target_compile_features(${target} PRIVATE c_std_17)
endforeach()

add_test(NAME lexer COMMAND test_lexer)
add_test(NAME parser COMMAND test_parser)
add_test(NAME codegen COMMAND test_codegen)
