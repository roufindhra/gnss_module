add_executable(test_smoke unit/test_smoke.cpp)
target_link_libraries(test_smoke PRIVATE gnss_bindings gnss_core)
add_test(NAME smoke COMMAND test_smoke)

add_executable(test_mat unit/test_mat.cpp)
target_link_libraries(test_mat PRIVATE gnss_bindings gnss_core)
add_test(NAME mat COMMAND test_mat)
