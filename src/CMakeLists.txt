add_library(fungcc_core INTERFACE)

target_include_directories(fungcc_core
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

add_executable(fungcc_driver
    driver/main.c
)

target_link_libraries(fungcc_driver
    PRIVATE
        fungcc_core
)
