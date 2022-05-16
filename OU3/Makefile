all : mdu

mdu : mdu.o queue.o list.o
	gcc -pthread -o mdu mdu.o queue.o list.o

mdu.o : mdu.c
	gcc -g -std=gnu11 -Werror -Wall -Wextra -Wpedantic -Wmissing-declarations -Wmissing-prototypes -Wold-style-definition -c mdu.c

queue.o : queue.c
	gcc -g -std=gnu11 -Werror -Wall -Wextra -Wpedantic -Wmissing-declarations -Wmissing-prototypes -Wold-style-definition -c queue.c queue.h util.h

list.o : list.c
	gcc -g -std=gnu11 -Werror -Wall -c list.c list.h util.h

clean : 
	rm -rf *.o mdu