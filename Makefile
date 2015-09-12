CFLAGS = -Wall -Wextra -Werror -pedantic -std=c++11

all: compiler.o tokenizer.o
	g++ $(CFLAGS) -o compiler bin/compiler.o bin/tokenizer.o

compiler.o: src/compiler.cpp
	g++ $(CFLAGS) -o bin/compiler.o -c src/compiler.cpp

tokenizer.o: src/tokenizer.h src/tokenizer.cpp
	g++ $(CFLAGS) -o bin/tokenizer.o -c src/tokenizer.cpp

clean:
	rm -f compiler bin/*.o src/*~ *~
