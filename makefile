CC = gcc
FLAGS = -Wall -g -fPIC

all: Sender.o Receiver.o Sender Receiver

Receiver: Receiver.o
	$(CC) $(FLAGS) -o Receiver Receiver.o
Sender: Sender.o
	$(CC) $(FLAGS) -o Sender Sender.o
Sender.o: Sender.c
	$(CC) $(FLAGS) -c Sender.c
Receiver.o: Receiver.c
	$(CC) $(FLAGS) -c Receiver.c

.PHONY:clean all

clean:
	rm -f *.o Sender Receiver