.. image:: https://neurojson.org/wiki/upload/neurojson_banner_long.png

##############################################################################
ZMAT: A portable C-library and MATLAB/Octave/Python toolbox for zlib/gzip/lzma/lz4/zstd/blosc2 data compression
##############################################################################

* Copyright (C) 2019-2026  Qianqian Fang <q.fang at neu.edu>
* License: GNU General Public License version 3 (GPL v3), see License*.txt
* Version: 1.2.preview (Kylie the Opossum - preview)
* URL: https://neurojson.org/zmat
* Github: https://github.com/NeuroJSON/zmat
* PyPi: https://pypi.org/project/zmat
* Acknowledgement: This project is part of the `NeuroJSON project <https://neurojson.org>`_
  supported by US National Institute of Health (NIH)
  grant `U24-NS124027 <https://reporter.nih.gov/project-details/10308329>`_

.. image:: https://github.com/NeuroJSON/zmat/actions/workflows/run_test.yml/badge.svg
    :target: https://github.com/NeuroJSON/zmat/actions/workflows/run_test.yml

.. image:: https://github.com/NeuroJSON/zmat/actions/workflows/build_linux_wheel.yml/badge.svg
    :target: https://github.com/NeuroJSON/zmat/actions/workflows/build_linux_wheel.yml

.. image:: https://github.com/NeuroJSON/zmat/actions/workflows/build_macos_wheel.yml/badge.svg
    :target: https://github.com/NeuroJSON/zmat/actions/workflows/build_macos_wheel.yml

.. image:: https://github.com/NeuroJSON/zmat/actions/workflows/build_windows_wheel.yml/badge.svg
    :target: https://github.com/NeuroJSON/zmat/actions/workflows/build_windows_wheel.yml

.. image:: https://img.shields.io/pypi/v/zmat
    :target: https://pypi.org/project/zmat/

#################
Table of Contents
#################
.. contents::
  :local:
  :depth: 3

============
What's New
============

----------------------------------------
ZMat 1.0.0 (Foxy the Fantastic Mr. Fox)
----------------------------------------

* **Python module** — ZMat is now available as a Python C extension (``pip install zmat``),
  with pre-built wheels for Linux, macOS, and Windows. The Python API supports all
  compression/encoding algorithms available in the MATLAB/Octave toolbox.

* **Octave 10+ compatibility** — special matrices (e.g., logical, complex) are now
  correctly handled in GNU Octave 10 and later.

* **Apple Silicon support** — the blosc2 backend now automatically detects SSE2
  availability on Apple Silicon (M-series) Macs.

* **Updated bundled libraries**:

  - `C-Blosc2 <https://blosc.org>`_ updated to v2.23.1
  - `Zstandard <https://facebook.github.io/zstd/>`_ updated to v1.5.7
  - `miniz <https://github.com/richgel999/miniz>`_ updated to v3.1.1
  - `LZ4 <https://lz4.github.io/lz4/>`_ updated to v1.10.0

* **New Linux MEX binaries** — pre-compiled MEX files for Linux are now
  included in the ``private`` folder alongside the existing macOS and Windows binaries.

* **Memory and safety fixes** — multiple safety, memory-leak, and efficiency
  issues in ``zmatlib.c`` have been resolved.

* AI coding assistant Claude has been used in the development of this release.

============
Introduction
============

ZMat provides both an easy-to-use C-based data compression library -
``libzmat``, a portable mex function to enable ``zlib/gzip/lzma/lz4/zstd/blosc2``
based data compression/decompression and ``base64`` encoding/decoding support
in MATLAB and GNU Octave, as well as a Python C extension module with the same
feature set. It is fast and compact, can process a large array within a fraction
of a second. 

Among all the supported compression methods, or codecs, ``lz4`` is among the fastest for
both compression/decompression; ``lzma`` is the slowest but offers the highest 
compression ratio; ``zlib/gzip`` have excellent balance between speed 
and compression time; ``zstd`` can be fast, although not as fast as ``lz4``,
at the low-compression levels, and can offer higher compression ratios
than ``zlib/gzip``, although not as high as ``lzma``, at high compression
levels. We understand there are many other existing general purpose data
compression algorithms. We prioritize the support of these compression
algorithms because they have widespread use.

