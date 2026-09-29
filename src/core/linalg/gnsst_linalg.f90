module gnsst_linalg
  use, intrinsic :: iso_c_binding, only: c_int, c_double
  implicit none
  private
  public :: gnsst_normal_acc, gnsst_cholesky, gnsst_cho_solve, gnsst_tri_inv, &
            gnsst_irls_igg3

contains

  ! Akumulasi persamaan normal (bobot diagonal):
  !   nrm(m,m) += A^T W A ,  atb(m) += A^T W b
  ! A(m,n) column-major, w(n) = diagonal P. Caller yang alokasi; nrm/atb
  ! berisi nol atau hasil akumulasi batch sebelumnya (sequential adjustment).
  subroutine gnsst_normal_acc(m, n, a, lda, w, b, nrm, ldn, atb) &
      bind(C, name="gnsst_normal_acc")
    integer(c_int), value, intent(in) :: m, n, lda, ldn
    real(c_double), intent(in) :: a(lda, n), w(n), b(n)
    real(c_double), intent(inout) :: nrm(ldn, m), atb(m)
    integer :: i, j, l
    real(c_double) :: awi
    do l = 1, n
      do j = 1, m
        awi = w(l) * a(j, l)
        atb(j) = atb(j) + awi * b(l)
        do i = j, m
          nrm(i, j) = nrm(i, j) + a(i, l) * awi
        end do
      end do
    end do
    ! simetri: cermin segitiga atas dari bagian bawah
    do j = 1, m - 1
      do i = j + 1, m
        nrm(j, i) = nrm(i, j)
      end do
    end do
  end subroutine gnsst_normal_acc

  ! Faktorisasi Cholesky in-place: N = L L^T, L segitiga bawah.
  ! Sisi segitiga atas N dikorupsi. status = 0 sukses; k>0 jika leading
  ! k x k minor tidak positive definite (konvensi gagal ala LAPACK).
  subroutine gnsst_cholesky(n, a, lda, status) bind(C, name="gnsst_cholesky")
    integer(c_int), value, intent(in) :: n, lda
    real(c_double), intent(inout) :: a(lda, n)
    integer(c_int), intent(out) :: status
    integer :: i, j, k
    real(c_double) :: s
    status = 0_c_int
    do j = 1, n
      s = a(j, j)
      do k = 1, j - 1
        s = s - a(j, k) * a(j, k)
      end do
      if (s <= 0.0_c_double) then
        status = int(j, c_int)
        return
      end if
      a(j, j) = sqrt(s)
      do i = j + 1, n
        s = a(i, j)
        do k = 1, j - 1
          s = s - a(i, k) * a(j, k)
        end do
        a(i, j) = s / a(j, j)
      end do
    end do
  end subroutine gnsst_cholesky

  ! Solve L L^T x = b; L segitiga bawah (hasil gnsst_cholesky).
  subroutine gnsst_cho_solve(n, l, ldl, b, x) bind(C, name="gnsst_cho_solve")
    integer(c_int), value, intent(in) :: n, ldl
    real(c_double), intent(in) :: l(ldl, n), b(n)
    real(c_double), intent(out) :: x(n)
    integer :: i, j
    real(c_double) :: s
    ! L y = b (substitusi maju)
    do i = 1, n
      s = b(i)
      do j = 1, i - 1
        s = s - l(i, j) * x(j)
      end do
      x(i) = s / l(i, i)
    end do
    ! L^T x = y (substitusi mundur)
    do i = n, 1, -1
      s = x(i)
      do j = i + 1, n
        s = s - l(j, i) * x(j)
      end do
      x(i) = s / l(i, i)
    end do
  end subroutine gnsst_cho_solve

  ! Invers segitiga bawah in-place (untuk kofaktor Q = N^-1 = L^-T L^-1;
  ! sisi atas L dikorupsi). status = 0 sukses; k>0 jika diagonal nol.
  subroutine gnsst_tri_inv(n, l, ldl, status) bind(C, name="gnsst_tri_inv")
    integer(c_int), value, intent(in) :: n, ldl
    real(c_double), intent(inout) :: l(ldl, n)
    integer(c_int), intent(out) :: status
    integer :: i, j, k
    real(c_double) :: s
    status = 0_c_int
    do j = 1, n
      if (l(j, j) == 0.0_c_double) then
        status = int(j, c_int)
        return
      end if
      l(j, j) = 1.0_c_double / l(j, j)
      do i = j + 1, n
        s = 0.0_c_double
        do k = j, i - 1
          s = s - l(i, k) * l(k, j)
        end do
        l(i, j) = s / l(i, i)
      end do
    end do
  end subroutine gnsst_tri_inv

  ! Least squares iteratif robust (IRLS) dengan fungsi bobot IGG-III.
  ! Model: v = A^T x - b, bobot awal w(n) (diagonal P).
  !   |t| <= k0                : w_i tetap (t = residual terstandarisasi)
  !   k0 < |t| < k1            : w_i * (k0/|t|) * ((k1-|t|)/(k1-k0))^2
  !   |t| >= k1                : 0  (observasi dibuang)
  ! Konvergensi: max|delta x| < tol. Q = kofaktor (N akhir)^-1,
  ! sigma0_sq = v^T P v / (n-m), iters = jumlah iterasi.
  ! status: 0 sukses; 1 derajat bebas <= 0; 10+k gagal Cholesky di iter k.
  subroutine gnsst_irls_igg3(m, n, a, lda, w, b, max_iter, tol, k0, k1, &
                             x, q, ldq, sigma0_sq, iters, status) &
      bind(C, name="gnsst_irls_igg3")
    integer(c_int), value, intent(in) :: m, n, lda, ldq, max_iter
    real(c_double), value, intent(in) :: tol, k0, k1
    real(c_double), intent(in) :: a(lda, n), w(n), b(n)
    real(c_double), intent(out) :: x(m), q(ldq, m), sigma0_sq
    integer(c_int), intent(out) :: iters, status

    real(c_double) :: nmat(m, m), linv(m, m), v(n), wcur(n), wprev(n)
    real(c_double) :: y(m), xnew(m), s0, dx, t, wi
    logical :: stable
    integer :: i, j, k, l

    status = 0_c_int
    iters = 0_c_int
    sigma0_sq = 0.0_c_double
    if (n - m <= 0) then
      status = 1_c_int
      return
    end if

    wcur = w
    x = 0.0_c_double
    y = 0.0_c_double
    stable = .false.

    do k = 1, max_iter
      iters = int(k, c_int)
      nmat = 0.0_c_double
      y = 0.0_c_double
      call gnsst_normal_acc(m, n, a, lda, wcur, b, nmat, m, y)
      call gnsst_cholesky(m, nmat, m, status)
      if (status /= 0_c_int) then
        status = int(10 + status, c_int)
        return
      end if
      call gnsst_cho_solve(m, nmat, m, y, xnew)
      dx = maxval(abs(xnew - x))
      x = xnew
      if (k > 1 .and. dx < tol .and. stable) exit

      ! residual & penimbangan ulang IGG-III. Standardisasi memakai skala
      ! robust MAD (sigma_rob = 1.4826 median|v|), bukan sigma0 a posteriori
      ! yang terkontaminasi outlier itu sendiri (Huber dkk.).
      s0 = 0.0_c_double
      do l = 1, n
        v(l) = -b(l)
        do j = 1, m
          v(l) = v(l) + a(j, l) * x(j)
        end do
        v(l) = abs(v(l))
      end do
      ! median|v| via insertion sort (n kecil)
      do l = 1, n - 1
        do j = l + 1, n
          if (v(j) < v(l)) then
            t = v(l)
            v(l) = v(j)
            v(j) = t
          end if
        end do
      end do
      t = v((n + 1) / 2) * 1.4826_c_double
      if (t <= tiny(1.0_c_double)) exit  ! fit exact, tidak ada yang dibuang
      wprev = wcur
      do l = 1, n
        v(l) = -b(l)
        do j = 1, m
          v(l) = v(l) + a(j, l) * x(j)
        end do
        ! standardisasi pakai bobot dasar w(l), bukan wcur: observasi yang
        ! sudah tertolak (wcur=0) tetap tertolak, tidak "hidup" lagi.
        wi = sqrt(w(l)) * abs(v(l)) / t
        if (wi <= k0) then
          wcur(l) = w(l)
        else if (wi < k1) then
          wcur(l) = w(l) * (k0 / wi) * ((k1 - wi) / (k1 - k0))**2
        else
          wcur(l) = 0.0_c_double
        end if
      end do
      stable = all(wcur == wprev)
    end do

    ! kofaktor Q dari N akhir (bobot konvergen)
    call gnsst_tri_inv(m, nmat, m, status)
    if (status /= 0_c_int) then
      status = int(10 + status, c_int)
      return
    end if
    linv = 0.0_c_double
    do j = 1, m
      do i = j, m
        linv(i, j) = nmat(i, j)
      end do
    end do
    q = 0.0_c_double
    do j = 1, m
      do i = j, m
        s0 = 0.0_c_double
        do l = 1, m
          s0 = s0 + linv(l, i) * linv(l, j)
        end do
        q(i, j) = s0
        q(j, i) = s0
      end do
    end do

    ! sigma0 akhir
    s0 = 0.0_c_double
    do l = 1, n
      v(l) = -b(l)
      do j = 1, m
        v(l) = v(l) + a(j, l) * x(j)
      end do
      s0 = s0 + wcur(l) * v(l) * v(l)
    end do
    sigma0_sq = s0 / real(n - m, c_double)
    status = 0_c_int
  end subroutine

end module gnsst_linalg
