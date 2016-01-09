/* Written November 19, 2010: */

/* A solution to Project Euler's problem 74 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX 1000000

int main(int argc, char* argv[])
{
       int chain_count = 0;
       int count;
       int digit_factorials[] = {1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880};
       int* met_sfd = NULL;
       int sfd;
       int x = 0;
       int y;

       met_sfd = malloc(10000000);
       if(!met_sfd)
       {
               printf("Could not allocate memory\n");
               return 0;
       }
       memset(met_sfd, 0, 10000000);

       while(x++<MAX)
       {
               y = x;
               count = 1;

               while(1)
               {
                       /* Sum factorials of digits */

                       sfd = 0;
                       while(y)
                       {
                               sfd += digit_factorials[y%10];
                               y/=10;
                       }

                       if(met_sfd[sfd]==x)
                               break;

                       count++;
                       met_sfd[sfd] = x;
                       y = sfd;
               }

               if(count==60)
                       chain_count++;
       }

       printf("%d chains below %d have sixty terms\n", chain_count, MAX);

       return 0;
}
