#include <stdlib.h>
#include <stdio.h>

int main()
{
    int c = 0b10000000;
    printf("%d\n", c);
    c = c >> 1;
    printf("%d\n", c);
    return 0;
    int max = 256;
    int a = 0;
    a = (a + 1) % max;

    printf("%d\n", a);

    a = (a + 1) % max;
    
    printf("%d\n", a);
    a = (a + 1) % max;
    
    printf("%d\n", a);
    a = (a + 1) % max;
    
    printf("%d\n", a);
    a = (a + 1) % max;
    
    printf("%d\n", a);

    a = (a + (344)) % max;
    printf("%d\n", a);

    return 0;
}