/*
 * Project Euler, Problem 76
 *
 * This is based on the solution I wrote in C
 * on 16 January 2011.
 */

main()
{
       printf("%d\n", part(100, 0));
       return 0;
}

part(x, y)
{
       int a = x;
       int b = 0;
       int c = 0;

       while(--a && ++b)
               if(a>=b && a>=y && b>=y)
                       c += part(a, b) + 1;
       return c;
}
