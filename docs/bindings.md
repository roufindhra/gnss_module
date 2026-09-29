# Konvensi Binding Fortran ↔ C++

Sumber kebenaran konvensi interop proyek ini. Semua fungsi inti numerik Fortran
yang diekspos ke C++ **wajib** mengikuti konvensi di bawah.

## Aturan dasar

- Sisi Fortran: `bind(C, name="gnsst_<nama>")` + `use, intrinsic :: iso_c_binding`.
- Sisi C++: deklarasi di dalam blok `extern "C"` pada `src/bindings/gnss_core.h`.
- Semua fungsi bernama dengan prefix `gnsst_`.
- Tidak ada name-mangling implisit — nama simbol ditentukan eksplisit oleh `bind(C)`.

## Mapping tipe

| C++        | Fortran (iso_c_binding)   | Passing            |
|------------|---------------------------|--------------------|
| `double`   | `real(c_double)`          | scalar: `value`    |
| `int`      | `integer(c_int)`          | scalar: `value`    |
| `double*`  | `real(c_double)` array    | pointer eksplisit  |
| `int*`     | `integer(c_int)` array    | pointer eksplisit  |

Scalar **selalu by-value** (`integer(c_int), value`), kecuali output scalar.
`logical`, `character`, dan tipe derived: hindari di boundary — konversi di salah
satu sisi sebelum memanggil fungsi binding.

## Array & matriks

- **Column-major di mana-mana.** C++ menyimpan dan meneruskan matriks dalam
  urutan kolom (`index = row + col*ld`), Fortran menerimanya tanpa konversi.
  Tidak ada transpose di boundary.
- **Explicit-shape + leading dimension.** Fortran menerima `a(lda, k)` dengan
  `lda` sebagai argumen `value` terpisah. Tidak memakai assumed-shape/CFI
  descriptor.
- Dimensi (`m`, `n`, `k`) dan leading dimension diteruskan dari C++.
- Pola loop Fortran: `do j` (kolom, luar) → `do i` (baris, dalam) agar akses
  memory-contiguous.

## Alokasi memori

- **Caller (C++) allocates.** Semua buffer dialokasi dan dibebaskan di sisi C++
  (`std::vector`, `new`/`delete`). Fortran tidak pernah alokasi memori yang
  dikembalikan ke C++, dan tidak pernah membebaskan buffer dari C++.
- Implikasi: tidak ada fungsi binding yang mengembalikan pointer hasil —
  C++ menyediakan buffer output sebagai argumen (pola `c, ldc` pada
  `gnsst_mat_mul`).
- Bila nanti modul estimator butuh state internal berukuran dinamis, pola
  handle opaque (Fortran-managed, free via fungsi `gnsst_<modul>_free`) akan
  ditambahkan saat itu — jangan digunakan sebelum dibutuhkan.

## Konvensi error

Fungsi binding mengembalikan `integer(c_int)` status: `0` = sukses, nilai lain =
kode error modul (di-dokumentasikan per fungsi). Subroutine tanpa kemungkinan
gagal (mis. `gnsst_mat_mul`) boleh `void`.

## Contoh call site C++

```cpp
#include <vector>
#include "gnss_core.h"

std::vector<double> a = {1.0, 4.0, 2.0, 5.0, 3.0, 6.0};  // A(2,3) column-major
std::vector<double> b = {7.0, 9.0, 11.0, 8.0, 10.0, 12.0}; // B(3,2)
std::vector<double> c(4);
gnsst_mat_mul(/*m=*/2, /*n=*/2, /*k=*/3,
              a.data(), /*lda=*/2, b.data(), /*ldb=*/3, c.data(), /*ldc=*/2);
// c = {58, 139, 64, 154} (kolom: (58,139) (64,154))
```
