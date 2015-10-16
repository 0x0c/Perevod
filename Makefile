CC = clang++
CFLAGS = -I/opt/local/include -std=c++11 `pkg-config --cflags opencv` `pkg-config --libs opencv`
LDLIBS = /opt/local/lib/libboost_system-mt.dylib

all:
	$(CC) main.cpp $(LDLIBS) $(CFLAGS)

run_s:
	./a.out -s 127.0.0.1 31400 31401
run_c:
	./a.out -c 127.0.0.1 31401 31400

clean:
	rm ./a.out%
