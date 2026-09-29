#ifndef GNSS_TOOL_LINALG_H
#define GNSS_TOOL_LINALG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Akumulasi persamaan normal (bobot diagonal):
   nrm(m,m) += A^T W A, atb(m) += A^T W b. Column-major, caller alokasi. */
void gnsst_normal_acc(int m, int n,
                      const double *a, int lda,
                      const double *w, const double *b,
                      double *nrm, int ldn, double *atb);

/* Cholesky in-place: N = L L^T (segitiga bawah). return status via *status:
   0 = sukses, k>0 = leading k x k minor tidak positive definite. */
void gnsst_cholesky(int n, double *a, int lda, int *status);

/* Solve L L^T x = b; L segitiga bawah hasil gnsst_cholesky. */
void gnsst_cho_solve(int n, const double *l, int ldl,
                     const double *b, double *x);

/* Invers segitiga bawah in-place. *status: 0 = sukses, k>0 = diagonal nol. */
void gnsst_tri_inv(int n, double *l, int ldl, int *status);

/* LS iteratif robust IRLS + bobot IGG-III (bobot awal w = diagonal P).
   Q = kofaktor (m x m), sigma0_sq = v^T P v / (n-m), iters = jumlah iterasi.
   *status: 0 sukses; 1 derajat bebas <= 0; 10+k Cholesky gagal di iter k. */
void gnsst_irls_igg3(int m, int n,
                     const double *a, int lda,
                     const double *w, const double *b,
                     int max_iter, double tol, double k0, double k1,
                     double *x, double *q, int ldq,
                     double *sigma0_sq, int *iters, int *status);

#ifdef __cplusplus
}
#endif

#endif /* GNSS_TOOL_LINALG_H */
