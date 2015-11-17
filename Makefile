CC = clang++
CFLAGS = -g -Ilib/ -Llib/ -I/opt/local/include -std=c++11 `pkg-config --cflags opencv` `pkg-config --libs opencv`
LDLIBS = /opt/local/lib/libboost_system-mt.dylib
PEREVOD_FILE = lib/Perevod.cpp lib/core/ImageSocket.cpp lib/impl/ImageSocketImpl.cpp lib/impl/tcp/ImageSocketImplTCP.cpp lib/util/Queue.cpp

all:
	$(CC) main.cpp $(PEREVOD_FILE) $(LDLIBS) $(CFLAGS) -o Perevod

test:
	./Perevod -t -c 127.0.0.1 31401 31400 &
	./Perevod -t -s 127.0.0.1 31400 31401 &

clean:
	rm -r ./Perevod.dSYM ./Perevod
