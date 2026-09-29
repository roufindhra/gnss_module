#include <cmath>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

#include "gnss_core.h"
#include "gnss_linalg.h"

namespace {

int failures = 0;

template <typename T>
void expectEq(T got, T want, const char* what) {
    if (got != want) {
        std::cerr << "FAIL: " << what << " got=" << got << " want=" << want << "\n";
        ++failures;
    }
}

void expectNear(double got, double want, double tol, const char* what) {
    if (!(std::fabs(got - want) <= tol)) {
        std::cerr << "FAIL: " << what << " got=" << got << " want=" << want << "\n";
        ++failures;
    }
}

// Matriks SPD acak deterministik: N = B B^T + n*I, column-major.
std::vector<double> randomSpd(int n, unsigned seed) {
    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> dist(-1.0, 1.0);
    std::vector<double> b(static_cast<size_t>(n) * n);
    for (auto& v : b) v = dist(gen);
    std::vector<double> nn(static_cast<size_t>(n) * n, 0.0);
    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < n; ++i) {
            double s = 0.0;
            for (int k = 0; k < n; ++k) s += b[static_cast<size_t>(k) * n + i] *
                                             b[static_cast<size_t>(k) * n + j];
            nn[static_cast<size_t>(j) * n + i] = s + static_cast<double>(n);
        }
    }
    return nn;
}

// A(m,n) column-major acak.
std::vector<double> randomMat(int m, int n, unsigned seed) {
    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> dist(-1.0, 1.0);
    std::vector<double> a(static_cast<size_t>(m) * n);
    for (auto& v : a) v = dist(gen);
    return a;
}

void normalAcc(int m, int n, const std::vector<double>& a,
               const std::vector<double>& w, const std::vector<double>& b,
               std::vector<double>& nrm, std::vector<double>& atb) {
    nrm.assign(static_cast<size_t>(m) * m, 0.0);
    atb.assign(m, 0.0);
    gnsst_normal_acc(m, n, a.data(), m, w.data(), b.data(), nrm.data(), m, atb.data());
}

// Salin segitiga bawah, nol-kan sisanya (tri_inv korupsi segitiga atas).
std::vector<double> cleanLower(const std::vector<double>& t, int n) {
    std::vector<double> c(static_cast<size_t>(n) * n, 0.0);
    for (int j = 0; j < n; ++j) {
        for (int i = j; i < n; ++i) {
            c[static_cast<size_t>(j) * n + i] = t[static_cast<size_t>(j) * n + i];
        }
    }
    return c;
}

// Q = L^-T L^-1 dari Linv (sudah clean).
std::vector<double> cofactorFrom(const std::vector<double>& linv, int n) {
    std::vector<double> q(static_cast<size_t>(n) * n, 0.0);
    for (int j = 0; j < n; ++j) {
        for (int i = j; i < n; ++i) {
            double s = 0.0;
            for (int k = 0; k < n; ++k) {
                // X(k,i) = linv[i*n+k], X(k,j) = linv[j*n+k]
                s += linv[static_cast<size_t>(i) * n + k] *
                     linv[static_cast<size_t>(j) * n + k];
            }
            q[static_cast<size_t>(j) * n + i] = s;
            q[static_cast<size_t>(i) * n + j] = s;
        }
    }
    return q;
}

// Solve LS lengkap: N = A^T W A, x = N^-1 A^T W b; kembalikan Q = N^-1 juga.
bool solveLs(int m, int n, const std::vector<double>& a,
             const std::vector<double>& w, const std::vector<double>& b,
             std::vector<double>& x, std::vector<double>* q) {
    std::vector<double> nrm, atb;
    normalAcc(m, n, a, w, b, nrm, atb);
    std::vector<double> nCopy = nrm;
    int status = -1;
    gnsst_cholesky(m, nCopy.data(), m, &status);
    if (status != 0) return false;
    gnsst_cho_solve(m, nCopy.data(), m, atb.data(), x.data());
    if (q) {
        gnsst_tri_inv(m, nCopy.data(), m, &status);
        if (status != 0) return false;
        *q = cofactorFrom(cleanLower(nCopy, m), m);
    }
    return true;
}

#ifdef HAVE_LAPACK
// Cross-check Cholesky terhadap dpotrf (ABI Fortran LAPACK).
extern "C" void dpotrf_(const char* uplo, const int* n, double* a, const int* lda,
                        int* info, size_t uploLen);
#endif

}  // namespace

