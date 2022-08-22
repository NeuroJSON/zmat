############################################################
#  ZMat: A portable C-library and MATLAB/Octave toolbox for inline data compression
#
#  Author: Qianqian Fang <q.fang at neu.edu>
############################################################

PKGNAME=zmat
LIBNAME=lib$(PKGNAME)
MEXNAME=zipmat
VERSION=0.9.9
SOURCE=src

all: mex oct lib dll

lib:
	-$(MAKE) -C $(SOURCE) lib
dll:
	-$(MAKE) -C $(SOURCE) dll
mex:
	-$(MAKE) -C $(SOURCE) mex
oct:
	-$(MAKE) -C $(SOURCE) oct
clean:
	-rm -rf $(LIBNAME).* $(MEXNAME).mex*
	-$(MAKE) -C $(SOURCE) clean

.DEFAULT_GOAL := mex

