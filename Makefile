TARGET = postel
CC = gcc
LIBS = -luv $(shell pkg-config --libs glib-2.0 gtk+-3.0 goocanvas-2.0)
CFLAGS = $(shell pkg-config --cflags glib-2.0 gtk+-3.0 goocanvas-2.0)

.PHONY: clean all default

default: $(TARGET)
all: default

OBJECTS = $(patsubst src/%.c, src/%.o, $(wildcard src/*.c))
HEADERS = $(wildcard src/*.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f src/*.o
	-rm -f $(TARGET)
