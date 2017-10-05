/* strncmp example */
#include <stdio.h>
#include <string.h>

int main () {
  char str[][5] = { "<" , ">" , "fg" };
  char * a = "kljnosdcoinonwdc > sdiubweibiuewrbvui";
  
  int n;
  printf ("Looking in \'%s\' ...\n", a);
  for (n=0 ; n<3 ; n++)
    printf( "%s : %d\n", str[n], strncmp(a, str[n], strlen(a)) );

  return 0;
}
