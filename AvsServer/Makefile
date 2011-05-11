all: clhandler server common

fifo: ../common/fifo.h ../common/fifo.cpp
	g++ -c -o bin/fifo.o ../common/fifo.cpp

rtp: ../common/rtp.h ../common/rtp.cpp
	g++ -c -o bin/rtp.o ../common/rtp.cpp

common: fifo rtp ../common/common.h

clhandler: clhandler.h clhandler.cpp
	g++ -c -o bin/clhandler.o clhandler.cpp

main: globals.h main.cpp
	g++ -c -o bin/main.o main.cpp

server: main clhandler common rtp fifo
	g++ -o bin/server -lpthread bin/main.o bin/clhandler.o bin/rtp.o bin/fifo.o
