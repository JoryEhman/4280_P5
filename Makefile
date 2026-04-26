CC = g++
CFLAGS = -g -Wall -std=c++11
OBJS = main.o parser.o scanner.o staticSem.o codeGen.o
TARGET = P5

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

main.o: main.cpp
	$(CC) $(CFLAGS) -c main.cpp

parser.o: parser.cpp
	$(CC) $(CFLAGS) -c parser.cpp

scanner.o: scanner.cpp
	$(CC) $(CFLAGS) -c scanner.cpp

staticSem.o: staticSem.cpp
	$(CC) $(CFLAGS) -c staticSem.cpp

codeGen.o: codeGen.cpp
	$(CC) $(CFLAGS) -c codeGen.cpp

clean:
	rm -f $(OBJS) $(TARGET)
