add_flag(-Wpointer-arith)
add_flag(-Wunused-macros)
add_flag(-Wsign-conversion)
add_flag(-Wsign-compare)
add_flag(-Wunreachable-code-return)
add_flag(-Wmissing-variable-declarations)
add_flag(-fno-common)

add_flag(-ggdb DEBUG)
add_flag(-DDEBUG DEBUG)

add_flag("-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2" RELEASE)

add_executable(test_pmem_hash_map test_pmem_hash_map.cc)
target_include_directories(test_pmem_hash_map PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_include_directories(test_pmem_hash_map PUBLIC ${LIBPMEMOBJ++_INCLUDE_DIRS})
target_link_libraries(test_pmem_hash_map ${LIBPMEMOBJ++_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
