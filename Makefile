CC = gcc
CFLAGS = -g3 -std=c99 -pedantic -Wall -lm

all : Blotto Unit clean

Blotto : blotto.c entry.o smap.o 
	${CC} ${CFLAGS} -lm -o $@ $^

Unit : smap.o #smap_test_functions.o #smap_unit.o
	${CC} ${CFLAGS} -lm -o $@ $^

# snap_unit.o : smap_unit.c smap_test_functions.h smap.h
# 	${CC} ${CFLAGS} -c  smap_unit.c

# smap_test_functions.o : smap_test_functions.c smap_test_functions.h
# 	${CC} ${CFLAGS} -c  smap_test_functions.c

	
entry.o : entry.c entry.h
	${CC} ${CFLAGS} -c  entry.c

smap.o : smap.c smap.h
	${CC} ${CFLAGS} -c  smap.c



clean:
	rm -f *.o *.h.gch vgcore.*