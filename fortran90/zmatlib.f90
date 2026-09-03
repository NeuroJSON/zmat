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
!> Currently, zmat/libzmat supports the following compression algorithms:
!>    - zlib and gzip        : the most widely used algorithms for .zip and .gz files
!>    - lzma, lzip, and xz   : high compression ratio LZMA-based algorithms
!>    - lz4 and lz4hc        : real-time compression based on LZ4 and LZ4HC algorithms
!>    - zstd                 : Zstandard high-performance compression
!>    - blosc2 (5 variants)  : meta-compressor optimized for N-D numeric arrays
!>    - base64               : base64 encoding and decoding
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
!>  0: zmZlib
!>  1: zmGzip
!>  2: zmBase64
!>  3: zmLzip
!>  4: zmLzma
!>  5: zmLz4
!>  6: zmLz4hc
!>  7: zmZstd
!>  8: zmBlosc2Blosclz
!>  9: zmBlosc2Lz4
!> 10: zmBlosc2Lz4hc
!> 11: zmBlosc2Zlib
!> 12: zmBlosc2Zstd
!> 13: zmXz
!------------------------------------------------------------------------------

  integer(c_int), parameter :: zmZlib=0, zmGzip=1, zmBase64=2, zmLzip=3, zmLzma=4, &
                                zmLz4=5, zmLz4hc=6, zmZstd=7, &
                                zmBlosc2Blosclz=8, zmBlosc2Lz4=9, zmBlosc2Lz4hc=10, &
                                zmBlosc2Zlib=11, zmBlosc2Zstd=12, zmXz=13

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
!> @brief Compression/decompression with an optional block index
!
!> Identical to zmat_run except that zlib and gzip gain a threaded path. The
!> thread count is packed into the low bytes of "level" the same way the MATLAB
!> and Python bindings do: byte 0 is the compression level, byte 1 the thread
!> count. A thread count of 0 lets the library choose (ZMAT_DEFAULT_NTHREAD for
!> zlib and gzip); a negative one forces the historical single deflate stream.
!
!> On compression the blocks are closed with Z_FULL_FLUSH so the result stays an
!> ordinary zlib/gzip stream any decompressor can read, and "offsets" receives a
!> malloc'ed index of "noffsets" size_t values, laid out as compressed0,
!> uncompressed0, compressed1, ... with a final sentinel pair. Release it with
!> zmat_free once done. Pass a c_ptr holding C_NULL_PTR to decline the index.
!
!> On decompression, handing back that index inflates the blocks concurrently.
!> An index that does not describe the stream is rejected in favour of a serial
!> inflate, so a stale one costs time but never correctness.
!
!> @param[in] inputsize: input stream buffer length
!> @param[in] inputbuf: input stream buffer pointer
!> @param[out] outputsize: output stream buffer length
!> @param[out] outputbuf: output stream buffer pointer
!> @param[in] zipid: compression method id
!> @param[out] ret: encoder/decoder specific detailed error code (if error occurs)
!> @param[in] level: packed compression level and thread count
!> @param[in,out] offsets: block index, see above
!> @param[in,out] noffsets: number of size_t entries in offsets
!> @return return the coarse grained zmat error code; detailed error code is in ret.
!------------------------------------------------------------------------------

    integer(c_int) function zmat_run_indexed(inputsize, inputbuf, outputsize, outputbuf, zipid, ret, level, &
                                             offsets, noffsets) bind(C)
      use iso_c_binding, only: c_char,c_size_t,c_int,c_ptr
      integer(c_size_t), value :: inputsize
      integer(c_int), value :: zipid, level
      integer(c_size_t),  intent(out) :: outputsize
      integer(c_int),  intent(out) :: ret
      type(c_ptr), value, intent(in)  :: inputbuf
      type(c_ptr),intent(out) :: outputbuf
      type(c_ptr),intent(inout) :: offsets
      integer(c_size_t),intent(inout) :: noffsets
    end function zmat_run_indexed

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
