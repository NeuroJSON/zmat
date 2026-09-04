#################################################################
#  CMake configure file for ZMat
#  Qianqian Fang <q.fang at neu.edu>
#  2020/05/23
#################################################################

cmake_minimum_required(VERSION 3.5)

project(zmat)

option(USE_ZLIB   "Use system zlib instead of bundled miniz" OFF)
option(USE_LZ4    "Build with LZ4 support"                   ON)
option(USE_ZSTD   "Build with Zstandard support"             ON)
option(USE_LZMA   "Build with LZMA/lzip support"             ON)
option(USE_BLOSC2 "Build with Blosc2 meta-compressor"        ON)
option(STATIC_LIB "Build static library (OFF = shared)"      ON)

find_package(Matlab)
find_package(Threads)

set(CMAKE_C_FLAGS   "-g -Wall -O3 -fPIC")
set(CMAKE_CXX_FLAGS "-g -Wall -O3 -fPIC")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../)

# ---------------------------------------------------------------------------
# Include directories and compile definitions
# ---------------------------------------------------------------------------

include_directories(../include)

# --- zlib / miniz ---
if(USE_ZLIB)
    find_package(ZLIB REQUIRED)
else()
    add_compile_definitions(NO_ZLIB _LARGEFILE64_SOURCE=1)
    include_directories(miniz)
endif()

# --- LZ4 ---
if(USE_LZ4)
    include_directories(lz4)
else()
    add_compile_definitions(NO_LZ4)
endif()

# --- Zstandard ---
if(USE_ZSTD)
    include_directories(blosc2/internal-complibs/zstd)
else()
    add_compile_definitions(NO_ZSTD)
endif()

# --- LZMA / lzip / xz ---
if(USE_LZMA)
    set(LZMA_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/easylzma/lzma")
    set(LZMA_LEGACY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/easylzma/pavlov")

    if(EXISTS "${LZMA_SDK_DIR}/LzmaEnc.c")
        # New self-contained LZMA SDK (supports xz / LZMA2 / multi-threading)
        add_compile_definitions(ZMAT_USE_LZMA_SDK)
        include_directories(easylzma easylzma/lzma)

        set(LZMA_SOURCES
            easylzma/compress.c
            easylzma/decompress.c
            easylzma/lzma_header.c
            easylzma/lzip_header.c
            easylzma/common_internal.c
            easylzma/lzma/LzmaEnc.c
            easylzma/lzma/LzmaDec.c
            easylzma/lzma/LzmaLib.c
            easylzma/lzma/LzFind.c
            easylzma/lzma/Bra.c
            easylzma/lzma/BraIA64.c
            easylzma/lzma/Bra86.c
            easylzma/lzma/Alloc.c
            easylzma/lzma/7zCrc.c
            easylzma/lzma/7zCrcOpt.c
            easylzma/lzma/CpuArch.c
            easylzma/lzma/Lzma2Dec.c
            easylzma/lzma/Lzma2DecMt.c
            easylzma/lzma/Lzma2Enc.c
            easylzma/lzma/MtCoder.c
            easylzma/lzma/MtDec.c
            easylzma/lzma/Xz.c
            easylzma/lzma/XzCrc64.c
            easylzma/lzma/XzCrc64Opt.c
            easylzma/lzma/XzEnc.c
            easylzma/lzma/XzDec.c
            easylzma/lzma/Sha256.c
            easylzma/lzma/Sha256Opt.c
            easylzma/lzma/Delta.c
            easylzma/lzma/7zStream.c
            # Threading support — always required: LzmaEnc, MtCoder, MtDec all
            # reference these unconditionally regardless of platform
            easylzma/lzma/LzFindMt.c
            easylzma/lzma/LzFindOpt.c
            easylzma/lzma/Threads.c
        )

        # COMPRESS_MF_MT enables the lzip parallel-chunk path in zmatlib.c (non-Windows only)
        if(NOT WIN32)
            add_compile_definitions(COMPRESS_MF_MT)
        endif()

    else()
        # Legacy pavlov subset (no xz support)
        include_directories(easylzma easylzma/pavlov)

        set(LZMA_SOURCES
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
else()
    add_compile_definitions(NO_LZMA)
    set(LZMA_SOURCES "")
endif()

# --- Blosc2 ---
if(USE_BLOSC2)
    include_directories(blosc2/include)
    add_subdirectory(blosc2)
else()
    add_compile_definitions(NO_BLOSC2)
endif()

# ---------------------------------------------------------------------------
# Collect all source files
# ---------------------------------------------------------------------------

set(ZMAT_SOURCES zmatlib.c)

# miniz (when not using system zlib)
if(NOT USE_ZLIB)
    list(APPEND ZMAT_SOURCES miniz/miniz.c)
endif()

# LZ4
if(USE_LZ4)
    list(APPEND ZMAT_SOURCES lz4/lz4.c lz4/lz4hc.c)
endif()

# LZMA / lzip / xz
list(APPEND ZMAT_SOURCES ${LZMA_SOURCES})

# ---------------------------------------------------------------------------
# Build library target
# ---------------------------------------------------------------------------

if(STATIC_LIB)
    add_library(zmat STATIC ${ZMAT_SOURCES})
else()
    add_library(zmat SHARED ${ZMAT_SOURCES})
endif()

# ---------------------------------------------------------------------------
# Link options
# ---------------------------------------------------------------------------

if(USE_BLOSC2)
    target_link_libraries(zmat blosc2_static)
    if(Threads_FOUND)
        target_link_libraries(zmat Threads::Threads)
    endif()
    if(UNIX AND NOT APPLE)
        target_link_libraries(zmat dl)
    endif()
elseif(Threads_FOUND AND USE_LZMA AND NOT WIN32)
    # LZMA SDK uses pthreads for multi-threaded match-finder
    target_link_libraries(zmat Threads::Threads)
endif()

if(USE_ZLIB)
    target_link_libraries(zmat ZLIB::ZLIB)
endif()

if(USE_ZSTD AND NOT USE_BLOSC2)
    # zstd is embedded inside blosc2; only link separately when blosc2 is off
    target_link_libraries(zmat zstd)
endif()

# ---------------------------------------------------------------------------
# MEX target (requires MATLAB)
# ---------------------------------------------------------------------------

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
