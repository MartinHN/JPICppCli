cmake_minimum_required(VERSION 3.2)

project(grideyeTest)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -O0 -ftime-trace -I/home/tinmar/Dev/cereal/include")


set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})

set(GETEST_INCLUDE_DIR ${PROJECT_SOURCE_DIR}../lib)
# set(GETEST_BIN_DIR ${PROJECT_SOURCE_DIR}/bin)
# set(GETEST_LIB_DIR ${PROJECT_SOURCE_DIR}/lib)

# set(GETEST_HEADERS_DIR ${PROJECT_SOURCE_DIR}/src/division)

include_directories(${GETEST_INCLUDE_DIR})
# include_directories(${GETEST_HEADERS_DIR})

# add_subdirectory(src)
# add_subdirectory(test)



################################
# GTest
################################
project(googletest-git NONE)

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.8.0
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(BUILD_GTEST ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)


add_executable(test test.cpp)
target_link_libraries(test gtest gtest_main)
