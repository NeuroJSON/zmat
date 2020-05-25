!------------------------------------------------------------------------------
! ZMat - A portable C-library and MATLAB/Octave toolbox for inline data compression 
!------------------------------------------------------------------------------
!
! module: zmatlib
!
!> @author Qianqian Fang <q.fang at neu.edu>
!
!> @brief An interface to call the zmatlib C-library
!
! DESCRIPTION:
!> ZMat provides an easy-to-use interface for stream compression and decompression.
!>
!> It can be compiled as a MATLAB/Octave mex function (zipmat.mex/zmat.m) and compresses 
!> arrays and strings in MATLAB/Octave. It can also be compiled as a lightweight
!> C-library (libzmat.a/libzmat.so) that can be called in C/C++/FORTRAN etc to 
!> provide stream-level compression and decompression.
!>
!> Currently, zmat/libzmat supports 6 different compression algorthms, including
!>    - zlib and gzip : the most widely used algorithm algorithms for .zip and .gz files
!>    - lzma and lzip : high compression ratio LZMA based algorithms for .lzma and .lzip files
!>    - lz4 and lz4hc : real-time compression based on LZ4 and LZ4HC algorithms
!>    - base64        : base64 encoding and decoding
!
!> @section slicense License
!>          GPL v3, see LICENSE.txt for details
!------------------------------------------------------------------------------

module zmatlib
  use iso_c_binding, only: c_char,c_size_t,c_int,c_ptr, c_loc, c_f_pointer
  implicit none

!------------------------------------------------------------------------------
!> @brief Compression/encoding methods
!
! DESCRIPTION:
!> 0: zmZlib
!> 1: zmGzip
!> 2: zmBase64
!> 3: zmLzip
!> 4: zmLzma
!> 5: zmLz4
!> 6: zmLz4hc
!------------------------------------------------------------------------------

  integer(c_int), parameter :: zmZlib=0, zmGzip=1, zmBase64=2, zmLzip=3, zmLzma=4, zmLz4=5, zmLz4hc=6

  interface

!------------------------------------------------------------------------------
!> @brief Main interface to perform compression/decompression
!
!> @param[in] inputsize: input stream buffer length
!> @param[in] inputstr: input stream buffer pointer
!> @param[out] outputsize: output stream buffer length
!> @param[out] outputbuf: output stream buffer pointer
!> @param[out] ret: encoder/decoder specific detailed error code (if error occurs)
!> @param[in] iscompress: 0: decompression, 1: use default compression level; 
!>	     negative interger: set compression level (-1, less, to -9, more compression)
!> @return return the coarse grained zmat error code; detailed error code is in ret.
!------------------------------------------------------------------------------

    integer(c_int) function zmat_run(inputsize, inputbuf, outputsize, outputbuf, zipid, ret, level) bind(C)
      use iso_c_binding, only: c_char,c_size_t,c_int,c_ptr
      integer(c_size_t), value :: inputsize
      integer(c_int), value :: zipid, level
      integer(c_size_t),  intent(out) :: outputsize
      integer(c_int),  intent(out) :: ret
      type(c_ptr), value, intent(in)  :: inputbuf
      type(c_ptr),intent(out) :: outputbuf
    end function zmat_run

!------------------------------------------------------------------------------
!> @brief Simplified interface to perform compression, same as zmat_run(...,1)
!
!> @param[in] inputsize: input stream buffer length
!> @param[in] inputstr: input stream buffer pointer
!> @param[out] outputsize: output stream buffer length
!> @param[out] outputbuf: output stream buffer pointer
!> @param[out] ret: encoder/decoder specific detailed error code (if error occurs)
!> @return return the coarse grained zmat error code; detailed error code is in ret.
!------------------------------------------------------------------------------

    integer(c_int) function zmat_encode(inputsize, inputbuf, outputsize, outputbuf, zipid, ret) bind(C)
      use iso_c_binding, only: c_char,c_size_t,c_int,c_ptr
      integer(c_size_t), value :: inputsize
      integer(c_int), value :: zipid
      integer(c_size_t),  intent(out) :: outputsize
      integer(c_int),  intent(out) :: ret
      type(c_ptr), value, intent(in)  :: inputbuf
      type(c_ptr),intent(out) :: outputbuf
    end function zmat_encode

!------------------------------------------------------------------------------
!> @brief Simplified interface to perform decompression, same as zmat_run(...,0)
!
!> @param[in] inputsize: input stream buffer length
!> @param[in] inputstr: input stream buffer pointer
!> @param[out] outputsize: output stream buffer length
!> @param[out] outputbuf: output stream buffer pointer
!> @param[out] ret: encoder/decoder specific detailed error code (if error occurs)
!> @return return the coarse grained zmat error code; detailed error code is in ret.
!------------------------------------------------------------------------------

    integer(c_int) function zmat_decode(inputsize, inputbuf, outputsize, outputbuf, zipid, ret) bind(C)
      use iso_c_binding, only: c_char,c_size_t,c_int,c_ptr
      integer(c_size_t), value :: inputsize
      integer(c_int), value :: zipid
      integer(c_size_t),  intent(out) :: outputsize
      integer(c_int),  intent(out) :: ret
      type(c_ptr), value, intent(in)  :: inputbuf
      type(c_ptr),intent(out) :: outputbuf
    end function zmat_decode

!------------------------------------------------------------------------------
!> @brief Deallocating the C-allocated output buffer, must be called after each zmat use
!
!> @param[in,out] outputbuf: output stream buffer pointer
!------------------------------------------------------------------------------

    subroutine zmat_free(outputbuf) bind(C)
      use iso_c_binding, only: c_ptr
      type(c_ptr),intent(inout) :: outputbuf
    end subroutine zmat_free

  end interface
end module
