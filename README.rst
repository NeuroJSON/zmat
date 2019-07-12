##############################################################################                                                      
  ZMAT: A portable data compression/decompression toolbox for MATLAB/Octave             
##############################################################################

* Copyright (C) 2019  Qianqian Fang <q.fang at neu.edu>
* License: GNU General Public License version 3 (GPL v3), see License*.txt
* Version: 0.8 (Mox-the-fox)
* URL: http://github.com/fangq/zmat

#################
Table of Contents
#################
.. contents::
  :local:
  :depth: 3

============
Introduction
============

ZMat is a portable mex function to enable zlib/gzip/lzma/lzip based 
data compression/decompression and base64 encoding/decoding support 
in MATLAB and GNU Octave. It is fast and compact, can process a 
large array within a fraction of a second.

ZMat accepts 3 types of inputs: char-based strings, numerical arrays
or vectors, or logical arrays/vectors. Any other input format will 
result in an error unless you typecast the input into ```int8/uint8```
format. A multi-dimensional numerical array is accepeted, and the
original input's type/dimension info is stored in the 2nd output
``"info"``. If one calls ``zmat`` with both the encoded data (in byte vector)
and the ``"info"`` structure, zmat will first decode the binary data 
and then restore the original input's type and size.

ZMat uses zlib - an open-source and widely used library for data
compression. On Linux/Mac OSX, you need to have libz.so or libz.dylib
installed in your system library path (defined by the environment
variables ``LD_LIBRARY_PATH`` or ``DYLD_LIBRARY_PATH``, respectively).

The pre-compiled mex binaries for MATLAB are stored inside the 
subfolder named "private". Those precompiled for GNU Octave are
stored in the subfolder named "octave", with one operating system
per subfolder.

If you do not want to compile zmat yourself, you can download the
precompiled package by either clicking on the "Download ZIP" button
on the above URL, or use the below git command:

.. code:: shell

    git clone https://github.com/fangq/zmat.git

================
Installation
================

The installation of ZMat is no different from any other simple
MATLAB toolboxes. You only need to download/unzip the  package
to a folder, and add the folder's path (that contains zmat.m and 
the "private" folder) to MATLAB's path list by using the 
following command:

.. code:: matlab

    addpath('/path/to/zmat');

For Octave, one needs to copy the zipmat.mat file inside the "``octave``",
from the subfolder matching the OS into the "``private``" subfolder.

If you want to add this path permanently, you need to type "``pathtool``", 
browse to the zmat root folder and add to the list, then click "Save".
Then, run "``rehash``" in MATLAB, and type "``which zmat``", if you see an 
output, that means ZMax is installed for MATLAB/Octave.

If you use MATLAB in a shared environment such as a Linux server, the
best way to add path is to type 

.. code:: shell

   mkdir ~/matlab/
   nano ~/matlab/startup.m

and type ``addpath('/path/to/zmax')`` in this file, save and quit the editor.
MATLAB will execute this file every time it starts. For Octave, the file
you need to edit is ``~/.octaverc`` , where "``~``" is your home directory.

================
Using ZMat
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
  output=zmat(input, info)
 
  A portable data compression/decompression toolbox for MATLAB/GNU Octave
  
  author: Qianqian Fang <q.fang at neu.edu>
  initial version created on 04/30/2019
 
  input:
       input: a char, non-complex numeric or logical vector or array
       iscompress: (optional) if iscompress is 1, zmat compresses/encodes the input, 
              if 0, it decompresses/decodes the input. Default value is 1.
              if one defines iscompress as the info struct (2nd output of
              zmat) during encoding, zmat will perform a
              decoding/decompression operation and recover the original
              input using the info stored in the info structure.
       method: (optional) compression method, currently, zmat supports the below methods
              'zlib': zlib/zip based data compression (default)
              'gzip': gzip formatted data compression
              'lzip': lzip formatted data compression
              'lzma': lzma formatted data compression
              'base64': encode or decode use base64 format
 
  output:
       output: a uint8 row vector, storing the compressed or decompressed data
       info: (optional) a struct storing additional info regarding the input data, may have
             'type': the class of the input array
             'size': the dimensions of the input array
             'byte': the number of bytes per element in the input array
             'method': a copy of the 3rd input indicating the encoding method
             'status': the zlib function return value, including potential error codes (<0)
 
  example:
 
    [ss, info]=zmat(eye(5))
    orig=zmat(ss,0)
    orig=zmat(ss,info)
    ss=char(zmat('zmat test',1,'base64'))
    orig=char(zmat(ss,0,'base64'))
 
  -- this function is part of the zmat toolbox (http://github.com/fangq/zmat)

---------
examples
---------

Under the ``"example"`` folder, you can find a demo script showing the 
basic utilities of ZMat. Running the ``"demo_zmat_basic.m"`` script, 
you can see how to compress/decompress a simple array, as well as apply
base64 encoding/decoding to strings.

Please run these examples and understand how ZMat works before you use
it to process your data.


==========================
Compile ZMat
==========================

To recompile ZMat, you first need to check out ZMat source code, along
with the needed submodules from the Github repository using the below 
command

.. code:: shell

      git clone --recursive https://github.com/fangq/zmat.git zmat

Next, you need to make sure your system has ``cmake``, ``gcc``, ``g++``,
``mex`` and ``mkoctfile`` (if compiling for Octave is needed). If not, 
please install CMake, gcc, MATLAB and GNU Octave and add the paths to 
these utilities to the system PATH environment variable.

The first step of compilation is to compile Eazylzma (https://github.com/lloyd/easylzma),
a submodule that zmat needs to apply lzma compression.

Please use the below commands in a terminal window

.. code-block:: shell

      cd zmat/src/eazylzma
      cmake .
      make clean all

if there is any dependency is missing, please install and rerun the compilation.
If successful, a static library named ``zmat/src/easylzma/easylzma-0.0.8/lib/libeasylzma_s.a``
is generated.

The next step is to compile zmat. You may choose one of the two methods:

Method 1: please open MATLAB or Octave, and run the below commands

.. code-block:: matlab

      cd zmat/src
      compilezmat

Method 2: please open a terminal, and run the below shall commands

.. code-block:: shell

      cd zmat/src
      make clean mex

to create the mex file for MATLAB, and run ``make clean oct`` to compile
the mex file for Octave. 

The compilex mex files are named as ``zipmat.mex*`` under the zmat root folder.
One may move those into the ``private`` folder to overwrite the existing files,
or leave them in the root folder. MATLAB/Octave will use these files when 
``zmat`` is called.

==========================
Contribution and feedback
==========================

ZMat is an open-source project. This means you can not only use it and modify
it as you wish, but also you can contribute your changes back to JSONLab so
that everyone else can enjoy the improvement. For anyone who want to contribute,
please download JSONLab source code from its source code repositories by using the
following command:


.. code:: shell

      git clone --recursive https://github.com/fangq/zmat.git zmat

or browsing the github site at

.. code:: shell

      https://github.com/fangq/zmat
 

You can make changes to the files as needed. Once you are satisfied with your
changes, and ready to share it with others, please submit your changes as a
"pull request" on github.  The project maintainer, Dr. Qianqian Fang will
review the changes and choose to accept the patch.

We appreciate any suggestions and feedbacks from you. Please use the iso2mesh
mailing list to report any questions you may have regarding ZMat:

`iso2mesh-users <https://groups.google.com/forum/#!forum/iso2mesh-users>`_

(Subscription to the mailing list is needed in order to post messages).
