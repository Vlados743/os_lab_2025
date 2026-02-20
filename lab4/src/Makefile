all: parallel_min_max process_memory parallel_sum

parallel_min_max: parallel_min_max.o find_min_max.o utils.o
	gcc -o parallel_min_max parallel_min_max.o find_min_max.o utils.o

parallel_min_max.o: parallel_min_max.c
	gcc -c parallel_min_max.c

find_min_max.o: find_min_max.c
	gcc -c find_min_max.c

utils.o: utils.c
	gcc -c utils.c

process_memory: process_memory.c
	gcc -o process_memory process_memory.c

parallel_sum: parallel_sum.o sum.o utils.o
	gcc -o parallel_sum parallel_sum.o sum.o utils.o -pthread

parallel_sum.o: parallel_sum.c sum.h utils.h
	gcc -c parallel_sum.c

sum.o: sum.c sum.h
	gcc -c sum.c

clean:
	rm -f *.o parallel_min_max process_memory parallel_sum