#ifndef MYLIB_H
#define MYLIB_H

#include <stdio.h>

// 커서를 (x, y) 위치로 이동 (Windows gotoxy 대체)
void gotoxy(int x, int y) {
    printf("\033[%d;%dH", y, x);
}

// 화면 전체 지우기 (Windows system("cls") 대체)
void clear_screen(void) {
    printf("\033[2J\033[H");   // 지우고 + 커서를 맨 위로
}

#endif