CFLAGS:=-c -Wall -Weffc++ -g -std=c++11 -Iinclude
LDFLAGS:=-lboost_system -lpthread
# TODO change the makefile for StompEMIClient
all: StompEMIClient

StompEMIClient: bin/ConnectionHandler.o bin/StompClient.o bin/event.o bin/StompProtocol.o
	g++ -o bin/StompEMIClient bin/ConnectionHandler.o bin/StompClient.o bin/event.o bin/StompProtocol.o $(LDFLAGS)
	
bin/ConnectionHandler.o: src/ConnectionHandler.cpp
	g++ $(CFLAGS) -o bin/ConnectionHandler.o src/ConnectionHandler.cpp

bin/event.o: src/event.cpp
	g++ $(CFLAGS) -o bin/event.o src/event.cpp

bin/StompClient.o: src/StompClient.cpp
	g++ $(CFLAGS) -o bin/StompClient.o src/StompClient.cpp

bin/StompProtocol.o: src/StompProtocol.cpp
	g++ $(CFLAGS) -o bin/StompProtocol.o src/StompProtocol.cpp

.PHONY: clean
clean:
	rm -f bin/*
	