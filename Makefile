CC = clang++
CFLAGS = -g -I/opt/local/include -std=c++11 `pkg-config --cflags opencv` `pkg-config --libs opencv`
LDLIBS = /opt/local/lib/libboost_system-mt.dylib

all:
	$(CC) main.cpp Perevod.cpp $(LDLIBS) $(CFLAGS) -o Perevod

test:
	./Perevod -t -c 127.0.0.1 31401 31400 &
	./Perevod -t -s 127.0.0.1 31400 31401 &

clean:
	rm -r ./Perevod.dSYM ./Perevod
