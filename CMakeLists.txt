cmake_minimum_required(VERSION 3.3)
project(test_pmem_hash_map C CXX)

set(LIBPMEMOBJ_REQUIRED_VERSION 1.4)

set(CXX_STANDARD_REQUIRED ON)

set(CXX_STANDARD 11 CACHE STRING "C++ language standard")
set(CMAKE_CXX_STANDARD ${CXX_STANDARD})

include(FindPerl)
include(FindThreads)
include(CMakeDependentOption)
include(CMakePackageConfigHelpers)
include(CheckCXXSourceCompiles)
include(CheckCXXCompilerFlag)
include(GNUInstallDirs)

if(NOT WIN32)
  find_package(PkgConfig QUIET)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

if(NOT PERL_FOUND)
  message(FATAL_ERROR "Perl not found")
endif()
if (PERL_VERSION_STRING VERSION_LESS 5.16)
  message(FATAL_ERROR "Too old Perl (<5.16)")
endif()

if(PKG_CONFIG_FOUND)
  pkg_check_modules(LIBPMEMOBJ REQUIRED libpmemobj>=1.4)
else()
  find_package(LIBPMEMOBJ REQUIRED 1.4)
endif()

if(PKG_CONFIG_FOUND)
  pkg_check_modules(VALGRIND QUIET valgrind)
else()
  find_package(VALGRIND QUIET)
endif()

if(PKG_CONFIG_FOUND)
  pkg_check_modules(LIBUNWIND QUIET libunwind)
else()
  find_package(LIBUNWIND QUIET)
endif()
if(NOT LIBUNWIND_FOUND)
  message(WARNING "libunwind not found. Stack traces from tests will not be reliable")
endif()

# Check for existence of pmemvlt (introduced after 1.4 release)
set(SAVED_CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES})
set(CMAKE_REQUIRED_INCLUDES ${LIBPMEMOBJ_INCLUDE_DIRS})
CHECK_CXX_SOURCE_COMPILES(
  "#include <libpmemobj.h>
  struct pmemvlt vlt;
  int main() {}"
  PMEMVLT_PRESENT)
set(CMAKE_REQUIRED_INCLUDES ${SAVED_CMAKE_REQUIRED_INCLUDES})

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.hpp")

configure_file(${CMAKE_SOURCE_DIR}/cmake/libpmemobj++.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/libpmemobj++.pc @ONLY)

# my test
add_flag(-Wpointer-arith)
add_flag(-Wsign-compare)
add_flag(-Wunreachable-code-return)
add_flag(-Wmissing-variable-declarations)
add_flag(-fno-common)

add_flag(-ggdb DEBUG)
add_flag(-DDEBUG DEBUG)
add_flag("-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2" RELEASE)
