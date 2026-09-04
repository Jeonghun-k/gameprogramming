/*
#include <stdio.h>
#include <windows.h> 윈도우 버전

void gotoxy(int x, int y);
int main(void)
{
gotoxy(2,4);
printf("Hello");
gotoxy(40, 20);
printf("Hello");
return 0;
}

// 윈도우 버전
void gotoxy(int x, int y){
  COORD Pos = {x - 1, y - 1};
  SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Pos);
}

//커서 위치 제어
*/

#include <stdio.h>
#include "mylib.h"
/* 
//mylib.h를 받아와서 하기 때문에 이 구문은 없어져도됨

void gotoxy(int x, int y) {
    printf("\033[%d;%dH", y, x);  // 커서를 (y행, x열)로 이동
}
*/

int main(void) {
    printf("\033[2J");  // 화면 지우기
    gotoxy(2, 4);
    printf("Hello");
    gotoxy(40, 20);
    printf("Hello");
    fflush(stdout);
    printf("\n");
    return 0;
}