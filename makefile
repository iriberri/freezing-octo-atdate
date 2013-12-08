all: atdate

CFLAGS=-include /usr/include/errno.h

atdate: atdate.o
	gcc -o atdate atdate.o

atdate.o: atdate.c
	gcc -c atdate.c