Starting in v0.9.9, we added support to a high-performance meta-compressor,
called ``blosc2`` (https://blosc.org). The ``blosc2`` compressor is not single
compression method, but a container format that supports a diverse set of 
strategies to losslessly compress data, especially optimized for storing and
retrieving N-D numerical data of uniform binary types. Users can choose
between ``blosc2blosclz``, ``blosc2lz4``, ``blosc2lz4hc``, ``blosc2zlib`` and
``blosc2zstd``, to access these blosc2 codecs.

The ``libzmat`` library, including the static library (``libzmat.a``) and the
dynamic library ``libzmat.so`` or ``libzmat.dll``, provides a simple interface to 
conveniently compress or decompress a memory buffer:

.. code:: c

  int zmat_run(
        const size_t inputsize,     /* input buffer data length */
        unsigned char *inputstr,    /* input buffer */
        size_t *outputsize,         /* output buffer data length */
        unsigned char **outputbuf,  /* output buffer */
        const int zipid,            /* 0: zlib, 1: gzip, 2: base64, 3: lzma, 4: lzip, 5: lz4, 6: lz4hc 
                                       7: zstd, 8: blosc2blosclz, 9: blosc2lz4, 10: blosc2lz4hc,
                                       11: blosc2zlib, 12: blosc2zstd */
        int *status,                /* return status for error handling */
        const int clevel            /* 1 to compress (default level); 0 to decompress, -1 to -9 (-22 for zstd): setting compression level */
      );

When ``blosc2`` codecs are used, users can set additional compression flags, including
number of threads, byte-shuffling length, can be set via the ``flags.param`` interface
in the following data structure, and pass on ``flags.iscompress`` as the last 
input to ``zmat_run``.

.. code:: c

    union cflag {
        int iscompress;      /* combined flag used to pass on to zmat_run */
        struct settings {    /* unpacked flags */
            char clevel;     /* compression level */
            char nthread;    /* number of compression/decompression threads */
            char shuffle;    /* byte shuffle length */
            char typesize;   /* for ND-array, the byte-size for each array element */
        } param;
    } flags = {0};

The zmat library is highly portable and can be directly embedded in the source code 
to provide maximal portability. In the ``test`` folder, we provided sample codes
to call ``zmat_run/zmat_encode/zmat_decode`` for stream-level compression and 
decompression in C and Fortran90. The Fortran90 C-binding module can be found 
in the ``fortran90`` folder.

The ZMat MATLAB function accepts 3 types of inputs: char-based strings, numerical arrays
or vectors, or logical arrays/vectors. Any other input format will 
result in an error unless you typecast the input into ``int8/uint8``
format. A multi-dimensional numerical array is accepted, and the
original input's type/dimension info is stored in the 2nd output
``"info"``. If one calls ``zmat`` with both the encoded data (in byte vector)
and the ``"info"`` structure, zmat will first decode the binary data 
and then restore the original input's type and size.

The pre-compiled mex binaries for MATLAB are stored inside the 
subfolder named ``private``. Those precompiled for GNU Octave are
stored in the subfolder named ``octave``, with one operating system
per subfolder. The ``PKG_ADD`` script should automatically select
the correct mex file when one types ``addpath`` in Octave.
These precompiled mex files are expected to run out-of-box
across a wide-range of MATLAB (tested as old as R2008) and Octave (tested
as old as v3.8).

If you do not want to compile zmat yourself, you can download the
precompiled package by either clicking on the "Download ZIP" button
on the above URL, or use the below git command:

.. code:: shell

    git clone https://github.com/NeuroJSON/zmat.git

================
Installation
================

The installation of ZMat is no different from any other simple
MATLAB toolboxes. You only need to download/unzip the  package
to a folder, and add the folder's path (that contains ``zmat.m`` and 
the ``"private"`` folder) to MATLAB's path list by using the 
following command:

.. code:: matlab

    addpath('/path/to/zmat');

For Octave, one needs to copy the ``zipmat.mat`` file inside the "``octave``",
from the subfolder matching the OS into the "``private``" subfolder.

If you want to add this path permanently, you need to type "``pathtool``", 
browse to the zmat root folder and add to the list, then click "Save".
Then, run "``rehash``" in MATLAB, and type "``which zmat``", if you see an 
output, that means ZMat is installed for MATLAB/Octave.

