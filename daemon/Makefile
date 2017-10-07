CC=gcc
CFLAGS=-c -Wall -g -std=gnu99
LDFLAGS=-lpthread
EXECUTABLE=yashd 
SOURCES=daemon.c parser.c tokenizer.c executor.c jobatron.c
OBJECTS=$(SOURCES:.c=.o)

#all: $(SOURCES) $(EXECUTABLE) yash
all : yash

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@
    
yash: client.o
	$(CC) -g -o yash client.c 
        
clean:
	rm *.o yash
