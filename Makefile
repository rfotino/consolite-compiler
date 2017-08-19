CC = g++
CFLAGS = -Wall -Wextra -Werror -pedantic -std=c++11

EXEC = compiler
SOURCES = $(wildcard src/*.cpp)
OBJECTS = $(SOURCES:src/%.cpp=bin/%.o)

all: $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXEC)

bin/%.o: src/%.cpp
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(EXEC) $(OBJECTS)
