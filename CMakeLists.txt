cmake_minimum_required(VERSION 3.10)
project(CVMpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)

add_library(cvmpp_core
    src/bytecode_io.cpp
    src/lexer.cpp
    src/parser.cpp
    src/compiler.cpp
    src/vm.cpp
)

target_include_directories(cvmpp_core PUBLIC include)

add_executable(cvmpp
    src/main.cpp
)

target_link_libraries(cvmpp PRIVATE cvmpp_core)

if(BUILD_TESTING)
    add_executable(cvmpp_tests
        tests/test_pipeline.cpp
    )

    target_link_libraries(cvmpp_tests PRIVATE cvmpp_core)

    add_test(NAME cvmpp_tests COMMAND cvmpp_tests)
endif()
