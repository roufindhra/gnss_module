add_executable(test_smoke unit/test_smoke.cpp)
target_link_libraries(test_smoke PRIVATE gnss_bindings gnss_core)
add_test(NAME smoke COMMAND test_smoke)

add_executable(test_mat unit/test_mat.cpp)
target_link_libraries(test_mat PRIVATE gnss_bindings gnss_core)
add_test(NAME mat COMMAND test_mat)

find_package(LAPACK QUIET)
add_executable(test_linalg unit/test_linalg.cpp)
target_link_libraries(test_linalg PRIVATE gnss_bindings gnss_core)
if(LAPACK_FOUND)
  target_compile_definitions(test_linalg PRIVATE HAVE_LAPACK)
  target_link_libraries(test_linalg PRIVATE ${LAPACK_LIBRARIES})
endif()
add_test(NAME linalg COMMAND test_linalg
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(test_line_reader unit/test_line_reader.cpp)
target_link_libraries(test_line_reader PRIVATE gnss_io)
add_test(NAME line_reader COMMAND test_line_reader
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(test_rinex_header unit/test_rinex_header.cpp)
target_link_libraries(test_rinex_header PRIVATE gnss_io)
add_test(NAME rinex_header COMMAND test_rinex_header
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(test_rinex_obs unit/test_rinex_obs.cpp)
target_link_libraries(test_rinex_obs PRIVATE gnss_io)
add_test(NAME rinex_obs COMMAND test_rinex_obs
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(test_rinex_edge unit/test_rinex_edge.cpp)
target_link_libraries(test_rinex_edge PRIVATE gnss_io)
add_test(NAME rinex_edge COMMAND test_rinex_edge
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
