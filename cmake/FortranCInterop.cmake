# Konvensi interop proyek ini: semua fungsi Fortran yang dipanggil dari C++
# wajib memakai bind(C) + ISO_C_BINDING, sehingga tidak perlu deteksi
# name-mangling otomatis (module FortranCInterface hanya dibutuhkan untuk
# kode Fortran lama tanpa bind(C)).
#
# Modul ini memastikan kompatibilitas toolchain campuran:
#   - GCC:     g++ + gfortran (kombinasi aman, default di Arch Linux)
#   - Clang:   clang++ + gfortran (perlu runtime gfortran saat link)
#   - Intel:   icpx + ifx (belum diuji; laporkan bila digunakan)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_Fortran_COMPILER_ID STREQUAL "GNU")
  message(WARNING "gnss-tool: kombinasi ${CMAKE_CXX_COMPILER_ID} + ${CMAKE_Fortran_COMPILER_ID} belum diuji")
endif()
