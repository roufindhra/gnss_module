add_executable(gnss-tool main.cpp)
target_link_libraries(gnss-tool PRIVATE gnss_bindings gnss_core)
