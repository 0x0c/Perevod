CC = clang++
CFLAGS = -g -I/opt/local/include -std=c++11 `pkg-config --cflags opencv` `pkg-config --libs opencv`
LDLIBS = /opt/local/lib/libboost_system-mt.dylib

all:
	$(CC) main.cpp $(LDLIBS) $(CFLAGS) -o Perevod

c:
	./Perevod -s 127.0.0.1 31400 31401
s:
	./Perevod -c 127.0.0.1 31401 31400

clean:
	rm ./Perevod ./Perevod.dSYM