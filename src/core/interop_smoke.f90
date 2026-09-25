module gnsst_smoke
  use, intrinsic :: iso_c_binding, only: c_int, c_double
  implicit none
  private
  public :: gnsst_smoke_dot
contains
  function gnsst_smoke_dot(n, a, b) result(dot) bind(C, name="gnsst_smoke_dot")
    integer(c_int), value, intent(in) :: n
    real(c_double), intent(in) :: a(n), b(n)
    real(c_double) :: dot
    integer :: i
    dot = 0.0_c_double
    do i = 1, n
      dot = dot + a(i)*b(i)
    end do
  end function gnsst_smoke_dot
end module gnsst_smoke
