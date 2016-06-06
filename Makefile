
all: clean compile run

clean:
	rm -f detect.o
	rm -f detect

compile:
	gcc -g -O0 detect.c -o detect

run:
	./detect ; echo $$?

