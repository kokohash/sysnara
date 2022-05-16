all : mexec

mexec : mexec.o
	gcc -g -std=gnu11 -Wall -o mexec mexec.o

mexec.o : mexec.c
	gcc -g -std=gnu11 -Wall   -c -o mexec.o mexec.c