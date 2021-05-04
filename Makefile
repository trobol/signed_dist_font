TARGET = prog
LIBS = -lm -lfreetype -lm
CC = gcc
CFLAGS = -g -Wall -I/usr/include/freetype2 -I/usr/include/libpng16 

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@ 

clean:
	-rm -f *.o
	-rm -f $(TARGET)

	