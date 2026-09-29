add_library(gnss_io STATIC
  rinex/obs/line_reader.cpp
  rinex/obs/header_parser.cpp
  rinex/obs/parser_v2.cpp
  rinex/obs/parser_v3.cpp
  rinex/obs/parser.cpp
)
target_include_directories(gnss_io PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_compile_features(gnss_io PUBLIC cxx_std_17)
