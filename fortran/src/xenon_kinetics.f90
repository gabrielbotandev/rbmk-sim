! Iodine-135 / xenon-135 chain integrator (classic RK4), Fortran 2008.
!
! ALGORITHMIC TWIN of the reference implementation in
! kernel/src/xenon_cpp.cpp (xenon_step_cpp). The two must stay in lockstep;
! the kernel test "active integrator matches the reference C++
! implementation" enforces parity whenever this library is built.
!
! Constants mirror kernel/include/rbmk/kernel/constants.hpp (the source of
! truth). All values are simplified, public, educational toy numbers.
module rbmk_xenon_kinetics
   use, intrinsic :: iso_c_binding, only: c_double
   implicit none
   private
   public :: rbmk_f_xenon_step

   real(c_double), parameter :: lambda_iodine = 2.87e-5_c_double  ! 1/s
   real(c_double), parameter :: lambda_xenon = 2.09e-5_c_double   ! 1/s
   real(c_double), parameter :: burnup_rate = 7.0e-5_c_double     ! 1/s at P=1
   real(c_double), parameter :: direct_yield = &
      lambda_iodine*(0.05_c_double/0.95_c_double)

contains

   pure subroutine chain_rates(iodine, xenon, power, di, dx)
      real(c_double), intent(in) :: iodine, xenon, power
      real(c_double), intent(out) :: di, dx
      di = lambda_iodine*(power - iodine)
      dx = direct_yield*power + lambda_iodine*iodine &
           - lambda_xenon*xenon - burnup_rate*power*xenon
   end subroutine chain_rates

   subroutine rbmk_f_xenon_step(iodine, xenon, power_frac, dt_s) &
      bind(c, name="rbmk_f_xenon_step")
      real(c_double), intent(inout) :: iodine, xenon
      real(c_double), intent(in) :: power_frac, dt_s
      real(c_double) :: p, h
      real(c_double) :: k1i, k1x, k2i, k2x, k3i, k3x, k4i, k4x

      p = max(power_frac, 0.0_c_double)
      h = dt_s

      call chain_rates(iodine, xenon, p, k1i, k1x)
      call chain_rates(iodine + 0.5_c_double*h*k1i, &
                       xenon + 0.5_c_double*h*k1x, p, k2i, k2x)
      call chain_rates(iodine + 0.5_c_double*h*k2i, &
                       xenon + 0.5_c_double*h*k2x, p, k3i, k3x)
      call chain_rates(iodine + h*k3i, xenon + h*k3x, p, k4i, k4x)

      iodine = iodine + (h/6.0_c_double)*(k1i + 2.0_c_double*k2i &
                                          + 2.0_c_double*k3i + k4i)
      xenon = xenon + (h/6.0_c_double)*(k1x + 2.0_c_double*k2x &
                                        + 2.0_c_double*k3x + k4x)
      iodine = max(iodine, 0.0_c_double)
      xenon = max(xenon, 0.0_c_double)
   end subroutine rbmk_f_xenon_step

end module rbmk_xenon_kinetics
