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

MEX=mex
AR=$(CC)
ECHO       := echo

BINARY:=blosc2
OUTPUT_DIR=$(ZMATDIR)

DOXY       := doxygen
DOCDIR     := $(ZMATDIR)/doc
DOXYCFG=zmat.cfg

INCLUDEDIRS=-I../../lz4 -I../include -I../internal-complibs/zstd

ifeq ($(HAVE_ZLIB),miniz)
  HAVE_ZLIB=yes
  INCLUDEDIRS+=-I../../miniz
endif

CUOMPLINK=

ARCH = $(shell uname -m)
PLATFORM = $(shell uname -s)

DLLFLAG=-fPIC

CPPOPT=-g -Wall -Wextra -O3 -msse2 -DNDEBUG $(DLLFLAG) -std=gnu99 -Wno-unused-variable #-g -Wall -std=c99 # -DUSE_OS_TIMER

ifeq ($(HAVE_ZLIB),yes)
  CPPOPT+=-DHAVE_ZLIB
endif

ifeq ($(HAVE_ZSTD),yes)
  CPPOPT+=-DHAVE_ZSTD
endif

ifeq ($(HAVE_LZ4),yes)
  CPPOPT+=-DHAVE_LZ4
endif

OUTPUTFLAG:=-o
OBJSUFFIX=.o
EXESUFFIX=.mex*

FILES=bitshuffle-sse2 blosc2 blosc2-stdio blosclz delta directories fastcopy \
      frame schunk sframe shuffle shuffle-generic bitshuffle-generic \
      shuffle-sse2 stune timestamp trunc-prec

ifeq ($(findstring CYGWIN,$(PLATFORM)), CYGWIN)
  CPPOPT =-c -DWIN32
  OBJSUFFIX=.obj
  EXESUFFIX=
  DLLFLAG=
  MEX=cmd /c mex
else ifeq ($(findstring Darwin,$(PLATFORM)), Darwin)

else
  CPPOPT+=
  ifeq ($(findstring x86_64,$(ARCH)), x86_64)
     CPPOPT +=
  endif
endif

ifeq ($(MAKECMDGOALS),lib)
   AR         :=ar
   ARFLAGS    :=cr
   BINARY     :=libblosc2.a
   AROUTPUT   :=
   LINKOPT    :=
   OUTPUT_DIR :=$(LIBDIR)
   ifeq ($(findstring Darwin,$(PLATFORM)), Darwin)
      OUTPUTFLAG :=
   endif
endif

ifeq ($(MAKECMDGOALS),dll)
   OUTPUTFLAG :=-o
   BINARY     :=libblosc2.so
   OUTPUT_DIR :=$(LIBDIR)

   ifeq ($(findstring Darwin,$(PLATFORM)), Darwin)
      ARFLAGS    :=-shared -Wl,-install_name,$(BINARY).1 -lz
   else
      ARFLAGS    :=-shared -Wl,-soname,$(BINARY).1 -lz
   endif
endif

ifeq ($(MAKECMDGOALS),dll)
   BINARY     :=libblosc2.so
endif

dll:   CPPOPT     +=$(DLLFLAG)
dll:   AR         :=$(CC)
dll:   ARFLAGS    ?=-shared -Wl,-soname,$(BINARY).1
dll:   LINKOPT    :=$(LDFLAGS)
dll:   AROUTPUT   :=-o

all: lib

TARGETSUFFIX:=$(suffix $(BINARY))

doc: makedocdir
	$(DOXY) $(DOXYCFG)

OBJS      := $(addsuffix $(OBJSUFFIX), $(FILES))

all dll lib mex oct: $(OUTPUT_DIR)/$(BINARY)

makedirs:
	@if test ! -d $(OUTPUT_DIR); then $(MKDIR) $(OUTPUT_DIR); fi

makedocdir:
	@if test ! -d $(DOCDIR); then $(MKDIR) $(DOCDIR); fi

$(OUTPUT_DIR)/$(BINARY): makedirs $(OBJS)
$(OUTPUT_DIR)/$(BINARY): $(OBJS)
	@$(ECHO) Building $@
	$(AR) $(ARFLAGS) $(OUTPUTFLAG) $@ $(OBJS) $(LINKOPT)

%$(OBJSUFFIX): %.cpp
	$(CXX) $(INCLUDEDIRS) $(CPPOPT) -c -o $@ $<

%$(OBJSUFFIX): %.c
	@$(ECHO) Building $@
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDEDIRS) $(CPPOPT) -c -o $@ $<

%$(OBJSUFFIX): %.cu
	@$(ECHO) Building $@
	$(CUDACC) -c $(CUCCOPT) -o $@  $<

clean:
	-rm -f $(OBJS) $(OUTPUT_DIR)/$(BINARY)$(EXESUFFIX) zmat$(OBJSUFFIX) $(LIBDIR)/*

.PHONY: all lib dll

.DEFAULT_GOAL := all

