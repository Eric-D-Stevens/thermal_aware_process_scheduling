CC=gcc
CFLAGS=-I

all: perf_arr single

perf_arr: perf_arr.c
	gcc -o pa perf_arr.c

single:
	gcc -o sa single_cpu.c
	
