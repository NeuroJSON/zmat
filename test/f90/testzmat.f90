!------------------------------------------------------------------------------
!  ZMatLib test program
!------------------------------------------------------------------------------
!
!> @author Qianqian Fang <q.fang at neu.edu>
!> @brief A demo program to call zmatlib functions to encode and decode
!
! DESCRIPTION:
!> This demo program shows how to call zmat_run/zmat_encode/zmat_decode to
!> perform buffer encoding and decoding
!------------------------------------------------------------------------------

program zmatdemo

! step 1: add the below line to use zmatlib unit
use zmatlib
implicit none

character (len=128) inputstr
integer :: ret, res
integer(kind=8) :: inputlen, outputlen
type(c_ptr) :: outputbuf
character(kind=c_char),pointer :: myout(:)

inputstr="__o000o__(o)(o)__o000o__ =^_^=  __o000o__(o)(o)__o000o__"
inputlen=len(trim(inputstr))

print *, trim(inputstr)

res=zmat_run(inputlen,inputstr,outputlen, outputbuf, zmBase64, ret, 1);

call c_f_pointer(outputbuf, myout, [outputlen])
print *, myout

end program
