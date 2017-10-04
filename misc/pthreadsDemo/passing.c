/* Demonstrates the simplest passing and returning information 
   to and from a thread.
 */
#include <stdio.h>
#include <pthread.h>

void *mythread(void *arg) {
     int m = (int) arg; // cast the passed void* back to int  
     printf("%d\n", m);
     return (void *) (arg + 1);// return an int but cast it to void** because that is the interface
}
int argin = 100;
int main(int argc, char *argv[]) {
     pthread_t p;
     int rc, m;
     
    
     pthread_create(&p, NULL, mythread, (void *) argin); // passing an int but create expects void* so cast it
     pthread_join(p, (void **) &m); // join expects a pointer to the returned value you expect to get. We are expecting a void*
                                    // so the actual data_type that join needs is a void**. We pass the address of m where the
                                    // the returned void* (which is actually an int) is placed.
     printf("returned %d\n", m);
     return 0;
}