If you use MATLAB in a shared environment such as a Linux server, the
best way to add path is to type 

.. code:: shell

   mkdir ~/matlab/
   nano ~/matlab/startup.m

and type ``addpath('/path/to/zmat')`` in this file, save and quit the editor.
MATLAB will execute this file every time it starts. For Octave, the file
you need to edit is ``~/.octaverc`` , where "``~``" is your home directory.

================
Installing ZMat on Linux Distributions
================

One can directly install zmat on Fedora Linux 29 or later via the 
below shell command

.. code:: shell

   sudo dnf install octave-zmat

Similarly, the below command installs the ``libzmat`` library for developing
software using this library:

.. code:: shell

   sudo dnf install zmat zmat-devel zmat-static

The above command installs the dynamic library, C/Fortran90 header files and
static library, respectively

Similarly, if one uses Debian (11) or Ubuntu 21.04 or newer, the command to
install zmat toolbox for Octave (and optionally for MATLAB) is

.. code:: shell

   sudo apt-get install octave-zmat matlab-zmat

and that for installing the development environment is

.. code:: shell

   sudo apt-get install libzmat1 libzmat1-dev

A Ubuntu (16.04/18.04) user can use the same commands as Debian to install these 
packages but one must first run 

.. code:: shell

   sudo add-apt-repository ppa:fangq/ppa
   sudo apt-get update

to enable the `relevant PPA <http://https://launchpad.net/~fangq/+archive/ubuntu/ppa>`_
(personal package achieve) first.

================
Installing ZMat for Python
================

The easiest way to install ZMat for Python is via ``pip``:

.. code:: shell

   pip install zmat

To install from source, clone the repository and build the extension:

.. code:: shell

   git clone https://github.com/NeuroJSON/zmat.git
   cd zmat/python
   pip install .

All compression libraries (miniz, easylzma, lz4, zstd, blosc2) are embedded
in the source and compiled directly into the module — no system libraries are
required. The build can be customized with the following environment variables:

+------------------------+----------+-------------------------------------------+
| Variable               | Default  | Description                               |
+========================+==========+===========================================+
| ``ZMAT_USE_SYSTEM_ZLIB=1`` | off | Link against system ``-lz`` instead of   |
|                        |          | embedded miniz                            |
+------------------------+----------+-------------------------------------------+
| ``ZMAT_NO_LZMA=1``     | off      | Disable lzma/lzip support                 |
+------------------------+----------+-------------------------------------------+
| ``ZMAT_NO_LZ4=1``      | off      | Disable lz4/lz4hc support                |
+------------------------+----------+-------------------------------------------+
| ``ZMAT_NO_ZSTD=1``     | off      | Disable zstd support                      |
+------------------------+----------+-------------------------------------------+
| ``ZMAT_NO_BLOSC2=1``   | off      | Disable blosc2 support                    |
+------------------------+----------+-------------------------------------------+

For example, to build without blosc2 for a smaller binary:

.. code:: shell

   ZMAT_NO_BLOSC2=1 pip install .

================
Using ZMat in Python
================

.. code-block:: python

   import zmat

   # Compress and decompress using the default algorithm (zlib)
   data = b"Hello, ZMat! " * 1000
   compressed = zmat.compress(data)
   print(f"Original: {len(data)} bytes -> Compressed: {len(compressed)} bytes")

   restored = zmat.decompress(compressed)
   assert restored == data

   # Use a different algorithm
   fast      = zmat.compress(data, method='lz4')    # fastest
   small     = zmat.compress(data, method='lzma')   # smallest
   balanced  = zmat.compress(data, method='zstd')   # good balance

   # Base64 encoding / decoding
   encoded = zmat.encode(data, method='base64')
   decoded = zmat.decode(encoded, method='base64')
   assert decoded == data

The four main API functions are:

* ``zmat.compress(data, method='zlib', level=1)`` — compress a bytes-like object
* ``zmat.decompress(data, method='zlib')`` — decompress a bytes-like object
* ``zmat.encode(data, method='base64')`` — encode data (e.g., base64)
* ``zmat.decode(data, method='base64')`` — decode data (e.g., base64)

