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

## Modul linalg (`src/core/linalg/`)

Deklarasi C++ di `src/bindings/gnss_linalg.h`; konvensi di atas berlaku penuh.

### Keputusan: from-scratch vs LAPACK

Operasi (Cholesky, triangular inverse, normal equation, IRLS) **ditulis sendiri**
di Fortran — from-scratch, edukatif, nol dependensi runtime. LAPACK tidak dipakai
di inti. Trade-off:

- *Ditulis sendiri*: kontrol penuh atas algoritma (bagian pembelajaran utama
  proyek ini), tanpa dependensi; risiko bug numerik ditanggung sendiri.
- *LAPACK*: teruji luas dan lebih cepat (BLAS backend), tetapi menambah
  dependensi sistem dan menghilangkan nilai edukasi inti.
- **Kompromi yang dipilih**: unit test (`test_linalg`) memverifikasi silang
  Cholesky terhadap `dpotrf` LAPACK bila ditemukan saat konfigurasi CMake
  (`find_package(LAPACK)` → `HAVE_LAPACK`); tanpa LAPACK test tetap jalan
  dengan kasus analitik. LAPACK hanya di-link pada test target, bukan `gnss_core`.

### Solver least squares

Normal equation `N = AᵀWA` + Cholesky `LLᵀ` (standar penyesuaian geodetik;
Teunissen *Adjustment Theory*). Matriks bobot P = **diagonal** (vektor `w(n)`)
— cukup untuk GNSS dan IRLS; matriks P penuh tidak didukung. Kofaktor
`Q = N⁻¹ = L⁻ᵀL⁻¹` via `gnsst_tri_inv`; perhatikan `tri_inv` bersifat in-place
dan **mengkorupsi segitiga atas** — jangan membaca bagian atas setelah panggilan.

### IRLS IGG-III (`gnsst_irls_igg3`)

Penimbangan ulang iteratif untuk robust estimation (siap dipakai modul QC).
Standardisasi residual memakai **skala robust MAD** (`σ = 1.4826·median|v|`),
bukan σ̂₀ a posteriori yang terkontaminasi outlier itu sendiri; bobot dasar
`w(l)` yang dipakai untuk standardisasi (bukan bobot iterasi) supaya
observasi tertolak tetap tertolak. Konvergensi: `max|Δx| < tol` dan bobot stabil.
Kode status: `0` sukses; `1` derajat bebas ≤ 0; `10+k` Cholesky gagal iterasi k.

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
