/* pintos list demo
   Author: Ramesh Yerraballi
   This is a demonstration of the list data structure api provided in pintos
   I took the list.h, list.c out of the kernel so you can just use these
   files: (list_use.c list.h and list.c) to try out and learn how it works
 */
#include <stdio.h>
#include "list.h"

/* This is the element type you are interested in making a list of */ 
struct foo {   
     struct list_elem elem;   
     int bar; //could be a key for ordering
     int beer; // other important information
};

/* We pass this as a function pointer to routines in the list api that 
   work with ordering */
bool compare(struct list_elem *a, struct list_elem *b, void *aux) {

     if (list_entry(a, struct foo, elem)->bar < list_entry(b, struct foo, elem)->bar)
	  return true;
     else
	  return false;
}

int main(int argc,char **argv) {
     struct list foo_list;
     struct list_elem *e;
     struct foo a1,a2,a3;
     
     
     list_init (&foo_list);
     a1.bar=1;a1.beer=42;
     a2.bar=2;a2.beer=24;
     a3.bar=0;a3.beer=10;
     list_push_front(&foo_list, &(a1.elem));
         /* <-h<->1:42<->t-> */
     list_push_back(&foo_list, &(a3.elem));
        /* <-h<->1:42 <-> 0:10<->t-> */
     /* put in middle: a little complex. get the head, find its next
        which is 1:42, whose next is 0:10 and insert BEFORE it */
     list_insert(list_next(list_next(list_head(&foo_list))), &(a2.elem));
       /* <-h<->1:42 <-> 2:24 <-> 0:10<->t-> */
     /* You could also do this to get the same effect: 
     list_insert(list_prev(list_tail(&foo_list)), &(a2.elem)); 
       /* ->1:42 <-> 2:24 <-> 0:10-> */
     
     /* Following iteration-loop will print them out */
     printf("List Contents:\n");
     for (e = list_begin (&foo_list); e != list_end (&foo_list);
	  e = list_next (e))
     {
	  struct foo *f = list_entry (e, struct foo, elem);
	  printf("%d:%d\n",f->bar,f->beer);
	  
     }
     list_sort (&foo_list, &compare, NULL);
     printf("List Contents after sort by bar:\n");
     for (e = list_begin (&foo_list); e != list_end (&foo_list);
	  e = list_next (e))
     {
	  struct foo *f = list_entry (e, struct foo, elem);
	  printf("%d:%d\n",f->bar,f->beer);
	  
     }
}
