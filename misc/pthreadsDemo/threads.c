/* Example to demonstrate the working of pthreads:
   a. Shows how to pass a structure to the thread as its input
   b. Shows how to handle concurrent acccess to shared variables using pthrea locks
   c. Shows how to pass information back to the caller without a return statement 
      but by changing contents of the passed struct.
*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
 
#define NUM_THREADS 2

static volatile int count=0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
 
/* create thread argument struct for thr_func() */
typedef struct _thread_data_t {
  int tid;
  int stuff;
} thread_data_t;
 
/* thread function Ex1: thread_data_t Input; no output*/
void *thr_func(void *arg) {
  thread_data_t *data = (thread_data_t *)arg;
  int i;
  
  printf("hello from thr_func, thread id: %d\n", data->tid);
  for (i = 0; i < 1e7; i++){
       // uncomment lock/unlock to see race condition
       pthread_mutex_lock(&lock); // wrap CS in lock ...
         count++; // Critical section involving read-modify-write to the shared variable
       pthread_mutex_unlock(&lock); // ... unlock
  }
  data->stuff = i;
  pthread_exit(NULL);
  // No return
}

 
int main(int argc, char **argv) {
  pthread_t thr[NUM_THREADS];
  int i, rc;
  /* create a thread_data_t argument array */
  thread_data_t thr_data[NUM_THREADS];
 
  /* create threads */
  for (i = 0; i < NUM_THREADS; ++i) {
    thr_data[i].tid = i;
    if ((rc = pthread_create(&thr[i], NULL, thr_func, &thr_data[i]))) {
      fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
      return EXIT_FAILURE;
    }
  }
  /* block until all threads complete */
  for (i = 0; i < NUM_THREADS; ++i) {
    pthread_join(thr[i], NULL);
    printf("Thread %d stuff = %d\n", thr_data[i].tid,thr_data[i].stuff);
  }

  printf("Count is %d\n", count);
  return EXIT_SUCCESS;
}
