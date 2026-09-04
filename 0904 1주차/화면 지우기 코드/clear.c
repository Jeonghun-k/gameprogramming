#include <stdio.h>
#include <stdlib.h>
#include "mylib.h"

int main(void)
{
char ch;
printf("문자를 입력하고 Enter>");
scanf("%c", &ch);
// system("cls"); 윈도우에서 사용
clear_screen();
printf("입력된 문자 %c\n", ch);
return 0;
}