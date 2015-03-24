TARGET = postel
LIBS = -lluajit-5.1 -luv \
	$(shell pkg-config --libs glib-2.0 gtk+-3.0 cairo goocanvas-2.0)
CC = gcc
CFLAGS = -g -Wall -Wno-unused-but-set-variable -Wno-unused-variable \
	-I/usr/local/include -I/usr/include/gtk-3.0 -L/usr/local/lib\
	$(shell pkg-config --cflags glib-2.0 gdk-2.0 atk goocanvas-2.0)

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
