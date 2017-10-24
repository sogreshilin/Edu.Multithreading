CC = gcc
CFLAGS = -std=c99 -Wall -Werror -pthread
SOURCE_DIR = ../utils
SOURCES = main.c $(SOURCE_DIR)/util.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = a.out

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
