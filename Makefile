
all: clean compile

clean:
	rm -f detect

compile:
	gcc probes.s detect.c -o detect

run:
	./detect ; echo $$?

