module gnsst_mat
  use, intrinsic :: iso_c_binding, only: c_int, c_double
  implicit none
  private
  public :: gnsst_mat_mul
contains
  ! C(m,n) = A(m,k) * B(k,n), semua column-major, caller (C++) yang alokasi.
  ! Layout-sensitif: test memakai matriks non-simetrik terhadap transpose
  ! agar salah indexing langsung terdeteksi.
  subroutine gnsst_mat_mul(m, n, k, a, lda, b, ldb, c, ldc) bind(C, name="gnsst_mat_mul")
    integer(c_int), value, intent(in) :: m, n, k, lda, ldb, ldc
    real(c_double), intent(in) :: a(lda, k), b(ldb, n)
    real(c_double), intent(out) :: c(ldc, n)
    integer :: i, j, l
    do j = 1, n
      do i = 1, m
        c(i, j) = 0.0_c_double
      end do
      do l = 1, k
        do i = 1, m
          c(i, j) = c(i, j) + a(i, l)*b(l, j)
        end do
      end do
    end do
  end subroutine gnsst_mat_mul
end module gnsst_mat
