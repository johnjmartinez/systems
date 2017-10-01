all: threads passing
threads: threads.c
	gcc -o threads threads.c -lpthread
passing: passing.c
	gcc -o passing passing.c -lpthread
clean:
	rm passing threads