int main() {
    // ---- analitik: regresi garis y = 2x + 1, exact
    {
        // A(params x obs), column-major: kolom j = (x_j, 1), stride m=2
        const std::vector<double> x = {0.0, 1.0, 2.0, 3.0};
        std::vector<double> a(static_cast<size_t>(2) * 4);
        for (int j = 0; j < 4; ++j) {
            a[static_cast<size_t>(j) * 2 + 0] = x[j];
            a[static_cast<size_t>(j) * 2 + 1] = 1.0;
        }
        const std::vector<double> w(4, 1.0);
        std::vector<double> b(4);
        for (int i = 0; i < 4; ++i) b[i] = 2.0 * x[i] + 1.0;

        std::vector<double> xx(2), q;
        expectEq(solveLs(2, 4, a, w, b, xx, &q), true, "garis exact: solve sukses");
        expectNear(xx[0], 2.0, 1e-12, "garis exact slope");
        expectNear(xx[1], 1.0, 1e-12, "garis exact intercept");
        // Q = N^-1, N = [[14,6],[6,4]], det = 20 -> Q[0,0] = 0.2
        expectNear(q[0], 0.2, 1e-12, "garis exact Q[0,0]");
    }

    // ---- analitik: polinomial kuadrat p(x) = 3 - 2x + 0.5x^2
    {
        const std::vector<double> x = {-2.0, -1.0, 0.0, 1.0, 2.5};
        const int n = static_cast<int>(x.size());
        std::vector<double> a(static_cast<size_t>(3) * n);  // baris = x^0, x^1, x^2
        for (int j = 0; j < n; ++j) {
            a[static_cast<size_t>(j) * 3 + 0] = 1.0;
            a[static_cast<size_t>(j) * 3 + 1] = x[j];
            a[static_cast<size_t>(j) * 3 + 2] = x[j] * x[j];
        }
        const std::vector<double> w(n, 1.0);
        std::vector<double> b(n);
        for (int i = 0; i < n; ++i) b[i] = 3.0 - 2.0 * x[i] + 0.5 * x[i] * x[i];

        std::vector<double> xx(3), q;
        expectEq(solveLs(3, n, a, w, b, xx, &q), true, "kuadrat: solve sukses");
        expectNear(xx[0], 3.0, 1e-11, "kuadrat c0");
        expectNear(xx[1], -2.0, 1e-11, "kuadrat c1");
        expectNear(xx[2], 0.5, 1e-11, "kuadrat c2");
    }

    // ---- Cholesky cross-check LAPACK + kofaktor N*Q = I
    {
        const int n = 6;
        std::vector<double> nn = randomSpd(n, 42);
#ifdef HAVE_LAPACK
        std::vector<double> forLapack = nn;
        const char uplo = 'L';
        int info = -1;
        dpotrf_(&uplo, &n, forLapack.data(), &n, &info, 1);
        expectEq(info, 0, "dpotrf info");
#endif

        std::vector<double> ours = nn;
        int status = -1;
        gnsst_cholesky(n, ours.data(), n, &status);
        expectEq(status, 0, "cholesky status");
#ifdef HAVE_LAPACK
        double maxDiff = 0.0;
        for (int j = 0; j < n; ++j) {
            for (int i = j; i < n; ++i) {
                maxDiff = std::max(maxDiff, std::fabs(
                    ours[static_cast<size_t>(j) * n + i] -
                    forLapack[static_cast<size_t>(j) * n + i]));
            }
        }
        expectNear(maxDiff, 0.0, 1e-10, "cholesky vs dpotrf");
#endif

        // kofaktor: Q = N^-1 via tri_inv, verifikasi N Q = I
        std::vector<double> lInv = ours;
        gnsst_tri_inv(n, lInv.data(), n, &status);
        expectEq(status, 0, "tri_inv status");
        const std::vector<double> q = cofactorFrom(cleanLower(lInv, n), n);

        const std::vector<double> nOrig = randomSpd(n, 42);
        double maxErr = 0.0;
        for (int j = 0; j < n; ++j) {
            for (int i = 0; i < n; ++i) {
                double s = 0.0;
                for (int k = 0; k < n; ++k) {
                    s += nOrig[static_cast<size_t>(k) * n + i] *
                         q[static_cast<size_t>(j) * n + k];
                }
                maxErr = std::max(maxErr, std::fabs(s - (i == j ? 1.0 : 0.0)));
            }
        }
        expectNear(maxErr, 0.0, 1e-10, "N Q = I");
    }

    // ---- Cholesky: matriks non-SPD -> status != 0
    {
        std::vector<double> bad = {1.0, 2.0, 3.0, 2.0};  // kolom: (1,2) (3,2); det<0
        int status = -1;
        gnsst_cholesky(2, bad.data(), 2, &status);
        expectEq(status != 0, true, "cholesky non-SPD gagal");
    }

    // ---- normal_acc: verifikasi langsung terhadap hitung manual kecil
    {
        // A(2,2) = kolom (1,2), (3,4); w = (0.5, 2); b = (5, 7)
        const std::vector<double> a = {1.0, 2.0, 3.0, 4.0};
        const std::vector<double> w = {0.5, 2.0};
        const std::vector<double> b = {5.0, 7.0};
        std::vector<double> nrm, atb;
        normalAcc(2, 2, a, w, b, nrm, atb);
        // N[i,j] = sum_l w_l a(j,l) a(i,l); kolom A: (1,2) dan (3,4)
        expectNear(nrm[0], 0.5 * 1.0 + 2.0 * 9.0, 1e-12, "normal_acc N[0,0]");
        expectNear(nrm[1], 0.5 * 2.0 * 1.0 + 2.0 * 4.0 * 3.0, 1e-12, "normal_acc N[1,0]");
        expectNear(nrm[3], 0.5 * 4.0 + 2.0 * 16.0, 1e-12, "normal_acc N[1,1]");
        expectNear(nrm[2], nrm[1], 1e-15, "normal_acc simetri");
        // atb[j] = sum_l w_l a(j,l) b(l)
        expectNear(atb[0], 0.5 * 1.0 * 5.0 + 2.0 * 3.0 * 7.0, 1e-12, "normal_acc atb[0]");
        expectNear(atb[1], 0.5 * 2.0 * 5.0 + 2.0 * 4.0 * 7.0, 1e-12, "normal_acc atb[1]");
    }

    // ---- IRLS IGG-III: dataset pas + 1 gross outlier
    {
        const std::vector<double> x = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0};
        const int n = static_cast<int>(x.size());
        std::vector<double> a(static_cast<size_t>(2) * n);  // baris = (x, 1)
        std::vector<double> b(n), w(n, 1.0);
        for (int j = 0; j < n; ++j) {
            a[static_cast<size_t>(j) * 2 + 0] = x[j];
            a[static_cast<size_t>(j) * 2 + 1] = 1.0;
            b[j] = 2.0 * x[j] + 1.0;
        }
        const int outlier = 4;  // x = 4: y +50
        b[outlier] += 50.0;

        // OLS bias (bukti gross error mempengaruhi solusi biasa)
        std::vector<double> xols(2), qols;
        expectEq(solveLs(2, n, a, w, b, xols, &qols), true, "OLS outlier: sukses");
        const double olsBias = std::fabs(xols[0] - 2.0);
        expectEq(olsBias > 0.1, true, "OLS outlier terkontaminasi");

        // IRLS pulihkan parameter
        std::vector<double> xr(2), qr(static_cast<size_t>(4));
        double s0 = -1.0;
        int iters = -1, status = -1;
        gnsst_irls_igg3(2, n, a.data(), 2, w.data(), b.data(),
                        20, 1e-10, 1.5, 3.0,
                        xr.data(), qr.data(), 2, &s0, &iters, &status);
        expectEq(status, 0, "IRLS status");
        expectNear(xr[0], 2.0, 1e-6, "IRLS slope pulih");
        expectNear(xr[1], 1.0, 1e-6, "IRLS intercept pulih");
        expectEq(iters > 1 && iters < 20, true, "IRLS konvergen sebelum max_iter");
        expectNear(s0, 0.0, 1e-8, "IRLS sigma0 ~ 0 (outlier tertolak)");
    }

    // ---- IRLS: derajat bebas tidak cukup -> status 1
    {
        std::vector<double> a = {1.0, 1.0};
        std::vector<double> w = {1.0}, b = {2.0}, xx(1), qq(1);
        double s0;
        int iters, status;
        gnsst_irls_igg3(1, 1, a.data(), 1, w.data(), b.data(),
                        10, 1e-10, 1.5, 3.0,
                        xx.data(), qq.data(), 1, &s0, &iters, &status);
        expectEq(status, 1, "IRLS df<=0 status");
    }

    // ---- jaring leveling kecil: 2 titik tak bebas, 3 pengamatan
    // A baris = (1,0), (0,1), (-1,1); H_B = 10.123, H_C = 20.456
    // w = (2, 1, 1) -> N = [[3,-1],[-1,2]], Q = N^-1 = [[0.4,0.2],[0.2,0.6]]
    {
        const double hb = 10.123, hc = 20.456;
        std::vector<double> a = {1.0, 0.0,   // kolom 1: baris (H_B, H_C)
                                 0.0, 1.0,   // kolom 2
                                 -1.0, 1.0}; // kolom 3
        const std::vector<double> w = {2.0, 1.0, 1.0};
        std::vector<double> b = {hb, hc, hc - hb};

        std::vector<double> x(2), q;
        expectEq(solveLs(2, 3, a, w, b, x, &q), true, "leveling: sukses");
        expectNear(x[0], hb, 1e-12, "leveling H_B");
        expectNear(x[1], hc, 1e-12, "leveling H_C");
        expectNear(q[0], 0.4, 1e-12, "leveling Q[0,0]");
        expectNear(q[1], 0.2, 1e-12, "leveling Q[1,0]");
        expectNear(q[3], 0.6, 1e-12, "leveling Q[1,1]");

        // varian parameter via sigma0: v^T P v = 0 (exact) -> sigma0 = 0
        double s0;
        int iters, status;
        std::vector<double> qr(static_cast<size_t>(4)), xr(2);
        gnsst_irls_igg3(2, 3, a.data(), 2, w.data(), b.data(),
                        10, 1e-10, 1.5, 3.0,
                        xr.data(), qr.data(), 2, &s0, &iters, &status);
        expectEq(status, 0, "leveling IRLS status");
        expectNear(xr[0], hb, 1e-12, "leveling IRLS H_B");
        expectNear(s0, 0.0, 1e-15, "leveling IRLS sigma0 exact");
    }

    if (failures > 0) {
        std::cerr << failures << " assertion gagal\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
