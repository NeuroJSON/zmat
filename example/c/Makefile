LIBTYPE?=-static

all:
	$(CC) -g -Wall -pedantic testzmat.c -o testzmat -I../../include -L../../lib $(LIBTYPE) -lzmat -lz -lpthread
clean:
	-rm -f testzmat