All functions accept ``bytes``, ``bytearray``, or any object supporting
Python's buffer protocol (including NumPy arrays).

ZMat's zlib and gzip output is fully compatible with Python's standard library:

.. code-block:: python

   import zmat, zlib, gzip, io

   data = b"interoperability test " * 100

   # zmat -> Python zlib
   assert zlib.decompress(zmat.compress(data, method='zlib')) == data

   # Python zlib -> zmat
   assert zmat.decompress(zlib.compress(data), method='zlib') == data

   # zmat -> Python gzip
   compressed = zmat.compress(data, method='gzip')
   with gzip.open(io.BytesIO(compressed), 'rb') as f:
       assert f.read() == data

ZMat can capture a NumPy array's dtype, shape, and memory order alongside the
compressed bytes, so the original array is restored automatically on
decompression. This mirrors the ``[ss, info] = zmat(arr)`` /
``zmat(ss, info)`` pattern from the MATLAB/Octave toolbox:

.. code-block:: python

   import numpy as np
   import zmat

   arr = np.random.rand(1000, 1000)   # float64, C-contiguous

   # compress — capture metadata
   compressed, info = zmat.compress(arr, method='lz4', info=True)
   # info: {'type': 'float64', 'shape': (1000, 1000), 'byte': 8,
   #        'method': 'lz4', 'order': 'C'}

   # decompress — restored to original dtype and shape
   restored = zmat.decompress(compressed, info=info)
   assert np.array_equal(restored, arr)

   # Fortran-contiguous arrays are handled correctly too
   arr_f = np.asfortranarray(np.eye(100))
   compressed, info = zmat.compress(arr_f, info=True)
   restored = zmat.decompress(compressed, info=info)
   assert restored.flags['F_CONTIGUOUS']

For numerical arrays, blosc2 methods with byte-shuffle can achieve
better compression ratios:

.. code-block:: python

   # blosc2 with byte-shuffle (typesize=8 for float64)
   compressed, info = zmat.compress(arr, method='blosc2zstd', info=True)
   restored = zmat.decompress(compressed, info=info)

The low-level ``zmat.zmat()`` interface provides full control over blosc2
multi-threading and shuffle options:

.. code-block:: python

   output = zmat.zmat(data, iscompress=1, method='blosc2zstd',
                      nthread=4, shuffle=1, typesize=8)

``iscompress=1`` compresses at the default level; ``iscompress=0``
decompresses; a negative value (e.g., ``-9``) sets the compression level.

================
Using ZMat in MATLAB
================

ZMat provides a single mex function, ``zipmat.mex*`` -- for both compressing/encoding
or decompresing/decoding data streams. The help info of the function is shown
below

----------
zmat.m
----------

