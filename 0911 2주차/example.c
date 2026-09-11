#include <stdio.h>
int main()
{
    char string[20];
    char c;
    scanf("%s", string);
    scanf("%c", &c);
    printf("%s\n", string);
    printf("!!%c!!\n", c);
    
    return 0;
}
