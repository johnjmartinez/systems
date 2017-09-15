CC=gcc
CFLAGS=-c -Wall -g -std=gnu99
LDFLAGS=
EXECUTABLE=yash
SOURCES=looper.c parser.c tokenizer.c executor.c signatron.c
OBJECTS=$(SOURCES:.c=.o)

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@
    
clean:
	rm *.o yash