.. code-block:: matlab

  output=zmat(input)
     or
  [output, info]=zmat(input, iscompress, method)
  [output, info]=zmat(input, iscompress, method, options ...)
  output=zmat(input, info)
 
  A portable data compression/decompression toolbox for MATLAB/GNU Octave
 
  author: Qianqian Fang <q.fang at neu.edu>
  initial version created on 04/30/2019
 
  input:
       input: a char, non-complex numeric or logical vector or array
       iscompress: (optional) if iscompress is 1, zmat compresses/encodes the input,
              if 0, it decompresses/decodes the input. Default value is 1.
 
              if iscompress is set to a negative integer, (-iscompress) specifies
              the compression level. For zlib/gzip, default level is 6 (1-9); for
              lzma/lzip, default level is 5 (1-9); for lz4hc, default level is 8 (1-16).
              the default compression level is used if iscompress is set to 1.
 
              zmat removes the trailing newline when iscompress=2 and method='base64'
              all newlines are kept when iscompress=3 and method='base64'
 
              if one defines iscompress as the info struct (2nd output of zmat), zmat
              will perform a decoding/decompression operation and recover the original
              input using the info stored in the info structure.
       method: (optional) compression method, currently, zmat supports the below methods
              'zlib': zlib/zip based data compression (default)
              'gzip': gzip formatted data compression
              'lzip': lzip formatted data compression
              'lzma': lzma formatted data compression
              'lz4':  lz4 formatted data compression
              'lz4hc':lz4hc (LZ4 with high-compression ratio) formatted data compression
              'zstd':  zstd formatted data compression
              'blosc2blosclz':  blosc2 meta-compressor with blosclz compression
              'blosc2lz4':  blosc2 meta-compressor with lz4 compression
              'blosc2lz4hc':  blosc2 meta-compressor with lz4hc compression
              'blosc2zlib':  blosc2 meta-compressor with zlib/zip compression
              'blosc2zstd':  blosc2 meta-compressor with zstd compression
              'base64': encode or decode use base64 format
      options: a series of ('name', value) pairs, supported options include
              'nthread': followed by an integer specifying number of threads for blosc2 meta-compressors
              'typesize': followed by an integer specifying the number of bytes per data element (used for shuffle)
              'shuffle': shuffle methods in blosc2 meta-compressor, 0 disable, 1, byte-shuffle
 
  output:
       output: a uint8 row vector, storing the compressed or decompressed data;
              empty when an error is encountered
       info: (optional) a struct storing additional info regarding the input data, may have
             'type': the class of the input array
             'size': the dimensions of the input array
             'byte': the number of bytes per element in the input array
             'method': a copy of the 3rd input indicating the encoding method
             'status': the zlib/lzma/lz4 compression/decompression function return value,
                     including potential error codes; see documentation of the respective
                     libraries for details
             'level': a copy of the iscompress flag; if non-zero, specifying compression
                     level, see above
 
  example:
 
    [ss, info]=zmat(eye(5))
    orig=zmat(ss,0)
    orig=zmat(ss,info)
    ss=char(zmat('zmat test',1,'base64'))
    orig=char(zmat(ss,0,'base64'))
 
  -- this function is part of the zmat toolbox (https://github.com/NeuroJSON/zmat)

---------
examples
---------

Under the ``"example"`` folder, you can find a demo script showing the 
basic utilities of ZMat. Running the ``"demo_zmat_basic.m"`` script, 
you can see how to compress/decompress a simple array, as well as apply
base64 encoding/decoding to strings.

Please run these examples and understand how ZMat works before you use
it to process your data.

Under the ``"c"`` and ``"f90"`` folders, sample C/Fortran90 units calling
the compression/decompression APIs provided by zmat are also provided.
You may run ``"make"`` in each of the folders to build the binary and
execute the output program.

---------
unit tests
---------

Under the ``"test"`` folder, you can run ``"run_zmat_test.m"`` script to
run unit tests on the key features provided by zmat.

==========================
Compile ZMat
==========================

To recompile ZMat, you first need to check out ZMat source code, along
with the needed submodules from the Github repository using the below 
command

.. code:: shell

      git clone https://github.com/NeuroJSON/zmat.git zmat

Next, you need to make sure your system has ``gcc``, ``g++``,
``mex`` and ``mkoctfile`` (if compiling for Octave is needed). If not, 
please install gcc, MATLAB and GNU Octave and add the paths to 
these utilities to the system PATH environment variable.

To compile zmat, you may choose one of the three methods:

1. Method 1: please open MATLAB or Octave, and run the below commands

.. code-block:: matlab

      cd zmat/src
      compilezmat

The above script utilizes the MinGW-w64 MATLAB Compiler plugin.

To install the MinGW-w64 compiler plugin for MATLAB, please follow
the below steps

- If you have MATLAB R2017b or later, you may skip this step.
  To compile mcxlabcl in MATLAB R2017a or earlier on Windows, you must 
  pre-install the MATLAB support for MinGW-w64 compiler 
  https://www.mathworks.com/matlabcentral/fileexchange/52848-matlab-support-for-mingw-w64-c-c-compiler

  Note: it appears that installing the above Add On is no longer working
  and may give an error at the download stage. In this case, you should
  install MSYS2 from https://www.msys2.org/. Once you install MSYS2,
  run MSYS2.0 MinGW 64bit from Start menu, in the popup terminal window,
  type

.. code-block:: shell

     pacman -Syu
     pacman -S base-devel gcc git zlib-devel

Then, start MATLAB, and in the command window, run

.. code-block:: matlab

     setenv('MW_MINGW64_LOC','C:\msys64\usr');

- After installation of MATLAB MinGW support, you must type 
  ``mex -setup C`` in MATLAB and select "MinGW64 Compiler (C)". 
- Once you select the MingW C compiler, you should run ``mex -setup C++``
  again in MATLAB and select "MinGW64 Compiler (C++)" to compile C++.

2. Method 2: Compile with cmake (3.3 or later) 

Please open a terminal, and run the below shall commands

.. code-block:: shell

      cd zmat/src
      rm -rf build
      mkdir build && cd build
      cmake ../
      make clean
      make

if MATLAB was not installed in a standard path, you may change ``cmake ../`` to

.. code-block:: shell

      cmake -DMatlab_ROOT_DIR=/path/to/matlab/root ../

by default, this will first compile ``libzmat.a`` and then create the ``.mex`` file 
that is statically linked with ``libzmat.a``. If one prefers to create a dynamic
library ``libzmat.so`` and then a dynamically linked ``.mex`` file, this can
be done by

.. code-block:: shell

      cmake -DMatlab_ROOT_DIR=/path/to/matlab/root -DSTATIC_LIB=off ../


3. Method 3: please open a terminal, and run the below shall commands

.. code-block:: shell

      cd zmat/src
      make clean
      make mex

to create the mex file for MATLAB, and run ``make clean oct`` to compile
the mex file for Octave. 

The compiled mex files are named as ``zipmat.mex*`` under the zmat root folder.
One may move those into the ``private`` folder to overwrite the existing files,
or leave them in the root folder. MATLAB/Octave will use these files when 
``zmat`` is called.

==========================
Contribution and feedback
==========================

ZMat is an open-source project. This means you can not only use it and modify
it as you wish, but also you can contribute your changes back to ZMat so
that everyone else can enjoy the improvement. For anyone who want to contribute,
please download ZMat source code from its source code repositories by using the
following command:


.. code:: shell

      git clone https://github.com/NeuroJSON/zmat.git zmat

or browsing the github site at

.. code:: shell

      https://github.com/NeuroJSON/zmat
 

You can make changes to the files as needed. Once you are satisfied with your
changes, and ready to share it with others, please submit your changes as a
"pull request" on github.  The project maintainer, Dr. Qianqian Fang will
review the changes and choose to accept the patch.

We appreciate any suggestions and feedbacks from you. Please use the NeuroJSON
forum to report any questions you may have regarding ZMat:

`NeuroJSON forum <https://github.com/orgs/NeuroJSON/discussions>`_

(Subscription to the mailing list is needed in order to post messages).


==========================
Acknowledgement
==========================

ZMat bundles the below open-source data compression/encoding libraries

1. miniz: https://github.com/richgel999/miniz
  *  Copyright (c) 2013-2014 RAD Game Tools and Valve Software
  *  Copyright (c) 2010-2014 Rich Geldreich and Tenacious Software LLC
  *  License: MIT License (https://github.com/richgel999/miniz/blob/master/LICENSE)
2. Eazylzma: https://github.com/lloyd/easylzma
  *  Author: Lloyd Hilaiel (lloyd)
  *  License: public domain
3. Original LZMA library:
  *  Author: Igor Pavlov
  *  License: public domain
4. LZ4 library: https://lz4.github.io/lz4/
  *  Copyright (C) 2011-2019, Yann Collet.
  *  License: BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)
5. C-blosc2: https://blosc.org/
  *  Copyright (c) 2009-2018 Francesc Alted <francesc@blosc.org>
  *  Copyright (c) 2019-present The Blosc Development Team <blosc@blosc.org>
  *  License: BSD 3-Clause License (http://www.opensource.org/licenses/bsd-license.php)
6. ZStd: https://facebook.github.io/zstd/
  *  Copyright (c) Meta Platforms, Inc. and affiliates. All rights reserved.
  *  License: BSD 3-Clause License (http://www.opensource.org/licenses/bsd-license.php)
7. base64_{encode,decode} functions in the zmat.cpp
  *  Copyright (c) 2005, Jouni Malinen <j@w1.fi>
  *  License: BSD 3-Clause License (http://www.opensource.org/licenses/bsd-license.php)
  *  Source http://web.mit.edu/freebsd/head/contrib/wpa/src/utils/base64.c and base64.h