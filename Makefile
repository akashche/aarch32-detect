
all: clean compile run

clean:
	rm -f detect.o
	rm -f detect
	rm -f sigtest.o
	rm -f sigtest

compile:
	#gcc -g -O0 detect.c -o detect
	gcc -g -O0 sigtest_asm.s sigtest.c -o sigtest

run:
	./sigtest ; echo $$?

