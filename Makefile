CFLAGS = -Wall -Wextra -Werror -pedantic -std=c++11

all: compiler.o tokenizer.o parser.o syntax.o util.o
	g++ $(CFLAGS) -o compiler bin/compiler.o bin/tokenizer.o \
bin/parser.o bin/syntax.o bin/util.o

compiler.o: src/compiler.cpp
	g++ $(CFLAGS) -o bin/compiler.o -c src/compiler.cpp

tokenizer.o: src/tokenizer.h src/tokenizer.cpp
	g++ $(CFLAGS) -o bin/tokenizer.o -c src/tokenizer.cpp

parser.o: src/parser.h src/parser.cpp
	g++ $(CFLAGS) -o bin/parser.o -c src/parser.cpp

syntax.o: src/syntax.h src/syntax.cpp
	g++ $(CFLAGS) -o bin/syntax.o -c src/syntax.cpp

util.o: src/util.h src/util.cpp
	g++ $(CFLAGS) -o bin/util.o -c src/util.cpp

clean:
	rm -f compiler bin/*.o src/*~ *~
