all : mmake

mmake : mmake.o parser.o
	gcc -g -std=gnu11 -Wall -o mmake parser.o mmake.o

mmake.o : mmake.c parser.h
	gcc -g -std=gnu11 -Wall -c mmake.c

parser.o : parser.c parser.h
	gcc -g -std=gnu11 -Wall -c parser.c

clean : 
	rm -rf *.o 