cmake_minimum_required(VERSION 3.7)
project(haar-cascades-bot VERSION 0.1 LANGUAGES CXX)

set(CMAKE_C_FLAGS_DEBUG "-DBOT_DEBUG")

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

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_executable(haar-cascades-bot src/main.cpp)
set_property(TARGET haar-cascades-bot PROPERTY CXX_STANDARD 14)
target_link_libraries(haar-cascades-bot PRIVATE
        CONAN_PKG::SatoriVideo
        CONAN_PKG::Gsl)

if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/models/frontalface_default.xml")
  message(STATUS "Downloading Haar cascade for face detection")
  file(DOWNLOAD "https://raw.githubusercontent.com/opencv/opencv/master/data/haarcascades/haarcascade_frontalface_default.xml"
          "${CMAKE_CURRENT_BINARY_DIR}/models/frontalface_default.xml")
endif()

if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/models/smile.xml")
  message(STATUS "Downloading Haar cascade for smile detection")
  file(DOWNLOAD "https://raw.githubusercontent.com/opencv/opencv/master/data/haarcascades/haarcascade_smile.xml"
          "${CMAKE_CURRENT_BINARY_DIR}/models/smile.xml")
endif()
