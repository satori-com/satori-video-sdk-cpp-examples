cmake_minimum_required(VERSION 3.7)
project (empty-opencv-bot VERSION 1.0 LANGUAGES CXX)

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
  SET(CMAKE_BUILD_TYPE "Debug")
ENDIF()

# Download automatically, you can also just copy the conan.cmake file
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
  message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
  file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/master/conan.cmake"
                "${CMAKE_BINARY_DIR}/conan.cmake")
endif()
include(${CMAKE_BINARY_DIR}/conan.cmake)

conan_cmake_run(CONANFILE conanfile.txt
                BASIC_SETUP CMAKE_TARGETS
                UPDATE
                BUILD outdated)

add_executable(empty-opencv-bot src/main.cpp)
set_property(TARGET empty-opencv-bot PROPERTY CXX_STANDARD 14)

target_link_libraries(empty-opencv-bot PRIVATE
        CONAN_PKG::SatoriVideo)