#################################################################
#  Makefile for ZMAT
#  Qianqian Fang <q.fang at neu.edu>
#  2019/04/30
#################################################################

BACKEND ?=

ROOTDIR ?=..
ZMATDIR ?=$(ROOTDIR)
LIBDIR  ?=$(ROOTDIR)/lib

MKDIR      :=mkdir
HAVE_LZMA  ?=yes
HAVE_LZ4   ?=yes
HAVE_BLOSC2?=yes

MEX=mex
AR=$(CC)
ECHO       := echo

BINARY:=zipmat
OUTPUT_DIR=$(ZMATDIR)

DOXY       := doxygen
DOCDIR     := $(ZMATDIR)/doc
DOXYCFG=zmat.cfg

INCLUDEDIRS=-I../include

CUOMPLINK=

ARCH = $(shell uname -m)
PLATFORM = $(shell uname -s)

DLLFLAG=-fPIC
OMP=-fopenmp

CPPOPT=-g -Wall -O3 -fPIC #-g -Wall -std=c99 # -DUSE_OS_TIMER

OUTPUTFLAG:=-o
OBJSUFFIX=.o
EXESUFFIX=.mex*

FILES=zmatlib

ifeq ($(findstring CYGWIN,$(PLATFORM)), CYGWIN)
  ifeq ($(findstring x86_64,$(ARCH)), x86_64)
      LINKOPT=-L"$(CUDA_PATH)/lib/x64" $(CUDART)
  else
      LINKOPT=-L"$(CUDA_PATH)/lib/Win32" $(CUDART)
  endif
  INCLUDEDIRS +=-I"$(CUDA_PATH)/lib/include"
  CPPOPT =-c -D_CRT_SECURE_NO_DEPRECATE -DWIN32
  OBJSUFFIX=.obj
  EXESUFFIX=
  DLLFLAG=
  OMP=-fopenmp
  MEX=cmd /c mex
else ifeq ($(findstring Darwin,$(PLATFORM)), Darwin)

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

ifeq ($(HAVE_LZMA),no)
  CFLAGS+=-DNO_LZMA
else
  INCLUDEDIRS+=-Ieasylzma -Ieasylzma/pavlov
  FILES+=easylzma/compress easylzma/decompress \
         easylzma/lzma_header easylzma/lzip_header easylzma/common_internal \
         easylzma/pavlov/LzmaEnc easylzma/pavlov/LzmaDec easylzma/pavlov/LzmaLib \
         easylzma/pavlov/LzFind easylzma/pavlov/Bra easylzma/pavlov/BraIA64 \
         easylzma/pavlov/Alloc easylzma/pavlov/7zCrc
endif

ifeq ($(HAVE_BLOSC2),no)
  CFLAGS+=-DNO_BLOSC2
else
  ifeq ($(HAVE_LZ4),no)
    INCLUDEDIRS+=-Ilz4
    FILES+= lz4/lz4 lz4/lz4hc
  endif
  LINKOPT+=-Lblosc2/lib -lblosc2 -Lblosc2/internal-complibs/zstd-1.5.2 -lzstd
  INCLUDEDIRS+=-Iblosc2/include
endif

ifeq ($(HAVE_LZ4),no)
  CFLAGS+=-DNO_LZ4
else
  INCLUDEDIRS+=-Ilz4
  FILES+= lz4/lz4 lz4/lz4hc
endif




ifeq ($(MAKECMDGOALS),lib)
   AR         :=ar
   ARFLAGS    :=cr
   BINARY     :=libzmat.a
   AROUTPUT   :=
   LINKOPT    :=
   OUTPUT_DIR :=$(LIBDIR)
   ifeq ($(findstring Darwin,$(PLATFORM)), Darwin)
      OUTPUTFLAG :=
   endif
endif

