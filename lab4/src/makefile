CC=gcc
CFLAGS=-I.

all : parallel_min_max process_memory.o parallel_sum

parallel_min_max : utils.o find_min_max.o ../../lab3/src/utils.h ../../lab3/src/find_min_max.h
	$(CC) -o parallel_min_max utils.o find_min_max.o ../../lab3/src/parallel_min_max.c $(CFLAGS)

utils.o : ../../lab3/src/utils.h
	$(CC) -o utils.o -c ../../lab3/src/utils.c $(CFLAGS)

find_min_max.o : ../../lab3/src/utils.h ../../lab3/src/find_min_max.h
	$(CC) -o find_min_max.o -c ../../lab3/src/find_min_max.c $(CFLAGS)

process_memory.o: 
	$(CC) -o process_memory.o -c process_memory.c $(CFLAGS)

parallel_sum: parallel_sum.c utils.c sum.c
	$(CC) $(CFLAGS) -o parallel_sum parallel_sum.c utils.c sum.c

clean :
	rm utils.o find_min_max.o parallel_min_max process_memory.o parallel_sum

run:
	./parallel_sum --threads_num 4 --seed 123 --array_size 1000000
