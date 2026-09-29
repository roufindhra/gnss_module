#include <cmath>
#include <cstdlib>
#include <vector>

#include "gnss_core.h"

// C(2,2) = A(2,3) * B(3,2), semua column-major.
// A^T != A pada ukuran ini, jadi salah indexing langsung gagal.
int main() {
    // A = [[1,2,3],[4,5,6]]  -> kolom: (1,4) (2,5) (3,6)
    const std::vector<double> a = {1.0, 4.0, 2.0, 5.0, 3.0, 6.0};
    // B = [[7,8],[9,10],[11,12]] -> kolom: (7,9,11) (8,10,12)
    const std::vector<double> b = {7.0, 9.0, 11.0, 8.0, 10.0, 12.0};
    // C = [[58,64],[139,154]]
    const double expected[] = {58.0, 139.0, 64.0, 154.0};

    std::vector<double> c(4, 0.0);
    gnsst_mat_mul(2, 2, 3, a.data(), 2, b.data(), 3, c.data(), 2);

    for (int i = 0; i < 4; ++i) {
        if (std::fabs(c[i] - expected[i]) > 1e-12) {
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
