#include <cmath>
#include <cstdlib>

#include "gnss_core.h"

int main() {
    const double a[] = {1.0, 2.0, 3.0};
    const double b[] = {2.0, 2.0, 2.0};
    if (std::fabs(gnsst_smoke_dot(3, a, b) - 12.0) > 1e-12) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
