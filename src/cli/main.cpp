#include <cstdio>

#include "gnss_core.h"

int main() {
    const double a[] = {1.0, 2.0, 3.0};
    const double b[] = {4.0, 5.0, 6.0};
    std::printf("gnss-tool v0.1.0 | smoke dot(1,2,3).(4,5,6) = %.1f\n",
                gnsst_smoke_dot(3, a, b));
    return 0;
}
