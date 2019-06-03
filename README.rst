##############################################################################                                                      
  ZMAT: A portable data compression/decompression toolbox for MATLAB/Octave             
##############################################################################

* Copyright (C) 2019  Qianqian Fang <q.fang at neu.edu>
* License: GNU General Public License version 3 (GPL v3), see License*.txt
* Version: 0.8 (Mox-the-fox)
* Binaries: http://github.com/fangq/zmat_mex
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
in MATLAB and GNU Octave. It is fast and portable, can compress a 
large array within a fraction of a second.

ZMat accepts 3 types of inputs: char-based strings, uint8 arrays
or vectors, or int8 arrays/vectors. Any other input format will 
result in an error unless you typecast the input into int8/uint8
format. A multi-dimensional char/int8/uint8 array is accepeted
but will be processed as a 1D vector. One can reshape the output
after decoding using the 2nd output "info" from zmat.

ZMat uses zlib - an open-source and widely used library for data
compression. On Linux/Mac OSX, you need to have libz.so or libz.dylib
installed in your system library path (defined by the environment
variables LD_LIBRARY_PATH or DYLD_LIBRARY_PATH, respectively).

The pre-compiled mex binaries for both MATLAB and Octave are 
provided in a separate github repository

http://github.com/fangq/zmat_mex

If you do not want to compile zmat yourself, you can download the
precompiled package by either clicking on the "Download ZIP" button
on the above URL, or use the below git command:

.. code:: shell

    git clone https://github.com/fangq/zmat_mex.git

================
Installation
================

The installation of ZMat is no different from any other simple
MATLAB toolboxes. You only need to download/unzip the  package
to a folder, and add the folder's path to MATLAB/Octave's path list
by using the following command:

.. code:: matlab

    addpath('/path/to/zmax');

If you want to add this path permanently, you need to type "pathtool", 
browse to the zmat root folder and add to the list, then click "Save".
Then, run "rehash" in MATLAB, and type "which zmat", if you see an 
output, that means ZMax is installed for MATLAB/Octave.

If you use MATLAB in a shared environment such as a Linux server, the
best way to add path is to type 

.. code:: shell

   mkdir ~/matlab/
   nano ~/matlab/startup.m

and type addpath('/path/to/zmax') in this file, save and quit the editor.
MATLAB will execute this file every time it starts. For Octave, the file
you need to edit is ~/.octaverc , where "~" is your home directory.

================
Using ZMat
================

ZMat provides a single mex function, zmat.mex* -- for both compressing/encoding
or decompresing/decoding data streams. The help info of the function is shown
below

----------
zmat.m
----------

.. code-block:: matlab

  output=zmat(input)
     or
  [output, info]=zmat(input, iscompress, method)
 
  A portable data compression/decompression toolbox for MATLAB/GNU Octave
  
  author: Qianqian Fang <q.fang at neu.edu>
  date for initial version: 04/30/2019
 
  input:
       input: a string, int8 or uint8 array
       iscompress: (optional) if iscompress is 1, zmat compresses/encodes the input, 
              if 0, it decompresses/decodes the input. Default value is 1.
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
             'status': the zlib function return value, including potential error codes (<0)
 
  example:
 
    [ss, info]=zmat(uint8(eye(5)))
    orig=zmat(ss,0)
    orig=reshape(orig, info.size)
    ss=char(zmat('zmat test',1,'base64'))
    orig=char(zmat(ss,0,'base64'))
 
  -- this function is part of the ZMAT toolbox (http://github.com/fangq/zmat)


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
Contribution and feedback
==========================

ZMat is an open-source project. This means you can not only use it and modify
it as you wish, but also you can contribute your changes back to JSONLab so
that everyone else can enjoy the improvement. For anyone who want to contribute,
please download JSONLab source code from its source code repositories by using the
following command:


.. code:: shell

      git clone https://github.com/fangq/zmat.git zmat

or browsing the github site at

.. code:: shell

      https://github.com/fangq/zmat
 

You can make changes to the files as needed. Once you are satisfied with your
changes, and ready to share it with others, please cd the root directory of 
ZNat, and type

.. code:: shell

      git diff --no-prefix > yourname_featurename.patch
 

You then email the .patch file to ZMat's maintainer, Qianqian Fang, at
the email address shown in the beginning of this file. Qianqian will review 
the changes and commit it to the subversion if they are satisfactory.

We appreciate any suggestions and feedbacks from you. Please use the iso2mesh
mailing list to report any questions you may have regarding ZMat:

`iso2mesh-users <https://groups.google.com/forum/#!forum/iso2mesh-users>`_

(Subscription to the mailing list is needed in order to post messages).
