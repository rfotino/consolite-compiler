CFLAGS = -Wall -Wextra -Werror -std=c++11

all: compiler.o
	g++ $(CFLAGS) -o compiler bin/compiler.o

compiler.o: src/compiler.cpp
	g++ $(CFLAGS) -o bin/compiler.o -c src/compiler.cpp

clean:
	rm -f compiler bin/*.o src/*~ *~
