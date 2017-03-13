all: atdate

CFLAGS=-include /usr/include/errno.h

atdate: atdate.o
	ccache gcc -o atdate atdate.o

atdate.o: atdate.c
	ccache gcc -c atdate.c


