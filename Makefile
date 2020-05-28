CC=gcc
CFLAGS=-I

all: perf_arr

perf_arr: perf_arr.c
	gcc -o pa perf_arr.c
	
