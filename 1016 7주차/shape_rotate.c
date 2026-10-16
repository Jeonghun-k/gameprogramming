#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>

void rotation_right(int m[][3]);
void move_shape(int m[][3]);
void print_shape(int m[][3]);
void move_control(int m[][3]);
void gotoxy(int x, int y);
void print_direction(void);
void erase_shape(void);
void hide_cursor(void); // [수정] 커서 숨기기 함수 선언

int x = 35, y = 12;
int inx = 0, iny = 0;

int main(void)
{
    int shape1[3][3] = {0, 1, 0,
                        0, 1, 0,
                        1, 1, 1};
    hide_cursor(); // [수정] 시작할 때 커서 숨기기
    move_control(shape1);
    return 0;
}

void rotation_right(int m[][3])
{
    int i, j;
    int temp[3][3];
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            temp[j][2 - i] = m[i][j];
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            m[i][j] = temp[i][j];
}

void print_direction(void)
{
    gotoxy(25, 1);
    printf("화살표:이동,  스페이스 키:회전");
}

void move_control(int m[][3])
{
    int key; // [수정] getch()의 반환형(int)에 맞춰 224를 그대로 비교
    system("cls");
    do
    {
        while (!kbhit())
        {
            move_shape(m);
        }
        key = getch();
        if (key == 0 || key == 224)        // [수정] 방향키: 접두 값 다음의 실제 코드 읽기
            key = getch();
        else if (key != 32 && key != 27)   // [수정] H, K, M, P 등 다른 키는 무시
            continue;
        switch (key)
        {
        case 32:
            rotation_right(m);
            move_shape(m);
            break;
        case 72:
            inx = 0; iny = -1;
            move_shape(m);
            break;
        case 75:
            inx = -1; iny = 0;
            move_shape(m);
            break;
        case 77:
            inx = 1; iny = 0;
            move_shape(m);
            break;
        case 80:
            inx = 0; iny = 1;
            move_shape(m);
            break;
        default:
            break;
        }
    } while (key != 27);
    printf("\n");
}

void print_shape(int m[][3])
{
    int i, j;
    for (i = 0; i < 3; i++)
    {
        gotoxy(x, y + i);
        for (j = 0; j < 3; j++)
            if (m[i][j] == 1)
                printf("□");
            else
                printf(" "); // 원본은 공백 2칸, 현재 터미널(1칸 기준)에 맞춤
        printf("\n");
    }
}

void move_shape(int m[][3])
{
    do
    {
        print_shape(m);
        print_direction();
        Sleep(100);
        erase_shape(); // 이전 위치만 지우기
        x = x + inx;
        y = y + iny;
        if (y > 23)
            y = 23;
        else if (y < 2)
            y = 2;
        if (x > 75)
            x = 75;
        else if (x < 1)
            x = 1;
    } while (!kbhit());
}

void erase_shape(void)
{
    int i;
    for (i = 0; i < 3; i++)
    {
        gotoxy(x, y + i);
        printf("   "); // 3칸 (1칸 기준 터미널)
    }
}

void gotoxy(int px, int py) // [수정] 전역 x, y와 이름이 겹치지 않도록 변경
{
    COORD Pos = {px - 1, py - 1};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Pos);
}

void hide_cursor(void) // [수정] 커서 숨기기
{
    CONSOLE_CURSOR_INFO ci = {1, FALSE};
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
}