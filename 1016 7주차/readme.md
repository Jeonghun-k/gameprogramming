
# 게임 프로그래밍 — 게임확장모듈 정리

> 참고 교재: 명품 C언어 프로젝트 (생능출판, 안기수)

## 목차

1. [숫자의 변환과 표현 (자리 수 분리)](#1-숫자의-변환과-표현-자리-수-분리)
2. [재귀 호출로 자리 수 분리](#2-재귀-호출로-자리-수-분리)
3. [디지털 숫자](#3-디지털-숫자)
4. [수평·수직 슬라이드 바](#4-수평수직-슬라이드-바)
5. [도형의 연속적인 이동과 회전](#5-도형의-연속적인-이동과-회전)
6. [실행 환경 주의사항](#6-실행-환경-주의사항)

---

## 1. 숫자의 변환과 표현 (자리 수 분리)

숫자를 단위별로 분리해서 출력하는 방법은 두 가지다.

| 방법 | 입력 `756896`의 출력 순서 |
|---|---|
| 방법 1: 높은 단위부터 | 7 → 5 → 6 → 8 → 9 → 6 |
| 방법 2: 낮은 단위부터 (역순) | 6 → 9 → 8 → 6 → 5 → 7 |

> 슬라이드 본문에는 `756826`으로 적혀 있지만 표와 예제는 모두 `756896`이다. 오타로 보면 된다.

### 1-1. 문자열로 처리

`scanf("%s", ...)`로 문자 배열에 저장하면 각 자리가 배열 요소 하나씩 들어간다.

| 첨자 | `[0]` | `[1]` | `[2]` | `[3]` | `[4]` | `[5]` | `[6]` |
|---|---|---|---|---|---|---|---|
| 값 | `'7'` | `'5'` | `'6'` | `'8'` | `'9'` | `'6'` | `'\0'` |

**높은 단위부터 출력**

```c
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
    return 0;
}
```

**낮은 단위부터 출력**

```c
#include <stdio.h>
#include <string.h>
int main(void)
{
    char number[20];
    int length, i;
    printf("금액을 입력하고 Enter>");
    scanf("%s", number);
    length = strlen(number);
    for (i = length - 1; i >= 0; i--)   // 강의자료: i = length
        printf("%c\n", number[i]);
    return 0;
}
```

> **주의:** 강의자료는 `i = length`부터 시작한다. `number[length]`는 `'\0'`이라 맨 앞에 빈 줄 하나가 더 출력된다. 마지막 문자의 첨자는 `length - 1`이다.

### 1-2. 정수형으로 처리

정수는 첨자가 없으므로 **나눗셈(`/`)과 나머지(`%`)** 로 자리를 분리한다.

| 방법 1: 높은 단위부터 | 방법 2: 낮은 단위부터 |
|---|---|
| `756896 / 100000 = 7` | `756896 % 10 = 6` |
| `56896 / 10000 = 5` | `75689 % 10 = 9` |
| `6896 / 1000 = 6` | `7568 % 10 = 8` |
| `896 / 100 = 8` | `756 % 10 = 6` |
| `96 / 10 = 9` | `75 % 10 = 5` |
| `6 / 1 = 6` | `7 % 10 = 7` |

- **높은 단위부터**: 자릿수를 `log10()`으로 구하고, 나눌 단위를 `pow(10, n)`으로 만든다.
- **낮은 단위부터**: `% 10`으로 일의 자리를 꺼내고 `/ 10`으로 한 자리씩 줄인다.

```c
#include <stdio.h>
#include <conio.h>
#include <math.h>
void serial_number(long number);
void reverse_number(long number);

int main(void)
{
    long number = 12345698;
    printf("입력 숫자 : %ld\n\n", number);
    printf("높은 단위부터 출력\n");
    serial_number(number);
    printf("\n낮은 단위부터 출력\n");
    reverse_number(number);
    printf("press any key to continue.......");
    getch();
    return 0;
}

void serial_number(long number)
{
    int num;
    int i, length = 0;
    length = (int)(log10(number) + 1); // 최대 자리수 계산
    for (i = length; i >= 1; i--)
    {
        num = number / (long)pow(10, i - 1);
        printf("%d\n", num);           // 강의자료: %ld (num은 int)
        number = number - num * (long)pow(10, i - 1);
    }
    printf("\n");
}

void reverse_number(long number)
{
    while (number > 0)
    {
        printf("%ld\n", number % 10);
        number /= 10;
    }
}
```

> **참고**
> - `num`은 `int`이므로 서식은 `%d`가 맞다. Windows에서는 `int`와 `long`이 둘 다 4바이트라 `%ld`로도 우연히 맞게 나온다.
> - `pow()`는 실수 연산이라 일부 구형 MinGW에서 `(long)pow(10, 2)`가 `99`가 되는 경우가 있었다.
> - `number`가 0 이하이면 `log10`이 정상 값을 주지 않으므로 아무것도 출력되지 않는다.

---

## 2. 재귀 호출로 자리 수 분리

입력이 `123`일 때 각 자리는 이렇게 구한다.

| 방법 1: 높은 단위부터 | 방법 2: 낮은 단위부터 |
|---|---|
| `(123/100) % 10 → 1` | `(123) % 10 → 3` |
| `(123/10) % 10 → 2` | `(123/10) % 10 → 2` |
| `(123) % 10 → 3` | `(123/100) % 10 → 1` |

### 2-1. 높은 단위부터: 호출 **뒤에** 출력

```c
void serial_number(long number)
{
    if (number > 0)
    {
        serial_number(number / 10);
        printf("%ld\n", number % 10);
    }
    else
        return;
}
```

자기 자신을 먼저 호출하므로 `printf`가 **보류**되고, 반환 과정에서 보류된 출력이 역순으로 실행된다.

| 단계 | `n` | 하는 일 |
|---|---|---|
| 호출 1 | 123 | `serial(12)` 호출, `printf` 보류 |
| 호출 2 | 12 | `serial(1)` 호출, `printf` 보류 |
| 호출 3 | 1 | `serial(0)` 호출, `printf` 보류 |
| 호출 4 | 0 | 조건 거짓 → `return` |
| 반환 5 | 1 | `1 % 10` → **1** 출력 |
| 반환 6 | 12 | `12 % 10` → **2** 출력 |
| 반환 7 | 123 | `123 % 10` → **3** 출력 |

### 2-2. 낮은 단위부터: 호출 **전에** 출력

```c
void reverse_number(long number)
{
    printf("%ld\n", number % 10);
    if ((number / 10) > 0)
        reverse_number(number / 10);
    else
        return;
}
```

출력을 먼저 하고 호출하므로 보류되는 작업이 없다. 3 → 2 → 1 순서로 바로 출력된다.

> **꼬리재귀와의 관계:** `reverse_number`는 재귀 호출이 함수의 마지막 연산인 **꼬리재귀**라 반복문으로 바꾸기 쉽다(1-2의 `while` 버전). `serial_number`는 호출 뒤에 `printf`가 남아 있어 꼬리재귀가 아니며, 그 "남은 작업"이 바로 출력 순서를 뒤집는 원리다.

---

## 3. 디지털 숫자

입력한 정수를 5줄짜리 디지털 숫자로 출력한다. 배열 값이 `1`이면 `■`, `0`이면 **두 자리 공백**을 출력한다.

### 3-1. 숫자 데이터

0~9를 각각 `int` 배열 20칸(4열 × 5행)에 저장한다.

```c
int zero[20]  = {1,1,1,1, 1,0,0,1, 1,0,0,1, 1,0,0,1, 1,1,1,1};
int one[20]   = {0,0,1,0, 0,0,1,0, 0,0,1,0, 0,0,1,0, 0,0,1,0};
int two[20]   = {1,1,1,1, 0,0,0,1, 1,1,1,1, 1,0,0,0, 1,1,1,1};
int three[20] = {1,1,1,1, 0,0,0,1, 1,1,1,1, 0,0,0,1, 1,1,1,1};
int four[20]  = {1,0,0,1, 1,0,0,1, 1,1,1,1, 0,0,0,1, 0,0,0,1};
int five[20]  = {1,1,1,1, 1,0,0,0, 1,1,1,1, 0,0,0,1, 1,1,1,1};
int six[20]   = {1,0,0,0, 1,0,0,0, 1,1,1,1, 1,0,0,1, 1,1,1,1};
int seven[20] = {1,1,1,1, 0,0,0,1, 0,0,0,1, 0,0,0,1, 0,0,0,1};
int eight[20] = {1,1,1,1, 1,0,0,1, 1,1,1,1, 1,0,0,1, 1,1,1,1};
int nine[20]  = {1,1,1,1, 1,0,0,1, 1,1,1,1, 0,0,0,1, 0,0,0,1};
```

### 3-2. 줄 단위 출력

화면은 위에서 아래로 한 줄씩만 출력할 수 있다. 그래서 **각 줄마다 모든 자릿수를 차례로** 출력한다.

| 줄 (`line`) | 출력할 배열 요소 |
|---|---|
| 첫 번째 (0) | `[0]` ~ `[3]` |
| 두 번째 (1) | `[4]` ~ `[7]` |
| 세 번째 (2) | `[8]` ~ `[11]` |
| 네 번째 (3) | `[12]` ~ `[15]` |
| 다섯 번째 (4) | `[16]` ~ `[19]` |

공식: `line * 4` ~ `line * 4 + 3`

| 함수 | 역할 |
|---|---|
| `number_check` | 재귀로 높은 자리부터 숫자를 꺼내 해당 배열로 `digit_print` 호출 |
| `digit_print` | 배열의 해당 줄 4칸을 `■` / 공백으로 출력 |

### 3-3. 코드

```c
#include <stdio.h>
void number_check(int k, int i);
void digit_print(int dim[], int line);
/* 3-1의 zero ~ nine 배열 */

int main(void)
{
    int num, line;
    printf("디지털 숫자 출력 프로그램\n");
    printf("1 이상의 정수만 입력합니다. \n\n");
    printf("\n정수 숫자입력 후 Enter> ");
    scanf("%d", &num);
    printf("\n\n");
    for (line = 0; line <= 4; line++)
    {
        number_check(num, line);
        printf("\n");
    }
    return 0;
}

void digit_print(int dim[], int line)
{
    int i;
    for (i = line * 4; i <= line * 4 + 3; i++)
        if (dim[i] == 1)
            printf("■");
        else
            printf("  ");   // 두 자리 공백
    printf("  ");
}

void number_check(int k, int i)
{
    if (k >= 1)
    {
        number_check(k / 10, i);   // 2-1과 같은 원리: 높은 자리부터
        switch (k % 10)
        {
        case 0: digit_print(zero, i);  break;
        case 1: digit_print(one, i);   break;
        case 2: digit_print(two, i);   break;
        case 3: digit_print(three, i); break;
        case 4: digit_print(four, i);  break;
        case 5: digit_print(five, i);  break;
        case 6: digit_print(six, i);   break;
        case 7: digit_print(seven, i); break;
        case 8: digit_print(eight, i); break;
        case 9: digit_print(nine, i);  break;
        }
    }
}
```

> **주의**
> - PDF에서 코드를 복사하면 연속 공백이 하나로 합쳐진다. 공백 출력은 **두 칸**이 원본이다.
> - `main`이 끝나면 콘솔 창이 바로 닫힐 수 있다. `return 0;` 앞에 `getch();`(`conio.h`)를 넣으면 키 입력을 기다린다.

---

## 4. 수평·수직 슬라이드 바

화살표 키로 슬라이드(`■`)를 움직이고 현재 위치를 숫자로 표시한다.

| 구분 | 수평 슬라이드 바 | 수직 슬라이드 바 |
|---|---|---|
| 사용 키 | ← → | ↑ ↓ |
| 막대 그리기 | `draw_rectangle(길이, 1)` | `draw_rectangle(1, 길이)` |

### 4-1. 함수 구성

| 함수 | 역할 |
|---|---|
| `draw_vertical_slide` | 수직 슬라이드 바 표시 |
| `draw_horizontal_slide` | 수평 슬라이드 바 표시 |
| `move_arrow_key` | 화살표 키에 따라 좌표 증감 (범위 제한 포함) |
| `draw_rectangle` | 괘선 문자로 직사각형 그리기 |
| `gotoxy` | 커서 위치 이동 |

### 4-2. 핵심 개념

**괘선 문자 (완성형 CP949)**

`0xA6` 다음 바이트로 모양이 정해진다.

| `b[i]` | 바이트 | 문자 |
|---|---|---|
| `b[1]` | `0xA1` | `─` |
| `b[2]` | `0xA2` | `│` |
| `b[3]` | `0xA3` | `┌` |
| `b[4]` | `0xA4` | `┐` |
| `b[5]` | `0xA5` | `┘` |
| `b[6]` | `0xA6` | `└` |

**화살표 키 코드**

| 키 | 코드 |
|---|---|
| ↑ | 72 |
| ← | 75 |
| → | 77 |
| ↓ | 80 |
| ESC | 27 |

> 방향키를 누르면 `getch()`는 `224`(또는 `0`)를 먼저 반환하고, 그다음에 위 코드를 반환한다.

**`gotoxy`**

```c
void gotoxy(int x, int y)
{
    COORD Pos = {x - 1, y - 1};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Pos);
}
```

화면 좌표는 0부터 시작하므로, 1부터 세는 `x`, `y`에서 1을 빼서 넘긴다.

### 4-3. 강의 원본 코드

```c
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>
void draw_horizontal_slide(int x, int y, int length, char *s);
void draw_vertical_slide(int x, int y, int length, char *s);
void draw_rectangle(int c, int r);
void move_arrow_key(char key, int *x1, int *y1, int x_b, int y_b);
void gotoxy(int x, int y);

int main(void)
{
    char *slide = "■", key;
    int x = 1, y = 1;
    int h_slide_length, v_slide_length;
    printf("슬라이드바 표시\n\n");
    printf("수평 슬라이드바의 길이(최대 70)를 \n");
    printf("입력하고 Enter>");
    scanf("%d", &h_slide_length);
    printf("수직 슬라이드바의 길이(최대 19)를 \n");
    printf("입력하고 Enter>");
    scanf("%d", &v_slide_length);
    system("cls");
    do
    {
        draw_vertical_slide(1, y, v_slide_length, slide);
        draw_horizontal_slide(x, v_slide_length + 3, h_slide_length, slide);
        key = getch();
        move_arrow_key(key, &x, &y, h_slide_length, v_slide_length);
    } while (key != 27);
    return 0;
}

void draw_rectangle(int c, int r)
{
    int i, j;
    unsigned char a = 0xa6;
    unsigned char b[7];
    for (i = 1; i < 7; i++)
        b[i] = 0xa0 + i;

    printf("%c%c", a, b[3]);
    for (i = 0; i < c; i++)
        printf("%c%c", a, b[1]);
    printf("%c%c", a, b[4]);
    printf("\n");
    for (i = 0; i < r; i++)
    {
        printf("%c%c", a, b[2]);
        for (j = 0; j < c; j++)
            printf("  ");          // 두 칸 공백 (괘선 문자 폭)
        printf("%c%c", a, b[2]);
        printf("\n");
    }
    printf("%c%c", a, b[6]);
    for (i = 0; i < c; i++)
        printf("%c%c", a, b[1]);
    printf("%c%c", a, b[5]);
    printf("\n");
}

void draw_horizontal_slide(int x, int y, int length, char *s)
{
    int real_length = length / 2;
    gotoxy(1, y);
    draw_rectangle(real_length + 1, 1);
    gotoxy(x + 2, y + 1);
    printf("%s", s);
    gotoxy(real_length * 2 + 2, y - 1);
    printf("%2d", x);
}

void draw_vertical_slide(int x, int y, int length, char *s)
{
    gotoxy(x, 1);
    draw_rectangle(1, length);
    gotoxy(x + 2, y + 1);
    printf("%s", s);
    gotoxy(x + 6, length + 1);
    printf("%2d", y);
}

void move_arrow_key(char key, int *x1, int *y1, int x_b, int y_b)
{
    switch (key)
    {
    case 72: *y1 = *y1 - 1; if (*y1 < 1) *y1 = 1;     break; // 위
    case 75: *x1 = *x1 - 1; if (*x1 < 1) *x1 = 1;     break; // 왼쪽
    case 77: *x1 = *x1 + 1; if (*x1 > x_b) *x1 = x_b; break; // 오른쪽
    case 80: *y1 = *y1 + 1; if (*y1 > y_b) *y1 = y_b; break; // 아래
    default: return;
    }
}

void gotoxy(int x, int y)
{
    COORD Pos = {x - 1, y - 1};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Pos);
}
```

**`real_length = length / 2`, `+1`의 의미 (2칸 기준 콘솔)**

- `─` 하나가 화면 2칸이므로 길이 10은 `─` 5개로 그린다.
- 슬라이드 `■`도 2칸이라, 마지막 위치(`x = 10`)에서 잘리지 않도록 `─` 하나(2칸)를 여유로 더 둔다.
- 그래서 길이 10을 입력하면 `─`가 6개 보이는 것이 정상이다.

### 4-4. Windows Terminal(1칸 기준) 수정본

괘선과 `■`가 1칸으로 그려지는 환경에서, 입력한 길이만큼 막대가 그려지도록 좌표를 바꾼 버전. 바뀐 함수만 적는다.

```c
// main의 do 루프 안
draw_horizontal_slide(x, v_slide_length + 4, h_slide_length, slide); // +3 → +4

// draw_rectangle 안쪽 공백
printf(" ");                      // 1칸

void draw_horizontal_slide(int x, int y, int length, char *s)
{
    gotoxy(1, y);
    draw_rectangle(length, 1);    // 입력한 길이 그대로
    gotoxy(x + 1, y + 1);         // 내부 첫 칸(2열)부터
    printf("%s", s);
    gotoxy(length, y - 1);        // 막대 오른쪽 끝 위에 숫자
    printf("%2d", x);
}

void draw_vertical_slide(int x, int y, int length, char *s)
{
    gotoxy(x, 1);
    draw_rectangle(1, length);
    gotoxy(x + 1, y + 1);         // 내부 칸(2열)
    printf("%s", s);
    gotoxy(x + 4, length + 1);    // 오른쪽 테두리 옆
    printf("%2d", y);
}
```

> 가로 막대를 한 줄 내린 이유(`+3 → +4`): 가로 길이를 3 이하로 넣으면 숫자가 세로 막대의 아래 테두리와 겹친다.

---

## 5. 도형의 연속적인 이동과 회전

화살표 키로 도형을 상하좌우로 계속 이동시키고, 스페이스 키로 90도 회전시킨다.

### 5-1. 도형 표현

3×3 행렬에서 `1`은 `□`(또는 `■`), `0`은 공백으로 출력한다.

```
[도형 1]      [행렬 1]          시계 방향 90°      [행렬 2]
. . .         0 0 0             ──────────►       1 0 0
. ■ .         0 1 0                                1 1 0
■ ■ ■         1 1 1                                1 0 0
```

```c
for (i = 0; i < 3; i++)
{
    for (j = 0; j < 3; j++)
        if (m[i][j] == 1)
            printf("■");
        else
            printf("  ");
    printf("\n");
}
```

### 5-2. 회전 공식 (시계 방향 90°)

```
a[i][j]  →  b[j][2-i]
```

| 원래 위치 | 회전 후 위치 |
|---|---|
| `a[0][0]` | `b[0][2]` |
| `a[0][1]` | `b[1][2]` |
| `a[0][2]` | `b[2][2]` |

```c
for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
        b[j][2 - i] = a[i][j];
```

> 같은 배열에 바로 덮어쓰면 아직 옮기지 않은 값이 지워지므로, 임시 배열 `temp`에 먼저 옮긴 뒤 다시 복사한다.

### 5-3. 정적인 움직임 vs 동적인 움직임

| 정적인 움직임 | 동적인 움직임 |
|---|---|
| 키를 누를 때마다 **한 칸** 이동 | 키를 누르면 그 방향으로 **계속** 이동 |
| `x++`, `y--`로 좌표를 직접 변경 | `inx`, `iny`에 **방향**만 저장 |
| 이동 후 `move_shape` 한 번 호출 | `move_shape`가 다음 키 입력까지 반복 |

강의 코드는 **동적인 움직임** 방식이다.

### 5-4. 함수 구성

| 함수 | 역할 |
|---|---|
| `rotation_right` | 도형 시계 방향 회전 |
| `move_shape` | `kbhit()`가 참이 될 때까지 0.1초마다 이동하며 출력 |
| `print_shape` | 현재 좌표에 도형 출력 |
| `move_control` | 키 입력에 따라 방향 설정·회전 |
| `gotoxy` | 커서 위치 이동 |
| `print_direction` | 화면 위쪽에 조작 안내 출력 |

- `kbhit()`: 키가 눌렸는지만 확인하고 바로 반환한다(입력은 소비하지 않음).
- `Sleep(100)`: 0.1초 대기. 대문자 `S`이며 단위는 밀리초.
- 이동 범위: `x`는 1~75, `y`는 2~23 (1행은 안내 문구 자리).

### 5-5. 강의 원본 코드

```c
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

int x = 35, y = 12;
int inx = 0, iny = 0;

int main(void)
{
    int shape1[3][3] = {0, 1, 0,
                        0, 1, 0,
                        1, 1, 1};
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
    char key;
    do
    {
        while (!kbhit())
        {
            system("cls");
            move_shape(m);
        }
        key = getch();
        switch (key)
        {
        case 32: rotation_right(m);  move_shape(m); break;
        case 72: inx = 0;  iny = -1; move_shape(m); break;
        case 75: inx = -1; iny = 0;  move_shape(m); break;
        case 77: inx = 1;  iny = 0;  move_shape(m); break;
        case 80: inx = 0;  iny = 1;  move_shape(m); break;
        default: break;
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
                printf("  ");
        printf("\n");
    }
}

void move_shape(int m[][3])
{
    do
    {
        system("cls");
        print_shape(m);
        print_direction();
        x = x + inx;
        y = y + iny;
        if (y > 23)      y = 23;
        else if (y < 2)  y = 2;
        if (x > 75)      x = 75;
        else if (x < 1)  x = 1;
        Sleep(100);
    } while (!kbhit());
}

void gotoxy(int x, int y)
{
    COORD Pos = {x - 1, y - 1};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Pos);
}
```

### 5-6. 개선 사항 (강의 구조 유지)

| 문제 | 원인 | 해결 |
|---|---|---|
| 화면이 계속 깜빡임 | 0.1초마다 `system("cls")`로 전체 화면 삭제 | 이전 위치만 공백으로 지우는 `erase_shape` 사용, `cls`는 처음 한 번만 |
| `H` `K` `M` `P` 키로도 이동 | 문자 코드가 방향키 코드(72·75·77·80)와 같음 | `getch()`가 `0`/`224`일 때만 방향키로 처리, 나머지는 무시 |
| 커서 깜빡임 | 출력할 때마다 커서가 이동 | 시작 시 `hide_cursor()` 호출 |
| 매개변수 이름 중복 | `gotoxy(int x, int y)`가 전역 `x`, `y`를 가림 | 매개변수를 `px`, `py`로 변경 |

```c
// move_control: key를 int로, 방향키 판별 추가
int key;
system("cls");                         // 처음 한 번만
do
{
    while (!kbhit())
        move_shape(m);
    key = getch();
    if (key == 0 || key == 224)        // 방향키 → 실제 코드 읽기
        key = getch();
    else if (key != 32 && key != 27)   // 스페이스·ESC 외에는 무시
        continue;
    switch (key) { /* 강의 원본 그대로 */ }
} while (key != 27);

// move_shape: cls 대신 이전 위치만 지우기
do
{
    print_shape(m);
    print_direction();
    Sleep(100);
    erase_shape();
    x = x + inx;
    y = y + iny;
    /* 범위 제한은 원본 그대로 */
} while (!kbhit());

void erase_shape(void)
{
    int i;
    for (i = 0; i < 3; i++)
    {
        gotoxy(x, y + i);
        printf("   ");   // 도형 폭만큼 (1칸 기준이면 3칸, 2칸 기준이면 6칸)
    }
}

void gotoxy(int px, int py)
{
    COORD Pos = {px - 1, py - 1};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Pos);
}

void hide_cursor(void)
{
    CONSOLE_CURSOR_INFO ci = {1, FALSE};
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
}
```

> `do-while` 안의 `continue`는 아래쪽 `while (key != 27)` 조건으로 이동한다. 무시한 키는 27이 아니므로 루프가 계속된다.

---

## 6. 실행 환경 주의사항

### 6-1. 문자 폭: 콘솔에 따라 다르다

| 실행 창 | `■` `─` `│` 폭 | 공백 출력 |
|---|---|---|
| Windows 콘솔 호스트 (탭 없는 검은 cmd 창) | **2칸** | `"  "` (강의 원본) |
| Windows Terminal (탭 있는 창) | **1칸** | `" "` |

- 강의 코드는 **2칸 기준**이다(`length / 2`, `real_length * 2` 등).
- 화면이 어긋나면 먼저 어떤 창에서 실행되는지 확인한다.
- 기본 창 변경: Windows Terminal 설정 → 시작 → 기본 터미널 애플리케이션 → **Windows 콘솔 호스트**

### 6-2. 자주 겪는 문제

| 증상 | 원인 | 해결 |
|---|---|---|
| 한글이 깨짐 | 소스는 UTF-8, 콘솔은 CP949 | `tasks.json` args에 `-fexec-charset=CP949` |
| 실행 후 창이 바로 닫힘 | 프로그램 종료와 함께 창 종료 | 끝에 `getch();` |
| `getchar()` 하나로는 안 멈춤 | `scanf`가 남긴 `'\n'`을 바로 읽음 | 버퍼를 비운 뒤 `getchar()`, 또는 `getch()` |
| 빌드 실패, exe 없음 | 파일명에 `&`, 공백, 한글 등 | 영문·숫자·`_`만 사용 (예: `shape_rotate.c`) |
| PDF 복사 후 정렬이 어긋남 | 연속 공백이 하나로 합쳐짐 | 공백 출력 부분을 원본 폭으로 복원 |
| `Sleep`에 밑줄 | 소문자 `sleep`이거나 IntelliSense 설정 문제 | `Sleep`(대문자), `c_cpp_properties.json` 확인 |
