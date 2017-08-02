cmake_minimum_required(VERSION 3.7)
project (empty-bot VERSION 0.1 LANGUAGES CXX)

include(../conan.cmake)

conan_cmake_run(CONANFILE conanfile.txt
                BASIC_SETUP CMAKE_TARGETS
                UPDATE
                BUILD missing)

