LIBTYPE  ?= dynamic

all:
	$(CC) -g -Wall -pedantic testzmat.c -o testzmat -I../../include -L../../lib -Wl,-B$(LIBTYPE) -lzmat -Wl,-Bdynamic -lz -lpthread
clean:
	-rm -f testzmat
