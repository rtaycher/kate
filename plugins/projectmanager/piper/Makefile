
all: testor

test:  all
	./testor

clean:
	rm *.o testor core

testor: piper.o test.o
	g++ piper.o test.o -o testor -lkdecore

piper.o: piper.cpp piper.h Makefile
	g++ -c piper.cpp -g -o piper.o

test.o: test.cpp piper.h Makefile
	g++ -c test.cpp -g -o test.o