ifeq ($(MAKECMDGOALS),dll)
   OUTPUTFLAG :=-o
   BINARY     :=libzmat.so
   OUTPUT_DIR :=$(LIBDIR)

   ifeq ($(findstring Darwin,$(PLATFORM)), Darwin)
      ARFLAGS    :=-shared -Wl,-install_name,$(BINARY).1 -lz
   else
      ARFLAGS    :=-shared -Wl,-soname,$(BINARY).1 -lz
   endif
endif

ifeq ($(MAKECMDGOALS),dll)
   BINARY     :=libzmat.so
endif

dll:   CPPOPT     +=$(DLLFLAG)
dll:   AR         :=gcc
dll:   ARFLAGS    ?=-shared -Wl,-soname,$(BINARY).1
dll:   LINKOPT    +=$(LDFLAGS)
dll:   AROUTPUT   :=-o

oct mex: CPPOPT+= $(DLLFLAG)
oct:   OUTPUT_DIR=..
oct:   AR= CXXFLAGS='-O3' mkoctfile zmat.cpp
oct:   BINARY=zmat.mex
oct:   ARFLAGS    :=
oct:   LINKOPT+=--mex $(INCLUDEDIRS)

mex:   CXX=$(MEX)
mex:   OUTPUTFLAG:=-output
mex:   AR=$(MEX) zmat.cpp $(INCLUDEDIRS)
mex:   LINKOPT+= -cxx CXXLIBS='$$CXXLIBS -lz -static-libgcc -static-libstdc++' -outdir $(ZMATDIR)
mex:   ARFLAGS    :=
mex:   OUTPUT_DIR=..

all: mex

TARGETSUFFIX:=$(suffix $(BINARY))

doc: makedocdir
	$(DOXY) $(DOXYCFG)

OBJS      := $(addsuffix $(OBJSUFFIX), $(FILES))

all dll lib mex oct: $(OUTPUT_DIR)/$(BINARY)

blosc:
	@if [ $(HAVE_BLOSC2) = "yes" ]; then\
	    $(MAKE) -C blosc2 all;\
	fi

makedirs:
	@if test ! -d $(OUTPUT_DIR); then $(MKDIR) $(OUTPUT_DIR); fi

makedocdir:
	@if test ! -d $(DOCDIR); then $(MKDIR) $(DOCDIR); fi

$(OUTPUT_DIR)/$(BINARY): makedirs $(OBJS) blosc
$(OUTPUT_DIR)/$(BINARY): $(OBJS)
	@$(ECHO) Building $@
	$(AR) $(ARFLAGS) $(OUTPUTFLAG) $@ $(OBJS) $(LINKOPT) $(USERLINKOPT)

%$(OBJSUFFIX): %.cpp
	@$(ECHO) Building $@
	$(CXX) $(INCLUDEDIRS) $(CPPOPT) -c -o $@ $<

%$(OBJSUFFIX): %.c
	@$(ECHO) Building $@
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDEDIRS) $(CPPOPT) -c -o $@ $<

%$(OBJSUFFIX): %.cu
	@$(ECHO) Building $@
	$(CUDACC) -c $(CUCCOPT) -o $@  $<

clean:
	-rm -f $(OBJS) $(OUTPUT_DIR)/$(BINARY)$(EXESUFFIX) zmat$(OBJSUFFIX) $(LIBDIR)/*
	-$(MAKE) -C blosc2 clean

pretty:
	astyle \
	    --style=attach \
	    --indent=spaces=4 \
	    --indent-modifiers \
	    --indent-switches \
	    --indent-preproc-block \
	    --indent-preproc-define \
	    --indent-col1-comments \
	    --pad-oper \
	    --pad-header \
	    --align-pointer=type \
	    --align-reference=type \
	    --add-brackets \
	    --convert-tabs \
	    --lineend=linux \
	    --preserve-date \
	    --suffix=none \
	    --formatted \
	    --break-blocks \
	   "*.c" "../include/*.h" "*.cpp" "../example/c/*.c"

.PHONY: all mex oct lib dll

.DEFAULT_GOAL := all

