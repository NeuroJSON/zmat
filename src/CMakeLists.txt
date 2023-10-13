#################################################################
#  CMake configure file for ZMat
#  Qianqian Fang <q.fang at neu.edu>
#  2020/05/23
#################################################################

cmake_minimum_required(VERSION 3.3)

project(zmat)

option(USE_ZLIB "Use zlib instead of miniz" OFF)
option(USE_LZ4 "Use lz4" ON)
option(USE_BLOSC2 "Use blosc2" ON)
option(USE_ZSTD "Use ZStd" OFF)
option(USE_LZMA "Use lzma" ON)

find_package(Matlab)

option(STATIC_LIB "Build static library" ON)

# C Options
set(CMAKE_C_FLAGS "-g -Wall -O3 -fPIC")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../)

if(USE_ZLIB)
  find_package(ZLIB REQUIRED)
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_ZLIB -D_LARGEFILE64_SOURCE=1")
  include_directories(miniz)
endif()

# Add include directories
include_directories(../include)

if(USE_LZ4)
  include_directories(lz4)
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_LZ4")
endif()

if(USE_LZMA)
  include_directories(easylzma)
  include_directories(easylzma/pavlov)
endif()

if(USE_BLOSC2)
  include_directories(blosc2/include)
  add_subdirectory(blosc2/blosc)
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_BLOSC2")
endif()

if(USE_ZSTD)
  include_directories(blosc2/internal-complibs/zstd)
else()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_ZSTD")
endif()

# Add all project units

if(STATIC_LIB)
  add_library(zmat STATIC
    zmatlib.c
    miniz/miniz.c
    lz4/lz4.c
    lz4/lz4hc.c
    easylzma/compress.c
    easylzma/decompress.c
    easylzma/lzma_header.c
    easylzma/lzip_header.c
    easylzma/common_internal.c
    easylzma/pavlov/LzmaEnc.c
    easylzma/pavlov/LzmaDec.c
    easylzma/pavlov/LzmaLib.c
    easylzma/pavlov/LzFind.c
    easylzma/pavlov/Bra.c
    easylzma/pavlov/BraIA64.c
    easylzma/pavlov/Alloc.c
    easylzma/pavlov/7zCrc.c
    )
else()
# Add all project units
  add_library(zmat SHARED
    zmatlib.c
    miniz/miniz.c
    lz4/lz4.c
    lz4/lz4hc.c
    easylzma/compress.c
    easylzma/decompress.c
    easylzma/lzma_header.c
    easylzma/lzip_header.c
    easylzma/common_internal.c
    easylzma/pavlov/LzmaEnc.c
    easylzma/pavlov/LzmaDec.c
    easylzma/pavlov/LzmaLib.c
    easylzma/pavlov/LzFind.c
    easylzma/pavlov/Bra.c
    easylzma/pavlov/BraIA64.c
    easylzma/pavlov/Alloc.c
    easylzma/pavlov/7zCrc.c
    )
endif()

# Link options
target_link_libraries(
    zmat
    )

if(Matlab_FOUND)
    if(${CMAKE_VERSION} VERSION_LESS "3.24.0")
        matlab_add_mex(
          NAME zipmat
          SRC zmat.cpp
          LINK_TO zmat
        )
    else()
        matlab_add_mex(
          NAME zipmat
          SRC zmat.cpp
          NO_IMPLICIT_LINK_TO_MATLAB_LIBRARIES
          LINK_TO ${Matlab_MEX_LIBRARY} ${Matlab_MX_LIBRARY} zmat
        )
    endif()
endif()

