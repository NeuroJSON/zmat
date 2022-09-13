############################################################
#  blosc2 makefile
############################################################

BLOSC_SRC=blosc
ZSTD_SRC=internal-complibs/zstd-1.5.2
LIBBLOSC2=$(BLOSC_SRC)/libblosc2.a


all: $(LIBBLOSC2) zstd

$(LIBBLOSC2):
	-$(MAKE) -C $(BLOSC_SRC) lib
zstd:
	-$(MAKE) -C $(ZSTD_SRC) libzstd.a
clean:
	-$(MAKE) -C $(BLOSC_SRC) clean
	-$(MAKE) -C $(ZSTD_SRC) clean

.DEFAULT_GOAL := all

