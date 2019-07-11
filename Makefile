CC=gcc
CFLAGS=-c -Wall -O2
LDFLAGS=
SOURCES=main.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=sgrotate

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@
