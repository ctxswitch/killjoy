CC=gcc

all: killjoy.o
	$(CC) -o killjoy killjoy.o

killjoy.o: killjoy.c 
	$(CC) -c killjoy.c -o killjoy.o

clean: 
	rm -f *.o killjoy