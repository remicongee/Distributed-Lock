# Makefile

CC = g++

FLAG = -lpthread -std=c++11

CDEMO = Demo/ClientDemo.cpp
CSRSC = Source/Client.cpp

FDEMO = Demo/FollowerDemo.cpp
FSRSC = Source/Follower.cpp

LDEMO = Demo/LeaderDemo.cpp
LSRSC = Source/Leader.cpp

OBJS = Client.o Follower.o Leader.o

all: $(OBJS)
	@echo "Make Completed."
	chmod a+x run.sh

Client.o: 
	$(CC) $(CDEMO) $(CSRSC) $(FLAG) -o Client.o

Follower.o: 
	$(CC) $(FDEMO) $(FSRSC) $(FLAG) -o Follower.o

Leader.o: 
	$(CC) $(LDEMO) $(LSRSC) $(FLAG) -o Leader.o

.PHONY: clean
clean:
	@-rm -rf *.o
	
