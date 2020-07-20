test:
	as -o test.o -g test.s
	#ld -dynamic-linker /lib/ld-linux.so.2 -lc -o test test.o
	ld -o test test.o
clean:
	rm -rf test *.o
all: test

