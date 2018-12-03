cmake_minimum_required(VERSION 3.3)
project(test_pmem_hash_map CXX)

set(CXX_STANDARD_REQUIRED ON)
set(CXX_STANDARD 11 CACHE STRING "C++ language standard")
set(CMAKE_CXX_STANDARD ${CXX_STANDARD})

include(FindThreads)
include(CheckCXXCompilerFlag)
include(${CMAKE_SOURCE_DIR}/cmake/functions.cmake)

find_package(PkgConfig QUIET)

message(STATUS "ENV{PKG_CONFIG_PATH}: " $ENV{PKG_CONFIG_PATH})
set(ENV{PKG_CONFIG_PATH} $ENV{PKG_CONFIG_PATH} /lib64/pkgconfig) # 
set(ENV{PKG_CONFIG_PATH} $ENV{PKG_CONFIG_PATH}:/usr/local/lib64/pkgconfig)
message(STATUS "after set: " $ENV{PKG_CONFIG_PATH})
if(PKG_CONFIG_FOUND)
  pkg_check_modules(LIBPMEMOBJ++ REQUIRED libpmemobj++)
else()
  find_package(LIBPMEMOBJ++ REQUIRED)
endif()

link_directories(${LIBPMEMOBJ++_LIBRARY_DIRS})
#link_directories(${LIBPMEM_LIBRARY_DIRS})

# my test
add_flag(-Wpointer-arith)
add_flag(-Wsign-compare)
add_flag(-Wunreachable-code-return)
add_flag(-Wmissing-variable-declarations)
add_flag(-fno-common)

add_flag(-ggdb DEBUG)
add_flag(-DDEBUG DEBUG)
add_flag("-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2" RELEASE)


option(BUILD_TESTS "build tests" ON)

set(TEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/test
  CACHE STRING "working directory for tests")

if(BUILD_TESTS)
  if(TEST_DIR)
    enable_testing()
  else()
    message(WARNING "TEST_DIR is empty - 'make test' will not work")
  endif()
  add_subdirectory(tests)
endif()

# set(CMAKE_BUILD_TYPE "Debug")
# set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -o0 -Wall -g2 -ggdb")
# set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -o2 -Wall")

