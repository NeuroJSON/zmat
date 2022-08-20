all:
	$(CC) -g -Wall -pedantic testzmat.c -o testzmat -DNO_LZMA -DNO_LZ4 -I../../include -L../../lib -lzmat -lz
clean:
	-rm -f testzmat
