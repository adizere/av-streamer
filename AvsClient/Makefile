CFLAGS:=$(CFLAGS) -D__AvsServer__ -D__AvsClient__

all: client common

fifo: ../common/fifo.h ../common/fifo.cpp
	g++ $(CFLAGS) -c -o bin/fifo.o ../common/fifo.cpp

rtp: ../common/rtp.h ../common/rtp.cpp
	g++ -c -o bin/rtp.o ../common/rtp.cpp

common: fifo rtp ../common/common.h

main: globals.h main.cpp
	g++ -c -o bin/main.o main.cpp

client: main common rtp fifo
	g++ -o bin/client -lpthread bin/main.o bin/rtp.o bin/fifo.o
