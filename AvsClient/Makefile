INCLUDEDIRS:=-I/usr/local/include
CFLAGS:=$(CFLAGS) -D__AvsClient__
AVLIBS:=-L/usr/local/lib -L/usr/lib -lpthread -lSDL -lavformat -lavcodec -lavutil -lswscale

all: client common

fifo: ../common/fifo.h ../common/fifo.cpp
	g++ -g $(CFLAGS) -c -o bin/fifo.o ../common/fifo.cpp

rtp: ../common/rtp.h ../common/rtp.cpp
	g++ -g -c -o bin/rtp.o ../common/rtp.cpp

audiovideo: ../common/audiovideo.h ../common/audiovideo.cpp
	g++ -g $(CFLAGS) $(INCLUDEDIRS) -c -o bin/audiovideo.o ../common/audiovideo.cpp

common: fifo rtp audiovideo ../common/common.h

main: globals.h main.cpp
	g++ -g $(CFLAGS) $(AVLIBS) -c -o bin/main.o main.cpp

client: main common rtp fifo audiovideo
	g++ -g $(INCLUDEDIRS) $(AVLIBS) -o bin/client -lpthread bin/main.o bin/rtp.o bin/fifo.o bin/audiovideo.o
