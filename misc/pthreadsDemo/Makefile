all: threads passing
threads: threads.c
	gcc -o _threads threads.c -lpthread
passing: passing.c
	gcc -o _passing passing.c -lpthread
clean:
	rm _passing _threads
