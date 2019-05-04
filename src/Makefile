#################################################################
#  Makefile for ZMAT
#  Qianqian Fang <q.fang at neu.edu>
#  2019/04/30
#################################################################

BACKEND ?=

ROOTDIR ?= ..
ZMATDIR  ?= $(ROOTDIR)
MCXSRC :=$(ZMATDIR)/src

MKDIR      := mkdir

MEX=mex
AR=$(CC)

BINARY=zmat
OUTPUT_DIR=$(ZMATDIR)

DOXY       := doxygen
DOCDIR     := $(ZMATDIR)/doc
DOXYCFG=zmat.cfg

INCLUDEDIRS=

CUOMPLINK=

ARCH = $(shell uname -m)
PLATFORM = $(shell uname -s)

DLLFLAG=
OMP=-fopenmp

CPPOPT=#-g -Wall -std=c99 # -DUSE_OS_TIMER

OUTPUTFLAG:=-o
OBJSUFFIX=.o
EXESUFFIX=

FILES=

ifeq ($(findstring CYGWIN,$(PLATFORM)), CYGWIN)
  CC=nvcc
  CUOMPLINK=-Xcompiler
  ifeq ($(findstring x86_64,$(ARCH)), x86_64)
      LINKOPT=-L"$(CUDA_PATH)/lib/x64" $(CUDART)
  else
      LINKOPT=-L"$(CUDA_PATH)/lib/Win32" $(CUDART)
  endif
  INCLUDEDIRS +=-I"$(CUDA_PATH)/lib/include"
  CPPOPT =-c -D_CRT_SECURE_NO_DEPRECATE -DWIN32
  OBJSUFFIX=.obj
  EXESUFFIX=.exe
  DLLFLAG=
  OMP=/openmp
  MEX=cmd /c mex
  CPPOPT+=-Xcompiler $(OMP)
else ifeq ($(findstring Darwin,$(PLATFORM)), Darwin)
  CPPOPT+=
else
  CPPOPT+=
  CUCCOPT+=-Xcompiler $(OMP)
  ifeq ($(findstring x86_64,$(ARCH)), x86_64)
     CPPOPT +=
     CUCCOPT +=-m64
     ifeq "$(wildcard /usr/local/cuda/lib64)" "/usr/local/cuda/lib64"
        ifeq ($(BACKEND),cuda)
           LINKOPT=-L/usr/local/cuda/lib64 $(CUDART) -lm -lstdc++
        else ifeq ($(BACKEND),cudastatic)
           LINKOPT=-L/usr/local/cuda/lib64 $(CUDART) -lm -static-libgcc -static-libstdc++
        endif
     endif
  endif
endif

oct:   OUTPUT_DIR=..
oct:   AR= CXXFLAGS='-O3' LFLAGS='$(-lz)' LDFLAGS='$(LFLAGS)' mkoctfile zmat.cpp 
oct:   BINARY=zmat.mex
oct:   LINKOPT+=--mex $(INCLUDEDIRS)
oct:   CXX=mkoctfile

mex:   CXX=$(MEX)
mex:   OUTPUTFLAG:=-output
mex:   AR=$(MEX) zmat.cpp
mex:   LINKOPT+= -cxx CXXLIBS='$$CXXLIBS -lz' -outdir $(ZMATDIR)
mex:   OUTPUT_DIR=..

all: mex

doc: makedocdir
	$(DOXY) $(DOXYCFG)

OBJS      := $(addsuffix $(OBJSUFFIX), $(FILES))

TARGETSUFFIX:=$(suffix $(BINARY))

all mex oct: $(OUTPUT_DIR)/$(BINARY)

makedirs:
	@if test ! -d $(OUTPUT_DIR); then $(MKDIR) $(OUTPUT_DIR); fi

makedocdir:
	@if test ! -d $(DOCDIR); then $(MKDIR) $(DOCDIR); fi

$(OUTPUT_DIR)/$(BINARY): makedirs $(OBJS)
$(OUTPUT_DIR)/$(BINARY): $(OBJS)
	$(AR) $(OBJS) $(OUTPUTFLAG) $(OUTPUT_DIR)/$(BINARY) $(LINKOPT) $(USERLINKOPT)

%$(OBJSUFFIX): %.cpp
	$(CXX) $(INCLUDEDIRS) $(CPPOPT) -c -output $@ $<

%$(OBJSUFFIX): %.c
	$(CC) $(INCLUDEDIRS) $(CPPOPT) -c -output $@ $<

%$(OBJSUFFIX): %.cu
	$(CUDACC) -c $(CUCCOPT) -o $@  $<

clean:
	-rm -f $(OBJS) $(OUTPUT_DIR)/$(BINARY)$(EXESUFFIX)

.DEFAULT_GOAL := all

