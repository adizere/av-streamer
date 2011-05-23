INCLUDEDIRS:=-I/usr/local/include
CFLAGS:=$(CFLAGS) -D__AvsServer__ -D__AvsClient__
AVLIBS:=-L/usr/local/lib -L/usr/lib -lpthread -lSDL -lavformat -lavcodec -lavutil -lswscale

all: clhandler server common

fifo: ../common/fifo.h ../common/fifo.cpp
	g++ -c -o bin/fifo.o ../common/fifo.cpp

rtp: ../common/rtp.h ../common/rtp.cpp
	g++ -c -o bin/rtp.o ../common/rtp.cpp

audiovideo: ../common/audiovideo.h ../common/audiovideo.cpp
	g++ -g $(CFLAGS) $(INCLUDEDIRS) -c -o bin/audiovideo.o ../common/audiovideo.cpp

common: fifo rtp audiovideo ../common/common.h

clhandler: clhandler.h clhandler.cpp
	g++ $(CFLAGS) $(AVLIBS) -c -o bin/clhandler.o clhandler.cpp

main: globals.h main.cpp
	g++ $(CFLAGS) $(AVLIBS) -c -o bin/main.o main.cpp

server: main clhandler common rtp fifo audiovideo
	g++ -g $(INCLUDEDIRS) $(AVLIBS) bin/main.o bin/clhandler.o bin/rtp.o bin/fifo.o bin/audiovideo.o -o bin/server
