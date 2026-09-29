#ifndef GNSS_TOOL_CORE_H
#define GNSS_TOOL_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

double gnsst_smoke_dot(int n, const double *a, const double *b);

/* C(m,n) = A(m,k) * B(k,n); semua matriks column-major, caller yang alokasi. */
void gnsst_mat_mul(int m, int n, int k,
                   const double *a, int lda,
                   const double *b, int ldb,
                   double *c, int ldc);

#ifdef __cplusplus
}
#endif

#endif /* GNSS_TOOL_CORE_H */
