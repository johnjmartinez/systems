/* pintos hash demo
   Author: Ramesh Yerraballi
   This is a demonstration of the hash data structure api provided in pintos
   I took the hash.h, hash.c out of the kernel so you can just use these
   files: (hash_use.c hash.h and hash.c) to try out and learn how it works
 */
#include <stdio.h>
#include <stdlib.h>
#include "hash.h"

/* This is the element type you are interested in making a hash of */ 
struct foo {   
    struct hash_elem hash_elem;   
    int key; /* the key for ordering, can use any "comparable" type 
		a pointer for example (virtual addr of start of a page
		which can be the thought of as the page number left 
		shifted by 12 bits) 
	     */

    int value; // The payload - could be a struct
};

/* We pass this as a function pointer to routines in the hash api that 
   work with ordering */
/* Returns a hash value for page p. */
unsigned
page_hash (const struct hash_elem *p_, void *aux)
{
  const struct foo *p = hash_entry (p_, struct foo, hash_elem);
  return hash_bytes (&p->key, sizeof(p->key));
}

/* Returns true if foo a precedes foo b. */
bool
page_less (const struct hash_elem *a_, const struct hash_elem *b_,
           void *aux)
{
  const struct foo *a = hash_entry (a_, struct foo, hash_elem);
  const struct foo *b = hash_entry (b_, struct foo, hash_elem);

  return a->key < b->key;
}


int main(int argc,char **argv) {
     struct hash footbl;
     struct hash_elem *e;
     struct foo *a1,*a2, scratch, *deleted;
     
     
     hash_init (&footbl, page_hash, page_less, NULL);
     a1 = malloc(sizeof(struct foo));a2 = malloc(sizeof(struct foo));     
     a1->key=1;a1->value=42;
     a2->key=12;a2->value=116;
     
     hash_insert(&footbl, &a1->hash_elem);     
     hash_insert(&footbl, &a2->hash_elem);

     /* Lets search for the item whose key is 12 */
     scratch.key = 12;
     
     e = hash_find(&footbl, &scratch.hash_elem);
     if (e != NULL){
	 struct foo *result = hash_entry(e, struct foo, hash_elem);
	 printf("Value for key(%d) is %d\n",result->key, result->value);
     }
     
     /* Following iteration-loop will print them out in some order*/
     struct hash_iterator i;
     hash_first (&i, &footbl);
     while (hash_next (&i)){
	 struct foo *f = hash_entry (hash_cur (&i), struct foo, hash_elem);
	 printf("key(%d): Value(%d)\n",f->key, f->value);
     }
     
     /* Delete the foo with key 1 */
     scratch.key = 1; /* Use a foo to search with */     
     e = hash_delete(&footbl, &scratch.hash_elem); /* deletes from hash table and returns it */
     deleted = hash_entry (e, struct foo, hash_elem);
     printf("Deleted - key(%d): Value(%d)\n",deleted->key, deleted->value);     
     
}
