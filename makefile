CC = gcc
FLAGS = -Wall -g -fPIC

all: Sender.o Receiver.o testS testR

testR: Receiver.o
	$(CC) $(FLAGS) -o testR Receiver.o
testS: Sender.o
	$(CC) $(FLAGS) -o testS Sender.o
Sender.o: Sender.c
	$(CC) $(FLAGS) -c Sender.c
Receiver.o: Receiver.c
	$(CC) $(FLAGS) -c Receiver.c

.PHONY:clean all

clean:
	rm -f *.o testS testR