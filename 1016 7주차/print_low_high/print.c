#include <stdio.h>
#include <string.h>
int main(void)
{
    char number[20];
    int length, i;
    printf("금액을 입력하고 Enter>");
    scanf("%s", number);
    length = strlen(number);
    for (i = 0; i < length; i++)
        printf("%c\n", number[i]);
    printf("\nEnter를 누르면 종료합니다...");
    while (getchar() != '\n')
        ;      // scanf가 남긴 '\n' 비우기
    getchar(); // 실제로 Enter 대기
    return 0;
    return 0;
}
