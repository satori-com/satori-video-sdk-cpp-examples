cmake_minimum_required(VERSION 3.7)
project (empty-bot VERSION 0.1 LANGUAGES CXX)

include(../conan.cmake)

conan_cmake_run(CONANFILE conanfile.txt
                BASIC_SETUP CMAKE_TARGETS
                UPDATE
                BUILD missing)

add_executable(empty-bot main.cpp)
set_property(TARGET empty-bot PROPERTY CXX_STANDARD 14)

target_link_libraries(empty-bot PRIVATE
        CONAN_PKG::SatoriVideo)